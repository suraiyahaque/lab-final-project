# 🐛 Bug Report — Image Manipulation Software

A thorough code audit of every source file in this project. **11 bugs** found, ranked by severity.

---

## Summary Table

| ID | File | Severity | Status | Type | Description |
|---|---|---|---|---|---|
| BUG-1 | `bmp.c` | 🔴 Critical | ✅ **FIXED** | Data corruption | Padding skip inside per-pixel loop garbles entire image |
| BUG-2 | `gui.c` | 🟡 Moderate* | ✅ **FIXED** | Code cleanup | `status_label` duplicate declaration removed |
| BUG-3 | `gui.c` | 🔴 Critical | ✅ **FIXED** | Variable shadowing | Static globals remain NULL; locals shadow them |
| BUG-4 | `bmp.c` | 🔴 Critical | ✅ **FIXED** | Undefined behavior | Uninitialized `r`, `g`, `b` used on `fread` failure |
| BUG-5 | `gui.c` | 🟠 Major | ✅ **FIXED** | Dead code | All callbacks were stubs; full functionality now wired up |
| BUG-6 | `main.c` | 🟠 Major | ✅ **FIXED** | Memory leak | Image pointers never freed on exit |
| BUG-7 | `gui.c` | 🟠 Major | ✅ **FIXED** | Data safety | `save_undo()` now atomically preserves buffer on OOM |
| BUG-8 | `operation.c` | 🟡 Moderate | ✅ **FIXED** | Const violation | `const` cast-away; uses `image_get_pixel_const` |
| BUG-9 | `image.c` | 🟡 Moderate | ✅ **FIXED** | Integer overflow | `width * height` arithmetic guards added |
| BUG-10 | `gui.c` | 🟡 Moderate | ✅ **FIXED** | Design / UX | Single-level undo with swap semantics and canvas refresh |
| BUG-11 | `tasks.json` | 🟡 Moderate | ✅ **FIXED** | Build config | `compat.c` and 32-bit compiler path configured |

*\* Note: BUG-2 was reassessed from Critical to Moderate after deeper C standard analysis (tentative definitions).*

---

## 🔴 Critical Bugs

---

### BUG-1 — `bmp.c`: Row padding applied per-pixel instead of per-row

**File:** `bmp.c`, lines 127–179  
**Severity:** 🔴 Critical — **corrupts every loaded BMP image**

#### Problem

The pixel-reading loop inside `bmp_load()` contains **duplicated and misplaced logic**. Inside the inner `for (x = 0; x < width; x++)` loop:

1. **Lines 143–155** — A duplicated block reads B, G, R bytes and sets the pixel.
2. **Lines 157–159** — Skips padding bytes **inside the per-pixel loop**. Padding should only be skipped once **per row**, not once per pixel.
3. **Lines 160–171** — The **original** pixel read: gets the pixel again and sets `r`, `g`, `b` a second time (using potentially stale values from the first read).
4. **Line 174** — Skips padding **again** at the row level (the original correct location).

#### What happens at runtime

For every pixel (except the first in each row), the file position jumps forward by `padding` bytes *before* reading the next pixel's color data. Pixel data is read from **wrong file offsets**, producing a completely garbled image. The padding is also double-applied (once inside the x-loop, once after the x-loop).

#### Buggy code

