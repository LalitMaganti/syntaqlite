# cibuildwheel before-all hook for Windows.
# Builds the Rust static library + CLI binary, handling ARM64 cross-compilation.
$ErrorActionPreference = "Stop"

$RepoRoot = (git rev-parse --show-toplevel) | Out-String
$RepoRoot = $RepoRoot.Trim()
Set-Location $RepoRoot

# Build for native arch (always needed).
cargo build --release -p syntaqlite -p syntaqlite-cli

# If ARM64 is in the arch list, also cross-compile for it.
$Archs = $env:CIBW_ARCHS
if ($Archs -match "ARM64") {
    $Target = "aarch64-pc-windows-msvc"
    rustup target add $Target
    cargo build --release -p syntaqlite -p syntaqlite-cli --target $Target
}

# Copy native CLI binary into the package so setuptools includes it.
New-Item -ItemType Directory -Force -Path "python\syntaqlite\bin" | Out-Null
Copy-Item "target\release\syntaqlite.exe" "python\syntaqlite\bin\syntaqlite.exe"
