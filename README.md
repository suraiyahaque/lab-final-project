# Image Manipulation Software

A desktop image editing application written in **C**, using the **IUP** (Portable User Interface) toolkit for the GUI. The application loads, manipulates, and saves 24-bit BMP images with operations including grayscale conversion, brightness adjustment, color inversion, flipping, rotation, cropping, and blur.

---

## Table of Contents

- [Architecture Overview](#architecture-overview)
- [Module Breakdown](#module-breakdown)
- [Features](#features)
- [Build Instructions](#build-instructions)
- [Dependencies](#dependencies)
- [Usage](#usage)
- [Project Structure](#project-structure)

---

## Architecture Overview

The project follows a modular C architecture with clear separation of concerns:

```
┌─────────────────────────────────────────────┐
│                  main.c                     │
│          (Entry point & bootstrap)          │
├─────────────┬───────────────┬───────────────┤
│   gui.c     │  operation.c  │    bmp.c      │
│  (IUP GUI)  │ (Image ops)   │ (BMP I/O)     │
├─────────────┴───────────────┴───────────────┤
│              image.c / image.h              │
│         (Core image data structure)         │
├─────────────────────────────────────────────┤
│          third_party/iup (IUP Toolkit)      │
└─────────────────────────────────────────────┘
```

**Data flow**: `main.c` initializes IUP, creates pointers for the working image and an undo buffer, passes them to the GUI layer, and enters the event loop. GUI callbacks invoke operations from `operation.c` and file I/O from `bmp.c`, all operating on the shared `Image` struct defined in `image.h`.

---

## Module Breakdown

### `image.h` / `image.c` — Core Image Representation
- **`Pixel`** struct: holds `r`, `g`, `b` (each `unsigned char`).
- **`Image`** struct: holds `width`, `height`, and a flat `Pixel *data` array.
- Functions: `image_create`, `image_free`, `image_copy`, `image_get_pixel`, `image_get_pixel_const`.
- Pixel addressing uses row-major order: `data[y * width + x]`.

### `bmp.h` / `bmp.c` — BMP File I/O
- Reads and writes **24-bit uncompressed BMP** files (BITMAPINFOHEADER, size = 40).
- Handles both top-down and bottom-up BMP storage.
- Uses `#pragma pack(push, 1)` for correct struct alignment of BMP headers.
- Row padding calculated as `(4 - (width * 3) % 4) % 4`.

### `operation.h` / `operation.c` — Image Operations
| Function | Type | Description |
|---|---|---|
| `apply_grayscale` | In-place | Luminance formula: `0.299R + 0.587G + 0.114B` |
| `apply_brightness` | In-place | Adds a value to each channel with clamping [0, 255] |
| `apply_invert` | In-place | `255 - channel` for each RGB component |
| `apply_horizontal_flip` | In-place | Swaps columns left ↔ right |
| `apply_vertical_flip` | In-place | Swaps rows top ↔ bottom |
| `apply_rotate_90` | Returns new | 90° clockwise rotation; swaps width/height |
| `apply_crop` | Returns new | Extracts a sub-rectangle |
| `apply_blur` | Returns new | 3×3 box blur (average of neighbors) |

### `gui.h` / `gui.c` — User Interface (IUP)
- Creates a window with a toolbar of 12 buttons (Open, Save, Grayscale, Brightness, Invert, H-Flip, V-Flip, Rotate, Crop, Blur, Undo, Exit).
- Manages global state via `static Image **current_image` and `static Image **undo_image`.
- Undo system stores a single previous state and swaps on undo.
- Uses IUP widgets: `IupDialog`, `IupCanvas`, `IupButton`, `IupHbox`, `IupVbox`, `IupLabel`.

### `main.c` — Entry Point
- Calls `IupOpen()`, `gui_init()`, `gui_run()` (IupMainLoop), and `gui_close()`.
- Declares `Image *image = NULL` and `Image *undo_image = NULL` as the application state.

### `compat.c` — MinGW Compatibility
- Provides linker symbols `__imp___argv` and `__imp___argc` for MinGW compatibility with IUP DLLs.

---

## Features

- **Load/Save** 24-bit BMP files
- **Grayscale** conversion (ITU-R BT.601 luminance)
- **Brightness** adjustment (additive, clamped)
- **Color Inversion** (negative)
- **Horizontal Flip** (mirror)
- **Vertical Flip** (mirror)
- **90° Rotation** (clockwise)
- **Crop** to a sub-region
- **Box Blur** (3×3 kernel)
- **Single-level Undo**

---

## Build Instructions

### Prerequisites
- **GCC** (MinGW-w64 recommended on Windows)
- **IUP Toolkit** DLLs (included in `third_party/iup/`)

### One-Click Execution (Windows Batch Scripts)
- **`run_mingw32.bat`**: For systems with standard 32-bit MinGW (`gcc`) configured in `PATH`.
- **`run.bat`**: Uses explicit `C:\MinGW\bin\gcc.exe` path.

Double-click either file to automatically terminate running instances, rebuild `app.exe`, and launch the software.

### Compile via Command Line
```bash
gcc -g main.c bmp.c gui.c image.c operation.c compat.c \
    -Ithird_party/iup/include \
    -Lthird_party/iup \
    -liupcontrols -liupcd -liup \
    -lgdi32 -lcomdlg32 -lcomctl32 -luuid -loleaut32 -lole32 \
    -o app.exe
```

### Compile via VS Code
Use the built-in build task (Ctrl+Shift+B) — configured in `.vscode/tasks.json`.

### Run
Ensure all `.dll` files are in the same directory as `app.exe`, then:
```bash
./app.exe
```

---

## Dependencies

| Dependency | Purpose | Included |
|---|---|---|
| [IUP 3.x](https://www.tecgraf.puc-rio.br/iup/) | Cross-platform GUI toolkit | ✅ (DLLs + headers) |
| MinGW-w64 GCC | C compiler | ❌ (must install) |
| Windows SDK libs | `gdi32`, `comdlg32`, etc. | ❌ (system libraries) |

---

## Project Structure

```
Image Manipulation Software/
├── .vscode/
│   └── tasks.json          # VS Code build task
├── third_party/
│   └── iup/
│       ├── include/         # IUP header files
│       └── *.dll            # IUP runtime libraries
├── main.c                   # Entry point
├── image.h / image.c        # Core image data structure
├── bmp.h / bmp.c            # BMP file read/write
├── operation.h / operation.c # Image manipulation algorithms
├── gui.h / gui.c            # IUP-based user interface
├── compat.c                 # MinGW linker compatibility
├── app.exe                  # Compiled binary
└── README.md                # This file
```

---

## License

No license file is provided. Contact the project author for usage terms.
