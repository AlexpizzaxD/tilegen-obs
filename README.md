# TileGen OBS

TileGen OBS is an OBS Studio plugin for generating animated pattern overlays (grids of shapes, images, characters, and more).

## Inspiration

This project is inspired by **OBS Shader - StreamUP Pattern Tiler** by [Andilippi](https://andilippi.co.uk). It is an original implementation developed with AI assistance for LaViduka.

## Features

- Multiple grid patterns: square, brick, hexagonal
- Shapes: dot, ring, square, diamond, plus, polygon, star
- Image / OBS Source as shape
- Character atlas support (any character from a sprite sheet)
- Secondary color with blend modes
- Per-cell variation, twinkle, drift, pulse
- Scroll and offset controls

## Building

### Requirements

- Windows 10/11
- Build Tools for Visual Studio 2022 (or newer)
- CMake 3.28+
- Git

### Build with VSCode

1. Open this folder in VSCode.
2. Install extensions: C/C++, CMake Tools.
3. Run `CMake: Configure` and select the Visual Studio Build Tools kit.
4. Run `CMake: Build`.

### Build from command line

```powershell
cmake --preset windows-x64
cmake --build --preset windows-x64
```

## Installation

Copy the generated `.dll` to `obs-plugins/64bit/` and the `data` folder to `data/obs-plugins/tilegen-obs/` inside your OBS Studio installation directory.

## License

This project is licensed under the GNU General Public License v2.0.
