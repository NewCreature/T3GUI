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

static const char *long_text =
"Arma virumque cano, Troiae qui primus ab oris\n"
"Italiam, fato profugus, Laviniaque venit\n"
"litora, multum ille et terris iactatus et alto\n"
"vi superum saevae memorem Iunonis ob iram;\n"
"multa quoque et bello passus, dum conderet urbem,\n"
"inferretque deos Latio, genus unde Latinum,\n"
"Albanique patres, atque altae moenia Romae.\n"
"\n"
"Musa, mihi causas memora, quo numine laeso,\n"
"quidve dolens, regina deum tot volvere casus\n"
"insignem pietate virum, tot adire labores\n"
"impulerit. Tantaene animis caelestibus irae?\n";

static const char *list[] = {
   "Latin",
   "Greek",
   "Dutch",
   "Etruscan",
   "English",
   "Klingon"
};

const char *get_language(int index, int *nelem, void *dummy)
{
   if (index < 0) {
      assert(nelem);
      *nelem = sizeof(list) / sizeof(*list);
      return NULL;
   }

   return list[index];
}

static EGG_DIALOG test_dlg[] = {
   /* proc              x   y    w   h  fg         bg             key,  flags,   d1, d2, dp1, dp2, dp3, id */
   { egg_clear_proc,    0,  0,   0,  0, egg_black, egg_highblue,  0,    0,       0,0, NULL, NULL, NULL },
   { egg_window_frame_proc,0,0,  0,  0, egg_white, egg_black,     0,    0,       ALLEGRO_ALIGN_CENTRE,0, "Title bar", NULL, NULL },
   { egg_box_proc,     10, 10, 100,100, egg_black, egg_red,       0,    0,       0,0, NULL, NULL, NULL },
   { egg_text_proc,    10, 10, 100,100, egg_white, egg_red,       0,    0,       ALLEGRO_ALIGN_CENTRE,0, "Text string!", NULL, NULL },
   { egg_edit_proc,   -30,-50, 300, 32, egg_black, egg_white,     0,    0,       0,0, "", NULL, NULL, 0x0003 },
   { egg_button_proc,  10,115, 100, 64, egg_blue,  egg_yellow,    0,    0,       0,0, "Click me!", NULL, NULL, 0x0001 },
   { egg_button_proc,  10,184, 100, 64, egg_blue,  egg_green,     0,    0,       0,0, "Click me!", NULL, NULL },
   { egg_rounded_button_proc,10,253,100,64,egg_blue,egg_maroon,   0,    D_EXIT,  0,0, "Close",     NULL, NULL },
   { egg_check_proc,  150, 10,  64, 32, egg_black, egg_white,     0,    0,       1,0, "test", NULL, NULL },
   { egg_text_button_proc,150,50,100,48,egg_black, egg_white,     0,    0,       0,1, "Button 1", NULL, NULL },
   { egg_text_button_proc,150,100,100,48,egg_black,egg_white,     0,    0,       0,1, "Button 2", NULL, NULL },
   { egg_text_button_proc,150,150,100,48,egg_black,egg_white,     0,    0,       0,1, "Button 3", NULL, NULL },
   { egg_radio_proc,   -50, 0,  16, 16, egg_black, egg_trans,     0,    0,       0,0, NULL, NULL, NULL,  },
   { egg_radio_proc,   -50,20,  16, 16, egg_black, egg_trans,     0,    0,       0,0, NULL, NULL, NULL,  },
   { egg_radio_proc,   -50,40,  16, 16, egg_black, egg_trans,     0,    0,       0,0, NULL, NULL, NULL,  },
   { egg_radio_proc,   -30, 0,  16, 16, egg_red,   egg_trans,     0,    0,       1,0, NULL, NULL, NULL,  },
   { egg_radio_proc,   -30,20,  16, 16, egg_red,   egg_trans,     0,    0,       1,0, NULL, NULL, NULL,  },
   { egg_radio_proc,   -30,40,  16, 16, egg_red,   egg_trans,     0,    0,       1,0, NULL, NULL, NULL,  },
   { egg_slider_proc,    0,320,120, 48, egg_black, egg_white,     0,    0,      50,0, NULL, NULL, NULL,  },
   { egg_scroll_proc, -120,380,300, 16, egg_black, egg_white,     0,    0,     550,0, NULL, NULL, NULL,  },
   { egg_slider_proc, -100, 10, 16,300, egg_black, egg_white,     0,    0,     550,0, NULL, NULL, NULL,  },
   { egg_scroll_proc, -120, 10, 16,300, egg_black, egg_white,     0,    0,     550,0, NULL, NULL, NULL, 0x0002 },
   { egg_textbox_proc, 256, 10,300,224, egg_black, egg_white,     0,    D_SCROLLBAR,0,0, NULL, NULL, NULL,  },
   { egg_scroll_proc, -220, 10, 16,300, egg_green, egg_blue,      0,    D_HIDDEN,550,0, NULL, NULL, NULL,  },
   { egg_list_proc,    206,240,350, 48, egg_black, egg_white,     0,    0,       0,0, get_language },
   { egg_keyboard_proc,  0,  0,  0,  0, egg_trans, egg_trans,     'q',  D_EXIT,  0,0, NULL },
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
   ALLEGRO_FONT *alt_font = al_load_font("data/DS_Mysticora.ttf", 18, 0);

   if (!font) {
      return EXIT_FAILURE;
   }

   EGG_DIALOG_THEME *theme = egg_create_new_theme();
   egg_add_dialog_theme(theme, egg_text_button_proc,
      al_map_rgb(255, 255, 255), al_map_rgb(255, 215, 0), al_map_rgb(255, 215, 0),
      alt_font, NULL, NULL, NULL, NULL);
   egg_add_dialog_theme(theme, egg_textbox_proc,
      al_map_rgb(128, 0, 0), al_map_rgb(255, 255, 255), al_map_rgb(128, 128, 128),
      NULL, NULL, NULL, NULL, NULL);
   egg_apply_theme(test_dlg, theme);

   for (int n=0; test_dlg[n].proc; n++) {
      if (test_dlg[n].proc == egg_textbox_proc) {
         test_dlg[n].dp = (char *)long_text;
      }
   }

   EGG_DIALOG *widget = egg_find_widget_id(test_dlg, 1);
   if (widget) {
      widget->dp = "Skinned";
      widget->bmp_frame = load_nine_patch_bitmap("data/button2.9.png");
      widget->bmp_focus = load_nine_patch_bitmap("data/button2.focus.9.png");
      widget->bmp_select = load_nine_patch_bitmap("data/button2.select.9.png");
   }

   widget = egg_find_widget_id(test_dlg, 2);
   if (widget) {
      widget->bmp_handle = load_nine_patch_bitmap("data/grab.9.png");
   }

   char textstring[256];
   widget = egg_find_widget_id(test_dlg, 3);
   if (widget) {
      widget->dp = textstring;
      widget->d1 = sizeof(textstring);
   }
   snprintf(textstring, 256, "Edit me!");

   egg_centre_dialog(test_dlg);
   egg_set_gui_font(font);

   al_flip_display();
   printf("%d\n", egg_do_dialog(test_dlg, 0));

   return 0;
}

