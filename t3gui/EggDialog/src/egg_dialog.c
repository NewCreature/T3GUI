#include <allegro5/events.h>
#include <stdbool.h>
#include <ctype.h>
#include "egg_dialog/egg_dialog.h"
#include "assert.h"

#define EGGC_STOP    ALLEGRO_GET_EVENT_TYPE('E','G','G', 'S')
#define EGGC_PAUSE   ALLEGRO_GET_EVENT_TYPE('E','G','G', 'P')
#define EGGC_RESUME  ALLEGRO_GET_EVENT_TYPE('E','G','G', 'R')
static ALLEGRO_EVENT_SOURCE dialog_controller;
static bool init = false;

static const ALLEGRO_FONT *gui_default_font = NULL;

bool egg_initialise(void)
{
   if (init) return true;

   al_init_user_event_source(&dialog_controller);
   init = true;

   return true;
}

static void egg_event_destructor(ALLEGRO_USER_EVENT *event)
{
   return;
}

void egg_set_gui_font(const ALLEGRO_FONT *font)
{
   gui_default_font = font;
}

/* find_mouse_object:
 *  Finds which object the mouse is on top of.
 */
static int find_mouse_object(EGG_DIALOG *d, int mouse_x, int mouse_y)
{
   int mouse_object = -1;
   int c;
   int res;
   assert(d);

   for (c=0; d[c].proc; c++) {
      if ((mouse_x >= d[c].x) && (mouse_y >= d[c].y) &&
          (mouse_x < d[c].x + d[c].w) && (mouse_y < d[c].y + d[c].h) &&
          (!(d[c].flags & (D_HIDDEN | D_DISABLED)))) {
         /* check if this object wants the mouse */
         res = egg_object_message(d+c, MSG_WANTMOUSE, 0);
         if (!(res & D_DONTWANTMOUSE)) {
            mouse_object = c;
         }
      }
   }

   return mouse_object;
}



/* offer_focus:
 *  Offers the input focus to a particular object.
 */
static int offer_focus(EGG_DIALOG *dialog, int obj, int *focus_obj, int force)
{
   int res = D_O_K;
   assert(dialog);
   assert(focus_obj);

   if ((obj == *focus_obj) ||
         ((obj >= 0) && (dialog[obj].flags & (D_HIDDEN | D_DISABLED))))
      return D_O_K;

   /* check if object wants the focus */
   if (obj >= 0) {
      res = egg_object_message(dialog+obj, MSG_WANTFOCUS, 0);
      if (res & D_WANTFOCUS)
         res ^= D_WANTFOCUS;
      else
         obj = -1;
   }

   if ((obj >= 0) || (force)) {
      /* take focus away from old object */
      if (*focus_obj >= 0) {
         res |= egg_object_message(dialog+*focus_obj, MSG_LOSTFOCUS, 0);
         if (res & D_WANTFOCUS) {
            if (obj < 0)
               return D_O_K;
            else
               res &= ~D_WANTFOCUS;
         }
         dialog[*focus_obj].flags &= ~D_GOTFOCUS;
         res |= D_REDRAW_ALL;
      }

      *focus_obj = obj;

      /* give focus to new object */
      if (obj >= 0) {
         dialog[obj].flags |= D_GOTFOCUS;
         res |= egg_object_message(dialog+obj, MSG_GOTFOCUS, 0);
         res |= D_REDRAW_ALL;
      }
   }

   return res;
}



/* find_dialog_focus:
 *  Searches the dialog for the object which has the input focus, returning
 *  its index, or -1 if the focus is not set. Useful after do_dialog() exits
 *  if you need to know which object was selected.
 */
int egg_find_dialog_focus(EGG_DIALOG *dialog)
{
   int c;
   assert(dialog);

   for (c=0; dialog[c].proc; c++)
      if (dialog[c].flags & D_GOTFOCUS)
         return c;

   return -1;
}



#define MAX_OBJECTS     512

typedef struct OBJ_LIST
{
   int index;
   int diff;
} OBJ_LIST;


/* Weight ratio between the orthogonal direction and the main direction
   when calculating the distance for the focus algorithm. */
#define DISTANCE_RATIO  8

/* Maximum size (in bytes) of a dialog array. */
#define MAX_SIZE  0x10000  /* 64 kb */

enum axis { X_AXIS, Y_AXIS };



/* obj_list_cmp:
 *  Callback function for qsort().
 */
static int obj_list_cmp(const void *e1, const void *e2)
{
   return (((OBJ_LIST *)e1)->diff - ((OBJ_LIST *)e2)->diff);
}



/* cmp_tab:
 *  Comparison function for tab key movement.
 */
static int cmp_tab(const EGG_DIALOG *d1, const EGG_DIALOG *d2)
{
   int ret = (int)((const unsigned long)d2 - (const unsigned long)d1);

   /* Wrap around if d2 is before d1 in the dialog array. */
   if (ret < 0)
      ret += MAX_SIZE;

   return ret;
}



/* cmp_shift_tab:
 *  Comparison function for shift+tab key movement.
 */
static int cmp_shift_tab(const EGG_DIALOG *d1, const EGG_DIALOG *d2)
{
   int ret = (int)((const unsigned long)d1 - (const unsigned long)d2);

   /* Wrap around if d2 is after d1 in the dialog array. */
   if (ret < 0)
      ret += MAX_SIZE;

   return ret;
}



/* min_dist:
 *  Returns the minimum distance between dialogs 'd1' and 'd2'. 'main_axis'
 *  is taken account to give different weights to the axes in the distance
 *  formula, as well as to shift the actual position of 'd2' along the axis
 *  by the amount specified by 'bias'.
 */
static int min_dist(const EGG_DIALOG *d1, const EGG_DIALOG *d2, enum axis main_axis, int bias)
{
   int x_left = d1->x - d2->x - d2->w + 1;
   int x_right = d2->x - d1->x - d1->w + 1;
   int y_top = d1->y - d2->y - d2->h + 1;
   int y_bottom = d2->y - d1->y - d1->h + 1;

   if (main_axis == X_AXIS) {
      x_left -= bias;
      x_right += bias;
      y_top *= DISTANCE_RATIO;
      y_bottom *= DISTANCE_RATIO;
   }
   else {
      x_left *= DISTANCE_RATIO;
      x_right *= DISTANCE_RATIO;
      y_top -= bias;
      y_bottom += bias;
   }

   if (x_left > 0) { /* d2 is left of d1 */
      if (y_top > 0)  /* d2 is above d1 */
         return x_left + y_top;
      else if (y_bottom > 0)  /* d2 is below d1 */
         return x_left + y_bottom;
      else  /* vertically overlapping */
         return x_left;
   }
   else if (x_right > 0) { /* d2 is right of d1 */
      if (y_top > 0)  /* d2 is above d1 */
         return x_right + y_top;
      else if (y_bottom > 0)  /* d2 is below d1 */
         return x_right + y_bottom;
      else  /* vertically overlapping */
         return x_right;
   }
   /* horizontally overlapping */
   else if (y_top > 0)  /* d2 is above d1 */
      return y_top;
   else if (y_bottom > 0)  /* d2 is below d1 */
      return y_bottom;
   else  /* overlapping */
      return 0;
}



