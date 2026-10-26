#!/bin/sh
# Builds the release binary and packs the release archive that setup.sh
# downloads. Produces yoink-linux-<arch>.tar.gz with the layout:
#   yoink
#   org.yoink.Yoink.desktop
#   org.yoink.Yoink.xml
#   icons/*.svg
set -eu

ARCH="$(uname -m)"
case "$ARCH" in
x86_64 | aarch64 | arm64) ;;
*) echo "Unsupported architecture: $ARCH" >&2; exit 1 ;;
esac

BUILD="$PWD/build/package"
STAGE="$BUILD/stage"

cmake -S . -B "$BUILD" -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD" -j "$(nproc)"

rm -rf "$STAGE"
mkdir -p "$STAGE/icons"
cp "$BUILD/yoink" "$STAGE/"
cp data/org.yoink.Yoink.desktop "$STAGE/"
cp data/org.yoink.Yoink.metainfo.xml "$STAGE/org.yoink.Yoink.xml"
cp data/icons/*.svg "$STAGE/icons/"

OUT="yoink-linux-$ARCH.tar.gz"
tar -czf "$OUT" -C "$STAGE" .
sha256sum "$OUT"

echo
echo "Created $OUT. Attach it to the GitHub release as the asset named:"
echo "  $OUT"
