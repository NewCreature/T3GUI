#include <ctype.h>
#include "egg_dialog/egg_dialog.h"
#include "unicode.h"

static int min(int x, int y) { return (x<y)?x:y; }
static int max(int x, int y) { return (x>y)?x:y; }
static int clamp(int x, int y, int z) {return max(x, min(y, z));}

void initialise_vertical_scroll(const EGG_DIALOG *parent, EGG_DIALOG *scroll, int w)
{
   scroll->proc   = egg_scroll_proc;

   scroll->x      = parent->x + parent->w - w;
   scroll->y      = parent->y;
   scroll->w      = w;
   scroll->h      = parent->h;

   scroll->fg     = parent->fg;
   scroll->bg     = parent->bg;
   scroll->mg     = parent->mg;

   scroll->flags  = parent->flags;
   scroll->d1     = parent->d1 - parent->h;
   scroll->d2     = parent->d2;

   scroll->mousex = parent->mousex;
   scroll->mousey = parent->mousey;
}

void initialise_horizontal_scroll(const EGG_DIALOG *parent, EGG_DIALOG *scroll, int h)
{
   scroll->proc   = egg_scroll_proc;

   scroll->x      = parent->x;
   scroll->y      = parent->y + parent->h - h;
   scroll->w      = parent->w;
   scroll->h      = h;

   scroll->fg     = parent->fg;
   scroll->bg     = parent->bg;
   scroll->mg     = parent->mg;

   scroll->flags  = parent->flags;
   scroll->d1     = parent->d1 - parent->w;
   scroll->d2     = parent->d2;

   scroll->mousex = parent->mousex;
   scroll->mousey = parent->mousey;
}

/*
 * Sample dialog routines/widgets
 */


int egg_window_frame_proc(int msg, EGG_DIALOG *d, int c)
{
   assert(d);
   int ret = D_O_K;
   const ALLEGRO_FONT *font = d->dp2;
   const char *text = d->dp;
   int title_height = 0;
   if (text == NULL) text = "";
   if (!font) font = d->font;
   if (!font) font = d->default_font;
   title_height = al_get_font_line_height(font);

   if (msg == MSG_START) {
      int min_x, min_y, max_x, max_y;
      egg_get_dialog_bounding_box(d, &min_x, &min_y, &max_x, &max_y);
      d->x = min_x-4;
      d->w = max_x - min_x+8;
      d->y = min_y - 4;
      d->h = max_y - min_y+8;

      d->y -= title_height;
      d->h += title_height;
   }

   if (msg == MSG_DRAW) {
      int flags = d->d1;
      int x = d->x;

      if (flags == ALLEGRO_ALIGN_CENTRE) x += d->w/2;
      if (flags == ALLEGRO_ALIGN_RIGHT)  x += d->w;

      al_draw_filled_rectangle(d->x+0.5, d->y+0.5, d->x+d->w-0.5, d->y+d->h-0.5, d->mg);
      al_draw_rectangle(d->x+0.5, d->y+0.5, d->x+d->w-0.5, d->y+d->h-0.5, d->bg, 1.0);

      al_draw_filled_rectangle(d->x+0.5, d->y+0.5, d->x+d->w-0.5, d->y+title_height-0.5, d->bg);
      al_draw_text(font, d->fg, x, d->y, d->d1, text);
   }

   if (msg == MSG_MOUSEDOWN) {
      if (d->mousey < d->y+title_height) {
         d->d2 = d->mousex;
         d->d3 = d->mousey;
         d->flags |= D_TRACKMOUSE;
      }
   }

   if (msg == MSG_MOUSEUP)
      d->flags &= ~D_TRACKMOUSE;

   if (msg == MSG_MOUSEMOVE && d->flags&D_TRACKMOUSE) {
      int dx = d->mousex - d->d2;
      int dy = d->mousey - d->d3;
      d->d2 = d->mousex;
      d->d3 = d->mousey;
      for (int c=0; d->root[c].proc; c++) {
         d->root[c].x += dx;
         d->root[c].y += dy;
      }
      ret |= D_REDRAW;
   }
   return ret;
}

int egg_frame_proc(int msg, EGG_DIALOG *d, int c)
{
   assert(d);
   if (d->id == EGG_NO_ID) return D_O_K;

   /* Find out how many children there are and how we're supposed to tile
    * them.
    */

   if (msg == MSG_START) {
      bool vertical = d->d1;
      int h_align = d->d2;    /* Left/centre/right */
      int v_align = d->d3;    /* Top/centre/bottom */

      int child_count = 0;
      int required_space = 0;
      int available_space = vertical ? d->h : d->w;
      for (int n = 0; d->root[n].proc; n++) {
         if (d->root[n].parent == d->id) {
            child_count++;
            required_space += vertical ? d->root[n].h
                                       : d->root[n].w;
         }
      }

      if (child_count == 0) return D_O_K;

      /* TODO: resize widgets if space is too small */
      if (required_space > available_space) required_space = available_space;

      int ds = (available_space - required_space) / (1 + child_count);
      int x = d->x;
      int y = d->y;

      if (vertical) {
         if (v_align != EGG_WALIGN_CENTRE) ds = (available_space - required_space) / child_count;
         if (v_align != EGG_WALIGN_LEFT)   y += ds;
         for (int n = 0; d->root[n].proc; n++) {
            if (d->root[n].parent == d->id) {
               d->root[n].y = y;
               switch (h_align) {
                  case EGG_WALIGN_CENTRE:
                     d->root[n].x = x + (d->w - d->root[n].w)/2;
                     break;
                  case EGG_WALIGN_LEFT:
                     d->root[n].x = x;
                     break;
                  case EGG_WALIGN_RIGHT:
                     d->root[n].x = x + d->w - d->root[n].w;
                     break;
               }
               y += d->root[n].h + ds;
            }
         }
      } else {
         if (h_align != EGG_WALIGN_CENTRE) ds = (available_space - required_space) / child_count;
         if (h_align != EGG_WALIGN_LEFT)   x += ds;
         for (int n = 0; d->root[n].proc; n++) {
            if (d->root[n].parent == d->id) {
               switch (v_align) {
                  case EGG_WALIGN_CENTRE:
                     d->root[n].y = y + (d->h - d->root[n].h)/2;
                     break;
                  case EGG_WALIGN_TOP:
                     d->root[n].y = y;
                     break;
                  case EGG_WALIGN_BOTTOM:
                     d->root[n].y = y + d->h - d->root[n].h;
                     break;
               }
               d->root[n].x = x;
               x += d->root[n].w + ds;
            }
         }
      }
   }

   return D_O_K;
}

