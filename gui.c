#include "gui.h"
#include "image.h"
#include "bmp.h"
#include "operation.h"

#include "third_party/iup/include/iup.h"
#include "third_party/iup/include/iupcontrols.h"
#include "third_party/iup/include/iupdraw.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Image **current_image;
static Image **undo_image;

static Ihandle *status_label;

static Ihandle *dialog = NULL;
static Ihandle *main_box = NULL;
static Ihandle *button_box = NULL;
static Ihandle *canvas = NULL;

static void set_status(const char *text)
{
    IupSetAttribute(status_label, "TITLE", text);
}

static int save_undo(void)
{
    Image *copy;

    if (*current_image == NULL)
        return 0;

    copy = image_copy(*current_image);
    if (copy == NULL)
    {
        IupMessage("Warning", "Failed to allocate memory to save undo state.");
        return 0;
    }

    if (*undo_image != NULL)
    {
        image_free(*undo_image);
        *undo_image = NULL;
    }

    *undo_image = copy;
    return 1;
}

static void refresh_canvas(void)
{
    if (canvas != NULL)
        IupUpdate(canvas);
}

static int canvas_action_cb(Ihandle *ih)
{
    int canvas_w, canvas_h;

    IupDrawBegin(ih);
    IupDrawGetSize(ih, &canvas_w, &canvas_h);

    if (*current_image != NULL)
    {
        Ihandle *img;
        char status_text[128];

        img = IupImageRGB(
            (*current_image)->width,
            (*current_image)->height,
            (const unsigned char *)(*current_image)->data
        );

        IupSetHandle("_DISPLAY_IMAGE", img);
        IupDrawImage(ih, "_DISPLAY_IMAGE", 0, 0, -1, -1);
        IupDestroy(img);

        sprintf(status_text, "Image: %d x %d pixels",
                (*current_image)->width,
                (*current_image)->height);
        set_status(status_text);
    }

    IupDrawEnd(ih);
    return IUP_DEFAULT;
}

static int open_callback(Ihandle *ih)
{
    Ihandle *filedlg;
    int status;
    char *filename;
    Image *loaded;

    (void)ih;

    filedlg = IupFileDlg();
    IupSetAttribute(filedlg, "DIALOGTYPE", "OPEN");
    IupSetAttribute(filedlg, "TITLE", "Open BMP File");
    IupSetAttribute(filedlg, "EXTFILTER",
                    "BMP Files|*.bmp|All Files|*.*|");
    IupPopup(filedlg, IUP_CENTERPARENT, IUP_CENTERPARENT);

    status = IupGetInt(filedlg, "STATUS");
    if (status == -1)
    {
        IupDestroy(filedlg);
        return IUP_DEFAULT;
    }

    filename = IupGetAttribute(filedlg, "VALUE");
    loaded = bmp_load(filename);
    IupDestroy(filedlg);

    if (loaded == NULL)
    {
        IupMessage("Error",
                   "Failed to load BMP file.\n"
                   "Only 24-bit uncompressed BMP is supported.");
        return IUP_DEFAULT;
    }

    if (*current_image != NULL)
        image_free(*current_image);

    if (*undo_image != NULL)
    {
        image_free(*undo_image);
        *undo_image = NULL;
    }

    *current_image = loaded;
    refresh_canvas();

    return IUP_DEFAULT;
}

static int save_callback(Ihandle *ih)
{
    Ihandle *filedlg;
    int status;
    char *filename;

    (void)ih;

    if (*current_image == NULL)
    {
        IupMessage("Error", "No image is loaded.");
        return IUP_DEFAULT;
    }

    filedlg = IupFileDlg();
    IupSetAttribute(filedlg, "DIALOGTYPE", "SAVE");
    IupSetAttribute(filedlg, "TITLE", "Save BMP File");
    IupSetAttribute(filedlg, "EXTFILTER",
                    "BMP Files|*.bmp|All Files|*.*|");
    IupSetAttribute(filedlg, "EXTDEFAULT", "bmp");
    IupPopup(filedlg, IUP_CENTERPARENT, IUP_CENTERPARENT);

    status = IupGetInt(filedlg, "STATUS");
    if (status == -1)
    {
        IupDestroy(filedlg);
        return IUP_DEFAULT;
    }

    filename = IupGetAttribute(filedlg, "VALUE");

    if (bmp_save(filename, *current_image) != 0)
    {
        IupDestroy(filedlg);
        IupMessage("Error", "Failed to save BMP file.");
        return IUP_DEFAULT;
    }

    IupDestroy(filedlg);
    set_status("Image saved successfully");

    return IUP_DEFAULT;
}

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
    refresh_canvas();
    set_status("Grayscale applied");

    return IUP_DEFAULT;
}