/* cmp_right:
 *  Comparison function for right arrow key movement.
 */
static int cmp_right(const EGG_DIALOG *d1, const EGG_DIALOG *d2)
{
   int bias;
   int screen_w = 3000;//al_get_display_width(al_get_current_display());

   /* Wrap around if d2 is not fully contained in the half-plan
      delimited by d1's right edge and not containing it. */
   if (d2->x < d1->x + d1->w)
      bias = +screen_w;
   else
      bias = 0;

   return min_dist(d1, d2, X_AXIS, bias);
}



/* cmp_left:
 *  Comparison function for left arrow key movement.
 */
static int cmp_left(const EGG_DIALOG *d1, const EGG_DIALOG *d2)
{
   int bias;
   int screen_w = 3000;//al_get_display_width(al_get_current_display());

   /* Wrap around if d2 is not fully contained in the half-plan
      delimited by d1's left edge and not containing it. */
   if (d2->x + d2->w > d1->x)
      bias = -screen_w;
   else
      bias = 0;

   return min_dist(d1, d2, X_AXIS, bias);
}



/* cmp_down:
 *  Comparison function for down arrow key movement.
 */
static int cmp_down(const EGG_DIALOG *d1, const EGG_DIALOG *d2)
{
   int bias;
   int screen_h = 3000;//al_get_display_height(al_get_current_display());

   /* Wrap around if d2 is not fully contained in the half-plan
      delimited by d1's bottom edge and not containing it. */
   if (d2->y < d1->y + d1->h)
      bias = +screen_h;
   else
      bias = 0;

   return min_dist(d1, d2, Y_AXIS, bias);
}



/* cmp_up:
 *  Comparison function for up arrow key movement.
 */
static int cmp_up(const EGG_DIALOG *d1, const EGG_DIALOG *d2)
{
   int bias;
   int screen_h = 3000;//al_get_display_height(al_get_current_display());

   /* Wrap around if d2 is not fully contained in the half-plan
      delimited by d1's top edge and not containing it. */
   if (d2->y + d2->h > d1->y)
      bias = -screen_h;
   else
      bias = 0;

   return min_dist(d1, d2, Y_AXIS, bias);
}



/* move_focus:
 *  Handles arrow key and tab movement through a dialog, deciding which
 *  object should be given the input focus.
 */
static int move_focus(EGG_DIALOG *d, int keycode, bool shift, int *focus_obj)
{
   int (*cmp)(const EGG_DIALOG *d1, const EGG_DIALOG *d2);
   OBJ_LIST obj[MAX_OBJECTS];
   int obj_count = 0;
   int fobj, c;
   int res = D_O_K;

   /* choose a comparison function */
   switch (keycode) {
      case ALLEGRO_KEY_TAB:   cmp = (shift == false) ? cmp_tab : cmp_shift_tab; break;
      case ALLEGRO_KEY_RIGHT: cmp = cmp_right; break;
      case ALLEGRO_KEY_LEFT:  cmp = cmp_left;  break;
      case ALLEGRO_KEY_DOWN:  cmp = cmp_down;  break;
      case ALLEGRO_KEY_UP:    cmp = cmp_up;    break;
      default:                return D_O_K;
   }

   /* fill temporary table */
   for (c=0; d[c].proc; c++) {
      if (((*focus_obj < 0) || (c != *focus_obj))
            && !(d[c].flags & (D_DISABLED | D_HIDDEN))) {
         obj[obj_count].index = c;
         if (*focus_obj >= 0)
            obj[obj_count].diff = cmp(d+*focus_obj, d+c);
         else
            obj[obj_count].diff = c;
         obj_count++;
         if (obj_count >= MAX_OBJECTS)
            break;
      }
   }

   /* sort table */
   qsort(obj, obj_count, sizeof(OBJ_LIST), obj_list_cmp);

   /* find an object to give the focus to */
   fobj = *focus_obj;
   for (c=0; c<obj_count; c++) {
      res |= offer_focus(d, obj[c].index, focus_obj, false);
      if (fobj != *focus_obj)
         break;
   }

   return res;
}



/* object_message:
 *  Sends a message to a widget.
 *  Do NOT call this from the callback function!
 */
int egg_object_message(EGG_DIALOG *dialog, int msg, int c)
{

   int ret = 0;
   assert(dialog);

   if (msg == MSG_DRAW) {
      if (dialog->flags & D_HIDDEN)
         return D_O_K;
      al_set_clipping_rectangle(dialog->x, dialog->y, dialog->w, dialog->h);
   }

   /* Callback? */
   if (dialog->callback) {
      ret = dialog->callback(msg, dialog, c);
      if (ret & D_CALLBACK)
         return ret & ~D_CALLBACK;
   }

   ret |= dialog->proc(msg, dialog, c);

   if (ret & D_REDRAWME) {
      dialog->flags |= D_DIRTY;
      ret &= ~D_REDRAWME;
   }

   return ret;
}


static inline void MESSAGE(EGG_DIALOG_PLAYER *player, int i, int msg, int c)
{
   if (i < 0) return;

   int r = egg_object_message(player->dialog+i, msg, c);
   if (r != D_O_K) {
      player->res |= r;
      player->obj = i;
   }
}

/* dialog_message:
 *  Sends a message to all the objects in a dialog. If any of the objects
 *  return values other than D_O_K, returns the value and sets obj to the
 *  object which produced it.
 */