int egg_box_proc(int msg, EGG_DIALOG *d, int c)
{
   int ret = D_O_K;
   assert(d);
   //printf("egg_box received message 0x%04x\n", msg);
   if (msg==MSG_DRAW) {
      ALLEGRO_COLOR fg = d->fg;

      if (d->bmp_frame) {
        NINE_PATCH_BITMAP *p9 = d->bmp_frame;
        int w = max(d->w, get_nine_patch_bitmap_min_width(p9));
        int h = max(d->h, get_nine_patch_bitmap_min_height(p9));
        draw_nine_patch_bitmap(p9, d->x+0.5, d->y+0.5, w, h);
       }
       else {
         al_draw_filled_rectangle(d->x+0.5, d->y+0.5, d->x+d->w-0.5, d->y+d->h-0.5, d->bg);
         al_draw_rectangle(d->x+0.5, d->y+0.5, d->x+d->w-0.5, d->y+d->h-0.5, fg, 1.0);
       }
   }

   return ret;
}

/* egg_button_proc:
 *  A button object (the dp field points to the text string). This object
 *  can be selected by clicking on it with the mouse or by pressing its
 *  keyboard shortcut. If the D_EXIT flag is set, selecting it will close
 *  the dialog, otherwise it will toggle on and off.
 */
int egg_button_proc(int msg, EGG_DIALOG *d, int c)
{
   int ret = D_O_K;
   ALLEGRO_COLOR c1, c2;
   int g;
   assert(d);

   const char *text = d->dp;
   const ALLEGRO_FONT *font = NULL;
   if (text == NULL) text = "";
   if (!font) font = d->font;
   if (!font) font = d->default_font;

   int select = D_SELECTED | D_TRACKMOUSE;

   switch (msg) {
      case MSG_DRAW:
         if (d->flags & select) {
            g = 1;
            c1 = d->bg;
            c2 = (d->flags & D_DISABLED) ? d->mg : d->fg;
         }
         else {
            g = 0;
            c1 = (d->flags & D_DISABLED) ? d->mg : d->fg;
            c2 = d->bg;
         }

         if (d->bmp_frame) {
            NINE_PATCH_BITMAP *p9 = d->bmp_frame;

            if (d->flags & D_GOTFOCUS && d->bmp_focus) p9 = d->bmp_focus;
            if (d->flags & select && d->bmp_select) p9 = d->bmp_select;

            int w = max(d->w, get_nine_patch_bitmap_min_width(p9));
            int h = max(d->h, get_nine_patch_bitmap_min_height(p9));
            draw_nine_patch_bitmap(p9, d->x+0.5, d->y+0.5, w, h);
            al_draw_text(font, c1, d->x+d->w/2+g, d->y+d->h/2-al_get_font_line_height(font)/2+g, ALLEGRO_ALIGN_CENTRE, text);
         } else { /* Default, primitives based */
            ALLEGRO_COLOR black = egg_black;
            al_draw_rectangle(d->x+0.5, d->y+0.5, d->x+g+d->w-1.5, d->y+g+d->h-1.5, black, 1.0);
            al_draw_filled_rectangle(d->x+0.5+g, d->y+0.5+g, d->x+d->w-2.5+g, d->y+d->h-2.5+g, c2);
            al_draw_rectangle(d->x+g+0.5, d->y+g+0.5, d->x+g+d->w-2.5, d->y+g+d->h-2.5, c1, 1.0);
            al_draw_text(font, c1, d->x+d->w/2+g, d->y+d->h/2-al_get_font_line_height(font)/2+g, ALLEGRO_ALIGN_CENTRE, text);

            if ((d->flags & D_GOTFOCUS) && (!(d->flags & select) || !(d->flags & D_EXIT))) {
               al_draw_rectangle(d->x+1.5+g, d->y+1.5+g, d->x+d->w-3.5+g, d->y+d->h-3.5+g, c1, 5.0);
            }
         }
         break;

      case MSG_WANTFOCUS:
         return ret | D_WANTFOCUS;

      case MSG_KEY:
         /* close dialog? */
         if (d->flags & D_EXIT) {
            return D_CLOSE;
         }

         /* or just toggle */
         d->flags ^= D_SELECTED;
         ret |= D_REDRAWME;
         break;

      case MSG_MOUSEDOWN:
         d->flags |= D_TRACKMOUSE;
         ret |= D_REDRAWME;
         break;

      case MSG_MOUSEUP:
         if (d->flags & D_TRACKMOUSE)
            ret |= D_REDRAWME;
         d->flags &= ~D_TRACKMOUSE;

         ALLEGRO_MOUSE_STATE mouse_state;
         al_get_mouse_state(&mouse_state);
         int state1, state2;
         int swap;

         /* what state was the button originally in? */
         state1 = d->flags & D_SELECTED;
         if (d->flags & D_EXIT)
            swap = false;
         else
            swap = state1;
         state2 = ((mouse_state.x >= d->x) && (mouse_state.y >= d->y) &&
                   (mouse_state.x < d->x + d->w) && (mouse_state.y < d->y + d->h));
         if (swap)
            state2 = !state2;
         if (((state1) && (!state2)) || ((state2) && (!state1)))
            d->flags ^= D_SELECTED;

         if ((d->flags & D_SELECTED) && (d->flags & D_EXIT)) {
            d->flags ^= D_SELECTED;
            return D_CLOSE;
         }
         break;
   }

   return ret;
}


/* egg_button_proc:
 *  A button object (the dp field points to the text string). This object
 *  can be selected by clicking on it with the mouse or by pressing its
 *  keyboard shortcut. If the D_EXIT flag is set, selecting it will close
 *  the dialog, otherwise it will toggle on and off.
 */
