# Manual test matrix

Automated drag-and-drop testing on Wayland is not currently practical here: a
background process cannot inject a native pointer drag into arbitrary source
applications without compositor-specific test hooks. Yoink is therefore checked
with the following manual matrix on a real Plasma 6 Wayland session.

Use a scratch data directory when testing destructive actions if you do not want
to disturb your everyday shelf. The storage path is shown in **Settings →
Storage**.

## Build and startup

- [ ] Configure and build from a clean directory with no warnings.
- [ ] Launch with no arguments: the shelf starts collapsed on the configured edge.
- [ ] Launch with `--show`: the shelf opens.
- [ ] Launch with `--autostart`: the shelf starts collapsed and quiet.
- [ ] Launch a second instance with `--show`: the existing instance opens and
      the second process exits successfully.
- [ ] Launch with `--toggle`: the existing instance toggles.
- [ ] Launch with `--settings`: the existing instance opens settings.
- [ ] Launch with `--quit`: the existing instance exits and the tray icon
      disappears.
- [ ] Quit and relaunch: items and settings are restored.
- [ ] Remove the shelf data file: Yoink starts empty without crashing.
- [ ] Write invalid JSON to the data file: Yoink backs it up, starts empty, and
      shows an error.
- [ ] Write a newer `schemaVersion`: Yoink is read-only and shows the read-only
      banner in settings.

## Edge handle and layout

- [ ] The handle is discoverable but visually restrained at rest.
- [ ] Hovering the handle opens the shelf.
- [ ] Moving the pointer away closes it after the short delay.
- [ ] Clicking the handle keeps the shelf open until it is closed.
- [ ] The shelf never hides while a drag is in progress.
- [ ] The handle is reachable at left, right, top, and bottom edges.
- [ ] Position 0, 50, and 100 place the shelf correctly at each edge.
- [ ] Changing monitors moves the shelf to the selected monitor.
- [ ] Disconnecting the selected monitor falls back to the primary screen.
- [ ] A panel on the same edge is respected; the shelf does not sit underneath
      it.
- [ ] The shelf stays inside the available screen area at every edge.
- [ ] With reduced motion enabled in Plasma, opening/closing is effectively
      instant and no animation is required to understand the state.

## Collecting content

- [ ] Drop one local file.
- [ ] Drop multiple local files at once.
- [ ] Drop a local folder.
- [ ] Drop plain text.
- [ ] Drop a URL.
- [ ] Drop a raw image (for example from a browser or image viewer).
- [ ] Drop a supported item when the shelf is empty and when it is full.
- [ ] Drop the same file twice: the duplicate is skipped with a message.
- [ ] Drop the same text twice: the duplicate is skipped.
- [ ] Drop unsupported content: it is rejected with a clear message.
- [ ] Drop a file that disappears before it can be imported: the unavailable
      path is rejected.
- [ ] Drop more than 60 items: the limit is enforced and reported.
- [ ] Drop text over 256 KiB: it is rejected.
- [ ] Drop an image over 20 megapixels or 6 MiB encoded: it is rejected.

## Item presentation and actions

- [ ] Files and folders show the correct icon and metadata.
- [ ] Image files and raw images show a rounded preview.
- [ ] Missing files are dimmed and explain that they are unavailable.
- [ ] Single-click a file/folder/URL: it is selected, nothing is opened.
- [ ] Double-click a file/folder: it opens.
- [ ] Double-click a URL: it opens in the browser.
- [ ] Double-click a text item: it is copied to the clipboard.
- [ ] A click after dragging an item does not activate it by mistake.
- [ ] Right-click an item to open its menu; right-clicking an unselected item
      selects it first.
- [ ] Copy file, text, link, and image items to the clipboard.
- [ ] Show a file or folder in its containing folder.
- [ ] Select several items including mixed kinds, right-click one of them: the
      menu shows the selection count, Copy N items, and Remove N items.
- [ ] With only files/folders/URLs selected, the menu also offers Open N item(s);
      with only files/folders/images, also Show in Folder.
