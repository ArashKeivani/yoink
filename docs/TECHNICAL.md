# Technical notes

Build, packaging, storage, and platform details for Yoink. For everyday usage,
see the [README](../README.md).

## Requirements

Yoink targets **KDE Plasma 6 on Wayland** and uses the `wlr-layer-shell`
protocol provided by KWin. On a non-Wayland session Yoink falls back to a
regular always-on-top window, but edge positioning, input masking, and some
focus behavior are not guaranteed there.

Build requirements:

- CMake 3.22 or newer
- C++20 compiler
- Qt 6.6 or newer: Core, Gui, Widgets, Qml, Quick, QuickControls2, Svg
- KDE Frameworks 6: Config, ColorScheme, StatusNotifierItem
- LayerShellQt

Typical package names on Arch Linux:

```sh
sudo pacman -S --needed base-devel cmake ninja qt6-base qt6-declarative qt6-svg \
  kf6-config kf6-colorscheme kf6-statusnotifieritem layer-shell-qt
```

On Debian/Ubuntu-style systems the development packages are usually named
`qt6-base-dev`, `qt6-declarative-dev`, `qt6-svg-dev`, `libkf6config-dev`,
`libkf6colorscheme-dev`, `libkf6statusnotifieritem-dev`, and
`liblayershellqtinterface-dev`.

## Build

Debug build:

```sh
cmake -S . -B build/dev -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/dev -j"$(nproc)"
```

Release build:

```sh
cmake -S . -B build/release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/release -j"$(nproc)"
```

Run the development build:

```sh
./build/dev/yoink
```

Reconfiguring an existing build directory picks up the type it was configured
with; pass `-DCMAKE_BUILD_TYPE=` there to switch it.

## Install into a prefix

```sh
cmake --install build/release --prefix "$HOME/.local"
```

When installing desktop file or autostart users, make sure the install prefix's
`bin` directory is on `PATH` before using the desktop entry.

## Release packaging

Maintainers produce the release archive that `setup.sh` downloads by running
`./package.sh`; it emits `yoink-linux-<arch>.tar.gz` with the release binary,
desktop file, and icons, ready to attach to a GitHub release.

## Storage behavior

The exact paths are shown in **Settings → Storage**.

- Shelf data: `~/.local/share/Yoink/items.json`
- Copied image data: `~/.local/share/Yoink/images/`
- Settings: `~/.config/Yoinkrc`
- Autostart entry: `~/.config/autostart/org.yoink.Yoink.desktop`

Shelf writes use `QSaveFile`, so a failed write does not replace the previous
good file. Corrupt or unknown-format data is backed up next to the original and
reported in the UI. A data file from a newer schema is treated as read-only.

File and folder items are references, not copies. If an original is moved or
deleted, Yoink marks the item unavailable and refuses to open or drag it. Raw
image data received through the image MIME type is stored as a PNG inside
Yoink's data directory; image files dropped as file URLs remain references.

### Limits

- Maximum 60 items.
- Maximum text item: 256 KiB (UTF-8).
- Maximum raw image: 20 megapixels and 6 MiB encoded PNG.
- The shelf card is roughly 336 × 436 logical pixels and clamped to the
  available screen area.
- No cloud sync, history, tags, or search.

### Portal warning when running uninstalled

When launched from a build directory without Yoink installed, Qt logs
`Failed to register with host portal … App info not found for 'org.yoink.Yoink'`.
The desktop entry the portal looks up is not in a standard applications
directory yet, so registration fails; this is harmless. Installing Yoink
(`cmake --install`) makes it go away.

## Wayland and drag-and-drop limitations

Native drag-and-drop is ultimately controlled by the source application and the
compositor. Yoink cannot recover data that an application never exposes.

- Some applications offer only plain text for a drag that visually looks like a
  file or image. Yoink collects what the source actually provides.
- Some applications do not publish local file URLs, or use private MIME types.
  Those drops are rejected with an explanatory message.
- Dragging out of a layer-shell surface is supported through Qt's native drag
  implementation, but the target application decides whether it accepts copy,
  move, image, or URL data.
- There is no portable Wayland mechanism for a background application to inject
  synthetic pointer drags. Automated drag-and-drop tests are therefore not
  included; the manual test matrix is in [`TESTING.md`](TESTING.md).
- Global keyboard shortcuts are not registered. Yoink's shortcuts work while
  its surface has keyboard focus.
- Monitor hot-plug and panel changes are handled by re-computing the layer
  surface layout. If the configured monitor disappears, Yoink falls back to the
  primary screen.

## Development

The code is organized into:

```text
src/core/   Shelf model, MIME import, persistence, settings, theme, placement
src/app/    Single instance, autostart, tray integration
src/ui/     Layer-shell window, drop surface, controller, image provider
src/qml/    Shelf, settings, menus, and reusable controls
data/       Desktop entry, AppStream metadata, icons
```

Formatting follows the KDE Frameworks style configured in `.clang-format`.
There is no automated test suite yet; correctness is currently verified with the
manual matrix and a clean build.
