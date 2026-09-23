# Convert GhostFrame PNG icon to multi-resolution Windows ICO format
param(
    [string]$SourcePng = "src\icon\ghostframe.png",
    [string]$DestIco = "resources\ghostframe.ico"
)

Add-Type -AssemblyName System.Drawing

$root = $PSScriptRoot
if ($root -match "scripts$") {
    $root = Split-Path $root -Parent
}

$pngFullPath = if ([System.IO.Path]::IsPathRooted($SourcePng)) { $SourcePng } else { Join-Path $root $SourcePng }
$icoFullPath = if ([System.IO.Path]::IsPathRooted($DestIco)) { $DestIco } else { Join-Path $root $DestIco }

if (-not (Test-Path $pngFullPath)) {
    Write-Host "[ERROR] Source PNG not found at: $pngFullPath" -ForegroundColor Red
    exit 1
}

$destDir = Split-Path $icoFullPath -Parent
if (-not (Test-Path $destDir)) {
    New-Item -ItemType Directory -Path $destDir -Force | Out-Null
}

Write-Host "[*] Converting '$pngFullPath' to multi-resolution ICO '$icoFullPath'..." -ForegroundColor Cyan

$srcBmp = [System.Drawing.Bitmap]::FromFile($pngFullPath)
$sizes = @(256, 128, 64, 48, 32, 16)

$ms = New-Object System.IO.MemoryStream
$bw = New-Object System.IO.BinaryWriter($ms)

# Write ICONDIR header (6 bytes)
$bw.Write([uint16]0)                # Reserved, must be 0
$bw.Write([uint16]1)                # Resource type, 1 for icon
$bw.Write([uint16]$sizes.Length)    # Number of images

$offset = 6 + (16 * $sizes.Length)
$imagesData = @()

foreach ($sz in $sizes) {
    $resized = New-Object System.Drawing.Bitmap($sz, $sz, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g = [System.Drawing.Graphics]::FromImage($resized)
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
    $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $g.CompositingQuality = [System.Drawing.Drawing2D.CompositingQuality]::HighQuality
    $g.Clear([System.Drawing.Color]::Transparent)
    $g.DrawImage($srcBmp, 0, 0, $sz, $sz)
    $g.Dispose()

    $imgStream = New-Object System.IO.MemoryStream
    $resized.Save($imgStream, [System.Drawing.Imaging.ImageFormat]::Png)
    $resized.Dispose()

    $bytes = $imgStream.ToArray()
    $imgStream.Dispose()
    $imagesData += ,$bytes

    # Write ICONDIRENTRY (16 bytes)
    $w = if ($sz -eq 256) { [byte]0 } else { [byte]$sz }
    $h = if ($sz -eq 256) { [byte]0 } else { [byte]$sz }
    $bw.Write($w)                           # Width (0 = 256)
    $bw.Write($h)                           # Height (0 = 256)
    $bw.Write([byte]0)                      # Color palette count
    $bw.Write([byte]0)                      # Reserved
    $bw.Write([uint16]1)                    # Color planes
    $bw.Write([uint16]32)                   # Bits per pixel
    $bw.Write([uint32]$bytes.Length)        # Image data size
    $bw.Write([uint32]$offset)              # Image data offset

    $offset += $bytes.Length
}

# Write raw PNG image blocks
foreach ($bytes in $imagesData) {
    $bw.Write($bytes)
}

$srcBmp.Dispose()

$fileBytes = $ms.ToArray()
$bw.Dispose()
$ms.Dispose()

[System.IO.File]::WriteAllBytes($icoFullPath, $fileBytes)

# Also copy a duplicate to src\icon\ghostframe.ico
$altIco = Join-Path $root "src\icon\ghostframe.ico"
[System.IO.File]::WriteAllBytes($altIco, $fileBytes)

Write-Host "[+] Successfully created multi-resolution ICO (256, 128, 64, 48, 32, 16) at:" -ForegroundColor Green
Write-Host "    $icoFullPath" -ForegroundColor White
Write-Host "    $altIco" -ForegroundColor White
