#include <stdio.h>
#include "egg_dialog/egg_dialog.h"
#include "t3gui.h"

static EGG_DIALOG_PLAYER ** t3gui_dialog_player;
static int t3gui_dialog_players = 0;
static int t3gui_allocated_dialog_players = 0;
static ALLEGRO_EVENT_QUEUE * t3gui_queue = NULL;

static EGG_DIALOG * t3gui_allocate_element(int count)
{
  EGG_DIALOG * dp;

  dp = malloc(sizeof(EGG_DIALOG) * count);
  if(dp)
  {
    memset(dp, 0, sizeof(EGG_DIALOG) * count);
  }
  return dp;
}

static void t3gui_destroy_element(EGG_DIALOG * dp)
{
  free(dp);
}

static void t3gui_expand_dialog_element(T3GUI_DIALOG * dp)
{
  EGG_DIALOG * old_element;
  int e, old_e;

  old_e = dp->allocated_elements;
  e = (dp->elements / T3GUI_DIALOG_ELEMENT_CHUNK_SIZE + 1) * T3GUI_DIALOG_ELEMENT_CHUNK_SIZE;
  if(e != old_e)
  {
    old_element = dp->element;
    dp->element = t3gui_allocate_element(e);
    if(dp->element)
    {
      memcpy(dp->element, old_element, sizeof(EGG_DIALOG) * old_e);
      dp->allocated_elements = e;
      t3gui_destroy_element(old_element);
    }
    else
    {
      dp->element = old_element;
    }
  }
}

T3GUI_DIALOG * t3gui_create_dialog(void)
{
  T3GUI_DIALOG * dp;

  dp = malloc(sizeof(T3GUI_DIALOG));
  if(dp)
  {
    dp->allocated_elements = 0;
    dp->element = t3gui_allocate_element(T3GUI_DIALOG_ELEMENT_CHUNK_SIZE);
    if(dp->element)
    {
      dp->allocated_elements = T3GUI_DIALOG_ELEMENT_CHUNK_SIZE;
    }
    dp->elements = 0;
  }
  return dp;
}

void t3gui_destroy_dialog(T3GUI_DIALOG * dp)
{
  t3gui_destroy_element(dp->element);
  free(dp);
}

void t3gui_dialog_add_widget(T3GUI_DIALOG * dialog, int (*proc)(int msg, EGG_DIALOG * d, int c), int x, int y, int w, int h, ALLEGRO_COLOR fg, ALLEGRO_COLOR bg, int key, uint64_t flags, int d1, int d2, void * dp, void * dp2, void * dp3)
{
  t3gui_expand_dialog_element(dialog);
  if(dialog->elements < dialog->allocated_elements)
  {
    dialog->element[dialog->elements].proc = proc;
  	dialog->element[dialog->elements].x = x;
  	dialog->element[dialog->elements].y = y;
  	dialog->element[dialog->elements].w = w;
  	dialog->element[dialog->elements].h = h;
  	dialog->element[dialog->elements].fg = fg;
  	dialog->element[dialog->elements].bg = bg;
  	dialog->element[dialog->elements].key = key;
  	dialog->element[dialog->elements].flags = flags;
  	dialog->element[dialog->elements].d1 = d1;
  	dialog->element[dialog->elements].d2 = d2;
  	dialog->element[dialog->elements].dp = dp;
  	dialog->element[dialog->elements].dp2 = dp2;
  	dialog->element[dialog->elements].dp3 = dp3;
    dialog->elements++;
  }
}

static EGG_DIALOG_PLAYER ** t3gui_allocate_player(int count)
{
  EGG_DIALOG_PLAYER ** dp;

  dp = malloc(sizeof(EGG_DIALOG_PLAYER *) * T3GUI_DIALOG_PLAYER_CHUNK_SIZE);
  if(dp)
  {
    memset(dp, 0, sizeof(EGG_DIALOG_PLAYER *) * T3GUI_DIALOG_PLAYER_CHUNK_SIZE);
  }
  return dp;
}

static void t3gui_destroy_player(EGG_DIALOG_PLAYER ** dp, int e)
{
  free(dp);
}