int egg_dialog_message(EGG_DIALOG *dialog, int msg, int c, int *obj)
{
   int count, res, r, force, try;
   assert(dialog);

   force = ((msg == MSG_START) || (msg == MSG_END) || (msg >= MSG_USER));

   res = D_O_K;

   /* If a menu spawned by a d_menu_proc object is active, the dialog engine
    * has effectively been shutdown for the sake of safety. This means that
    * we can't send the message to the other objects in the dialog. So try
    * first to send the message to the d_menu_proc object and, if the menu
    * is then not active anymore, send it to the other objects as well.
    */
   //if (active_menu_player) {
   //   try = 2;
   //   menu_dialog = active_menu_player->dialog;
   //}
   //else
      try = 1;

   for (; try > 0; try--) {
      for (count=0; dialog[count].proc; count++) {
         //if ((try == 2) && (&dialog[count] != menu_dialog))
         //   continue;

         if ((force) || (!(dialog[count].flags & D_HIDDEN))) {
            r = egg_object_message(dialog+count, msg, c);

            if (r != D_O_K) {
               res |= r;
               if (obj)
                  *obj = count;
            }

            if ((msg == MSG_IDLE) && (dialog[count].flags & (D_DIRTY | D_HIDDEN)) == D_DIRTY) {
               dialog[count].flags &= ~D_DIRTY;
               res |= D_REDRAW;
            }
         }
      }

      //if (active_menu_player)
      //   break;
   }

   return res;
}

/* utolower:
 *  Unicode-aware version of the ANSI tolower() function.
 */
static int utolower(int c)
{
   if ((c >= 65 && c <= 90) ||
         (c >= 192 && c <= 214) ||
         (c >= 216 && c <= 222) ||
         (c >= 913 && c <= 929) ||
         (c >= 931 && c <= 939) ||
         (c >= 1040 && c <= 1071))
      return c + 32;
   if ((c >= 393 && c <= 394))
      return c + 205;
   if ((c >= 433 && c <= 434))
      return c + 217;
   if ((c >= 904 && c <= 906))
      return c + 37;
   if ((c >= 910 && c <= 911))
      return c + 63;
   if ((c >= 1025 && c <= 1036) ||
         (c >= 1038 && c <= 1039))
      return c + 80;
   if ((c >= 1329 && c <= 1366) ||
         (c >= 4256 && c <= 4293))
      return c + 48;
   if ((c >= 7944 && c <= 7951) ||
         (c >= 7960 && c <= 7965) ||
         (c >= 7976 && c <= 7983) ||
         (c >= 7992 && c <= 7999) ||
         (c >= 8008 && c <= 8013) ||
         (c >= 8040 && c <= 8047) ||
         (c >= 8072 && c <= 8079) ||
         (c >= 8088 && c <= 8095) ||
         (c >= 8104 && c <= 8111) ||
         (c >= 8120 && c <= 8121) ||
         (c >= 8152 && c <= 8153) ||
         (c >= 8168 && c <= 8169))
      return c + -8;
   if ((c >= 8122 && c <= 8123))
      return c + -74;
   if ((c >= 8136 && c <= 8139))
      return c + -86;
   if ((c >= 8154 && c <= 8155))
      return c + -100;
   if ((c >= 8170 && c <= 8171))
      return c + -112;
   if ((c >= 8184 && c <= 8185))
      return c + -128;
   if ((c >= 8186 && c <= 8187))
      return c + -126;
   if ((c >= 8544 && c <= 8559))
      return c + 16;
   if ((c >= 9398 && c <= 9423))
      return c + 26;

   switch (c) {
      case 256:
      case 258:
      case 260:
      case 262:
      case 264:
      case 266:
      case 268:
      case 270:
      case 272:
      case 274:
      case 276:
      case 278:
      case 280:
      case 282:
      case 284:
      case 286:
      case 288:
      case 290:
      case 292:
      case 294:
      case 296:
      case 298:
      case 300:
      case 302:
      case 306:
      case 308:
      case 310:
      case 313:
      case 315:
      case 317:
      case 319:
      case 321:
      case 323:
      case 325:
      case 327:
      case 330:
      case 332:
      case 334:
      case 336:
      case 338:
      case 340:
      case 342:
      case 344:
      case 346:
      case 348:
      case 350:
      case 352:
      case 354:
      case 356:
      case 358:
      case 360:
      case 362:
      case 364:
      case 366:
      case 368:
      case 370:
      case 372:
      case 374:
      case 377:
      case 379:
      case 381:
      case 386:
      case 388:
      case 391:
      case 395:
      case 401:
      case 408:
      case 416:
      case 418:
      case 420:
      case 423:
      case 428:
      case 431:
      case 435:
      case 437:
      case 440:
      case 444:
      case 453:
      case 456:
      case 459:
      case 461:
      case 463:
      case 465:
      case 467:
      case 469:
      case 471:
      case 473:
      case 475:
      case 478:
      case 480:
      case 482:
      case 484:
      case 486:
      case 488:
      case 490:
      case 492:
      case 494:
      case 498:
      case 500:
      case 506:
      case 508:
      case 510:
      case 512:
      case 514:
      case 516:
      case 518:
      case 520:
      case 522:
      case 524:
      case 526:
      case 528:
      case 530:
      case 532:
      case 534:
      case 994:
      case 996:
      case 998:
      case 1000:
      case 1002:
      case 1004:
      case 1006:
      case 1120:
      case 1122:
      case 1124:
      case 1126:
      case 1128:
      case 1130:
      case 1132:
      case 1134:
      case 1136:
      case 1138:
      case 1140:
      case 1142:
      case 1144:
      case 1146:
      case 1148:
      case 1150:
      case 1152:
      case 1168:
      case 1170:
      case 1172:
      case 1174:
      case 1176:
      case 1178:
      case 1180:
      case 1182:
      case 1184:
      case 1186:
      case 1188:
      case 1190:
      case 1192:
      case 1194:
      case 1196:
      case 1198:
      case 1200:
      case 1202:
      case 1204:
      case 1206:
      case 1208:
      case 1210:
      case 1212:
      case 1214:
      case 1217:
      case 1219:
      case 1223:
      case 1227:
      case 1232:
      case 1234:
      case 1236:
      case 1238:
      case 1240:
      case 1242:
      case 1244:
      case 1246:
      case 1248:
      case 1250:
      case 1252:
      case 1254:
      case 1256:
      case 1258:
      case 1262:
      case 1264:
      case 1266:
      case 1268:
      case 1272:
      case 7680:
      case 7682:
      case 7684:
      case 7686:
      case 7688:
      case 7690:
      case 7692:
      case 7694:
      case 7696:
      case 7698:
      case 7700:
      case 7702:
      case 7704:
      case 7706:
      case 7708:
      case 7710:
      case 7712:
      case 7714:
      case 7716:
      case 7718:
      case 7720:
      case 7722:
      case 7724:
      case 7726:
      case 7728:
      case 7730:
      case 7732:
      case 7734:
      case 7736:
      case 7738:
      case 7740:
      case 7742:
      case 7744:
      case 7746:
      case 7748:
      case 7750:
      case 7752:
      case 7754:
      case 7756:
      case 7758:
      case 7760:
      case 7762:
      case 7764:
      case 7766:
      case 7768:
      case 7770:
      case 7772:
      case 7774:
      case 7776:
      case 7778:
      case 7780:
      case 7782:
      case 7784:
      case 7786:
      case 7788:
      case 7790:
      case 7792:
      case 7794:
      case 7796:
      case 7798:
      case 7800:
      case 7802:
      case 7804:
      case 7806:
      case 7808:
      case 7810:
      case 7812:
      case 7814:
      case 7816:
      case 7818:
      case 7820:
      case 7822:
      case 7824:
      case 7826:
      case 7828:
      case 7840:
      case 7842:
      case 7844:
      case 7846:
      case 7848:
      case 7850:
      case 7852:
      case 7854:
      case 7856:
      case 7858:
      case 7860:
      case 7862:
      case 7864:
      case 7866:
      case 7868:
      case 7870:
      case 7872:
      case 7874:
      case 7876:
      case 7878:
      case 7880:
      case 7882:
      case 7884:
      case 7886:
      case 7888:
      case 7890:
      case 7892:
      case 7894:
      case 7896:
      case 7898:
      case 7900:
      case 7902:
      case 7904:
      case 7906:
      case 7908:
      case 7910:
      case 7912:
      case 7914:
      case 7916:
      case 7918:
      case 7920:
      case 7922:
      case 7924:
      case 7926:
      case 7928:
         return c + 1;
      case 304:
         return c + -199;
      case 376:
         return c + -121;
      case 385:
         return c + 210;
      case 390:
         return c + 206;
      case 398:
         return c + 79;
      case 399:
         return c + 202;
      case 400:
         return c + 203;
      case 403:
         return c + 205;
      case 404:
         return c + 207;
      case 406:
      case 412:
         return c + 211;
      case 407:
         return c + 209;
      case 413:
         return c + 213;
      case 415:
         return c + 214;
      case 422:
      case 425:
      case 430:
         return c + 218;
      case 439:
         return c + 219;
      case 452:
      case 455:
      case 458:
      case 497:
         return c + 2;
      case 902:
         return c + 38;
      case 908:
         return c + 64;
      case 8025:
      case 8027:
      case 8029:
      case 8031:
         return c + -8;
      case 8124:
      case 8140:
      case 8188:
         return c + -9;
      case 8172:
         return c + -7;
   }
   return c;
}