```c
for (x = 0; x < width; x++)
{
    unsigned char b;
    unsigned char g;
    unsigned char r;
    Pixel *pixel;

    // ❌ DUPLICATE BLOCK START — should be removed
    if (fread(&b, sizeof(unsigned char), 1, file) ==1 &&
        fread(&g, sizeof(unsigned char), 1, file) == 1 &&
        fread(&r, sizeof(unsigned char), 1, file) == 1)
    {
        pixel=image_get_pixel(image, x,image_y);
        if(pixel!=NULL){
            pixel->b=b;
            pixel->g=g;
            pixel->r=r;
        }
    }

        if(padding>0){               // ❌ WRONG — padding skip inside x-loop
            fseek(file,padding,SEEK_CUR);
        }
    // ❌ DUPLICATE BLOCK END

    pixel = image_get_pixel(image, x, image_y);  // Original code below

    if (pixel == NULL)
    {
        image_free(image);
        fclose(file);
        return NULL;
    }

    pixel->r = r;   // ⚠️ Uses r,g,b but fread may have failed
    pixel->g = g;
    pixel->b = b;
}

if (fseek(file, padding, SEEK_CUR) != 0)   // Original padding skip (per-row)
{
    ...
}
```

#### Fix

Remove the entire duplicated block (lines 143–159). Keep only the original per-pixel read (lines 160–171) and per-row padding skip (line 174). Also add proper `fread` calls before lines 169–171:

```c
for (x = 0; x < width; x++)
{
    unsigned char b, g, r;
    Pixel *pixel;

    if (fread(&b, sizeof(unsigned char), 1, file) != 1 ||
        fread(&g, sizeof(unsigned char), 1, file) != 1 ||
        fread(&r, sizeof(unsigned char), 1, file) != 1)
    {
        image_free(image);
        fclose(file);
        return NULL;
    }

    pixel = image_get_pixel(image, x, image_y);

    if (pixel == NULL)
    {
        image_free(image);
        fclose(file);
        return NULL;
    }

    pixel->r = r;
    pixel->g = g;
    pixel->b = b;
}

if (fseek(file, padding, SEEK_CUR) != 0)
{
    image_free(image);
    fclose(file);
    return NULL;
}
```

---

### BUG-2 — `gui.c`: `status_label` declared twice; `set_status()` crashes

**File:** `gui.c`, line 12 and line 256  
**Severity:** 🔴 Critical — **NULL pointer dereference / crash**

#### Problem

`status_label` is declared at **two** file-scope locations:

```c
// Line 12
static Ihandle *status_label;

// Line 256
static Ihandle *status_label = NULL;
```

Inside `gui_init()` at line 372:
```c
status_label = IupLabel("No image loaded");
```

This assignment goes to the **second** declaration (line 256) because it is the one in scope at that point in the file. However, `set_status()` is defined at lines 14–17, where the **first** declaration (line 12) is in scope — and that one is **never assigned**.

When `undo_callback()` calls `set_status("Undo completed")`, it passes a NULL `Ihandle*` to `IupSetAttribute()`, which will crash.

#### Fix

Remove the duplicate declaration at line 256. Keep only the one at line 12:

```diff
  static Ihandle *dialog = NULL;
  static Ihandle *main_box = NULL;
  static Ihandle *button_box = NULL;
  static Ihandle *canvas = NULL;
- static Ihandle *status_label = NULL;
```

---

### BUG-3 — `gui.c`: Local variables shadow static globals

**File:** `gui.c`, lines 252–256 vs lines 349, 367, 374, 384  
**Severity:** 🔴 Critical — **static globals remain NULL**

#### Problem

Four file-scope statics are declared at lines 252–256:

```c
static Ihandle *dialog = NULL;
static Ihandle *main_box = NULL;
static Ihandle *button_box = NULL;
static Ihandle *canvas = NULL;
```

But inside `gui_init()`, **new local variables** with the same names are declared, shadowing the statics:

```c
Ihandle * button_box = IupHbox(...);      // line 349 — shadows static
Ihandle *  canvas = IupCanvas(NULL);       // line 367 — shadows static
Ihandle *  main_box = IupVbox(...);        // line 374 — shadows static
Ihandle *dialog = IupDialog(main_box);     // line 384 — shadows static
```

The statics remain `NULL`. Any future code or callback that accesses the static `dialog`, `canvas`, `main_box`, or `button_box` will dereference a NULL pointer.

#### Fix

Remove the `Ihandle *` type prefix on the shadowing lines so they assign to the existing statics:

```diff
- Ihandle * button_box = IupHbox(
+ button_box = IupHbox(

- Ihandle *  canvas = IupCanvas(NULL);
+ canvas = IupCanvas(NULL);

- Ihandle *  main_box = IupVbox(
+ main_box = IupVbox(

- Ihandle *dialog = IupDialog(main_box);
+ dialog = IupDialog(main_box);
```

---

### BUG-4 — `bmp.c`: Uninitialized variables `r`, `g`, `b` used on `fread` failure

**File:** `bmp.c`, lines 138–171  
**Severity:** 🔴 Critical — **undefined behavior**

#### Problem

The variables `b`, `g`, `r` are declared at lines 138–140 but **never initialized**:

```c
unsigned char b;
unsigned char g;
unsigned char r;
```

If the `fread` block at lines 143–145 fails (returns ≠ 1), execution falls through to lines 169–171 where these **uninitialized** values are written to the pixel:

```c
pixel->r = r;   // ❌ garbage value
pixel->g = g;   // ❌ garbage value
pixel->b = b;   // ❌ garbage value
```

This is undefined behavior per the C standard.

#### Fix

Initialize the variables, or properly handle `fread` failure by returning NULL:

```c
unsigned char b = 0;
unsigned char g = 0;
unsigned char r = 0;
```

Or better — treat `fread` failure as a fatal error (see BUG-1 fix).

---

## 🟠 Major Logic Errors

---

### BUG-5 — `gui.c`: All operation callbacks are non-functional stubs

**File:** `gui.c`, lines 35–223  
**Severity:** 🟠 Major — **the entire application is non-functional**

#### Problem

Every single GUI callback is a placeholder that only shows an `IupMessage` dialog:

| Callback | Shows |
|---|---|
| `open_callback` | `"BMP loading will be connected here."` |
| `save_callback` | `"BMP saving will be connected here."` |
| `grayscale_callback` | `"Grayscale operation will be connected here."` |
| `brightness_callback` | `"Brightness operation will be connected here."` |
| `invert_callback` | `"Invert operation will be connected here."` |
| `horizontal_flip_callback` | `"Horizontal flip operation will be connected here."` |
| `vertical_flip_callback` | `"Vertical flip operation will be connected here."` |
| `rotate_callback` | `"Rotate operation will be connected here."` |
| `crop_callback` | `"Crop operation will be connected here."` |
| `blur_callback` | `"Blur operation will be connected here."` |

None of them call the actual functions in `bmp.c` (`bmp_load`, `bmp_save`) or `operation.c` (`apply_grayscale`, `apply_brightness`, etc.). Those modules are compiled and linked but are **100% dead code**.

#### Impact

The application displays a window with 12 buttons, but clicking any of them (except Exit and Undo) does absolutely nothing useful. The ~600 lines of BMP I/O and image processing code are never executed.

#### Fix

Wire up each callback to its corresponding function. For example:

```c
static int grayscale_callback(Ihandle *ih)
{
    (void)ih;

    if (*current_image == NULL)
    {
        IupMessage("Error", "No image is loaded.");
        return IUP_DEFAULT;
    }

    save_undo();
    apply_grayscale(*current_image);
    set_status("Grayscale applied");

    return IUP_DEFAULT;
}
```

---

### BUG-6 — `main.c`: Image memory never freed on exit

**File:** `main.c`, lines 24–34  
**Severity:** 🟠 Major — **memory leak**

#### Problem

`main()` allocates `Image *image` and `Image *undo_image` (via GUI callbacks during the event loop), but after `gui_run()` returns and before `return 0`, neither is freed.

```c
Image *image = NULL;
Image *undo_image = NULL;
gui_init(&image, &undo_image);
gui_run();
gui_close();
// ❌ image and undo_image are never freed
return 0;
```

#### Fix

```c
gui_close();
image_free(image);
image_free(undo_image);
return 0;
```

---

