# Yoink

Yoink is a small, native drag-and-drop shelf for the edge of a KDE Plasma 6
Wayland desktop. Drag files, folders, text, links, and images onto the nearly
invisible edge handle; they stay within reach until you need them again.

Yoink is built with C++20, Qt 6, KDE Frameworks 6, and LayerShellQt. It follows
the active Plasma color scheme, font, and animation settings rather than
shipping a fixed theme.

## Features

- Collect files, folders, text, links, and images from native drag-and-drop.
- Show item names, icons, metadata, and image previews.
- Reject duplicate items and clearly report unsupported or unavailable content.
- Drag items back out with normal copy/move semantics. Yoink never deletes or
  modifies the original content merely because it was collected.
- Remove individual items, clear the shelf, and undo the last removal.
- Pin the shelf open, close it without clearing it, and reopen it from the tray
  or with the command line.
- Persist the shelf across restarts with atomic writes and corruption recovery.
- Choose the screen edge, position, and monitor.
- Configure how long notifications stay visible (Settings → Notification).
- Optional start-at-login entry, single-instance behavior, and a status notifier
  tray icon.
- Keyboard-accessible controls, visible focus rings, and support for Plasma's
  reduced-motion animation factor.
- Inline settings and context menus, so no Wayland popup surfaces are required.

## Requirements

Yoink targets **KDE Plasma 6 on Wayland** and uses the `wlr-layer-shell` protocol.
KWin provides this protocol. On a non-Wayland session Yoink falls back to a
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

Install into a prefix:

```sh
cmake --install build/release --prefix "$HOME/.local"
```

When installing desktop file or autostart users, make sure the install prefix's
`bin` directory is on `PATH` before using the desktop entry.

## Usage

- Hover the thin handle at the configured screen edge to open the shelf.
- Click the handle to keep it open until you close it.
- Drop content onto the shelf. Supported content is collected immediately.
- Drag an item out to copy it (or move it, for local files/folders, when the
  target supports move). The original item is left untouched by Yoink.
- Group-select items with `Ctrl+click` (toggle) and `Shift+click` (extend
  range); `Ctrl+A` selects all and `Escape` clears the selection. Dragging a
  selected item drags the whole selection out as one drop; dragging an
  unselected item drags only that item.
- Click an item to select it; double-click opens it. Double-clicking a text
  item copies it to the clipboard. `Enter` opens the focused item.
- Right-click an item for open/copy/show-in-folder/remove actions. With several
  items selected, the menu acts on the whole selection: Copy and Remove always,
  plus Open and Show in Folder when every selected item supports them.
- Use the header buttons to pin the shelf open, open settings, or close it.
- Use the tray icon to show/hide Yoink or open settings.
- Press `Escape` to close an overlay or the shelf. Press `Ctrl+,` while the shelf
  is open to open settings.

Launching Yoink starts with the shelf collapsed (just the edge handle); it only
opens expanded if it was pinned. `--show`, `--toggle`, and `--settings` open it
explicitly.

In settings, the position slider follows the pointer live on the left and right
edges. On the top and bottom edges the card slides sideways while the position
changes, so the new position applies when you release the slider instead. The
notification timeout slider (1–8 s) controls how long toasts stay visible;
error toasts stay at least four seconds.

### Command line

```text
yoink              Start Yoink and show the shelf
yoink --show       Show the shelf in a running instance
yoink --toggle     Toggle the shelf in a running instance
yoink --settings   Open settings in a running instance
yoink --quit       Quit the running instance
yoink --autostart  Start quietly, as at login
```

Only one Yoink instance runs at a time. Additional invocations forward their
request to the existing instance over a local socket.

## Data and settings

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

## Limits

- Maximum 60 items.
- Maximum text item: 256 KiB (UTF-8).
- Maximum raw image: 20 megapixels and 6 MiB encoded PNG.
- The shelf card is roughly 336 × 436 logical pixels and clamped to the
  available screen area.
- No cloud sync, history, tags, or search.

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
  included; the manual test matrix is in [`docs/TESTING.md`](docs/TESTING.md).
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

## License

Yoink is licensed under the GNU General Public License, version 2 or later.
See [`LICENSE`](LICENSE).