int egg_rounded_button_proc(int msg, EGG_DIALOG *d, int c)
{
   int ret = D_O_K;
   assert(d);

   const char *text = d->dp;
   const ALLEGRO_FONT *font = d->dp2;
   if (text == NULL) text = "";
   if (!font) font = d->font;
   if (!font) font = d->default_font;

   int select = D_SELECTED | D_TRACKMOUSE;

   if (msg == MSG_DRAW) {
      ALLEGRO_COLOR c1, c2;
      float rx = 4;
      float ry = 4;

      if (d->flags & select) {
         c1 = d->bg;
         c2 = (d->flags & D_DISABLED) ? d->mg : d->fg;
      }
      else {
         c1 = (d->flags & D_DISABLED) ? d->mg : d->fg;
         c2 = d->bg;
      }
      if (d->bmp_frame) {
         NINE_PATCH_BITMAP *p9 = d->bmp_frame;

         if (d->flags & D_GOTFOCUS && d->bmp_focus) p9 = d->bmp_focus;
         if (d->flags & select && d->bmp_select) p9 = d->bmp_select;

         int w = max(d->w, get_nine_patch_bitmap_min_width(p9));
         int h = max(d->h, get_nine_patch_bitmap_min_height(p9));
         draw_nine_patch_bitmap(p9, d->x+0.5, d->y+0.5, w, h);
         al_draw_text(font, c1, d->x+d->w/2, d->y+d->h/2-al_get_font_line_height(font)/2, ALLEGRO_ALIGN_CENTRE, text);
      } else { /* Default, primitives based */

         al_draw_rounded_rectangle(d->x+1.5, d->y+1.5, d->x+d->w-0.5, d->y+d->h-0.5, rx, ry, c1, 2.0);
         al_draw_filled_rounded_rectangle(d->x+0.5, d->y+0.5, d->x+d->w-1.5, d->y+d->h-1.5, rx, ry, c2);
         al_draw_rounded_rectangle(d->x+0.5, d->y+0.5, d->x+d->w-1.5, d->y+d->h-1.5, rx, ry, c1, 1.0);
         al_draw_text(font, c1, d->x+d->w/2, d->y+d->h/2-al_get_font_line_height(font)/2, ALLEGRO_ALIGN_CENTRE, text);

         if ((d->flags & D_GOTFOCUS) && (!(d->flags & select) || !(d->flags & D_EXIT))) {
            al_draw_rounded_rectangle(d->x+1.5, d->y+1.5, d->x+d->w-2.5, d->y+d->h-2.5, rx, ry, c1, 3.0);
         }
      }

      return ret;
   }

   return ret | egg_button_proc(msg, d, c);
}

/* d_clear_proc:
 *  Simple dialog procedure which just clears the screen. Useful as the
 *  first object in a dialog.
 */
int egg_clear_proc(int msg, EGG_DIALOG *d, int c)
{
   int ret = D_O_K;
   assert(d);

   if (msg == MSG_DRAW) {
      al_set_clipping_rectangle(0, 0, al_get_display_width(al_get_current_display()), al_get_display_height(al_get_current_display()));
      al_clear_to_color(d->bg);
   }

   return ret;
}

/* d_bitmap_proc:
 *  Simple dialog procedure: draws the bitmap which is pointed to by dp.
 */
int egg_bitmap_proc(int msg, EGG_DIALOG *d, int c)
{
   int ret = D_O_K;

   ALLEGRO_BITMAP *b;
   assert(d);

   b = (ALLEGRO_BITMAP *)d->dp;
   if (msg==MSG_DRAW && b)
      al_draw_bitmap(b, d->x, d->y, 0);

   return ret;
}


/* d_bitmap_proc:
 *  Simple dialog procedure: draws the bitmap which is pointed to by dp.
 */
int egg_scaled_bitmap_proc(int msg, EGG_DIALOG *d, int c)
{
   int ret = D_O_K;

   ALLEGRO_BITMAP *b;
   assert(d);

   b = (ALLEGRO_BITMAP *)d->dp;
   if (msg==MSG_DRAW && b)
      al_draw_scaled_bitmap(b, 0, 0, al_get_bitmap_width(b), al_get_bitmap_height(b), d->x, d->y, d->w, d->h, 0);

   return ret;
}

/* d_text_proc:
 *  Simple dialog procedure: draws the text string which is pointed to by dp.
 */
int egg_text_proc(int msg, EGG_DIALOG *d, int c)
{
   int ret = D_O_K;

   assert(d);
   if (msg==MSG_DRAW) {
      const char *text = d->dp;
      int flags = d->d1;
      int x = d->x;
      ALLEGRO_COLOR fg = (d->flags & D_DISABLED) ? d->mg : d->fg;
      const ALLEGRO_FONT *font = d->dp2;
      if (!font) font = d->font;
      if (!font) font = d->default_font;
      assert(font);

      if (!text) text = "";

      if (flags == ALLEGRO_ALIGN_CENTRE) x += d->w/2;
      if (flags == ALLEGRO_ALIGN_RIGHT)  x += d->w;

      al_draw_text(font, fg, x, d->y, flags, text);
   }

   return ret;
}

/* d_check_proc:
 *  Who needs C++ after all? This is derived from d_button_proc,
 *  but overrides the drawing routine to provide a check box.
 */
int egg_check_proc(int msg, EGG_DIALOG *d, int c)
{
   int ret = D_O_K;
   assert(d);

   if (msg==MSG_DRAW) {
      const char *text = d->dp;
      const ALLEGRO_FONT *font = d->dp2;
      ALLEGRO_COLOR fg = (d->flags & D_DISABLED) ? d->mg : d->fg;
      ALLEGRO_COLOR bg = d->bg;
      int h = d->h;
      int textw = 0;

      if (d->w < h) h = d->w;

      if (!font) font = d->font;
      if (!font) font = d->default_font;

      float lw = 1.0;
      if (d->flags & D_GOTFOCUS) lw = 2.0;

      if (text) textw = al_get_text_width(font, text);

      /* Draw check box */
      float x1 = d->x+0.5 + (lw - 1.0)/2;
      float y1 = d->y+0.5 + (lw - 1.0)/2;
      float x2 = d->x+h-0.5;
      float y2 = d->y+h-0.5;
      int tx, ty;

      if (d->d1) {   /* Text to the right of checkbox */
         tx = x2 + 2.;
         ty = d->y+0.5 + (h - al_get_font_line_height(font))/2;
      } else {       /* Text to the left of checkbox */
         //tx = x1 - (textw + 0.5);
         ty = d->y+0.5 + (h - al_get_font_line_height(font))/2;
         tx = d->x + 0.5;
         x1 = x1 + d->w - h;
         x2 = x2 + d->w - h;
      }

      if (textw)
         al_draw_text(font, fg, tx, ty, ALLEGRO_ALIGN_LEFT, text);

      al_draw_filled_rectangle(x1, y1, x2, y2, bg);
      al_draw_rectangle(x1, y1, x2, y2, fg, lw);
      if (d->flags & D_SELECTED) {
         al_draw_line(x1, y1, x2, y2, fg, 1.0);
         al_draw_line(x1, y2, x2, y1, fg, 1.0);
      }

      return ret;
   }

   return ret | egg_button_proc(msg, d, 0);
}


static ALLEGRO_COLOR interpolate_colour(ALLEGRO_COLOR from, ALLEGRO_COLOR to, int factor, int max_factor)
{
   float r, g, b, a;
   float r1, g1, b1, a1;
   float r2, g2, b2, a2;
   float f;

   al_unmap_rgba_f(from, &r1, &g1, &b1, &a1);
   al_unmap_rgba_f(to,   &r2, &g2, &b2, &a2);

   f = (float)factor / max_factor;
   if (f < 0.0) f = 0.0;
   if (f > 1.0) f = 1.0;

   r = r1 + (r2 - r1) * f;
   g = g1 + (g2 - g1) * f;
   b = b1 + (b2 - b1) * f;
   a = a1 + (a2 - a1) * f;

   return al_map_rgba_f(r, g, b, f);
}