EGG_DIALOG_PLAYER *egg_init_dialog(EGG_DIALOG *dialog, int focus_obj)
{
   EGG_DIALOG_PLAYER *player = calloc(1, sizeof *player);

   if (!init) egg_initialise();

   player->dialog = dialog;

   egg_set_default_font(player, gui_default_font);

   /* Initialise event channels */
   player->input = al_create_event_queue();
   al_init_user_event_source(&player->event_src);

   /* Initialise focus and other bookkeeping */
   player->pass_events = true;
   player->focus_follows_mouse = true;
   player->obj = -1;
   player->mouse_obj = -1;
   player->click_obj = -1;

   player->stick = 0;
   player->x_axis = 0;
   player->y_axis = 1;
   player->joy_x = 0;
   player->joy_y = 0;
   player->joy_b = 0;

   /* Set root widget for all dialog items */
   for (int c=0; dialog[c].proc; c++)
      dialog[c].root = dialog;

   /* Notify all objects that the dialog is starting */
   player->res |= egg_dialog_message(dialog, MSG_START, 0, &player->obj);
   player->redraw = true;

   /* Clear focus flag from all objects */
   for (int c=0; dialog[c].proc; c++)
      dialog[c].flags &= ~(D_GOTFOCUS | D_GOTMOUSE | D_TRACKMOUSE | D_INTERNAL);

   /* Find object that has the mouse */
   ALLEGRO_MOUSE_STATE mouse_state;
   al_get_mouse_state(&mouse_state);
   player->mouse_obj = find_mouse_object(dialog, mouse_state.x, mouse_state.y);
   if (player->mouse_obj >= 0)
      dialog[player->mouse_obj].flags |= D_GOTMOUSE;

   /* If no focus object has been specified it defaults to the mouse object */
   if (focus_obj < 0) focus_obj = player->mouse_obj;
   player->focus_obj = focus_obj;

   /* Offer focus to the focus object */
   if ((focus_obj >= 0) && ((egg_object_message(dialog+focus_obj, MSG_WANTFOCUS, 0)) & D_WANTFOCUS))
      dialog[focus_obj].flags |= D_GOTFOCUS;

   /* Set default "grey-out" colour */
   for (int c=0; dialog[c].proc; c++) {
      ALLEGRO_COLOR grey = egg_silver;
      dialog[c].mg = grey;
   }

   return player;
}

static void pass_through_event(EGG_DIALOG_PLAYER *player, ALLEGRO_EVENT *event)
{
   if (player->pass_events) {
      _al_event_source_lock(&player->event_src);
      _al_event_source_emit_event(&player->event_src, event);
      _al_event_source_unlock(&player->event_src);
   }
}

/* TODO: secondary click, mouse wheel scrolling.
 * Keep track of key shifts.
 */
