# Changelog

## 1.9.2 - 2026-09-26

### Fixes
* App icon modernized: glass magnifying-glass look, enlarged, brighter cyan lens
* Finder Tool icon and cursor modernized to match, multi-resolution
* App tool buttons are now multi-resolution icons (16–40px) instead of a single blurry 16px bitmap.
* New hamburger icon for the titlebar/system-menu corner (taskbar icon unchanged)
* Fixed pinned-corner detection (was unreliable except when minimized)
* Fixed window position going off-screen after monitor/resolution changes
  
## 1.9.0 - 2026-09-25

### Added

* Portable mode (#16) — drop a winspy.ini next to winspy.exe and all settings are read/written there instead of the registry, so the whole app can run from a USB drive or be copied between machines with nothing left behind. Falls back to %AppData%\Catch22\WinSpy\winspy.ini if no portable ini is present. Registry-based settings storage has been removed.
* High-DPI support (#11) — WinSpy is now Per-Monitor-V2 DPI aware and renders crisply on scaled displays instead of being blurrily stretched by Windows. This also required rescaling everything that DPI awareness exposed as hard-coded pixel sizes: the pin toolbar, the More/Less button, the Handle/Caption/Rectangle/Style dialog buttons, the window tree's icons and row spacing, and the style-list line height (previously clipping text at higher DPI).

### Fixes
* Window Styles editor: toggling WS_EX_TOPMOST had no effect (#14) — per MSDN, this extended style can only be changed via SetWindowPos, not SetWindowLong. The style editor now applies it correctly.
* "Bring to Front" did nothing for minimized or background windows (#15) — it now restores minimized windows, raises them above other windows, and actually activates them.
* Smaller Release builds (#10) — Release configs now build for minimum size instead of max speed, with string pooling and no exception handling/RTTI (neither is used anywhere in the codebase).

## 1.7

### Added
* Windows Vista support
* Tree hierarchy now groups by process.

## 1.6

### Added
* Multi-monitor support
* Now works correctly for all versions of Windows.

## 1.5

### Added
* Retrieve passwords from password-edit controls
* Edit window styles
* Alter window captions
* Show / Hide / Enable / Disable / Adjust any window in the system
* Improved user-interface
* View the complete system window hierarchy

