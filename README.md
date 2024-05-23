# Bladebomber

This game was created during the first RIVES game jam to demonstrate RIV API.
This was intentionally made in pure C to demonstrate RIV API and capabilities.

## Notes

The code was typed fast during a game jam in 3 days, it should not be used seriously as base for a serious game.
Files:
 - blabebomber.c - the game implementation
 - gfx.h - configuration of objects and its graphics
 - sfx.h - configuration of sound effects
 - maps.h - all level maps
 - utils.h - some math utilities

## Levels

The maps were designed inside [Tiled map editor](https://www.mapeditor.org/) and can be found in `maps` directory, all of them were converted to `.lua` files, then to `maps.h` file using a minimal lua script in `maps/conv.lua`.

## Authors

- edubart - programming & sound design
- isabella - level design & pixel art

The original sprites are based on https://o-lobster.itch.io/simple-dungeon-crawler-16x16-pixel-pack free pixel pack, and credits goes to o_lobster .
