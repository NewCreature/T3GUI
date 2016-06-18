/* EGGDialog, an Allgro 4-style GUI library for Allegro 5.
 * Version 0.9
 * Copyright (C) 2013 Evert Glebbeek
 *
 * See LICENCE for more information.
 */
#ifndef A45_DIALOG_H
#define A45_DIALOG_H

#include <allegro5/allegro.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include <stdint.h>
#include "egg_dialog/nine_patch.h"

#define EGG_DIALOG_VERSION          0
#define EGG_DIALOG_SUBVERSION       9
#define EGG_DIALOG_VERSION_STRING   "0.9"

struct EGG_DIALOG;

typedef AL_METHOD(int, EGG_DIALOG_PROC, (int msg, struct EGG_DIALOG *d, int c));

#define RGBA(r,g,b,a) { (r), (g), (b), (a) }
#define RGB(r,g,b)    RGBA(r,g,b,1.0)

/* Standard dialog colours */
#define egg_black       RGB(0.0, 0.0, 0.0)
#define egg_blue        RGB(0.0, 0.0, 0.5)
#define egg_green       RGB(0.0, 0.5, 0.0)
#define egg_cyan        RGB(0.0, 0.5, 0.5)
#define egg_red         RGB(0.5, 0.0, 0.0)
#define egg_purple      RGB(0.5, 0.0, 0.5)
#define egg_maroon      RGB(0.5, 0.5, 0.0)
#define egg_silver      RGB(0.5, 0.5, 0.5)
#define egg_grey        RGB(0.25, 0.25, 0.25)
#define egg_highblue    RGB(0.5, 0.5, 1.0)
#define egg_highgreen   RGB(0.5, 1.0, 0.5)
#define egg_highcyan    RGB(0.5, 1.0, 1.0)
#define egg_highred     RGB(1.0, 0.5, 0.5)
#define egg_magenta     RGB(1.0, 0.5, 1.0)
#define egg_yellow      RGB(1.0, 1.0, 0.5)
#define egg_white       RGB(1.0, 1.0, 1.0)
#define egg_trans       RGBA(1.0, 1.0, 1.0, 0.0)

typedef struct EGG_DIALOG {
   EGG_DIALOG_PROC proc;
   int x, y, w, h;               /* position and size of the object */
   ALLEGRO_COLOR fg, bg;         /* foreground and background colors */
   int key;                      /* keyboard shortcut (ASCII code) */
   uint64_t flags;               /* flags about the object state */
   int d1, d2;                   /* any data the object might require */
   void *dp, *dp2, *dp3;         /* pointers to more object data */

   uint32_t id;                  /* Unique ID by which to find this widget */
   EGG_DIALOG_PROC callback;

   ALLEGRO_COLOR mg;
   int d3;
   const ALLEGRO_FONT *font;
   int parent;

   /* Private/internal data */
   struct EGG_DIALOG *root;
   const ALLEGRO_FONT *default_font;
   int mousex, mousey;

   /* Bitmaps (for skinning the dialog) */
   NINE_PATCH_BITMAP *bmp_frame;       /* "Standard" look */
   NINE_PATCH_BITMAP *bmp_select;      /* "Selected" look */
   NINE_PATCH_BITMAP *bmp_focus;       /* "Focus"/"Highlight" look */
   NINE_PATCH_BITMAP *bmp_handle;      /* Handle, for scroll bars/sliders */
} EGG_DIALOG;

/* stored information about the state of an active GUI dialog */
typedef struct EGG_DIALOG_PLAYER {
   int obj;
   int res;
   int mouse_obj;
   int focus_obj;
   int click_obj;
   int stick, x_axis, y_axis;
   int joy_x, joy_y, joy_b;
   bool redraw;
   bool draw_veto;
   bool focus_follows_mouse;
   bool pass_events;
   bool paused;
   EGG_DIALOG *dialog;
   ALLEGRO_EVENT_QUEUE *input;
   ALLEGRO_EVENT_SOURCE event_src;
   ALLEGRO_THREAD *thread;
   const ALLEGRO_FONT *font;
} EGG_DIALOG_PLAYER;

/* Define/add theme to dialogs */
typedef struct EGG_DIALOG_THEME_ITEM {
   EGG_DIALOG_PROC proc;
   ALLEGRO_COLOR fg, bg, mg;

   ALLEGRO_FONT *font;

   NINE_PATCH_BITMAP *bmp_frame;
   NINE_PATCH_BITMAP *bmp_select;
   NINE_PATCH_BITMAP *bmp_focus;
   NINE_PATCH_BITMAP *bmp_handle;
} EGG_DIALOG_THEME_ITEM;

typedef struct EGG_DIALOG_THEME {
   int dt_items;
   int dt_max_items;
   EGG_DIALOG_THEME_ITEM *items;
} EGG_DIALOG_THEME;

/* Special dialog ID to indicate that this item has no ID */
#define EGG_NO_ID       0