/* Similar to d_textbut_agui_proc, but it works with a coloured gui_font */
/*  dp2 can hold a gui_font structure                                    */
/*  dp3 holds the corresponding palette                              */
/*  d1 measures how far hue has been changed (0-100%)                */
/*  d2 holds the change in saturation (fixed)                        */
/* The palette is taken from dp2 or dp3 depending on wether it is selected */
/*  selected or not. */
int egg_text_button_proc(int msg, EGG_DIALOG *d, int c)
{
   int ret = D_O_K;

   if (msg==MSG_START)
      d->d1 = 0;

   if (msg==MSG_DRAW) {
      ALLEGRO_COLOR colour, fg, bg;
      const char *text = d->dp;

      const ALLEGRO_FONT *font = d->dp2;
      if (!font) font = d->font;
      if (!font) font = d->default_font;

      fg = d->fg;
      bg = d->bg;

      if (d->flags & D_GOTFOCUS) {
         colour = bg;
         if (d->d1<66 && d->d2) {
            d->d1+=2;
            ret |= D_REDRAWME;
            colour = interpolate_colour(fg, bg, d->d1, 64);
         }
      } else {
         colour = fg;
         if (d->d1>-2 && d->d2) {
            d->d1-=2;
            ret |= D_REDRAWME;
            colour = interpolate_colour(fg, bg, d->d1, 64);
         }
      }

      int dx, dy, w, h;
      al_get_text_dimensions(font, d->dp, &dx, &dy, &w, &h);
      al_draw_textf(font, colour, d->x+(d->w-w)/2, d->y, 0, "%s", text);

      return ret;
   }

   return ret | egg_button_proc(msg, d, c);
}

/* d_radio_proc:
 *  GUI procedure for radio buttons.
 *  Parameters: d1-button group number; d2-button style (0=circle,1=square);
 *  dp-text to appear as label to the right of the button.
 */
int egg_radio_proc(int msg, EGG_DIALOG *d, int c)
{
   int x, y, h, r, ret;
   ALLEGRO_COLOR fg, bg;
   int centerx, centery;
   float lw = 1;
   assert(d);

   const char *text = d->dp;
   const ALLEGRO_FONT *font = d->dp2;
   if (!font) font = d->font;
   if (!font) font = d->default_font;

   ret = D_O_K;

   switch(msg) {
      case MSG_DRAW:
         fg = (d->flags & D_DISABLED) ? d->mg : d->fg;
         bg = d->bg;

         h = al_get_font_line_height(font);
         y = d->y+(d->h-(h-al_get_font_descent(font)))/2;

         if (text)
            al_draw_text(font, fg, d->x+h+h/2, y, 0, text);

         if (d->bmp_frame) {
            NINE_PATCH_BITMAP *p9 = d->bmp_frame;

            if (d->flags & D_GOTFOCUS && d->bmp_focus) p9 = d->bmp_focus;
            if (d->flags & D_SELECTED && d->bmp_select) p9 = d->bmp_select;

            int w = max(d->w, get_nine_patch_bitmap_min_width(p9));
            int h = max(d->h, get_nine_patch_bitmap_min_height(p9));
            draw_nine_patch_bitmap(p9, d->x+0.5, d->y+0.5, w, h);
         } else { /* Default, primitives based */
            x = d->x;
            r = min(h, d->h)/2-0.5;

            centerx = d->x+r;
            centery = d->y+d->h/2;

            if (d->flags & D_GOTFOCUS) lw = 3.;

            al_draw_circle(centerx, centery, r, fg, lw);
            if (d->flags & D_SELECTED)
               al_draw_filled_circle(centerx, centery, r/2, fg);
         }
#if 0
         switch (d->d2) {

            case 1:
               rect(gui_bmp, d->x, y, x+h-1, y+h-1, fg);
               if (d->flags & D_SELECTED)
                  rectfill(gui_bmp, centerx-r/2, centery-r/2, centerx+r/2-1, centery+r/2-1, fg);
               break;

            default:
               circle(gui_bmp, centerx, centery, r, fg);
               if (d->flags & D_SELECTED)
                  circlefill(gui_bmp, centerx, centery, r/2, fg);
               break;
         }
#endif

         return D_O_K;

      case MSG_KEY:
      case MSG_CLICK:
         if (d->flags & D_SELECTED) {
            return D_O_K;
         }
         break;

      case MSG_RADIO:
         if ((c == d->d1) && (d->flags & D_SELECTED)) {
            d->flags &= ~D_SELECTED;
            d->flags |= D_DIRTY;
         }
         break;
   }

   ret = egg_button_proc(msg, d, 0);

   if (((msg==MSG_KEY) || (msg==MSG_MOUSEUP)) &&
         (d->flags & D_SELECTED) && (!(d->flags & D_EXIT))) {
      d->flags &= ~D_SELECTED;
      egg_dialog_message(d->root, MSG_RADIO, d->d1, NULL);
      d->flags |= D_SELECTED;
      ret |= D_REDRAWME;
   }

   return ret;
}

/* d_slider_proc:
 *  A slider control object. This object returns a value in d2, in the
 *  range from 0 to d1. It will display as a vertical slider if h is
 *  greater than or equal to w; otherwise, it will display as a horizontal
 *  slider. dp can contain an optional bitmap to use for the slider handle;
 *  dp2 can contain an optional callback function, which is called each
 *  time d2 changes. The callback function should have the following
 *  prototype:
 *
 *  int function(void *dp3, int d2);
 *
 *  The d_slider_proc object will return the value of the callback function.
 */