### BUG-7 — `gui.c`: `save_undo()` return value ignored

**File:** `gui.c`, lines 75, 95, 115, 135, 155, 175, 195, 215  
**Severity:** 🟠 Major — **silent data loss**

#### Problem

Every operation callback calls `save_undo()` but **ignores its return value**:

```c
save_undo();   // returns 0 on failure, 1 on success — but never checked
```

Inside `save_undo()`, the **existing** undo image is freed first (line 23), then `image_copy()` is called. If `image_copy()` fails (out of memory), the function returns 0 — but the caller proceeds with the destructive operation anyway. The user's previous undo state is **destroyed** and the new one was never saved.

#### Fix

Check the return value and warn the user:

```c
if (!save_undo())
{
    IupMessage("Warning", "Could not save undo state. Continue anyway?");
}
```

---

## 🟡 Moderate Issues

---

### BUG-8 — `operation.c`: `const` cast-away using `image_get_pixel`

**File:** `operation.c`, lines 178, 227, 288  
**Severity:** 🟡 Moderate — **const correctness violation**

#### Problem

Three functions (`apply_rotate_90`, `apply_crop`, `apply_blur`) accept `const Image *image` but cast it to `Image *` to call `image_get_pixel`:

```c
source_pixel = image_get_pixel((Image *)image, x, y);   // ❌ casts away const
```

The `image_get_pixel_const()` function exists specifically for this purpose but is never used in `operation.c`.

#### Fix

```c
const Pixel *source_pixel = image_get_pixel_const(image, x, y);
```

---

### BUG-9 — `image.c`: Integer overflow in pixel buffer allocation

**File:** `image.c`, line 20  
**Severity:** 🟡 Moderate — **heap buffer overflow for large images**

#### Problem

```c
image->data = malloc(width * height * sizeof(Pixel));
```

`width` and `height` are `int`. If `width * height` exceeds `INT_MAX` (e.g., 50000 × 50000 = 2.5 billion), the multiplication overflows, producing a small positive or negative value. `malloc` allocates a tiny buffer, and the subsequent `memset` and pixel writes cause a **heap buffer overflow**.

#### Fix

```c
size_t total = (size_t)width * (size_t)height;
if (total > SIZE_MAX / sizeof(Pixel))
    return NULL;   // overflow guard

image->data = malloc(total * sizeof(Pixel));
```

---

### BUG-10 — `gui.c`: Single-level undo with swap semantics

**File:** `gui.c`, lines 19–33 and 225–244  
**Severity:** 🟡 Moderate — **confusing UX design flaw**

#### Problem

The undo system only stores a **single** previous state. Calling undo **swaps** the current and undo images — meaning pressing Undo twice returns to the state *after* the operation, not before it. This creates redo-like behavior, not true undo.

Multiple successive operations each destroy the previous undo state. Users expecting multi-level undo will lose work.

#### Suggestion

Implement an undo stack (linked list of `Image*`) for proper multi-level undo, or at minimum document the single-undo limitation in the UI.

---

### BUG-11 — `tasks.json`: `compat.c` missing from build

**File:** `.vscode/tasks.json`, lines 10–14  
**Severity:** 🟡 Moderate — **potential linker failure**

#### Problem

The VS Code build task compiles:
- `main.c`, `bmp.c`, `gui.c`, `image.c`, `operation.c`

But **not** `compat.c`, which provides MinGW compatibility symbols:

```c
__attribute__((section(".data"))) char **__imp___argv __asm__("__imp___argv") = 0;
__attribute__((section(".data"))) int __imp___argc __asm__("__imp___argc") = 0;
```

Without this file, linking will fail on MinGW configurations that expect these symbols from the CRT.

#### Fix

Add `compat.c` to the build args:

```diff
  "${workspaceFolder}/operation.c",
+ "${workspaceFolder}/compat.c",
  "-I${workspaceFolder}/third_party/iup/include",
```
