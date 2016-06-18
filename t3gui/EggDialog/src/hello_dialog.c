#include <allegro5/allegro.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_opengl.h>
#include <allegro5/allegro_native_dialog.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <allegro5/allegro_opengl.h>
#include <time.h>
#include <stdio.h>
#include <math.h>
#include "egg_dialog/egg_dialog.h";

static ALLEGRO_BITMAP *bkg = NULL;

/* Draw the background image when the dialog is drawn */
static int draw_background(int msg, EGG_DIALOG *d, int c)
{
   if (msg == MSG_DRAW && bkg) {
      al_set_clipping_rectangle(0,0,al_get_display_width(al_get_current_display()),al_get_display_height(al_get_current_display()));
      al_draw_bitmap(bkg, 0, 0, 0);
   }
   return D_O_K;
}

static EGG_DIALOG hello_dlg[] = {
   { egg_window_frame_proc,   .fg = egg_white, .bg = egg_blue,  .d1 = ALLEGRO_ALIGN_CENTRE, .dp = "Title bar", .callback = draw_background},

   /* Vertical frame, containing a text label and a second frame */
   { egg_frame_proc,          .w = 520, .h = 240, .d1 = EGG_WALIGN_VERTICAL, .d2 = EGG_WALIGN_CENTRE, .d3 = EGG_WALIGN_CENTRE, .id = 1 },
   { egg_text_proc,           .w = 120, .h = 32, .fg = egg_black, .bg = egg_trans, .d1 = ALLEGRO_ALIGN_CENTRE, .dp = "Hello World!", .parent = 1 },

   /* Horizontal frame, containing two buttons */
   { egg_frame_proc,          .w = 520, .h = 64, .d1 = EGG_WALIGN_HORIZONTAL, .d2 = EGG_WALIGN_CENTRE, .d3 = EGG_WALIGN_CENTRE, .id = 2, .parent = 1 },
   { egg_rounded_button_proc, .w = 120, .h = 64, .fg = egg_black, .bg = egg_white, .flags = D_EXIT, .dp = "Hi!", .parent = 2  },
   { egg_rounded_button_proc, .w = 120, .h = 64, .fg = egg_black, .bg = egg_white, .flags = D_EXIT, .dp = "Goodbye!", .parent = 2  },
   { NULL }
};

int main(void)
{
   if (!al_init()) {
      fprintf(stderr, "Failed to initialise Allegro!\n");
      return EXIT_FAILURE;
   }
   al_init_image_addon();
   al_init_font_addon();
   al_init_ttf_addon();
   al_init_acodec_addon();
   al_init_primitives_addon();
   egg_initialise();

   al_install_mouse();
   al_install_joystick();
   al_install_keyboard();

   al_create_display(800, 600);

   ALLEGRO_FONT *font = al_load_font("data/DejaVuSans.ttf", 18, 0);
   bkg = al_load_bitmap("data/bkg.png");

   if (!font) {
      return EXIT_FAILURE;
   }

   egg_centre_dialog(hello_dlg);
   egg_set_gui_font(font);

   printf("%d\n", egg_do_dialog(hello_dlg, 0));

   return 0;
}