static void t3gui_expand_player(void)
{
  EGG_DIALOG_PLAYER ** old_player;
  int e, old_e;
  int i;

  old_e = t3gui_allocated_dialog_players;
  e = (t3gui_dialog_players / T3GUI_DIALOG_PLAYER_CHUNK_SIZE + 1) * T3GUI_DIALOG_PLAYER_CHUNK_SIZE;
  if(e != old_e)
  {
    old_player = t3gui_dialog_player;
    t3gui_dialog_player = t3gui_allocate_player(e);
    if(t3gui_dialog_player)
    {
      t3gui_allocated_dialog_players = e;
      for(i = 0; i < old_e; i++)
      {
        memcpy(t3gui_dialog_player[i], old_player[i], sizeof(EGG_DIALOG_PLAYER));
      }
      t3gui_destroy_player(old_player, old_e);
    }
    else
    {
      t3gui_dialog_player = old_player;
    }
  }
}

bool t3gui_show_dialog(T3GUI_DIALOG * dp, ALLEGRO_EVENT_QUEUE * qp)
{
  /* make sure we have enough space allocated for the new player */
  if(t3gui_allocated_dialog_players == 0)
  {
    t3gui_dialog_player = t3gui_allocate_player(T3GUI_DIALOG_PLAYER_CHUNK_SIZE);
    if(t3gui_dialog_player)
    {
      t3gui_allocated_dialog_players = T3GUI_DIALOG_PLAYER_CHUNK_SIZE;
    }
    else
    {
      return false;
    }
  }
  t3gui_expand_player();

  /* initialize the player */
  t3gui_dialog_player[t3gui_dialog_players] = egg_init_dialog(dp->element, 0);
  if(t3gui_dialog_player[t3gui_dialog_players])
  {
    /* pause previous player */
    if(t3gui_dialog_players > 0)
    {
      egg_pause_dialog(t3gui_dialog_player[t3gui_dialog_players - 1]);
    }
    egg_listen_for_events(t3gui_dialog_player[t3gui_dialog_players], al_get_keyboard_event_source());
    egg_listen_for_events(t3gui_dialog_player[t3gui_dialog_players], al_get_mouse_event_source());
    if(qp)
    {
      t3gui_queue = qp;
      al_register_event_source(qp, egg_get_player_event_source(t3gui_dialog_player[t3gui_dialog_players]));
    }
    egg_start_dialog(t3gui_dialog_player[t3gui_dialog_players]);
    t3gui_dialog_players++;
    return true;
  }
  return false;
}

bool t3gui_close_dialog(T3GUI_DIALOG * dp)
{
  int i, j;

  for(i = 0; i < t3gui_dialog_players; i++)
  {
    if(dp->element == t3gui_dialog_player[i]->dialog)
    {
      if(t3gui_queue && t3gui_dialog_players == 1)
      {
        al_unregister_event_source(t3gui_queue, egg_get_player_event_source(t3gui_dialog_player[i]));
      }
      egg_stop_dialog(t3gui_dialog_player[i]);
      egg_shutdown_dialog(t3gui_dialog_player[i]);
      for(j = i; j < t3gui_dialog_players - 1; j++)
      {
        t3gui_dialog_player[j] = t3gui_dialog_player[j + 1];
      }
      t3gui_dialog_players--;
      if(t3gui_dialog_players <= 0)
      {
        t3gui_destroy_player(t3gui_dialog_player, t3gui_allocated_dialog_players);
      }
      return true;
    }
  }
  return false;
}

void t3gui_render(void)
{
  int i;

  for(i = 0; i < t3gui_dialog_players; i++)
  {
    egg_draw_dialog(t3gui_dialog_player[i]);
  }
}

typedef int (getbuttonfuncptr)(EGG_DIALOG * d, void *dp3);

int t3gui_push_button_proc(int msg, EGG_DIALOG *d, int c)
{
   int ret = D_O_K;
   assert(d);

   const char *text = d->dp;
   const ALLEGRO_FONT *font = d->dp2;
   if (text == NULL) text = "";
   if (!font) font = d->font;
   if (!font) font = d->default_font;

   switch (msg) {
       case MSG_MOUSEUP:
       {
           if (d->flags & D_TRACKMOUSE)
              ret |= D_REDRAWME;
           d->flags &= ~D_TRACKMOUSE;

           ALLEGRO_MOUSE_STATE mouse_state;
           al_get_mouse_state(&mouse_state);

           if(d->dp2)
           {
               getbuttonfuncptr *func = d->dp2;
               func(d, d->dp3);
           }
           if (d->flags & D_EXIT) {
              d->flags ^= D_SELECTED;
              return D_CLOSE;
           }
           break;
       }
       default:
       {
           return egg_button_proc(msg, d, c);
       }
   }
   return ret;
}
