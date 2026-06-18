from PIL import Image, ImageSequence
import os

def fix_gif_disposal(input_path, output_path):
    """
    Binabasa ang GIF at ginagawang standalone (baked) ang bawat frame.
    Sinisiguro nito na bawat frame ay papalit sa luma, imbes na papatong.
    """
    if not os.path.exists(input_path):
        print(f"Error: File not found -> {input_path}")
        return

    with Image.open(input_path) as im:
        frames = []
        # Gumawa ng transparent canvas na kasing laki ng original GIF
        canvas = Image.new("RGBA", im.size)
        
        for frame in ImageSequence.Iterator(im):
            # Disposal method check (2 = Restore to Background/Clear)
            disposal = frame.info.get('disposal', 1)
            if disposal == 2:
                canvas = Image.new("RGBA", im.size)
            
            # I-paste ang kasalukuyang frame sa canvas (ito ang 'baking' process)
            frame_rgba = frame.convert("RGBA")
            canvas.paste(frame_rgba, (0, 0), frame_rgba)
            
            # I-append ang kopya ng canvas sa ating listahan ng frames
            frames.append(canvas.copy())

        # I-save muli bilang GIF pero naka-set ang disposal sa 2 (Replace)
        frames[0].save(
            output_path,
            save_all=True,
            append_images=frames[1:],
            duration=im.info.get('duration', 100),
            loop=0,
            disposal=2  # Mahalaga: Sinasabi nito sa renderer na burahin ang lumang frame
        )
        print(f"Successfully fixed: {output_path}")

# Halimbawa ng paggamit:
# fix_gif_disposal("Graphics/pokemon_sprite.gif", "Graphics/pokemon_sprite_fixed.gif")