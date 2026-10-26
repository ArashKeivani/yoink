#!/bin/sh
# Removes the Yoink installation from ~/.local.
# Usage:
#   curl -fsSL <url>/uninstall.sh | sh
#   sh uninstall.sh           # remove app files and autostart entry,
#                             # keep shelf data and settings
#   sh uninstall.sh --purge   # also remove data and settings
set -eu

PREFIX="${YOINK_PREFIX:-$HOME/.local}"
BIN="$PREFIX/bin/yoink"
APP_ID="org.yoink.Yoink"

purge=0
[ "${1:-}" = "--purge" ] && purge=1

if ! [ -e "$BIN" ] && ! [ -f "$PREFIX/share/applications/$APP_ID.desktop" ] && [ "$purge" -eq 0 ]; then
    echo "Yoink is not installed in $PREFIX. Nothing to do."
    exit 0
fi

if pgrep -x yoink >/dev/null 2>&1; then
    echo "Yoink is running. Quit it first: yoink --quit"
    exit 1
fi

echo "Removing Yoink from $PREFIX ..."
rm -f "$BIN"
rm -f "$PREFIX/share/applications/$APP_ID.desktop"
rm -f "$PREFIX/share/metainfo/$APP_ID.xml"
rm -f "$PREFIX/share/icons/hicolor/scalable/apps/$APP_ID.svg"
rm -f "$PREFIX/share/icons/hicolor/scalable/apps/$APP_ID-symbolic.svg"
rm -f "$PREFIX/share/licenses/yoink/LICENSE"

command -v update-desktop-database >/dev/null 2>&1 && update-desktop-database "$PREFIX/share/applications" >/dev/null 2>&1 || true
command -v gtk-update-icon-cache >/dev/null 2>&1 && gtk-update-icon-cache -qf "$PREFIX/share/icons/hicolor" >/dev/null 2>&1 || true

if [ "$purge" -eq 1 ]; then
    echo "Removing data and settings (--purge) ..."
    rm -f "$HOME/.config/Yoinkrc"
    rm -f "$HOME/.config/autostart/$APP_ID.desktop"
    rm -rf "$HOME/.local/share/Yoink"
else
    rm -f "$HOME/.config/autostart/$APP_ID.desktop"
    echo "Kept your shelf data (~/.local/share/Yoink) and settings. Remove them with --purge."
fi

echo "Yoink uninstalled."
