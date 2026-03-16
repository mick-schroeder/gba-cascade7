# CASCADE7

CASCADE7 is an open-source Game Boy Advance puzzle game.

## Controls

- `LEFT/RIGHT`: move the drop cursor
- `A`: drop the next piece
- `SELECT`: start a new seeded board
- `START`: reset to an empty board

## Build

Clone with submodules so the local `butano/` dependency is present:

```sh
git clone --recurse-submodules <your-repo-url>
cd gba-cascade7
make
```

This project is intended for the standard Butano + `devkitARM` setup.

This Butano checkout currently compiles the framework itself with a newer C++ standard, so the project build uses the framework default even though the CASCADE7 game code is intentionally written in a C++17-friendly style.

## Structure

- `include/cascade7/constants.h`: shared board and UI constants
- `include/cascade7/types.h`: puzzle data types
- `include/cascade7/board.h`: 7x7 board state and mutation helpers
- `include/cascade7/rules.h`: clear-resolution and gravity logic
- `include/cascade7/game.h`: game loop state and input-driven actions
- `include/cascade7/renderer.h`: Butano text renderer for the prototype UI
- `src/*.cpp`: implementations and entry point

## Notes

The current prototype uses Butano common font assets instead of custom graphics so the architecture can stabilize before adding polished visuals, animations, and audio.
