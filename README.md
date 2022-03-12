## Requirements & Download

https://github.com/marc2k3/fb2k-component/releases/latest

# foo_run_main

## Usage

### Executing main menu commands

Unlike the built in command line handler/`foo_runcmd`, this component has full support for dynamically generated menu commands meaning you can use `Edit` commands and switch playlists, change output devices etc.

To avoid ambiguity with common names that might appear more than once under different sub menus, you must supply the full path to the command. Examples:

```
foobar2000.exe /run_main:Edit/Sort/Randomize
foobar2000.exe /run_main:Library/Search

// use double quotes when command contains spaces
foobar2000.exe /run_main:"Playback/Device/Primary Sound Driver"
```

### Selecting and playing playlist items

The latest version adds 2 additional commands, `/select_item` and `/select_item_and_play`.

Note that this example requires `Playback follows cursor` to be enabled.

```
foobar2000.exe /select_item:5 /play
```

But there is no such requirement when using `/select_item_and_play`.

```
foobar2000.exe /select_item_and_play:5
```

When adding a track using the native `/add` command, you can take advantage of adding an optional delay like this.

```
foobar2000.exe /add "d:\path\to\blah.mp3" /select_item_and_play:1:1500
```

Without it, the command would execute before the added file had been processed. The delay is in milliseconds. I found during testing `1000` was too low a value but `1500` worked for a single track. YMMV depending on your system/number of files added.

If you add multiple files, consider using a fresh playlist and using 1 as the item index. If adding a single file, you can set the item index far in excess of the actual playlist item count to ensure the last item is selected/played.

# foo_cover_info

## Usage

Because it's not possible to query files for embedded album art within foobar2000, this component scans a selection of files and stores the results in a database. Updating of data is not automatic so if you add/remove art at a later date, it's up to you to run the scan again. Data is available in the following fields which are available wherever title formatting is supported.

```
%front_cover_width%
%front_cover_height%
%front_cover_size% (nicely formatted image size in KB/MB)
%front_cover_format%
%front_cover_bytes% (raw image size)
```

Note that database records are attached to the `%path%` of each file. As of `v0.1.1`, records are now preserved if files are renamed/moved with [File operations](https://wiki.hydrogenaud.io/index.php?title=Foobar2000:File_operations). If files are renamed externally, database records would be orphaned and the files would need scanning again. Like `foo_playcount`, database records have a lifetime of 4 weeks if they are not included in the `Media Library` or playlist.

Use the right click menu on any library/playlist selection to scan or clear existing info.

# foo_cover_resizer

## Usage

This component has 5 options available via the right click menu on any playlist/library selection. As of `v0.2.0`, writing `WEBP` is now supported.

### Cover Resizer/Resize

This option will resize existing embedded art. Reading most common image types is supported but you must choose `JPG`,`PNG` or `WEBP` when saving.

### Cover Resizer/Browse for file, resize and attach

This option lets you browse for an image file and will then resize it before attaching it to the current selection. Images already smaller than the specified max size will not be processed. You should attach those via native `foobar2000` option under the `Tagging` menu.

### Cover Utils/Convert without resizng

Converts existing embedded art without resizing. You must choose `JPG`, `PNG` or `WEBP`.

### Cover Utils/Browse for file, convert and attach

Use the file picker and then choose to convert the file to `JPG`, `PNG` or `WEBP`.

### Cover Utils/Remove all except front

Since most people only want front covers, this is a handy method for removing all the other types.

If you don't like the context menu commands that appear for features that you won't use, they can be hidden using the main preferences (`File>Preferences>Display>Context menu`).