static int brightness_callback(Ihandle *ih)
{
    int value = 0;

    (void)ih;

    if (*current_image == NULL)
    {
        IupMessage("Error", "No image is loaded.");
        return IUP_DEFAULT;
    }

    if (!IupGetParam("Brightness", NULL, NULL,
                     "Adjustment (-255 to 255): %i\n", &value))
    {
        return IUP_DEFAULT;
    }

    save_undo();
    apply_brightness(*current_image, value);
    refresh_canvas();
    set_status("Brightness adjusted");

    return IUP_DEFAULT;
}

static int invert_callback(Ihandle *ih)
{
    (void)ih;

    if (*current_image == NULL)
    {
        IupMessage("Error", "No image is loaded.");
        return IUP_DEFAULT;
    }

    save_undo();
    apply_invert(*current_image);
    refresh_canvas();
    set_status("Colors inverted");

    return IUP_DEFAULT;
}

static int horizontal_flip_callback(Ihandle *ih)
{
    (void)ih;

    if (*current_image == NULL)
    {
        IupMessage("Error", "No image is loaded.");
        return IUP_DEFAULT;
    }

    save_undo();
    apply_horizontal_flip(*current_image);
    refresh_canvas();
    set_status("Horizontal flip applied");

    return IUP_DEFAULT;
}

static int vertical_flip_callback(Ihandle *ih)
{
    (void)ih;

    if (*current_image == NULL)
    {
        IupMessage("Error", "No image is loaded.");
        return IUP_DEFAULT;
    }

    save_undo();
    apply_vertical_flip(*current_image);
    refresh_canvas();
    set_status("Vertical flip applied");

    return IUP_DEFAULT;
}

static int rotate_callback(Ihandle *ih)
{
    Image *rotated;

    (void)ih;

    if (*current_image == NULL)
    {
        IupMessage("Error", "No image is loaded.");
        return IUP_DEFAULT;
    }

    save_undo();
    rotated = apply_rotate_90(*current_image);

    if (rotated == NULL)
    {
        IupMessage("Error", "Rotation failed.");
        return IUP_DEFAULT;
    }

    image_free(*current_image);
    *current_image = rotated;
    refresh_canvas();
    set_status("Rotated 90 degrees");

    return IUP_DEFAULT;
}

static int crop_callback(Ihandle *ih)
{
    Image *cropped;
    int cx = 0;
    int cy = 0;
    int cw;
    int ch;

    (void)ih;

    if (*current_image == NULL)
    {
        IupMessage("Error", "No image is loaded.");
        return IUP_DEFAULT;
    }

    cw = (*current_image)->width;
    ch = (*current_image)->height;

    if (!IupGetParam("Crop", NULL, NULL,
                     "X: %i\nY: %i\nWidth: %i\nHeight: %i\n",
                     &cx, &cy, &cw, &ch))
    {
        return IUP_DEFAULT;
    }

    save_undo();
    cropped = apply_crop(*current_image, cx, cy, cw, ch);

    if (cropped == NULL)
    {
        IupMessage("Error",
                   "Crop failed.\n"
                   "Check that the region is within the image.");
        return IUP_DEFAULT;
    }

    image_free(*current_image);
    *current_image = cropped;
    refresh_canvas();
    set_status("Image cropped");

    return IUP_DEFAULT;
}

static int blur_callback(Ihandle *ih)
{
    Image *blurred;

    (void)ih;

    if (*current_image == NULL)
    {
        IupMessage("Error", "No image is loaded.");
        return IUP_DEFAULT;
    }

    save_undo();
    blurred = apply_blur(*current_image);

    if (blurred == NULL)
    {
        IupMessage("Error", "Blur failed.");
        return IUP_DEFAULT;
    }

    image_free(*current_image);
    *current_image = blurred;
    refresh_canvas();
    set_status("Blur applied");

    return IUP_DEFAULT;
}

