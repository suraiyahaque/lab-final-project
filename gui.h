#ifndef GUI_H
#define GUI_H

#include "image.h"

void gui_init(Image **image, Image **undo_image);
void gui_run(void);
void gui_close(void);

#endif