int egg_slider_proc(int msg, EGG_DIALOG *d, int c)
{
   ALLEGRO_BITMAP *slhan = NULL;
   int oldpos, newpos;
   int vert = true;        /* flag: is slider vertical? */
   int hh = 7;             /* handle height (width for horizontal sliders) */
   int hmar;               /* handle margin */
   int slp;                /* slider position */
   int mp;                 /* mouse position */
   int irange;
   int slx, sly, slh, slw;
   int msx, msy;
   int retval = D_O_K;
   int upkey, downkey;
   int pgupkey, pgdnkey;
   int homekey, endkey;
   int delta;
   float slratio, slmax, slpos;
   int (*proc)(void *cbpointer, int d2value);
   int oldval;
   assert(d);

   /* check for slider direction */
   if (d->h < d->w)
      vert = false;

   /* set up the metrics for the control */
   if (d->dp != NULL) {
      slhan = (ALLEGRO_BITMAP *)d->dp;
      if (vert)
         hh = al_get_bitmap_height(slhan);
      else
         hh = al_get_bitmap_width(slhan);
   }

   if (vert) {
      if (d->w/4 > hh) hh = d->w/4;
   } else {
      if (d->h/4 > hh) hh = d->h/4;
   }

   hmar = hh/2;
   irange = (vert) ? d->h : d->w;
   slmax = irange-hh;
   slratio = slmax / (d->d1);
   slpos = slratio * d->d2;
   slp = slpos;

   switch (msg) {

      case MSG_DRAW:
         if (vert) {
            al_draw_filled_rectangle(d->x+0.5, d->y+0.5, d->x+d->w/2-2.5, d->y+d->h-1.5, d->bg);
            al_draw_filled_rectangle(d->x+0.5+d->w/2-1, d->y+0.5, d->x+d->w/2+0.5, d->y+d->h-1.5, d->fg);
            al_draw_filled_rectangle(d->x+0.5+d->w/2+2, d->y+0.5, d->x+d->w-1.5, d->y+d->h-1.5, d->bg);
         }
         else {
            al_draw_filled_rectangle(d->x+0.5, d->y+0.5, d->x+d->w-1.5, d->y+d->h/2-2.5, d->bg);
            al_draw_filled_rectangle(d->x+0.5, d->y+0.5+d->h/2-1, d->x+d->w-1.5, d->y+d->h/2+0.5, d->fg);
            al_draw_filled_rectangle(d->x+0.5, d->y+0.5+d->h/2+2, d->x+d->w-1.5, d->y+d->h-1.5, d->bg);
         }

         /* okay, background and slot are drawn, now draw the handle */
         if (slhan) {
            if (vert) {
               slx = d->x+(d->w/2)-(al_get_bitmap_width(slhan)/2);
               sly = d->y+(d->h-1)-(hh+slp);
            }
            else {
               slx = d->x+slp;
               sly = d->y+(d->h/2)-(al_get_bitmap_height(slhan)/2);
            }
            al_draw_bitmap(slhan, slx, sly, 0);
         }
         else {
            /* draw default handle */
            if (vert) {
               slx = d->x;
               sly = d->y+(d->h)-(hh+slp);
               sly = d->y+slp;
               slw = d->w-1;
               slh = hh-1;
            }
            else {
               slx = d->x+slp;
               sly = d->y;
               slw = hh-1;
               slh = d->h-1;
            }

            /* draw body */
            float rx = 6;
            float ry = 6;
            int m = (slw < slh)?slw:slh;
            if (rx > m/2) rx = m/2;
            if (ry > m/2) ry = m/2;

            al_draw_filled_rounded_rectangle(slx+2, sly, slx+(slw-2), sly+slh, rx, ry, d->fg);
#if 0
            rectfill(gui_bmp, slx+2, sly, slx+(slw-2), sly+slh, sfg);
            vline(gui_bmp, slx+1, sly+1, sly+slh-1, sfg);
            vline(gui_bmp, slx+slw-1, sly+1, sly+slh-1, sfg);
            vline(gui_bmp, slx, sly+2, sly+slh-2, sfg);
            vline(gui_bmp, slx+slw, sly+2, sly+slh-2, sfg);
            vline(gui_bmp, slx+1, sly+2, sly+slh-2, d->bg);
            hline(gui_bmp, slx+2, sly+1, slx+slw-2, d->bg);
            putpixel(gui_bmp, slx+2, sly+2, d->bg);
#endif
         }

         if (d->flags & (D_GOTFOCUS | D_TRACKMOUSE))
            al_draw_rectangle(d->x, d->y, d->x+d->w-1, d->y+d->h-1, d->fg, 4.0);
         break;

      case MSG_WANTFOCUS:
         return D_WANTFOCUS;

      case MSG_LOSTFOCUS:
         if (d->flags & D_TRACKMOUSE)
            return D_WANTFOCUS;
         break;

      case MSG_KEY:
         if (!(d->flags & D_GOTFOCUS))
            return D_WANTFOCUS;
         else
            return D_O_K;

      case MSG_KEYREPEAT:
      case MSG_KEYUP:
         /* handle movement keys to move slider */

         if (vert) {
            upkey = ALLEGRO_KEY_DOWN;
            downkey = ALLEGRO_KEY_UP;
            pgupkey = ALLEGRO_KEY_PGDN;
            pgdnkey = ALLEGRO_KEY_PGUP;
            homekey = ALLEGRO_KEY_HOME;
            endkey = ALLEGRO_KEY_END;
         }
         else {
            upkey = ALLEGRO_KEY_RIGHT;
            downkey = ALLEGRO_KEY_LEFT;
            pgupkey = ALLEGRO_KEY_PGDN;
            pgdnkey = ALLEGRO_KEY_PGUP;
            homekey = ALLEGRO_KEY_HOME;
            endkey = ALLEGRO_KEY_END;
         }

         if (c == upkey)
            delta = 1;
         else if (c == downkey)
            delta = -1;
         else if (c == pgdnkey)
            delta = -d->d1 / 16;
         else if (c == pgupkey)
            delta = d->d1 / 16;
         else if (c == homekey)
            delta = -d->d2;
         else if (c == endkey)
            delta = d->d1 - d->d2;
         else
            delta = 0;

         if (delta) {
            oldpos = slp;
            oldval = d->d2;

            while (1) {
               d->d2 = d->d2+delta;
               slpos = slratio*d->d2;
               slp = (int)slpos;
               if ((slp != oldpos) || (d->d2 <= 0) || (d->d2 >= d->d1))
                  break;
            }

            if (d->d2 < 0)
               d->d2 = 0;
            if (d->d2 > d->d1)
               d->d2 = d->d1;

            retval = D_USED_CHAR;

            if (d->d2 != oldval) {
               /* call callback function here */
               if (d->dp2) {
                  proc = d->dp2;
                  retval |= (*proc)(d->dp3, d->d2);
               }

               retval |= D_REDRAWME;
            }
         }
         break;

      case MSG_VBALL:   // MSG_WHEEL
         if (vert) {
            oldval = d->d2;
            d->d2-=c;
            if (d->d2 < 0) d->d2 = 0;
            if (d->d2 > d->d1) d->d2 = d->d1;
            if (d->d2 != oldval) {
               /* call callback function here */
               if (d->dp2) {
                  proc = d->dp2;
                  retval |= (*proc)(d->dp3, d->d2);
               }

               retval |= D_REDRAWME;
            }
         }
         break;

      case MSG_HBALL:
         if (!vert) {
            oldval = d->d2;
            d->d2-=c;
            if (d->d2 < 0) d->d2 = 0;
            if (d->d2 > d->d1) d->d2 = d->d1;
            if (d->d2 != oldval) {
               /* call callback function here */
               if (d->dp2) {
                  proc = d->dp2;
                  retval |= (*proc)(d->dp3, d->d2);
               }

               retval |= D_REDRAWME;
            }
         }
         break;

      case MSG_CLICK:
         /* track the mouse until it is released */
         break;

      case MSG_MOUSEUP:
         d->flags &= ~D_TRACKMOUSE;
         break;


      case MSG_MOUSEDOWN:
         d->flags |= D_TRACKMOUSE;

      case MSG_MOUSEMOVE:
         msx = d->mousex;
         msy = d->mousey;
         oldval = d->d2;
         if (vert)
            //mp = (d->y+d->h-hmar)-msy;
            mp = msy-(d->y-hmar);
         else
            mp = msx-(d->x+hmar);
         if (mp < 0)
            mp = 0;
         if (mp > irange-hh)
            mp = irange-hh;
         slpos = mp;
         slmax = slpos / slratio;
         newpos = slmax;
         if (newpos != oldval) {
            d->d2 = newpos;

            /* call callback function here */
            if (d->dp2 != NULL) {
               proc = d->dp2;
               retval |= (*proc)(d->dp3, d->d2);
            }
            retval |= D_REDRAWME;
         }
         break;
   }

   return retval;
}

