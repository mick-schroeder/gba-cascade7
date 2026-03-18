# CASCADE7

CASCADE7 is an open-source Game Boy Advance puzzle game built with Butano.

![CASCADE7 Logo](CASCADE7-LOGO.png)

![CASCADE7 Screenshot](CASCADE7-SCREENSHOT.png)

## Overview

CASCADE7 is a 7x7 cascade puzzle game for Game Boy Advance. Each turn, the player drops a disc into one of seven columns. Discs are numbered `1-7` or blank, and numbered discs clear when their value matches the same number of discs in their row or column. Clears also strike adjacent blanks: the first hit cracks a blank disc, and the second hit reveals it as a numbered disc. After a set number of turns, a full row rises from the bottom of the board, increasing the pressure. There is no time limit, and the goal is to survive as long as possible by creating clears, triggering cascades, and pushing for high scores. Clearing the entire board awards a `70,000` point bonus.

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

## Credits

- [Mick Schroeder](https://www.mickschroeder.com)
- [Engine framework: Butano](https://github.com/GValiente/butano)

## License

Released under the [MIT License](LICENSE).