/* Widget alignment flags, for the frame widget */
#define EGG_WALIGN_HORIZONTAL 0
#define EGG_WALIGN_VERTICAL   1
#define EGG_WALIGN_LEFT       ALLEGRO_ALIGN_LEFT
#define EGG_WALIGN_CENTRE     ALLEGRO_ALIGN_CENTRE
#define EGG_WALIGN_RIGHT      ALLEGRO_ALIGN_RIGHT
#define EGG_WALIGN_TOP        ALLEGRO_ALIGN_LEFT
#define EGG_WALIGN_BOTTOM     ALLEGRO_ALIGN_RIGHT

#ifndef EGG_NO_FLAGS

/* bits for the flags field */
#define D_EXIT          0x0001   /* object makes the dialog exit */
#define D_SELECTED      0x0002   /* object is selected */
#define D_GOTFOCUS      0x0004   /* object has the input focus */
#define D_GOTMOUSE      0x0008   /* mouse is on top of object */
#define D_HIDDEN        0x0010   /* object is not visible */
#define D_DISABLED      0x0020   /* object is visible but inactive */
#define D_DIRTY         0x0040   /* object needs to be redrawn */
#define D_INTERNAL      0x0080   /* reserved for internal use */
#define D_TRACKMOUSE    0x0100   /* object is tracking the mouse */
#define D_VSCROLLBAR    0x0200   /* object uses the scrollbar in the next widget entry (vertical) */
#define D_HSCROLLBAR    0x0400   /* object uses the scrollbar in the second next widget entry (horizontal) */
#define D_SCROLLBAR     D_VSCROLLBAR
#define D_USER          0x0800   /* from here on is free for your own use */

/* Event types */
#define EGG_EVENT_CLOSE       ALLEGRO_GET_EVENT_TYPE('e','g','g', 'c')
#define EGG_EVENT_CANCEL      ALLEGRO_GET_EVENT_TYPE('e','g','g', 'x')
#define EGG_EVENT_REDRAW      ALLEGRO_GET_EVENT_TYPE('e','g','g', 'd')
#define EGG_EVENT_LOSTFOCUS   ALLEGRO_GET_EVENT_TYPE('e','g','g', 'l')

/* return values for the dialog procedures */
#define D_O_K           0        /* normal exit status */
#define D_CLOSE         1        /* request to close the dialog */
#define D_REDRAW        2        /* request to redraw the dialog */
#define D_REDRAWME      4        /* request to redraw this object */
#define D_WANTFOCUS     8        /* this object wants the input focus */
#define D_USED_CHAR     16       /* object has used the keypress */
#define D_USED_KEY      D_USED_CHAR
#define D_REDRAW_ALL    32       /* request to redraw all active dialogs */
#define D_DONTWANTMOUSE 64       /* this object does not want mouse focus */
#define D_CALLBACK      128      /* for callback functions: don't call super function */

#define D_REDRAW_ANY (D_REDRAW | D_REDRAWME | D_REDRAW_ALL)

/* messages for the dialog procedures */
#define MSG_START       1        /* start the dialog, initialise */
#define MSG_END         2        /* dialog is finished - cleanup */
#define MSG_DRAW        3        /* draw the object */
#define MSG_CLICK       4        /* mouse click on the object */
#define MSG_DCLICK      5        /* double click on the object */
#define MSG_KEY         6        /* keyboard shortcut */
#define MSG_CHAR        7        /* unicode keyboard input */
#define MSG_KEYDOWN     8        /* key was pressed */
#define MSG_KEYUP       9        /* key was released */
#define MSG_KEYREPEAT   10       /* key is held down */
#define MSG_WANTFOCUS   11       /* does object want the input focus? */
#define MSG_GOTFOCUS    12       /* got the input focus */
#define MSG_LOSTFOCUS   13       /* lost the input focus */
#define MSG_GOTMOUSE    14       /* mouse on top of object */
#define MSG_LOSTMOUSE   15       /* mouse moved away from object */
#define MSG_IDLE        16       /* update any background stuff */
#define MSG_RADIO       17       /* clear radio buttons */
#define MSG_WHEEL       18       /* mouse wheel moved */
#define MSG_VBALL       MSG_WHEEL/* mouse ball moved (vertical) */
#define MSG_HBALL       19       /* mouse ball moved (horizontal) */
#define MSG_MOUSEDOWN   20       /* mouse button pressed */
#define MSG_MOUSEUP     21       /* mouse button released */
#define MSG_WANTMOUSE   22       /* does object want the mouse? */
#define MSG_MOUSEMOVE   23       /* mouse moved and object is tracking the mouse */
#define MSG_TIMER       24       /* timer tick */
#define MSG_USER        25       /* from here on are free... */

#endif

bool egg_initialise(void);
void egg_set_gui_font(const ALLEGRO_FONT *font);

int egg_object_message(EGG_DIALOG *dialog, int msg, int c);
int egg_dialog_message(EGG_DIALOG *dialog, int msg, int c, int *obj);

