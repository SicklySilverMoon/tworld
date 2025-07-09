# Tile World

Tile World is an emulation of the game "Chip's Challenge" for the Atari Lynx,
created by Chuck Sommerville, and later ported to MS Windows by Microsoft (among
other ports).

## Important Note

Tile World is an emulation of the "Chip's Challenge" game engines only. It does
not come with the CHIPS.dat file that contains the original level set. That
file, which is copyrighted and cannot be freely distributed, was originally
distributed with the MS version of "Chip's Challenge". If you have a copy of
this version of the game, you can use that file to play the original games in
Tile World. If you do not have a copy of this file, however, you can still play
Tile World with the many freely available level files created by fans of the
original game.

## Getting Tile World

### Prebuilt (Windows)

Extract the contents of the release archive into its own folder, eg.
`C:\Users\you\Documents\tworld`. If you have a copy of CHIPS.dat, copy it into
`data` subfolder. This will let you play the original Chip's Challenge levelset.

If you have any other levelset (.dat) files you would like to play in Tile
World, copy those to the `data` subfolder also. All sets in that folder should
appear in the main menu.

To run the game, execute `tworld.exe` in the main Tile World directory, or make
a shortcut for it.

### Building (Linux/Windows/macOS)

Before building, ensure you have CMake, a C compiler, the SDL2 and Qt5/6
libraries (with developement support).

Build Tile World as a usual CMake program using the following commands:

```sh
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Relase ..
cmake --build .
```

Running `make install` (as root) will install a `tworld2` binary and all the
necessary resources.

On Unix-like systems, addings sets or changing any resources, unfortunately,
requires root permission, as those are placed in `/usr/local/share/tworld`. This
may be changed in a future release.

## Levelsets

As mentioned above, Tile World does not come with the original "Chip's
Challenge" levels for copyright reasons. If you do have a copy of the original
game, copy the CHIPS.dat file into the `data` folder to play it.

This distribution/repository comes with six levelsets: intro, CCLP1, CCLP2,
CCLP3, CCLP4, CCLP5. "intro" is a short introduction to the elements of the
game. CCLPs (Chip's Challenge Level Packs) 1 through 5 are sets put together by
the community, containing levels voted on by fans. CCLP1 is meant as a
replacement for CHIPS.dat, while the others are considerably harder than the
original set.

Other user-made sets can be found on https://sets.bitbusters.club.

## Making levels

New levels for Tile World can be made using level editors. The two most popular
editors are CCEdit (part of [CCTools](https://cctools.zrax.net/)), and
[CCCreator](https://cccreator.bitbusters.club/).

## Resources

The community scoreboard for the original game and the CCLPs can be found on
https://scores.bitbusters.club.

Most of the community resources can be found on https://bitbusters.club,
including the [community Discord server](https://discord.gg/Xd4dUY9), the
[Chip Wiki](https://wiki.bitbusters.club), the
[Yahoo Groups archive](https://bitbusters.club/yahoo), and levelsets, editors,
and emulators for the sequel, Chip's Challenge 2.

Optimized TWS solutions for CC1 and CCLPs can be found at https://davidstolp.com/old/chips/tws/.

A list of ports of Tile World to various can be found at https://wiki.bitbusters.club/Tile_World#Ports.

The original Tile World 1 homepage is at
http://muppetlabs.com/~breadbox/software/tworld.

The source code repository for the latest fork can be found at
https://github.com/SicklySilverMoon/tworld.

The Tile World 2 homepage is at https://tw2.bitbusters.club.

## License

Tile World is copyright (C) 2001-2024 by Brian Raiter, Madhav Shanbhag, and Eric
Schmidt. This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2 of the License, or (at your option) any
later version. This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License,
included in this distribution in the file COPYING, for more details.

## Bugs

Bug reports are always appreciated and can be submitted via GitHub Issues at
https://github.com/SicklySilverMoon/tworld/issues/new. The list of known bugs
can be found in the BUGS file in the source repository.

## Credits

Tile World was written by Brian Raiter.

Tile World 2 - which simply improves upon the user interface of the original
Tile World program - was developed by Madhav Shanbhag.

Version 2.1 was produced by Eric Schmidt.

Version 2.3 was produced by Michael Hansen (Zrax), ChosenID, David Stolp
(pieguy), A Sickly Silver Moon, G lander, and Eevee.

The sound effects included in this distribution were created by Brian Raiter,
with assistance from SoX. Brian Raiter has explictly placed these files in the
public domain.

The tile images included in this distribution were created by Anders Kaseorg,
with assistance from POV-Ray. Anders Kaseorg has explicitly placed these files
in the public domain.

The introductory set of levels included in this distribution were created by
Brian Raiter. Brian Raiter has explictly placed these levels in the public
domain.

Thomas Harte and Michael Hansen have developed Mac OSX ports of Tile World and
Tile World 2 respectively.

Thanks to "The Architect" for his corrections to the font bitmap file.

"Chip's Challenge" was designed by Chuck Sommerville, who is also the author of
the original Lynx program.

"Chip's Challenge" is a registered trademark of Alpha Omega Publications.

Creating this program would have been flatly impossible without the help of
several fans of "Chip's Challenge". The author would particularly like to
acknowledge Anders Kaseorg for sharing the fruits of his investigations into the
game logic of the MS version and for being an effective bug hunter, Chuck
Sommerville for his pointers regarding the game logic of the Lynx version and
his unfailing support of this project, and "CCExplore" for his in-depth
investigations of esoteric game behavior.

Many other regulars of the annexcafe.chips.challenge newsgroup assisted with bug
reports, suggestions, and all-around encouragement. Their help is gratefully
acknowledged.

The anonymous author of the document describing the .dat file format, Don
Gregory, the "Charter Chipsters", and the contributors to the CC AVI library all
deserve mention as well -- this program would never have been written without
the information they made freely available.

Last but not least, a tip of the hat to John K. Elion for writing ChipEdit.
