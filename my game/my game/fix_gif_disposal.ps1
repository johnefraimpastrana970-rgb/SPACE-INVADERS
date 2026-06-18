$gifDir = "Graphics"
$gifs = Get-ChildItem -Path $gifDir -Filter "*.gif" -ErrorAction SilentlyContinue

if ($gifs.Count -eq 0) {
    Write-Output "No GIF files found"
    exit 1
}

Write-Output "Found $($gifs.Count) GIF files"

$fixedCount = 0

foreach ($gif in $gifs) {
    $gifPath = $gif.FullName
    
    Write-Output "Processing: $($gif.Name)"
    
    try {
        $outputPath = Join-Path $gif.DirectoryName ("$($gif.BaseName).tmp.gif")
        & convert.exe $gifPath -dispose Background $outputPath 2>&1 | Out-Null
        
        if ((Test-Path $outputPath)) {
            Remove-Item $gifPath -Force
            Rename-Item $outputPath -NewName $gif.Name
            Write-Output "  Fixed"
            $fixedCount++
        } else {
            Write-Output "  Failed"
            if (Test-Path $outputPath) { Remove-Item $outputPath -Force }
        }
    } catch {
        Write-Output "  Error: $_"
        if (Test-Path $outputPath) { Remove-Item $outputPath -Force }
    }
}

Write-Output "Fixed $fixedCount GIF files"