static int undo_callback(Ihandle *ih)
{
    Image *temp;

    (void)ih;

    if (*undo_image == NULL)
    {
        IupMessage("Error", "Nothing to undo.");
        return IUP_DEFAULT;
    }

    temp = *current_image;
    *current_image = *undo_image;
    *undo_image = temp;

    refresh_canvas();
    set_status("Undo completed");

    return IUP_DEFAULT;
}

static int exit_callback(Ihandle *ih)
{
    (void)ih;

    return IUP_CLOSE;
}

void gui_init(Image **image, Image **undo)
{
    current_image= image;
    undo_image = undo;
   

   Ihandle * open_button = IupButton("Open", NULL);
   Ihandle * save_button = IupButton("Save", NULL);
   Ihandle * grayscale_button = IupButton("Grayscale", NULL);
   Ihandle * brightness_button = IupButton("Brightness", NULL);
   Ihandle * invert_button = IupButton("Invert", NULL);
   Ihandle * horizontal_button = IupButton("Horizontal Flip", NULL);
   Ihandle * vertical_button = IupButton("Vertical Flip", NULL);
  Ihandle *  rotate_button = IupButton("Rotate 90", NULL);
  Ihandle *  crop_button = IupButton("Crop", NULL);
   Ihandle * blur_button = IupButton("Blur", NULL);
   Ihandle * undo_button = IupButton("Undo", NULL);
  Ihandle *  exit_button = IupButton("Exit", NULL);

    IupSetCallback(
        open_button,
        "ACTION",
        (Icallback)open_callback
    );

    IupSetCallback(
        save_button,
        "ACTION",
        (Icallback)save_callback
    );

    IupSetCallback(
        grayscale_button,
        "ACTION",
        (Icallback)grayscale_callback
    );

    IupSetCallback(
        brightness_button,
        "ACTION",
        (Icallback)brightness_callback
    );

    IupSetCallback(
        invert_button,
        "ACTION",
        (Icallback)invert_callback
    );

    IupSetCallback(
        horizontal_button,
        "ACTION",
        (Icallback)horizontal_flip_callback
    );

    IupSetCallback(
        vertical_button,
        "ACTION",
        (Icallback)vertical_flip_callback
    );

    IupSetCallback(
        rotate_button,
        "ACTION",
        (Icallback)rotate_callback
    );

    IupSetCallback(
        crop_button,
        "ACTION",
        (Icallback)crop_callback
    );

    IupSetCallback(
        blur_button,
        "ACTION",
        (Icallback)blur_callback
    );

    IupSetCallback(
        undo_button,
        "ACTION",
        (Icallback)undo_callback
    );

    IupSetCallback(
        exit_button,
        "ACTION",
        (Icallback)exit_callback
    );

   button_box = IupHbox(
        open_button,
        save_button,
        grayscale_button,
        brightness_button,
        invert_button,
        horizontal_button,
        vertical_button,
        rotate_button,
        crop_button,
        blur_button,
        undo_button,
        exit_button,
        NULL
    );

    IupSetAttribute(button_box, "GAP", "5");

  canvas = IupCanvas(NULL);
    IupSetAttribute(canvas, "EXPAND","YES");
    IupSetAttribute(canvas, "RASTERSIZE", "800x500");
    IupSetAttribute(canvas, "BGCOLOR", "220 220 220");
    IupSetCallback(canvas, "ACTION", (Icallback)canvas_action_cb);

    status_label = IupLabel("No image loaded");

  main_box = IupVbox(
        button_box,
        canvas,
        status_label,
        NULL
    );

    IupSetAttribute(main_box, "GAP", "5");
    IupSetAttribute(main_box, "MARGIN", "10x10");

    dialog = IupDialog(main_box);

    IupSetAttribute(
        dialog,
        "TITLE",
        "Image Manipulation Software"
    );

    IupSetAttribute(
        dialog,
        "RASTERSIZE",
        "1000x700"
    );

    IupSetCallback(
        dialog,
        "CLOSE_CB",
        (Icallback)exit_callback
    );

    IupShow(dialog);
    IupUpdate(dialog);
}

void gui_run(void)
{
    IupMainLoop();
}

void gui_close(void)
{
    IupClose();
}