- [ ] Remove a selection with the menu; all selected items disappear and undo
      restores them at the earliest original position.
- [ ] Copy a mixed selection to the clipboard and paste into a file manager:
      files and URLs arrive; paste into a text editor shows the joined text.
- [ ] Remove one item; the undo toast appears.
- [ ] Click Undo; the item returns in its original position.
- [ ] Clear all; the undo toast appears.
- [ ] Click Undo after Clear all; all items return.
- [ ] Remove an item and then add another item; undo still behaves sensibly.
- [ ] Drag a file out and drop it on a target that accepts copies.
- [ ] Drag a file out and drop it on a target that accepts moves; the target's
      move is honored and Yoink's reference remains.
- [ ] Drag text, a URL, and an image out; each target receives the expected data.
- [ ] Ctrl+click several items; they show the selection highlight.
- [ ] Shift+click extends the selection between two items.
- [ ] Ctrl+A selects all items; Escape clears the selection.
- [ ] Drag a selected item out and drop on a folder: all selected files arrive
      as one drop; text and URL selections arrive as text.
- [ ] Drag an unselected item while others are selected: only that item moves.
- [ ] If part of a selection is unavailable, the unavailable ones are skipped
      and a toast reports how many were skipped; a single unavailable item
      explains itself instead of starting a drag.
- [ ] Ctrl+Space toggles and Shift+Space extends the selection from the keyboard.
- [ ] Cancel a drag-out by pressing Escape or dropping outside a target; Yoink
      remains open and the item is not lost.
- [ ] Attempt to drag/open a missing file; an error is shown and nothing is
      deleted.

## Settings and tray

- [ ] Open settings with the header button, the tray, and `Ctrl+,`.
- [ ] Change edge and position while watching the shelf: the surface stays at
      the edge and the card slides smoothly within it, without jumping.
- [ ] On the left/right edges the position slider moves the card as you drag.
- [ ] On the top/bottom edges the card keeps still while dragging the position
      slider and lands on the new position when the slider is released.
- [ ] The expanded card has the same small margin from the active edge on
      left, right, top, and bottom; the handle is flush on all four sides.
- [ ] Change the notification timeout and trigger a toast: it disappears after
      roughly that time; error toasts stay at least four seconds.
- [ ] Restart after changing the notification timeout: the value persists.
- [ ] Settings → Storage shows `~/.local/share/Yoink`.
- [ ] Change monitor; the shelf moves immediately.
- [ ] Toggle Start at Login; the autostart file is created/removed and the
      setting survives a restart.
- [ ] Close settings with Escape and the back button.
- [ ] Toggle pin; the shelf stays open while unpinned auto-hide would normally
      close it.
- [ ] Close the shelf with the close button; items are retained.
- [ ] Open the shelf from the tray.
- [ ] Use the tray's Start at Login action; the switch in settings stays in sync.
- [ ] Tray Quit exits cleanly and saves pending changes.
- [ ] Make the settings file unwritable; a save error is shown and the app does
      not crash.

## Keyboard and accessibility

- [ ] Tab reaches the header buttons, list items, settings controls, and menu
      rows with a visible focus ring.
- [ ] Enter/Space activates the focused item or control.
- [ ] Delete removes the focused item.
- [ ] The Menu key opens the focused item's menu.
- [ ] Escape closes the item menu, shelf menu, settings, and shelf in that
      order.
- [ ] Screen-reader names are present for the handle, items, icon buttons, and
      toggles.

## Failure and shutdown

- [ ] Kill the process while the shelf is open; relaunch restores the last
      successfully saved state.
- [ ] Make the data directory read-only; collection and removal report an error
      instead of silently losing data.
- [ ] Fill the data disk; a save failure is reported and the previous data file
      remains intact.
- [ ] Quit while a toast is visible; shutdown completes without a crash.
- [ ] Start with no `XDG_RUNTIME_DIR`/local socket support; the fallback path is
      used and the app still starts.