static void *dialog_thread_func(ALLEGRO_THREAD *thread, void *arg)
{
   EGG_DIALOG_PLAYER *player = arg;

   /* Special timer to keep track of double clicks:
    * When a click is detected the timer is started and the clicked object
    * is recorded. Once the timer triggers it will check how many mouse
    * clicks there have been since it was started and then send a CLICK or
    * DCLICK message.
    * The timer disables itself immediately after it triggered.
    */
   ALLEGRO_TIMER *dclick_timer = al_create_timer(0.3);
   al_register_event_source(player->input, al_get_timer_event_source(dclick_timer));

   /* Special timer for joystick events, which need to be on a delay. */
   ALLEGRO_TIMER *joy_timer = al_create_timer(0.3);
   al_register_event_source(player->input, al_get_timer_event_source(joy_timer));

   bool running = true;
   bool shift = false;           /* Keep track of shift state, for passing around widget focus */
   int click_count = 0;
   player->redraw = true;
   while (running) {
      ALLEGRO_EVENT my_event;
      my_event.user.data2 = (unsigned long)player;
      if (player->redraw) {
         my_event.user.type = EGG_EVENT_REDRAW;
         al_emit_user_event(&player->event_src, &my_event, egg_event_destructor);
      }

      ALLEGRO_EVENT event;
      bool received_event = al_wait_for_event_timed(player->input, &event, 0.02);
      if (player->paused) {
         if (received_event) {
            switch (event.type) {
               case EGGC_STOP:
                  running = false;
                  break;

               case EGGC_PAUSE:
                  player->paused = true;
                  break;

               case EGGC_RESUME:
                  player->paused = false;
                  break;
            }
         }
      } else {
         if (received_event) {
            switch (event.type) {
               case ALLEGRO_EVENT_DISPLAY_CLOSE:
                  player->res |= D_CLOSE;
                  break;

               case ALLEGRO_EVENT_DISPLAY_RESIZE:
               case ALLEGRO_EVENT_DISPLAY_EXPOSE:
                  player->redraw = true;
                  my_event.user.type = EGG_EVENT_REDRAW;
                  al_emit_user_event(&player->event_src, &my_event, egg_event_destructor);
                  break;

               case ALLEGRO_EVENT_DISPLAY_HALT_DRAWING:
                  al_acknowledge_drawing_halt(event.display.source);
                  player->draw_veto = true;
                  break;

               case ALLEGRO_EVENT_DISPLAY_RESUME_DRAWING:
                  al_acknowledge_drawing_resume(event.display.source);
                  player->draw_veto = false;
                  player->redraw = true;
                  my_event.user.type = EGG_EVENT_REDRAW;
                  al_emit_user_event(&player->event_src, &my_event, egg_event_destructor);
                  break;

               case ALLEGRO_EVENT_TIMER:
                  if (event.timer.source == dclick_timer) { /* Double click? */
                     al_stop_timer(dclick_timer);

                     if (player->click_obj >= 0) {
                        int click = (click_count > 1) ? MSG_DCLICK : MSG_CLICK;
                        MESSAGE(player, player->click_obj, click, 1);
                     }

                     click_count = 0;
                  } else if (event.timer.source == joy_timer) {
                     al_stop_timer(joy_timer);

                     if (player->joy_x) {
                        if (player->joy_x > 0)
                           player->res |= move_focus(player->dialog, ALLEGRO_KEY_RIGHT, 0, &player->focus_obj);
                        else
                           player->res |= move_focus(player->dialog, ALLEGRO_KEY_LEFT, 0, &player->focus_obj);
                     }

                     if (player->joy_y) {
                        if (player->joy_y > 0)
                           player->res |= move_focus(player->dialog, ALLEGRO_KEY_UP, 0, &player->focus_obj);
                        else
                           player->res |= move_focus(player->dialog, ALLEGRO_KEY_DOWN, 0, &player->focus_obj);
                     }

                     if (player->joy_b) {
                        if (player->focus_obj >= 0)
                           MESSAGE(player, player->focus_obj, MSG_KEY, 0);
                     }

                     player->joy_x = player->joy_y = player->joy_b = 0;
                  } else {
                     player->res |= egg_dialog_message(player->dialog, MSG_TIMER, event.timer.count, &player->obj);
                  }
                  break;

               case EGGC_STOP:
                  running = false;
                  break;

               case EGGC_PAUSE:
                  player->paused = true;
                  break;

               case EGGC_RESUME:
                  player->paused = false;
                  break;

               case ALLEGRO_EVENT_KEY_DOWN:
                  switch (event.keyboard.keycode) {
                     case ALLEGRO_KEY_LSHIFT:
                     case ALLEGRO_KEY_RSHIFT:
                        shift = true;
                        break;
                  }
                  if (player->focus_obj >= 0)
                     MESSAGE(player, player->focus_obj, MSG_KEYDOWN, event.keyboard.keycode);
                  if (!(player->res & D_USED_KEY)) {
                     /* Pass-through? */
                     switch (event.keyboard.keycode) {
                        case ALLEGRO_KEY_ESCAPE:   /* Escape closes the dialog */
                        case ALLEGRO_KEY_ENTER:    /* pass <CR> or <SPACE> to selected object */
                        case ALLEGRO_KEY_SPACE:
                        case ALLEGRO_KEY_PAD_ENTER:
                        case ALLEGRO_KEY_TAB:      /* Move focus */
                        case ALLEGRO_KEY_RIGHT:
                        case ALLEGRO_KEY_LEFT:
                        case ALLEGRO_KEY_DOWN:
                        case ALLEGRO_KEY_UP:
                           break;

                        default:
                           pass_through_event(player, &event);
                     }
                  }
                  player->res &= ~D_USED_KEY;
                  break;

               case ALLEGRO_EVENT_KEY_UP:
                  switch (event.keyboard.keycode) {
                     case ALLEGRO_KEY_LSHIFT:   /* Shift released */
                     case ALLEGRO_KEY_RSHIFT:
                        shift = false;
                        break;
                  }

                  /* Check if focus object wants these keys */
                  if (player->focus_obj >= 0)
                     MESSAGE(player, player->focus_obj, MSG_KEYUP, event.keyboard.keycode);

                  /* If not, check hot keys */

                  /* If not, check whether any other object wants it */

                  /* No one else wants it, so it's ours */
                  if (~player->res & D_USED_KEY) {
                     switch (event.keyboard.keycode) {
                        case ALLEGRO_KEY_ESCAPE:   /* Escape closes the dialog */
                           player->obj = -1;
                           player->res |= D_CLOSE;
                           break;

                        case ALLEGRO_KEY_ENTER:    /* pass <CR> or <SPACE> to selected object */
                        case ALLEGRO_KEY_SPACE:
                        case ALLEGRO_KEY_PAD_ENTER:
                           if (player->focus_obj >= 0)
                              MESSAGE(player, player->focus_obj, MSG_KEY, 0);
                           break;

                        case ALLEGRO_KEY_TAB:      /* Move focus */
                        case ALLEGRO_KEY_RIGHT:
                        case ALLEGRO_KEY_LEFT:
                        case ALLEGRO_KEY_DOWN:
                        case ALLEGRO_KEY_UP:
                           player->res |= move_focus(player->dialog, event.keyboard.keycode, shift, &player->focus_obj);
                           break;

                        default:
                           /* Pass-through */
                           pass_through_event(player, &event);
                     }
                  }
                  player->res &= ~D_USED_KEY;
                  break;

               case ALLEGRO_EVENT_KEY_CHAR:
                  if (player->focus_obj >= 0) {
                     MESSAGE(player, player->focus_obj, MSG_CHAR, event.keyboard.unichar);
                     if (event.keyboard.repeat)
                        MESSAGE(player, player->focus_obj, MSG_KEYREPEAT, event.keyboard.keycode);
                  }
                  if (!(player->res & D_USED_CHAR)) {
                     /* Keyboard short-cut? */
                     int keychar = event.keyboard.unichar;
                     if (event.keyboard.keycode == ALLEGRO_KEY_BACKSPACE) keychar = 8;
                     if (event.keyboard.keycode == ALLEGRO_KEY_DELETE) keychar = 127;
                     //printf("%d %d %d %d\n", event.keyboard.unichar, event.keyboard.keycode, ALLEGRO_KEY_DELETE, ALLEGRO_KEY_BACKSPACE);
                     if (keychar) for (int c=0; player->dialog[c].proc; c++) {
                        if ( (utolower(player->dialog[c].key) ==
                                 utolower(keychar)) && (player->dialog[c].key == keychar) &&
                              (!(player->dialog[c].flags & (D_HIDDEN | D_DISABLED)))) {
                           MESSAGE(player, c, MSG_KEY, 0);
                           break;
                        }
                     }
                  }
                  if (!(player->res & D_USED_CHAR)) {
                     /* Pass-through? */
                     switch (event.keyboard.keycode) {
                        case ALLEGRO_KEY_ESCAPE:   /* Escape closes the dialog */
                        case ALLEGRO_KEY_ENTER:    /* pass <CR> or <SPACE> to selected object */
                        case ALLEGRO_KEY_SPACE:
                        case ALLEGRO_KEY_PAD_ENTER:
                        case ALLEGRO_KEY_TAB:      /* Move focus */
                        case ALLEGRO_KEY_RIGHT:
                        case ALLEGRO_KEY_LEFT:
                        case ALLEGRO_KEY_DOWN:
                        case ALLEGRO_KEY_UP:
                           break;

                        default:
                           pass_through_event(player, &event);
                     }
                  }
                  player->res &= ~D_USED_KEY;
                  break;

               case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
                  /* Reset number of mouse clicks if mouse focus has changed */
                  if (player->click_obj != player->mouse_obj) click_count = 0;

                  player->click_obj = player->mouse_obj;
                  if (player->click_obj >= 0) {
                     MESSAGE(player, player->mouse_obj, MSG_MOUSEDOWN, event.mouse.button);
                  }
                  break;

               case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
                  if (player->click_obj >= 0)
                     MESSAGE(player, player->click_obj, MSG_MOUSEUP, event.mouse.button);

                  if (player->click_obj == player->mouse_obj) {
                     al_start_timer(dclick_timer);
                     click_count++;
                  } else {
                     click_count = 0;
                     player->click_obj = -1;
                  }
                  break;

               case ALLEGRO_EVENT_MOUSE_AXES:
                  /* has mouse object changed? */
                  (void)0;
                  int mouse_obj = find_mouse_object(player->dialog, event.mouse.x, event.mouse.y);
                  if (mouse_obj != player->mouse_obj) {
                     if (player->mouse_obj >= 0) {
                        player->dialog[player->mouse_obj].flags &= ~D_GOTMOUSE;
                        MESSAGE(player, player->mouse_obj, MSG_LOSTMOUSE, 0);
                     }
                     if (mouse_obj >= 0) {
                        player->dialog[mouse_obj].flags |= D_GOTMOUSE;
                        MESSAGE(player, mouse_obj, MSG_GOTMOUSE, 0);
                     }
                     player->mouse_obj = mouse_obj;

                     /* move the input focus as well? */
                     if ((player->focus_follows_mouse) && (player->mouse_obj != player->focus_obj))
                        player->res |= offer_focus(player->dialog, player->mouse_obj, &player->focus_obj, true);
                  }

                  /* Are objects tracking the mouse? */
                  if (event.mouse.dx || event.mouse.dy) {
                     for (int n=0; player->dialog[n].proc; n++) {
                        player->dialog[n].mousex = event.mouse.x;
                        player->dialog[n].mousey = event.mouse.y;
                        if (player->dialog[n].flags & D_TRACKMOUSE) {
                           player->res |= egg_object_message(player->dialog+n, MSG_MOUSEMOVE, 0);
                        }
                     }
                  }
                  if (event.mouse.dz)
                     MESSAGE(player, player->mouse_obj, MSG_WHEEL, event.mouse.dz);
                  if (event.mouse.dw)
                     MESSAGE(player, player->mouse_obj, MSG_HBALL, event.mouse.dw);
                  break;

               case ALLEGRO_EVENT_JOYSTICK_CONFIGURATION:
                  al_reconfigure_joysticks();
                  break;

               case ALLEGRO_EVENT_JOYSTICK_AXIS:
                  if (event.joystick.stick == player->stick) {
                     if (event.joystick.axis == player->x_axis) {
                        if (event.joystick.pos >  0.95) player->joy_x =  1;
                        if (event.joystick.pos < -0.95) player->joy_x = -1;
                     }
                     if (event.joystick.axis == player->y_axis) {
                        if (event.joystick.pos >  0.95) player->joy_y = -1;
                        if (event.joystick.pos < -0.95) player->joy_y =  1;
                     }
                     al_start_timer(joy_timer);
                  }
                  break;

               case ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN:
                  break;

               case ALLEGRO_EVENT_JOYSTICK_BUTTON_UP:
                  player->joy_b = 1;
                  al_start_timer(joy_timer);
                  break;

            }
         } else { /* Send idle messages */
            player->res |= egg_dialog_message(player->dialog, MSG_IDLE, 0, &player->obj);
         }
      }

      /* Check for objects that need to be redrawn */
      for (int n=0; player->dialog[n].proc; n++)
         if (player->dialog[n].flags & D_DIRTY) {
            player->dialog[n].flags &= ~D_DIRTY;
            player->res |= D_REDRAW_ALL;
         }

      if (player->res & D_REDRAW_ANY)
         player->redraw = true;

      if (player->res & D_CLOSE) {
         my_event.user.type = EGG_EVENT_CLOSE;
         if (player->obj == -1)
            my_event.user.type = EGG_EVENT_CANCEL;
         my_event.user.data1 = player->obj;
         al_emit_user_event(&player->event_src, &my_event, egg_event_destructor);
      }

      /* Clear some status flags that should not be retained */
      player->res &= ~D_USED_CHAR;
   }

   al_unregister_event_source(player->input, al_get_timer_event_source(dclick_timer));
   al_unregister_event_source(player->input, al_get_timer_event_source(joy_timer));
   al_destroy_timer(joy_timer);
   al_destroy_timer(dclick_timer);

   return NULL;
}