EGG_DIALOG_PLAYER *egg_init_dialog(EGG_DIALOG *dialog, int focus_obj);
bool egg_start_dialog(EGG_DIALOG_PLAYER *player);
bool egg_stop_dialog(EGG_DIALOG_PLAYER *player);
void egg_shutdown_dialog(EGG_DIALOG_PLAYER *player);

bool egg_pause_dialog(EGG_DIALOG_PLAYER *player);
bool egg_resume_dialog(EGG_DIALOG_PLAYER *player);

EGG_DIALOG *egg_find_widget_id(EGG_DIALOG *dialog, uint32_t id);

void egg_get_dialog_bounding_box(EGG_DIALOG *dialog, int *min_x, int *min_y, int *max_x, int *max_y);
void egg_centre_dialog(EGG_DIALOG *dialog);
void egg_position_dialog(EGG_DIALOG *dialog, int x, int y);

int egg_do_dialog_interval(EGG_DIALOG *dialog, double speed_sec, int focus);
int egg_do_dialog(EGG_DIALOG *dialog, int focus);

void egg_set_default_font(EGG_DIALOG_PLAYER *player, const ALLEGRO_FONT *font);

bool egg_draw_dialog(EGG_DIALOG_PLAYER *player);

int egg_find_dialog_focus(EGG_DIALOG *dialog);

/* Tell a dialog player to listen for events from a particular source */
bool egg_listen_for_events(EGG_DIALOG_PLAYER *player, ALLEGRO_EVENT_SOURCE *src);

/* Get the event source associated with a dialog player */
ALLEGRO_EVENT_SOURCE *egg_get_player_event_source(EGG_DIALOG_PLAYER *player);


EGG_DIALOG_THEME *egg_create_new_theme(void);
void egg_destroy_new_theme(EGG_DIALOG_THEME *theme);
void egg_add_dialog_theme(EGG_DIALOG_THEME *theme, EGG_DIALOG_PROC proc, ALLEGRO_COLOR fg, ALLEGRO_COLOR bg, ALLEGRO_COLOR mg, ALLEGRO_FONT *font, NINE_PATCH_BITMAP *frame, NINE_PATCH_BITMAP *selected_frame, NINE_PATCH_BITMAP *highlight_frame, NINE_PATCH_BITMAP *handle);
void egg_apply_theme(EGG_DIALOG *dialog, EGG_DIALOG_THEME *theme);

/* ID/index conversions */
uint32_t egg_index_to_id(EGG_DIALOG *dialog, int index);
int egg_id_to_index(EGG_DIALOG *dialog, uint32_t id);

/* Allocation and testing for unique IDs.
 * Of limited use if dialogs are allocated statically because it's easier
 * to assign them by hand in that case.
 */
bool egg_id_is_unique(EGG_DIALOG *dialog, uint32_t id);
uint32_t get_unique_id(EGG_DIALOG *dialog);

/* TODO: revert to a colour palette and integer colour indices? */


/* Dialog procedures and helpers */
void initialise_vertical_scroll(const EGG_DIALOG *parent, EGG_DIALOG *scroll, int scroll_w);
void initialise_horizontal_scroll(const EGG_DIALOG *parent, EGG_DIALOG *scroll, int scroll_h);

int egg_window_frame_proc(int msg, EGG_DIALOG *d, int c);
int egg_frame_proc(int msg, EGG_DIALOG *d, int c);
int egg_box_proc(int msg, EGG_DIALOG *d, int c);
int egg_button_proc(int msg, EGG_DIALOG *d, int c);
int egg_rounded_button_proc(int msg, EGG_DIALOG *d, int c);
int egg_text_button_proc(int msg, EGG_DIALOG *d, int c);
int egg_clear_proc(int msg, EGG_DIALOG *d, int c);
int egg_bitmap_proc(int msg, EGG_DIALOG *d, int c);
int egg_scaled_bitmap_proc(int msg, EGG_DIALOG *d, int c);
int egg_text_proc(int msg, EGG_DIALOG *d, int c);
int egg_check_proc(int msg, EGG_DIALOG *d, int c);
int egg_radio_proc(int msg, EGG_DIALOG *d, int c);
int egg_slider_proc(int msg, EGG_DIALOG *d, int c);
int egg_scroll_proc(int msg, EGG_DIALOG *d, int c);
int egg_textbox_proc(int msg, EGG_DIALOG *d, int c);
int egg_list_proc(int msg, EGG_DIALOG *d, int c);
int egg_edit_proc(int msg, EGG_DIALOG *d, int c);
int egg_edit_integer_proc(int msg, EGG_DIALOG *d, int c);
int egg_keyboard_proc(int msg, EGG_DIALOG *d, int c);

/* TODO: add at least the following: */
//int d_text_list_proc (int msg, A4_DIALOG *d, int c);
//int d_menu_proc (int msg, A4_DIALOG *d, int c);

#endif
