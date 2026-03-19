# CASCADE7

[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)

CASCADE7 is an open-source Game Boy Advance puzzle game inspired by Drop7.

![CASCADE7 Logo](CASCADE7-LOGO.png)

![CASCADE7 Screenshot](CASCADE7-SCREENSHOT.png)

## Overview

CASCADE7 is a 7x7 cascade puzzle game for Game Boy Advance. Each turn, the player drops a disc into one of seven columns. Discs are numbered `1-7` or blank, and numbered discs clear when their value matches the contiguous occupied run length in their row or column. Clears also strike adjacent blanks: the first hit cracks a blank disc, and the second hit reveals it as a numbered disc. After a set number of turns, a full row rises from the bottom of the board, increasing the pressure. There is no time limit, and the goal is to survive as long as possible by creating clears, triggering cascades, and pushing for high scores. Clearing the entire board awards a `70,000` point bonus.

## Controls

- `LEFT/RIGHT`: move the drop cursor
- `A`: drop the next disc
- `L/R`: snap to board edges
- `START` or `SELECT`: pause / help / about / new game

## Rules

- Board size is `7x7`
- Numbered discs clear when their value matches the same number of discs in their row or column
- Blank discs crack on the first adjacent hit and reveal into numbered discs on the next hit
- The bottom row rises after a countdown and speeds up as levels increase

## Install

<a href="https://github.com/mick-schroeder/gba-cascade7/releases">
  <img src="https://img.shields.io/badge/Download_ROMs_from_the_Releases_tab-8A2BE2?&logo=github" alt="Download ROM" width="500">
</a>

## Build

Clone with submodules so the local `butano/` dependency is present:

```sh
git clone --recurse-submodules https://github.com/mick-schroeder/gba-cascade7
cd gba-cascade7
make
```

This project targets a standard Butano + `devkitARM` setup.

The build produces `CASCADE7.gba`.


## Asset Workflow

- `graphics/`: sprite sheets and background graphics
- `audio/`: sound effect

## Project Layout

- [`include/cascade7/game.h`](include/cascade7/game.h): game state and flow
- [`include/cascade7/rules.h`](include/cascade7/rules.h): match and clear rules
- [`include/cascade7/scoring.h`](include/cascade7/scoring.h): score tuning
- [`src/cascade7_game.cpp`](src/cascade7_game.cpp): runtime logic, RNG, progression, input
- [`src/cascade7_rules.cpp`](src/cascade7_rules.cpp): clear and reveal resolution
- [`src/cascade7_renderer.cpp`](src/cascade7_renderer.cpp): board, HUD, feedback, menus

## Author

- [Mick Schroeder](https://www.mickschroeder.com)

## Licensing & Credits

- © 2026 Mick Schroeder, LLC. — [MIT License](./LICENSE)  
- “CASCADE7” name and logo are trademarks of Mick Schroeder, LLC.  
- [Engine framework: Butano](https://github.com/GValiente/butano)

## Trademark Notice

“CASCADE7”, the game logo, and related marks are trademarks of Mick Schroeder, LLC.  
You may not use these names, logos, or identifying marks to distribute modified  
versions of this software or to publish derivative apps without express written permission.