/* start_dialog:
 * begin processing input and generating events from the selected dialog.
 */
bool egg_start_dialog(EGG_DIALOG_PLAYER *player)
{
   if (player->thread) return true; /* Already started */

   player->thread = al_create_thread(dialog_thread_func, (void *)player);
   if (!player->thread) return false;

   /* Listen to the controller */
   al_register_event_source(player->input, &dialog_controller);

   al_start_thread(player->thread);
   return true;
}

/* stop_dialog:
 * stop processing input and generating events from the selected dialog.
 */
bool egg_stop_dialog(EGG_DIALOG_PLAYER *player)
{
   assert(player);
   assert(player->thread);

   al_set_thread_should_stop(player->thread);
   ALLEGRO_EVENT event;
   event.user.data2 = (unsigned long)player;
   event.user.type = EGGC_STOP;
   al_emit_user_event(&dialog_controller, &event, NULL);
   al_join_thread(player->thread, NULL);
   al_destroy_thread(player->thread);
   al_unregister_event_source(player->input, &dialog_controller);
   player->thread = NULL;

   return true;
}

/* pause_dialog:
 * stop processing input and generating events from the selected dialog.
 */
bool egg_pause_dialog(EGG_DIALOG_PLAYER *player)
{
   assert(player);
   assert(player->thread);

   ALLEGRO_EVENT event;
   event.user.data2 = (unsigned long)player;
   event.user.type = EGGC_PAUSE;
   al_emit_user_event(&dialog_controller, &event, NULL);

   return true;
}

