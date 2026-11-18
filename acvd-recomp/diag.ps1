param(
    [string]$ElfPath = "D:\ACVD_dec\EBOOT.elf"
)
$ErrorActionPreference = "Stop"
$outDir = "D:\ACVD-Port\acvd-recomp\diag-out"
New-Item -ItemType Directory -Path $outDir -Force | Out-Null

Write-Host "=== Running acvd-recomp analyze on $ElfPath ==="
$analyzeOut = & "D:\ACVD-Port\local\build\release\acvd-recomp.exe" analyze $ElfPath 2>&1
$analyzeOut | Set-Content -Encoding utf8 "$outDir\main.analyze.txt"

Write-Host "=== Extracting imports & auditing HLE ==="
& "D:\ACVD-Port\local\build\release\dump_imports.exe" $ElfPath | Set-Content -Encoding utf8 "D:\ACVD-Port\acvd_imports_utf8.txt"
python "D:\ACVD-Port\acvd-recomp\audit_hle.py"

Write-Host "=== Diagnostic completed! Results saved to $outDir ==="
