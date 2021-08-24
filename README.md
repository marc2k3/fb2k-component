# Minimum requirements

`Windows 7` or later, `foobar2000` `v1.6` or later. 

# Installation

https://wiki.hydrogenaud.io/index.php?title=Foobar2000:How_to_install_a_component

## Download

https://github.com/marc2k3/fb2k-component/releases

## foo_run_main

Unlike the built in command line handler/`foo_runcmd`, this component has full support for dynamically generated menu commands meaning you can use `Edit` commands and switch playlists, change output devices etc.

You must supply the full path to the command. Examples:

```
foobar2000 /run_main:Edit/Sort/Randomize
foobar2000 /run_main:"Playback/Device/Primary Sound Driver" << use double quotes when command contains spaces
foobar2000 /run_main:Library/Search
```

## foo_cover_info

### Download

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

Note that database records are attached to the `%path%` of each file so if files are renamed or moved, associated data will be orphaned and the files will need re-scanning.

Use the right click on any library/playlist selection to scan or clear existing info.

## foo_cover_resizer

This component resizes embedded album art. There is full support for preserving the image type if they are JPG/PNG/TIFF/GIF/BMP or you can convert any format to JPG or PNG. Reading and resizing WEBP is supported but it cannot be written back as WEBP. JPG/PNG can be chosen as an alternative.

Additionally, there is a further option to convert the front cover to JPG without resizing. Lastly, there is an option to remove all embedded art except the front cover as this is the most common type people want to keep. Use the right click on any library/playlist selection to access the various options.