int egg_scroll_proc(int msg, EGG_DIALOG *d, int c)
{
   assert(d);
   int vert = true;
   int hh = 0;
   int slx, sly, slh, slw;
   int retval = D_O_K;
   int range = d->d1;
   int value = d->d2;
   int offset = 0;

   if (msg == MSG_DRAW) {
      /* check for slider direction */
      if (d->h < d->w)
         vert = false;

      if (vert) {
         hh = d->h * d->h / (range + d->h);

         if (hh > d->h) hh = d->h;

         offset = (d->h - hh) * value / range;
      } else {
         hh = d->w * d->w / (range + d->h);

         if (hh > d->w) hh = d->w;

         offset = (d->w - hh) * value / range;
      }

      if (vert) {
         slx = d->x+2;
         sly = d->y+2 + offset;
         slw = d->w-3;
         slh = hh-4;
      } else {
         slx = d->x+2 + offset;
         sly = d->y+2;
         slw = hh-3;
         slh = d->h-4;
      }

      al_draw_filled_rectangle(d->x+0.5, d->y+0.5, d->x+d->w-0.5, d->y+d->h-0.5, d->bg);
      al_draw_rectangle(d->x+0.5, d->y+0.5, d->x+d->w-0.5, d->y+d->h-0.5, d->fg, 1.0);

      /* draw body */
      if (d->bmp_handle) {
         NINE_PATCH_BITMAP *p9 = d->bmp_handle;
         int w = max(slw, get_nine_patch_bitmap_min_width(p9));
         int h = max(slh, get_nine_patch_bitmap_min_height(p9));
         draw_nine_patch_bitmap(p9, slx+0.5, sly+0.5, w, h);
      } else {
         al_draw_filled_rectangle(slx+0.5, sly+0.5, slx+slw-0.5, sly+slh-0.5, d->mg);
      }

      if (d->flags & D_GOTFOCUS)
         al_draw_rectangle(d->x+0.5, d->y+0.5, d->x+d->w-0.5, d->y+d->h-0.5, d->fg, 4.0);

      return retval;
   }

   return egg_slider_proc(msg, d, c);
}

static int draw_textbox(EGG_DIALOG *d, bool draw, int *margin)
{
   assert(d);
   const ALLEGRO_FONT *font = d->dp2;
   if (!font) font = d->font;
   if (!font) font = d->default_font;
   const char *text = d->dp;

   int r_margin = 0;
   int x, y;

   if (margin) r_margin = *margin;

   char *buf = malloc(strlen(text));
   while (true) {
      int x0 = d->x+4;
      int y0 = d->y - d->d2;
      int w = d->w - 4 - r_margin;

      const char *s = text;

      x = 0;
      y = 0;

      if (draw)
         egg_box_proc(MSG_DRAW, d, 0);

      /* Parse the text and determine how much space we need to draw all of it (with and without a scroll bar) */
      while (s && *s) {
         char *p = buf;

         if (*s && *s == '\n') {
            x = 0;
            y += al_get_font_line_height(font);
            s++;
            continue;
         }

         /* Copy next word */
         while (*s && uisspace(*s) && *s != '\n') {
            *p = *s;
            p++; s++;
         }
         while (*s && !uisspace(*s) && *s != '\n') {
            *p = *s;
            p++; s++;
         }
         *p = '\0';

         int tw = al_get_text_width(font, buf);

         if (x+4+tw < w) {
            p = buf;
            goto draw;
         }

         if (x == 0) {  /* Word is too long - must break the word (TODO) */
         }

         /* Strip leading space */
         p = buf;
         while (*p && uisspace(*p)) p++;
         tw = al_get_text_width(font, p);

         /* Line-wrap */
         y += al_get_font_line_height(font);
         x = 0;

draw:
         //if (!draw) printf("(%d %d) '%s'\n", x, y, p);
         if (draw) al_draw_textf(font, d->fg, x0+x, y0+y, 0, "%s", p);
         x += tw;
      }

      y += al_get_font_line_height(font);

      if (draw)
         break;

      if (r_margin)
         break;

      if (y < d->h)
         break;

      r_margin = 16;
   }

   free(buf);

   if (margin) *margin = r_margin;

   return y;
}

int egg_textbox_proc(int msg, EGG_DIALOG *d, int c)
{
   int ret = D_O_K;

   assert(d);
   bool external_scroll = d->flags & D_SCROLLBAR && d[1].proc == egg_scroll_proc;

   EGG_DIALOG dd = { .proc = egg_scroll_proc,
                     .x = d->x+d->w - d->d3,
                     .y = d->y,
                     .w = d->d3,
                     .h = d->h,
                     .fg = d->fg,
                     .bg = d->bg,
                     .mg = d->mg,
                     .flags = d->flags,
                     .d1 = d->d1 - d->h,
                     .d2 = d->d2,
                     .mousex = d->mousex,
                     .mousey = d->mousey };

   if (msg != MSG_START && external_scroll) {
      d->d1 = d[1].d1 + d->h;
      d->d2 = d[1].d2;
      d->flags |= d[1].flags & D_DIRTY;
   }

   switch (msg) {
      case MSG_START:
         /* Query size of required text box (d1) and size of scroll bar (d3) */
         d->d3 = 0;
         d->d1 = draw_textbox(d, false, &d->d3);
         //printf("%d\n", d->d1);
         if (external_scroll) {
            initialise_vertical_scroll(d, d+1, d->d3);
            d[1].flags &= ~D_HIDDEN;
            if (d->d3 <= 0) d[1].flags |= D_HIDDEN;
         }
         break;

      case MSG_DRAW:
         draw_textbox(d, true, &d->d3);
         if (d->d3 > 0 && !external_scroll)
            ret |= egg_scroll_proc(msg, &dd, c);
         break;

      case MSG_HBALL:
      case MSG_VBALL:
         if (external_scroll)
            ret |= d[1].proc(msg, d+1, c);
         else if (d->d3 > 0)
            ret |= egg_scroll_proc(msg, &dd, c);
         break;

      default:
         if (d->d3 > 0 && !external_scroll)
            ret |= egg_scroll_proc(msg, &dd, c);
         break;
   }

   if (msg != MSG_START && !external_scroll) {
      d->d1 = dd.d1 + d->h;
      d->d2 = dd.d2;
      d->flags = dd.flags;
   }

   return ret;
}

