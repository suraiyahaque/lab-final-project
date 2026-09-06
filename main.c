
#include <stdio.h>
#include <stdlib.h>

#include "third_party/iup/include/iup.h"
#include "third_party/iup/include/iupcontrols.h"


#include "gui.h"
#include "image.h"


int main(int argc, char **argv)
{


   printf("[1] Starting program...\n");
    if (IupOpen(&argc, &argv) == IUP_ERROR) {
        printf("[ERROR] IupOpen failed!\n");
        return 1;
    }

    printf("[2] Initializing GUI...\n");
    Image *image = NULL;
    Image *undo_image = NULL;
    gui_init(&image, &undo_image);

    printf("[3] Entering event loop...\n");
    gui_run();

    printf("[4] Exiting program...\n");
    gui_close();

    if (image != NULL)
        image_free(image);
    if (undo_image != NULL)
        image_free(undo_image);

    return 0;
}

