# Changelog

## foo_cover_info

## foo_cover_resizer

### 0.2.2
- If `foo_cover_info` is installed, updates are triggered automatically
when art is added, resized or converted by this component. `foo_cover_info`
itself remains dumb as a rock and has no clue when you add/remove art
via the standard `Properties` dialog or native `fooobar2000` context commands.

### 0.2.1
- Fixes a bug where converting any other type except `Front` didn't
work. Resizing should have worked as normal.

### 0.2.0
- Adds support for writing `WEBP`.
- Adds support for browsing for an image file, converting but not resizing
before attaching.

### 0.1.1
- Fixes a bug where failure to read images was not reported.

### 0.10
- Uses `Windows Imaging Component` for all image handling/resizing.
- Because of the above changes, `Windows 7` users must make sure their OS is fully updated.
- `Windows 7/8/8.1` requires this [package](https://storage.googleapis.com/downloads.webmproject.org/releases/webp/WebpCodecSetup.exe) for
reading `WEBP` files. `Windows 10/11` users should have `WEBP` support installed by default
from the `Windows Store`.

## foo_run_main