/* typedef for the listbox callback functions */
typedef const char *(getfuncptr)(int index, int *num_elem, void *dp3);

int egg_list_proc(int msg, EGG_DIALOG *d, int c)
{
   int ret = D_O_K;
   assert(d);

   getfuncptr *func = d->dp;
   const ALLEGRO_FONT *font = d->font;
   if (!font) font = d->default_font;
   int nelem = 0;
   int y = d->y;

   assert(func);

   func(-1, &nelem, d->dp3);

   EGG_DIALOG dd = { .proc = egg_scroll_proc,
                     .x = d->x+d->w - d->d3,
                     .y = d->y,
                     .w = d->d3,
                     .h = d->h,
                     .fg = d->fg,
                     .bg = d->bg,
                     .mg = d->mg,
                     .flags = d->flags,
                     .d1 = (nelem + 1) * al_get_font_line_height(font) - d->h,
                     .d2 = d->d2 * al_get_font_line_height(font),
                     .mousex = d->mousex,
                     .mousey = d->mousey };

   switch (msg) {
      case MSG_START:
         /* Query size of required text box (d1) and size of scroll bar (d3) */
         d->d3 = 16;
         //d->d1 = draw_textbox(d, false, &d->d3);
         //printf("%d\n", d->d1);
         break;

      case MSG_KEYUP:
         if (d->d3 > 0 && dd.d1 > 0 && d->mousex > dd.x)
            ret |= egg_scroll_proc(msg, &dd, c);
         else {
            int last_idx = d->d2 + d->h / al_get_font_line_height(font)-1;

            if (c == ALLEGRO_KEY_DOWN) {
               d->d1++;
               if (d->d1 > nelem-1) d->d1 = nelem-1;
               ret |= D_USED_KEY;
            } else if (c == ALLEGRO_KEY_UP) {
               d->d1--;
               if (d->d1 < 0) d->d1 = 0;
               ret |= D_USED_KEY;
            }
            if (ret & D_USED_KEY) {
               if (d->d1 < d->d2) d->d2--;
               if (d->d1 > last_idx) d->d2++;
               dd.d2 = d->d2 * al_get_font_line_height(font);

               ret |= D_REDRAWME;
            }
         }
         break;

      case MSG_MOUSEUP:
         if (d->d3 > 0 && dd.d1 > 0 && d->mousex > dd.x)
            ret |= egg_scroll_proc(msg, &dd, c);
         break;

      case MSG_MOUSEDOWN:
         if (d->d3 > 0 && dd.d1 > 0 && d->mousex > dd.x)
            ret |= egg_scroll_proc(msg, &dd, c);
         break;

      case MSG_CLICK:
         if (d->d3 <= 0 || d->mousex < dd.x) {
            int idx = d->d2 + (d->mousey - d->y) / al_get_font_line_height(font);
            if (idx >= nelem) idx = nelem-1;
            if (idx < 0) idx = 0;
            if (d->d1 != idx) {
               d->d1 = idx;
               ret |= D_REDRAWME;
            }
         }
         if (d->d3 > 0 && dd.d1 > 0 && d->mousex > dd.x)
            ret |= egg_scroll_proc(msg, &dd, c);
         break;

      /* TODO: handle scrolling with arrow keys (next/previous item in the list; scroll list as needed) */

      case MSG_DRAW:
         egg_box_proc(MSG_DRAW, d, 0);
         if (d->flags & D_GOTFOCUS)
            al_draw_rectangle(d->x+0.5, d->y+0.5, d->x+d->w-0.5, d->y+d->h-0.5, d->fg, 3.0);

         for (int n = d->d2; n<nelem; n++) {
            ALLEGRO_COLOR fg = d->fg;
            const char *text = func(n, NULL, d->dp3);
            if (d->d1 == n) {
               al_draw_filled_rectangle(d->x+2.5,y+2.5,d->x+d->w-1.5,y+al_get_font_line_height(font)+1.5, d->fg);
               fg = d->bg;
            }
            al_draw_text(font, fg, d->x+2, y+2, 0, text);
            y += al_get_font_line_height(font);
         }
         if (d->d3 > 0 && dd.d1 > 0)
            ret |= egg_scroll_proc(msg, &dd, c);
         break;

      default:
         if (d->d3 > 0 && dd.d1 > 0)
            ret |= egg_scroll_proc(msg, &dd, c);
         break;
   }

   if (msg != MSG_START) {
      d->d2 = dd.d2 / al_get_font_line_height(font);
      if (d->d2 >= nelem) d->d2 = nelem-1;
      if(d->d2 < 0) d->d2 = 0;
      d->flags = dd.flags;
   }

   return ret;
}

/* d_edit_proc:
 *  An editable text object (the dp field points to the string). When it
 *  has the input focus (obtained by clicking on it with the mouse), text
 *  can be typed into this object. The d1 field specifies the maximum
 *  number of characters that it will accept, and d2 is the text cursor
 *  position within the string.
 */
