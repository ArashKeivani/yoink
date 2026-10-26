#!/bin/sh
# Installs the Yoink binary and desktop integration into ~/.local.
# Usage:
#   curl -fsSL <url>/setup.sh | sh
#   sh setup.sh                       # install the latest release
#   YOINK_VERSION=v1.2.0 sh setup.sh  # install a specific release
#   sh setup.sh --build-from-source   # clone and build instead of downloading
set -eu

REPO="${YOINK_REPO:-ArashKeivani/yoink}"
PREFIX="${YOINK_PREFIX:-$HOME/.local}"
BIN_DIR="$PREFIX/bin"
BIN="$BIN_DIR/yoink"
VERSION_ARG="${YOINK_VERSION:-}"

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

die() {
    printf '%s\n' "$@" >&2
    exit 1
}

case "$(uname -s):$(uname -m)" in
Linux:x86_64) ARCH="x86_64" ;;
Linux:aarch64 | Linux:arm64) ARCH="aarch64" ;;
*) die "Yoink only installs on Linux x86_64 or aarch64." ;;
esac

fetch() {
    url="$1"
    out="$2"
    if command -v curl >/dev/null 2>&1; then
        curl -fSL --retry 3 -o "$out" "$url"
    elif command -v wget >/dev/null 2>&1; then
        wget -q -O "$out" "$url"
    else
        die "Need curl or wget to download Yoink."
    fi
}

refresh_caches() {
    command -v update-desktop-database >/dev/null 2>&1 && update-desktop-database "$PREFIX/share/applications" >/dev/null 2>&1 || true
    command -v gtk-update-icon-cache >/dev/null 2>&1 && gtk-update-icon-cache -qf "$PREFIX/share/icons/hicolor" >/dev/null 2>&1 || true
}

if [ -x "$BIN" ] && pgrep -x yoink >/dev/null 2>&1; then
    die "Yoink is running. Quit it first: yoink --quit"
fi

if [ "${1:-}" = "--build-from-source" ]; then
    SRC_DIR="$WORK/src"
    git clone --depth 1 "https://github.com/$REPO.git" "$SRC_DIR" || die "Couldn't clone the repository."
    command -v cmake >/dev/null 2>&1 || die "cmake is required to build from source."
    command -v ninja >/dev/null 2>&1 || die "ninja is required to build from source (or install make and adapt the cmake command)."
    echo "Building Yoink from source (this takes a few minutes)..."
    cmake -S "$SRC_DIR" -B "$WORK/build" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$PREFIX"
    cmake --build "$WORK/build" -j "$(nproc)"
    cmake --install "$WORK/build"
    if [ -f "$PREFIX/share/applications/org.yoink.Yoink.desktop" ]; then
        sed "s|^Exec=.*|Exec=$BIN|" "$PREFIX/share/applications/org.yoink.Yoink.desktop" > "$WORK/desktop.new"
        mv "$WORK/desktop.new" "$PREFIX/share/applications/org.yoink.Yoink.desktop"
    fi
else
    ASSET="yoink-linux-$ARCH.tar.gz"
    base="https://github.com/$REPO/releases/download"
    if [ -n "${YOINK_URL:-}" ]; then
        URL="$YOINK_URL"
    elif [ -n "$VERSION_ARG" ]; then
        URL="$base/$VERSION_ARG/$ASSET"
    else
        URL="https://github.com/$REPO/releases/latest/download/$ASSET"
    fi
    echo "Downloading Yoink ($ARCH)..."
    fetch "$URL" "$WORK/$ASSET" || die "Download failed. No release for $ARCH yet? Build from source: sh setup.sh --build-from-source"
    tar -xzf "$WORK/$ASSET" -C "$WORK"

    mkdir -p "$BIN_DIR" "$PREFIX/share/icons/hicolor/scalable/apps"
    [ -f "$WORK/yoink" ] || die "The release archive is missing the yoink binary."

    echo "Installing into $PREFIX ..."
    install -m 0755 "$WORK/yoink" "$BIN"
    if [ -f "$WORK/org.yoink.Yoink.desktop" ]; then
        for icon in "$WORK"/icons/*.svg; do
            [ -f "$icon" ] && install -m 0644 "$icon" "$PREFIX/share/icons/hicolor/scalable/apps/$(basename "$icon")"
        done
        sed "s|^Exec=.*|Exec=$BIN|" "$WORK/org.yoink.Yoink.desktop" > "$WORK/org.yoink.Yoink.desktop.new"
        install -D -m 0644 "$WORK/org.yoink.Yoink.desktop.new" "$PREFIX/share/applications/org.yoink.Yoink.desktop"
    fi
fi

refresh_caches

echo
echo "Yoink installed at $BIN"

missing=$(ldd "$BIN" 2>/dev/null | awk '/not found/ {print $1}' | sort -u || true)
if [ -n "$missing" ]; then
    echo
    echo "WARNING: missing shared libraries:" >&2
    echo "$missing" >&2
    echo "Install the Qt 6 runtime packages for your distribution (see the README)." >&2
fi
