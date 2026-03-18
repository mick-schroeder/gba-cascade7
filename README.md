# CASCADE7

CASCADE7 is an open-source Game Boy Advance puzzle game.

![CASCADE7 Logo](CASCADE7-LOGO.png)

![CASCADE7 Screenshot](CASCADE7-SCREENSHOT.png)

## Controls

- `LEFT/RIGHT`: move the drop cursor
- `A`: drop the next disc
- `START`: options

## Build

Clone with submodules so the local `butano/` dependency is present:

```sh
git clone --recurse-submodules https://github.com/mick-schroeder/gba-cascade7
cd gba-cascade7
make
```

This project is intended for the standard Butano + `devkitARM` setup.
