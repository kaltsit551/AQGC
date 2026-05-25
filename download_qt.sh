#!/bin/bash
# Download Qt 6.10.3 packages with curl (supports resume)

BASE_URL="https://download.qt.io/online/qtsdkrepository/windows_x86/desktop/qt6_6103/qt6_6103"
VERSION="6.10.3-0-202603310407"
DOWNLOAD_DIR="E:/drone/QGC-debug/qt_downloads"
mkdir -p "$DOWNLOAD_DIR"

# Only the packages QGC needs
PACKAGES=(
    "qtbase-Windows-Windows_11_24H2-MSVC2022-Windows-Windows_11_24H2-X86_64.7z"
    "qtdeclarative-Windows-Windows_11_24H2-MSVC2022-Windows-Windows_11_24H2-X86_64.7z"
    "qtsvg-Windows-Windows_11_24H2-MSVC2022-Windows-Windows_11_24H2-X86_64.7z"
    "qttools-Windows-Windows_11_24H2-MSVC2022-Windows-Windows_11_24H2-X86_64.7z"
    "qtmultimedia-Windows-Windows_11_24H2-MSVC2022-Windows-Windows_11_24H2-X86_64.7z"
    "qtquick3d-Windows-Windows_11_24H2-MSVC2022-Windows-Windows_11_24H2-X86_64.7z"
    "qtshadertools-Windows-Windows_11_24H2-MSVC2022-Windows-Windows_11_24H2-X86_64.7z"
    "qtgraphs-Windows-Windows_11_24H2-MSVC2022-Windows-Windows_11_24H2-X86_64.7z"
    "qtlocation-Windows-Windows_11_24H2-MSVC2022-Windows-Windows_11_24H2-X86_64.7z"
    "qtpositioning-Windows-Windows_11_24H2-MSVC2022-Windows-Windows_11_24H2-X86_64.7z"
    "qtspeech-Windows-Windows_11_24H2-MSVC2022-Windows-Windows_11_24H2-X86_64.7z"
    "qtserialport-Windows-Windows_11_24H2-MSVC2022-Windows-Windows_11_24H2-X86_64.7z"
    "qtimageformats-Windows-Windows_11_24H2-MSVC2022-Windows-Windows_11_24H2-X86_64.7z"
    "qtconnectivity-Windows-Windows_11_24H2-MSVC2022-Windows-Windows_11_24H2-X86_64.7z"
    "qtsensors-Windows-Windows_11_24H2-MSVC2022-Windows-Windows_11_24H2-X86_64.7z"
    "qtscxml-Windows-Windows_11_24H2-MSVC2022-Windows-Windows_11_24H2-X86_64.7z"
    "qtwebsockets-Windows-Windows_11_24H2-MSVC2022-Windows-Windows_11_24H2-X86_64.7z"
    "qthttpserver-Windows-Windows_11_24H2-MSVC2022-Windows-Windows_11_24H2-X86_64.7z"
    "qttranslations-Windows-Windows_11_24H2-MSVC2022-Windows-Windows_11_24H2-X86_64.7z"
    "d3dcompiler_47-x64.7z"
    "opengl32sw-64-mesa_11_2_2-signed_sha256.7z"
)

FAILED=()
for pkg in "${PACKAGES[@]}"; do
    url="${BASE_URL}/${VERSION}${pkg}"
    dest="${DOWNLOAD_DIR}/${pkg}"
    echo "=== Downloading: $pkg ==="
    # Retry up to 5 times with resume support
    curl -C - --retry 5 --retry-delay 3 -L -o "$dest" "$url"
    if [ $? -ne 0 ]; then
        FAILED+=("$pkg")
        echo "FAILED: $pkg"
    else
        echo "OK: $pkg"
    fi
    echo ""
done

if [ ${#FAILED[@]} -eq 0 ]; then
    echo "All downloads completed successfully!"
else
    echo "Failed downloads:"
    for f in "${FAILED[@]}"; do
        echo "  - $f"
    done
fi