/* resume_dialog:
 * stop processing input and generating events from the selected dialog.
 */
bool egg_resume_dialog(EGG_DIALOG_PLAYER *player)
{
   assert(player);
   assert(player->thread);

   ALLEGRO_EVENT event;
   event.user.data2 = (unsigned long)player;
   event.user.type = EGGC_RESUME;
   al_emit_user_event(&dialog_controller, &event, NULL);

   return true;
}

void egg_shutdown_dialog(EGG_DIALOG_PLAYER *player)
{
   if (!player) return;

   if (player->thread) egg_stop_dialog(player);

   player->res |= egg_dialog_message(player->dialog, MSG_END, 0, &player->obj);

   al_destroy_user_event_source(&player->event_src);
   al_destroy_event_queue(player->input);
   free(player);
}

bool egg_draw_dialog(EGG_DIALOG_PLAYER *player)
{
   if (!player->thread) return false;
   if (player->draw_veto) return false;

   for (int n=0; player->dialog[n].proc; n++)
      player->dialog[n].flags &= ~D_DIRTY;


   int clip_x, clip_y, clip_w, clip_h;
   al_get_clipping_rectangle(&clip_x, &clip_y, &clip_w, &clip_h);

   //printf("Post redraw message\n");
   player->redraw = false;
   player->res |= egg_dialog_message(player->dialog, MSG_DRAW, 0, &player->obj);
   player->res &= ~D_REDRAW_ANY;

   al_set_clipping_rectangle(clip_x, clip_y, clip_w, clip_h);

   return true;
}

int egg_do_dialog(EGG_DIALOG *dialog, int focus)
{
   return egg_do_dialog_interval(dialog, 0.0, focus);
}

int egg_do_dialog_interval(EGG_DIALOG *dialog, double speed_sec, int focus)
{
   EGG_DIALOG_PLAYER *player = egg_init_dialog(dialog, focus);
   ALLEGRO_TIMER *timer = NULL;

   egg_listen_for_events(player, al_get_keyboard_event_source());
   egg_listen_for_events(player, al_get_mouse_event_source());
   egg_listen_for_events(player, al_get_joystick_event_source());
   egg_listen_for_events(player, al_get_display_event_source(al_get_current_display()));

   if (speed_sec > 0.0) {
      timer = al_create_timer(speed_sec);
      egg_listen_for_events(player, al_get_timer_event_source(timer));
      al_start_timer(timer);
   }

   ALLEGRO_EVENT_QUEUE *menu_queue = al_create_event_queue();
   al_register_event_source(menu_queue, egg_get_player_event_source(player));

   egg_start_dialog(player);

   bool in_dialog = true;
   bool must_draw = false;
   while (in_dialog) {
      if (must_draw) {
         must_draw = false;
         egg_draw_dialog(player);
         al_flip_display();
      }
      al_wait_for_event(menu_queue, NULL);
      while (!al_is_event_queue_empty(menu_queue)) {
         ALLEGRO_EVENT event;
         al_get_next_event(menu_queue, &event);
         switch (event.type) {
            case EGG_EVENT_REDRAW:
               //printf("Redraw request\n");
               must_draw = true;
               break;

            case EGG_EVENT_CLOSE:
               focus = event.user.data1;
               in_dialog = false;
               break;

            case EGG_EVENT_CANCEL:
               focus = -1;
               in_dialog = false;
               break;

         }
      }
   }

   egg_stop_dialog(player);
   al_destroy_timer(timer);

   al_destroy_event_queue(menu_queue);
   egg_shutdown_dialog(player);
   return focus;
}



bool egg_listen_for_events(EGG_DIALOG_PLAYER *player, ALLEGRO_EVENT_SOURCE *src)
{
   if (!player) return false;

   al_register_event_source(player->input, src);
   return true;
}

ALLEGRO_EVENT_SOURCE *egg_get_player_event_source(EGG_DIALOG_PLAYER *player)
{
   if (!player) return NULL;

   return &player->event_src;
}

void egg_set_default_font(EGG_DIALOG_PLAYER *player, const ALLEGRO_FONT *font)
{
   assert(player);

   player->font = font;
   for (int n = 0; player->dialog[n].proc; n++)
      player->dialog[n].default_font = font;
}

EGG_DIALOG *egg_find_widget_id(EGG_DIALOG *dialog, uint32_t id)
{
   assert(dialog);
   int n;

   for (n = 0; dialog[n].proc; n++)
      if (dialog[n].id == id) return dialog+n;

   return NULL;
}


