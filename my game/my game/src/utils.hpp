#pragma once
#include <string>
#include <vector>
#include <raylib.h>

namespace Utils {
    static std::string ResolveAssetPath(const std::string& assetPath)
    {
        // If path is already absolute (contains Windows drive ':' or Unix root '/'),
        // return it as is to prevent double resolution bugs.
        if (assetPath.find(':') != std::string::npos || (assetPath.length() > 0 && assetPath[0] == '/')) {
            return assetPath;
        }

        std::string resolved = std::string(GetDirectoryPath(GetApplicationDirectory())) + "/" + assetPath;
        for (char &c : resolved) {
            if (c == '\\') c = '/';
        }
        return resolved;
    }

    // Structure for Texture Streaming
    struct GifStream {
        std::vector<Image> frames;
        Texture2D texture = { 0 };
        int lastUploadedFrame = -1;

        // Updates the single VRAM texture with the pixels of the current CPU frame
        void UpdateVRAM(int frameIndex) {
            if (texture.id > 0 && frameIndex != lastUploadedFrame && frameIndex >= 0 && frameIndex < (int)frames.size()) {
                UpdateTexture(texture, frames[frameIndex].data);
                lastUploadedFrame = frameIndex;
            }
        }

        void Draw(Rectangle src, Rectangle dst, Vector2 origin, float rotation, Color tint, Shader* shader = nullptr) {
            if (texture.id > 0) {
                if (shader) BeginShaderMode(*shader);
                DrawTexturePro(texture, src, dst, origin, rotation, tint);
                if (shader) EndShaderMode();
            }
        }

        void Unload() {
            for (auto& img : frames) UnloadImage(img);
            if (texture.id > 0) UnloadTexture(texture);
            frames.clear();
            texture = { 0 };
            lastUploadedFrame = -1;
        }

        bool IsValid() const { return !frames.empty() && texture.id > 0; }
    };

    // Custom GIF loader that handles frame composition to prevent "piling"
    /**
     * Memory-optimized GIF loader.
     * @param format: Use PIXELFORMAT_UNCOMPRESSED_R4G4B4A4 to save 50% VRAM for pixel art.
     * @param useNearestNeighbor: Use ImageResizeNN for faster scaling and sharper pixel art.
     */
    inline std::vector<Texture2D> LoadGifFrames(const std::string& assetPath, int targetWidth = 0, int targetHeight = 0, 
                                              bool clearEachFrame = true, PixelFormat format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
                                              bool useNearestNeighbor = true)
    {
        std::vector<Texture2D> textures;
        int frameCount = 0;
        
        std::string fullPath = ResolveAssetPath(assetPath);
        Image anim = LoadImageAnim(fullPath.c_str(), &frameCount);

        if (anim.data == nullptr || frameCount <= 0) {
            TraceLog(LOG_WARNING, "GIF Loader: Failed to load %s", fullPath.c_str());
            return textures;
        }

        int frameSize = GetPixelDataSize(anim.width, anim.height, anim.format);
        // The canvas acts as the internal buffer for frame composition
        Image canvas = GenImageColor(anim.width, anim.height, BLANK);

        for (int i = 0; i < frameCount; i++) {
            // To fix "piling", we clear the canvas before drawing the next frame.
            // If using delta-optimized GIFs, you would set clearEachFrame to false.
            if (clearEachFrame) ImageClearBackground(&canvas, BLANK);

            Image frameSlice = {
                .data = (unsigned char*)anim.data + ((size_t)frameSize * i),
                .width = anim.width,
                .height = anim.height,
                .mipmaps = 1,
                .format = anim.format
            };

            ImageDraw(&canvas, frameSlice, 
                {0, 0, (float)anim.width, (float)anim.height}, 
                {0, 0, (float)anim.width, (float)anim.height}, WHITE);
            
            // Create a temporary copy to manipulate so the canvas remains intact for disposal composition
            Image frameImg = ImageCopy(canvas); 
            
            if (targetWidth > 0 && targetHeight > 0) {
                // Nearest Neighbor is faster and more memory-efficient than high-quality bicubic filtering
                if (useNearestNeighbor) ImageResizeNN(&frameImg, targetWidth, targetHeight);
                else ImageResize(&frameImg, targetWidth, targetHeight);
            }

            // VRAM Optimization: Convert to a smaller format. 
            // For most pixel art, PIXELFORMAT_UNCOMPRESSED_R4G4B4A4 (16-bit) 
            // looks identical to R8G8B8A8 (32-bit) but uses exactly half the VRAM.
            if (frameImg.format != (int)format) {
                ImageFormat(&frameImg, format);
            }
            
            textures.push_back(LoadTextureFromImage(frameImg));
            UnloadImage(frameImg);
        }

        UnloadImage(canvas);
        UnloadImage(anim);
        return textures;
    }

    // Loads a GIF into a Stream (Images in RAM, 1 Texture in VRAM)
    inline GifStream LoadGifStream(const std::string& assetPath, int targetWidth = 0, int targetHeight = 0, 
                                  bool clearEachFrame = true, PixelFormat format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
                                  bool useNearestNeighbor = true)
    {
        GifStream stream;
        int frameCount = 0;
        
        std::string fullPath = ResolveAssetPath(assetPath);
        Image anim = LoadImageAnim(fullPath.c_str(), &frameCount);

        if (anim.data == nullptr || frameCount <= 0) {
            TraceLog(LOG_WARNING, "GifStream: Failed to load %s", fullPath.c_str());
            return stream;
        }

        int frameSize = GetPixelDataSize(anim.width, anim.height, anim.format);
        Image canvas = GenImageColor(anim.width, anim.height, BLANK);

        for (int i = 0; i < frameCount; i++) {
            if (clearEachFrame) ImageClearBackground(&canvas, BLANK);

            Image frameSlice = {
                .data = (unsigned char*)anim.data + ((size_t)frameSize * i),
                .width = anim.width,
                .height = anim.height,
                .mipmaps = 1,
                .format = anim.format
            };

            ImageDraw(&canvas, frameSlice, 
                {0, 0, (float)anim.width, (float)anim.height}, 
                {0, 0, (float)anim.width, (float)anim.height}, WHITE);
            
            Image frameImg = ImageCopy(canvas); 
            
            if (targetWidth > 0 && targetHeight > 0) {
                if (useNearestNeighbor) ImageResizeNN(&frameImg, targetWidth, targetHeight);
                else ImageResize(&frameImg, targetWidth, targetHeight);
            }

            if (frameImg.format != (int)format) ImageFormat(&frameImg, format);
            
            // Create the streaming texture from the very first frame
            if (i == 0) {
                stream.texture = LoadTextureFromImage(frameImg);
                stream.lastUploadedFrame = 0;
            }

            // Store the composed image data in CPU RAM
            stream.frames.push_back(frameImg);
        }

        UnloadImage(canvas);
        UnloadImage(anim);
        
        TraceLog(LOG_INFO, "GifStream: Loaded %s (%d frames) - VRAM saved: %d KB", 
                 assetPath.c_str(), frameCount, 
                 (int)((frameCount - 1) * GetPixelDataSize(stream.texture.width, stream.texture.height, stream.texture.format) / 1024));
        return stream;
    }
}