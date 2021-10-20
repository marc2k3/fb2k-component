## Requirements & Download

https://github.com/marc2k3/fb2k-component/releases/latest

# foo_run_main

Unlike the built in command line handler/`foo_runcmd`, this component has full support for dynamically generated menu commands meaning you can use `Edit` commands and switch playlists, change output devices etc.

You must supply the full path to the command. Examples:

```
foobar2000.exe /run_main:Edit/Sort/Randomize
foobar2000.exe /run_main:Library/Search

// use double quotes when command contains spaces
foobar2000.exe /run_main:"Playback/Device/Primary Sound Driver"
```

# foo_cover_info

### Breaking changes in component version `v0.1.0` released on `07/04/2021`.

- All previously saved data will be lost as the internal storage mechanism has changed. All files will need to be scanned again.
- Scanning from the `Library` menu is no longer an option. Just use the context menu anywhere.
- The minimum requirement for `foobar2000` has been bumped to `v1.6`.

### Usage

Because it's not possible to query files for embedded album art within foobar2000, this component scans a selection of files and stores the results in a database. Updating of data is not automatic so if you add/remove art at a later date, it's up to you to run the scan again. Data is available in the following fields which are available wherever title formatting is supported.

```
%front_cover_width%
%front_cover_height%
%front_cover_size% (nicely formatted image size in KB/MB)
%front_cover_format%
%front_cover_bytes% (raw image size)
```

Note that database records are attached to the `%path%` of each file so if files are renamed or moved, associated data will be orphaned and the files will need re-scanning. Like `foo_playcount`, database records have a lifetime of 4 weeks if they are not included in the `Media Library` or playlist.

Use the right click menu on any library/playlist selection to scan or clear existing info.

# foo_cover_resizer

This component has 4 options available via the right click menu.

### Cover Resizer/Resize

This option will resize existing embedded art. There is full support for preserving the image type if they are `JPG`/`PNG`/`TIFF`/`GIF`/`BMP` or you can convert any format to `JPG` or `PNG`. Reading and resizing `WEBP` is supported but it cannot be written back as `WEBP`. `JPG`/`PNG` can be chosen as an alternative.

### Cover Resizer/Attach image and Resize

This option lets you browse for an image file and will then resize (if needed) before attaching it to the current selection. It has the same `WEBP` limitations as the above method.

### Cover Utils/Convert front covers to JPG without resizing

Any art that is already `JPG` will be skipped.

### Cover Utils/Remove all except front

Since most people only want front covers, this is a handy method for removing all the other types.

If you don't like the context menu commands that appear for features that you won't use, they can be hidden using the main preferences (`File>Preferences>Display>Context menu`).