void egg_get_dialog_bounding_box(EGG_DIALOG *dialog, int *min_x, int *min_y, int *max_x, int *max_y)
{
   int c;
   assert(dialog);
   assert(min_x);
   assert(min_y);
   assert(max_x);
   assert(max_x);

   *min_x = INT_MAX;
   *min_y = INT_MAX;
   *max_x = INT_MIN;
   *max_y = INT_MIN;

   for (c=0; dialog[c].proc; c++) {
      if (dialog[c].x < *min_x)
         *min_x = dialog[c].x;

      if (dialog[c].y < *min_y)
         *min_y = dialog[c].y;

      if (dialog[c].x + dialog[c].w > *max_x)
         *max_x = dialog[c].x + dialog[c].w;

      if (dialog[c].y + dialog[c].h > *max_y)
         *max_y = dialog[c].y + dialog[c].h;
   }

}



/* centre_dialog:
 *  Moves all the objects in a dialog so that the dialog is centered in
 *  the screen.
 */
void egg_centre_dialog(EGG_DIALOG *dialog)
{
   int min_x = INT_MAX;
   int min_y = INT_MAX;
   int max_x = INT_MIN;
   int max_y = INT_MIN;
   int xc, yc;
   int c;
   assert(dialog);

   egg_get_dialog_bounding_box(dialog, &min_x, &min_y, &max_x, &max_y);

   int screen_w = al_get_display_width(al_get_current_display());
   int screen_h = al_get_display_height(al_get_current_display());

   /* how much to move by? */
   xc = (screen_w - (max_x - min_x)) / 2 - min_x;
   yc = (screen_h - (max_y - min_y)) / 2 - min_y;

   /* move it */
   for (c=0; dialog[c].proc; c++) {
      dialog[c].x += xc;
      dialog[c].y += yc;
   }
}


/* position_dialog:
 *  Moves all the objects in a dialog to the specified position.
 */
void egg_position_dialog(EGG_DIALOG *dialog, int x, int y)
{
   int min_x = INT_MAX;
   int min_y = INT_MAX;
   int xc, yc;
   int c;
   assert(dialog);

   /* locate the upper-left corner */
   for (c=0; dialog[c].proc; c++) {
      if (dialog[c].x < min_x)
         min_x = dialog[c].x;

      if (dialog[c].y < min_y)
         min_y = dialog[c].y;
   }

   /* move the dialog */
   xc = min_x - x;
   yc = min_y - y;

   for (c=0; dialog[c].proc; c++) {
      dialog[c].x -= xc;
      dialog[c].y -= yc;
   }
}


EGG_DIALOG_THEME *egg_create_new_theme(void)
{
   EGG_DIALOG_THEME *theme = malloc(sizeof *theme);

   theme->dt_items = 0;
   theme->dt_max_items = 0;
   theme->items = NULL;

   return theme;
}

void egg_destroy_new_theme(EGG_DIALOG_THEME *theme)
{
   if (theme) {
      free(theme->items);
      free(theme);
   }
}

void egg_add_dialog_theme(EGG_DIALOG_THEME *theme, EGG_DIALOG_PROC proc, ALLEGRO_COLOR fg, ALLEGRO_COLOR bg, ALLEGRO_COLOR mg, ALLEGRO_FONT *font, NINE_PATCH_BITMAP *frame, NINE_PATCH_BITMAP *selected_frame, NINE_PATCH_BITMAP *highlight_frame, NINE_PATCH_BITMAP *handle)
{
   assert(theme);

   if (theme->dt_items >= theme->dt_max_items) {
      theme->dt_max_items = (theme->dt_max_items+1)*2;
      theme->items = realloc(theme->items, theme->dt_max_items * sizeof *theme->items);
   }

   theme->items[theme->dt_items].proc = proc;
   theme->items[theme->dt_items].fg = fg;
   theme->items[theme->dt_items].bg = bg;
   theme->items[theme->dt_items].mg = mg;
   theme->items[theme->dt_items].font = font;
   theme->items[theme->dt_items].bmp_frame = frame;
   theme->items[theme->dt_items].bmp_select = selected_frame;
   theme->items[theme->dt_items].bmp_focus = highlight_frame;
   theme->items[theme->dt_items].bmp_handle = handle;
   theme->dt_items++;
}

void egg_apply_theme(EGG_DIALOG *dialog, EGG_DIALOG_THEME *theme)
{
   for (int c = 0; dialog[c].proc; c++) {
      for (int n = 0; n<theme->dt_items; n++) {
         if (dialog[c].proc == theme->items[n].proc ||
             (theme->items[n].proc == NULL && n == 0)) {
            dialog[c].fg = theme->items[n].fg;
            dialog[c].bg = theme->items[n].bg;
            dialog[c].mg = theme->items[n].mg;
            dialog[c].font = theme->items[n].font;
            dialog[c].bmp_frame = theme->items[n].bmp_frame;
            dialog[c].bmp_select = theme->items[n].bmp_select;
            dialog[c].bmp_focus = theme->items[n].bmp_focus;
            dialog[c].bmp_handle = theme->items[n].bmp_handle;
            if (n) break;
         }
      }
   }
}

uint32_t egg_index_to_id(EGG_DIALOG *dialog, int index)
{
   assert(dialog);
   if (index < 0) return EGG_NO_ID;

   /* Make sure we are in range */
   int n = 0;
   while (dialog[n].proc) n++;

   if (index > n) return EGG_NO_ID;

   return dialog[index].id;
}

int egg_id_to_index(EGG_DIALOG *dialog, uint32_t id)
{
   assert(dialog);
   if (id == EGG_NO_ID) return -1;

   int index = 0;
   int count = 0;
   int n = 0;
   while (dialog[n].proc) {
      if (dialog[n].id == id) {
         count++;
         index = n;
      }
      n++;
   }

   if (count == 1) return index;

   return EGG_NO_ID;
}

bool egg_id_is_unique(EGG_DIALOG *dialog, uint32_t id)
{
   if (id == EGG_NO_ID) return false;

   int count = 0;
   int n = 0;
   while (dialog[n].proc) {
      if (dialog[n].id == id)
         count++;
      n++;
   }

   if (count == 1) return true;

   return false;
}

uint32_t get_unique_id(EGG_DIALOG *dialog)
{
   uint32_t id = EGG_NO_ID+1;

   while (!egg_id_is_unique(dialog, id)) id++;

   return id;
}