int egg_edit_proc(int msg, EGG_DIALOG *d, int c)
{
   const ALLEGRO_FONT *font = d->font;
   ALLEGRO_COLOR fg, tc;
   int last_was_space, new_pos, i, k;
   int f, l, p, w, x, b, scroll, h;
   char buf[16];
   char *s, *t;
   assert(d);

   if (!font) font = d->default_font;

   s = d->dp;
   l = ustrlen(s);
   if (d->d2 > l)
      d->d2 = l;

   /* calculate maximal number of displayable characters */
   if (d->d2 == l)  {
      usetc(buf+usetc(buf, ' '), 0);
      x = al_get_text_width(font, buf);
   }
   else
      x = 0;

   b = 0;

   for (p=d->d2; p>=0; p--) {
      usetc(buf+usetc(buf, ugetat(s, p)), 0);
      x += al_get_text_width(font, buf);
      b++;
      if (x > d->w)
         break;
   }

   if (x <= d->w) {
      b = l;
      scroll = false;
   }
   else {
      b--;
      scroll = true;
   }

   switch (msg) {

      case MSG_START:
         d->d2 = l;
         break;

      case MSG_DRAW:
         h = min(d->h, al_get_font_line_height(font)+3);
         al_draw_filled_rectangle(d->x+0.5, d->y+0.5, d->x+d->w-0.5, d->y+h-0.5, d->bg);
         fg = (d->flags & D_DISABLED) ? d->mg : d->fg;
         x = 0;

         if (scroll) {
            p = d->d2-b+1;
            b = d->d2;
         }
         else
            p = 0;

         for (; p<=b; p++) {
            f = ugetat(s, p);
            usetc(buf+usetc(buf, (f) ? f : ' '), 0);
            w = al_get_text_width(font, buf);
            if (x+w > d->w)
               break;
            f = ((p == d->d2) && (d->flags & D_GOTFOCUS));
            tc = f ? d->bg : d->fg;

            if (f) {
               int dx, dy, w, hh;
               al_get_text_dimensions(font, buf, &dx, &dy, &w, &hh);
               if (w == 0) al_get_text_dimensions(font, "x", &dx, &dy, &w, &hh);
               al_draw_filled_rectangle(d->x+x+dx+0.5, d->y+0.5, d->x+x+dx+w-0.5, d->y+h-0.5, d->fg);
            }
            al_draw_text(font, tc, d->x+x, d->y+1, 0, buf);
            x += w;
         }
         break;

      case MSG_MOUSEDOWN:
         x = d->x;

         if (scroll) {
            p = d->d2-b+1;
            b = d->d2;
         }
         else
            p = 0;

         for (; p<b; p++) {
            usetc(buf+usetc(buf, ugetat(s, p)), 0);
            x += al_get_text_width(font, buf);
            if (x > d->mousex)
               break;
         }
         d->d2 = clamp(0, p, l);
         d->flags |= D_DIRTY;
         break;

      case MSG_WANTFOCUS:
      case MSG_LOSTFOCUS:
      case MSG_KEY:
         return D_WANTFOCUS;

      case MSG_KEYUP:
         if (c == ALLEGRO_KEY_TAB || c == ALLEGRO_KEY_ESCAPE) {
            d->flags |= D_INTERNAL;
            return D_O_K;
         }

         d->flags |= D_DIRTY;
         return D_USED_KEY;

      case MSG_KEYREPEAT:
      case MSG_KEYDOWN:
         (void)0;
         d->flags &= ~D_INTERNAL;
         ALLEGRO_KEYBOARD_STATE state;
         al_get_keyboard_state(&state);
         int key_shifts = 0;
         if (al_key_down(&state, ALLEGRO_KEY_LCTRL) || al_key_down(&state, ALLEGRO_KEY_RCTRL))
            key_shifts |= ALLEGRO_KEYMOD_CTRL;

         if (c == ALLEGRO_KEY_LEFT) {
            if (d->d2 > 0) {
               if (key_shifts & ALLEGRO_KEYMOD_CTRL) {
                  last_was_space = true;
                  new_pos = 0;
                  t = s;
                  for (i = 0; i < d->d2; i++) {
                     k = ugetx(&t);
                     if (uisspace(k))
                        last_was_space = true;
                     else if (last_was_space) {
                        last_was_space = false;
                        new_pos = i;
                     }
                  }
                  d->d2 = new_pos;
               }
               else
                  d->d2--;
            }
         }
         else if (c == ALLEGRO_KEY_RIGHT) {
            if (d->d2 < l) {
               if (key_shifts & ALLEGRO_KEYMOD_CTRL) {
                  t = s + uoffset(s, d->d2);
                  for (k = ugetx(&t); uisspace(k); k = ugetx(&t))
                     d->d2++;
                  for (; k && !uisspace(k); k = ugetx(&t))
                     d->d2++;
               }
               else
                  d->d2++;
            }
         }
         else if (c == ALLEGRO_KEY_HOME) {
            d->d2 = 0;
         }
         else if (c == ALLEGRO_KEY_END) {
            d->d2 = l;
         }
         else if (c == ALLEGRO_KEY_DELETE) {
            d->flags |= D_INTERNAL;
            if (d->d2 < l)
               uremove(s, d->d2);
         }
         else if (c == ALLEGRO_KEY_BACKSPACE) {
            d->flags |= D_INTERNAL;
            if (d->d2 > 0) {
               d->d2--;
               uremove(s, d->d2);
            }
         }
         else if (c == ALLEGRO_KEY_ENTER) {
            if (d->flags & D_EXIT) {
               d->flags |= D_DIRTY;
               return D_CLOSE;
            }
            else
               return D_O_K;
         }
         else if (c == ALLEGRO_KEY_TAB) {
            d->flags |= D_INTERNAL;
            return D_O_K;
         }
         else {
            /* don't process regular keys here: MSG_CHAR will do that */
            break;
         }
         d->flags |= D_DIRTY;
         return D_USED_KEY;

      case MSG_CHAR:
         if ((c >= ' ') && (uisok(c)) && ~d->flags & D_INTERNAL) {
            if (l < d->d1) {
               uinsert(s, d->d2, c);
               d->d2++;

               d->flags |= D_DIRTY;
            }
            return D_USED_CHAR;
         }
         break;
   }

   return D_O_K;
}

int egg_edit_integer_proc(int msg, EGG_DIALOG *d, int c)
{
   if (msg == MSG_CHAR)
      if (!isdigit(c)) return D_O_K;

   if (msg == MSG_DRAW) {
      char *string = d->dp;
      if (string == NULL || strlen(string) == 0) d->dp = "0";
      int ret = egg_edit_proc(msg, d, c);
      d->dp = string;
      return ret;
   }

   return egg_edit_proc(msg, d, c);
}

/* d_keyboard_proc:
 *  Invisible object for implementing keyboard shortcuts. When its key
 *  is pressed, it calls the function pointed to by dp (if any), which gets passed
 *  the dialog object as an argument. This function should return
 *  an integer, which will be passed back to the dialog manager. The key
 *  can be specified by putting an ASCII code in the key field or by
 *  putting ALLEGRO_KEY_* codes in d1 and d2.
 */
int egg_keyboard_proc(int msg, EGG_DIALOG *d, int c)
{
   int (*proc)(EGG_DIALOG *d);
   int ret = D_O_K;
   assert(d);

   switch (msg) {
      case MSG_START:
         d->w = d->h = 0;
         break;

      case MSG_CHAR:
         if ((c != d->d1) && (c != d->d2))
            break;

         ret |= D_USED_CHAR;
         /* fall through */

      case MSG_KEY:
         if (proc) {
            proc = d->dp;
            ret |= (*proc)(d);
         }
         if (d->flags & D_EXIT)
            ret |= D_CLOSE;
         break;
   }

   return ret;
}
