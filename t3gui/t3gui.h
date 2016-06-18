#ifndef T3GUI_H
#define T3GUI_H

#include "egg_dialog/egg_dialog.h"

#define T3GUI_DIALOG_ELEMENT_CHUNK_SIZE 32
#define T3GUI_DIALOG_PLAYER_CHUNK_SIZE  16

typedef struct
{

  EGG_DIALOG * element;
  int elements;
  int allocated_elements;

} T3GUI_DIALOG;

T3GUI_DIALOG * t3gui_create_dialog(void);
void t3gui_destroy_dialog(T3GUI_DIALOG * dp);
void t3gui_dialog_add_widget(T3GUI_DIALOG * dialog, int (*proc)(int msg, EGG_DIALOG * d, int c), int x, int y, int w, int h, ALLEGRO_COLOR fg, ALLEGRO_COLOR bg, int key, uint64_t flags, int d1, int d2, void * dp, void * dp2, void * dp3);
bool t3gui_show_dialog(T3GUI_DIALOG * dp, ALLEGRO_EVENT_QUEUE * qp);
bool t3gui_close_dialog(T3GUI_DIALOG * dp);
void t3gui_render(void);

/* widgets */
int t3gui_push_button_proc(int msg, EGG_DIALOG *d, int c);

#endif
