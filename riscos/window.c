/*
 * This file is part of NetSurf, http://netsurf.sourceforge.net/
 * Licensed under the GNU General Public License,
 *                http://www.opensource.org/licenses/gpl-license
 * Copyright 2003 Phil Mellor <monkeyson@users.sourceforge.net>
 * Copyright 2003 James Bursa <bursa@users.sourceforge.net>
 * Copyright 2003 John M Bell <jmb202@ecs.soton.ac.uk>
 */

/** \file
 * Browser window handling (implementation).
 */

#include <assert.h>
#include <string.h>
#include "oslib/wimp.h"
#include "oslib/wimpspriteop.h"
#include "netsurf/riscos/about.h"
#include "netsurf/riscos/gui.h"
#include "netsurf/riscos/theme.h"
#include "netsurf/utils/log.h"
#include "netsurf/utils/utils.h"

gui_window *window_list = 0;


/**
 * Create and open a new browser window.
 */

gui_window *gui_create_browser_window(struct browser_window *bw)
{
  int screen_width, screen_height, win_width, win_height;
  int toolbar_height = 0;
  wimp_window window;
  wimp_window_state state;
  wimp_outline outline;

  gui_window* g = (gui_window*) xcalloc(1, sizeof(gui_window));
  g->type = GUI_BROWSER_WINDOW;
  g->data.browser.bw = bw;

  ro_gui_screen_size(&screen_width, &screen_height);

  if (bw->flags & browser_TOOLBAR)
    toolbar_height = ro_theme_toolbar_height();

  win_width = screen_width * 3 / 4;
  if (1600 < win_width)
    win_width = 1600;
  win_height = win_width * 3 / 4;

  window.visible.x0 = (screen_width - win_width) / 2;
  window.visible.y0 = (screen_height - win_height) / 2;
  window.visible.x1 = window.visible.x0 + win_width;
  window.visible.y1 = window.visible.y0 + win_height;
  window.xscroll = 0;
  window.yscroll = 0;
  window.next = wimp_TOP;
  window.flags =
      wimp_WINDOW_MOVEABLE | wimp_WINDOW_NEW_FORMAT | wimp_WINDOW_BACK_ICON |
      wimp_WINDOW_CLOSE_ICON | wimp_WINDOW_TITLE_ICON | wimp_WINDOW_VSCROLL |
      wimp_WINDOW_HSCROLL | wimp_WINDOW_SIZE_ICON | wimp_WINDOW_TOGGLE_ICON |
      wimp_WINDOW_IGNORE_XEXTENT | wimp_WINDOW_IGNORE_YEXTENT |
      wimp_WINDOW_SCROLL_REPEAT;
  window.title_fg = wimp_COLOUR_BLACK;
  window.title_bg = wimp_COLOUR_LIGHT_GREY;
  window.work_fg = wimp_COLOUR_LIGHT_GREY;
  window.work_bg = wimp_COLOUR_WHITE;
  window.scroll_outer = wimp_COLOUR_DARK_GREY;
  window.scroll_inner = wimp_COLOUR_MID_LIGHT_GREY;
  window.highlight_bg = wimp_COLOUR_CREAM;
  window.extra_flags = 0;
  window.extent.x0 = 0;
  window.extent.y0 = win_height;
  window.extent.x1 = win_width;
  window.extent.y1 = toolbar_height;
  window.title_flags = wimp_ICON_TEXT | wimp_ICON_INDIRECTED | wimp_ICON_HCENTRED;
  window.work_flags = wimp_BUTTON_CLICK_DRAG << wimp_ICON_BUTTON_TYPE_SHIFT;
  window.sprite_area = wimpspriteop_AREA;
  window.xmin = 100;
  window.ymin = window.extent.y1 + 100;
  window.title_data.indirected_text.text = g->title;
  window.title_data.indirected_text.validation = -1;
  window.title_data.indirected_text.size = 255;
  window.icon_count = 0;
  g->window = wimp_create_window(&window);

  strcpy(g->title, "NetSurf");

  g->data.browser.toolbar = 0;
  if ((bw->flags & browser_TOOLBAR) != 0)
  {
    g->data.browser.toolbar = ro_theme_create_toolbar(g->url, g->status,
        g->throb_buf);
    g->data.browser.toolbar_width = -1;
    sprintf(g->throb_buf, "throbber0");
  }

  g->redraw_safety = SAFE;
  g->data.browser.reformat_pending = false;
  g->data.browser.old_width = 0;

  g->next = window_list;
  window_list = g;

  state.w = g->window;
  wimp_get_window_state(&state);
  state.next = wimp_TOP;
  ro_gui_window_open(g, (wimp_open*)&state);

	outline.w = g->window;
	wimp_get_window_outline(&outline);

	state.w = g->data.browser.toolbar;
	state.visible.x1 = outline.outline.x1 - 2;
	state.visible.y0 = state.visible.y1 - toolbar_height;
	state.xscroll = 0;
	state.yscroll = 0;
	state.next = wimp_TOP;

	g->data.browser.toolbar_width = state.visible.x1 - state.visible.x0;
	ro_theme_resize_toolbar(g->data.browser.toolbar,
			g->data.browser.toolbar_width,
			state.visible.y1 - state.visible.y0);

	wimp_open_window_nested((wimp_open *) &state, g->window,
			wimp_CHILD_LINKS_PARENT_VISIBLE_BOTTOM_OR_LEFT
					<< wimp_CHILD_LS_EDGE_SHIFT |
			wimp_CHILD_LINKS_PARENT_VISIBLE_BOTTOM_OR_LEFT
					<< wimp_CHILD_BS_EDGE_SHIFT |
			wimp_CHILD_LINKS_PARENT_VISIBLE_TOP_OR_RIGHT
					<< wimp_CHILD_RS_EDGE_SHIFT |
			wimp_CHILD_LINKS_PARENT_VISIBLE_TOP_OR_RIGHT
					<< wimp_CHILD_TS_EDGE_SHIFT);

  return g;
}


void gui_window_set_title(gui_window* g, char* title)
{
	strncpy(g->title, title, 255);
	wimp_force_redraw_title(g->window);
}


void gui_window_destroy(gui_window* g)
{
  assert(g != 0);

  if (g == window_list)
    window_list = g->next;
  else
  {
    gui_window* gg;
    assert(window_list != NULL);
    gg = window_list;
    while (gg->next != g && gg->next != NULL)
      gg = gg->next;
    assert(gg->next != NULL);
    gg->next = g->next;
  }

  xwimp_delete_window(g->window);
  if (g->data.browser.toolbar)
    xwimp_delete_window(g->data.browser.toolbar);

  xfree(g);
}

void gui_window_show(gui_window* g)
{
}

void gui_window_redraw(gui_window* g, unsigned long x0, unsigned long y0,
		unsigned long x1, unsigned long y1)
{
  if (g == NULL)
    return;

  wimp_force_redraw(g->window,
    ro_x_units(x0), ro_y_units(y1), ro_x_units(x1), ro_y_units(y0));
}

void gui_window_redraw_window(gui_window* g)
{
  wimp_window_info info;
  if (g == NULL)
    return;
  info.w = g->window;
  wimp_get_window_info_header_only(&info);
  wimp_force_redraw(g->window, info.extent.x0, info.extent.y0, info.extent.x1, info.extent.y1);
}

gui_safety gui_window_set_redraw_safety(gui_window* g, gui_safety s)
{
  gui_safety old;

  if (g == NULL)
    return SAFE;

  old = g->redraw_safety;
  g->redraw_safety = s;

  return old;
}


void ro_gui_window_redraw(gui_window* g, wimp_draw* redraw)
{
  osbool more;
  struct content *c = g->data.browser.bw->current_content;

  if (g->redraw_safety == SAFE && g->type == GUI_BROWSER_WINDOW && c != NULL)
  {
    more = wimp_redraw_window(redraw);
    wimp_set_font_colours(wimp_COLOUR_WHITE, wimp_COLOUR_BLACK);

    while (more)
    {
      content_redraw(c,
          (int) redraw->box.x0 - (int) redraw->xscroll,
          (int) redraw->box.y1 - (int) redraw->yscroll,
          c->width * 2, c->height * 2,
	  redraw->clip.x0, redraw->clip.y0,
	  redraw->clip.x1 - 1, redraw->clip.y1 - 1);
      more = wimp_get_rectangle(redraw);
    }
  }
  else
  {
    more = wimp_redraw_window(redraw);
    while (more)
      more = wimp_get_rectangle(redraw);
  }
}

void gui_window_set_scroll(gui_window* g, unsigned long sx, unsigned long sy)
{
  wimp_window_state state;
  if (g == NULL)
    return;
  state.w = g->window;
  wimp_get_window_state(&state);
  state.xscroll = ro_x_units(sx);
  state.yscroll = ro_y_units(sy);
  if ((g->data.browser.bw->flags & browser_TOOLBAR) != 0)
    state.yscroll += ro_theme_toolbar_height();
  ro_gui_window_open(g, (wimp_open*)&state);
}

unsigned long gui_window_get_width(gui_window* g)
{
  wimp_window_state state;
  state.w = g->window;
  wimp_get_window_state(&state);
  return browser_x_units(state.visible.x1 - state.visible.x0);
}


void gui_window_set_extent(gui_window *g, unsigned long width,
		unsigned long height)
{
	os_box extent = { 0, 0, 0, 0 };
	wimp_window_state state;
	int toolbar_height = 0;

	width *= 2;
	height *= 2;

	state.w = g->window;
	wimp_get_window_state(&state);

	/* account for toolbar height, if present */
	if (g->data.browser.bw->flags & browser_TOOLBAR)
		toolbar_height = ro_theme_toolbar_height();

	if (width < state.visible.x1 - state.visible.x0)
		width = state.visible.x1 - state.visible.x0;
	if (height < state.visible.y1 - state.visible.y0 - toolbar_height)
		height = state.visible.y1 - state.visible.y0 - toolbar_height;

	extent.y0 = -height;
	extent.x1 = width;
	extent.y1 = toolbar_height;
	wimp_set_extent(g->window, &extent);
	wimp_open_window((wimp_open *) &state);
}


void gui_window_set_status(gui_window* g, const char* text)
{
  if (strcmp(g->status, text) != 0)
  {
    strncpy(g->status, text, 255);
    wimp_set_icon_state(g->data.browser.toolbar, ICON_TOOLBAR_STATUS, 0, 0);
  }
}


void gui_window_message(gui_window* g, gui_message* msg)
{
  if (g == NULL || msg == NULL)
    return;

  switch (msg->type)
  {
    case msg_SET_URL:
      fprintf(stderr, "Set URL '%s'\n", msg->data.set_url.url);
      strncpy(g->url, msg->data.set_url.url, 255);
      wimp_set_icon_state(g->data.browser.toolbar, ICON_TOOLBAR_URL, 0, 0);
      break;
    default:
      break;
  }
}


/**
 * Open a window using the given wimp_open, handling toolbars and resizing.
 */

void ro_gui_window_open(gui_window *g, wimp_open *open)
{
	int width = open->visible.x1 - open->visible.x0;
	int height = open->visible.y1 - open->visible.y0;
	int toolbar_height = 0;
	struct content *content;
	wimp_window_state state;

	if (g->type != GUI_BROWSER_WINDOW) {
		wimp_open_window(open);
		return;
	}

	content = g->data.browser.bw->current_content;

	/* check for toggle to full size */
	state.w = g->window;
	wimp_get_window_state(&state);
	if ((state.flags & wimp_WINDOW_TOGGLED) &&
			(state.flags & wimp_WINDOW_BOUNDED_ONCE) &&
			!(state.flags & wimp_WINDOW_FULL_SIZE)) {
		open->visible.y0 = 0;
		open->visible.y1 = 0x1000;
		height = 0x1000;
	}

	/* account for toolbar height, if present */
	if (g->data.browser.bw->flags & browser_TOOLBAR) {
		toolbar_height = ro_theme_toolbar_height();
		height -= toolbar_height;
	}

	/* the height should be no less than the content height */
	if (content && height < content->height * 2)
		height = content->height * 2;

	/* change extent if necessary */
	if (g->data.browser.old_width != width ||
			g->data.browser.old_height != height) {
		if (content && g->data.browser.old_width != width) {
			g->data.browser.reformat_pending = true;
			gui_reformat_pending = true;
		}
		g->data.browser.old_width = width;
		g->data.browser.old_height = height;

		if (content && width < content->width * 2)
			width = content->width * 2;
		else {
			os_box extent = { 0, -height, width, toolbar_height };
			wimp_set_extent(g->window, &extent);
		}
	}

	wimp_open_window(open);

	/* update extent to actual size if toggled */
	if ((state.flags & wimp_WINDOW_TOGGLED) &&
			(state.flags & wimp_WINDOW_BOUNDED_ONCE) &&
			!(state.flags & wimp_WINDOW_FULL_SIZE)) {
		width = open->visible.x1 - open->visible.x0;
		height = open->visible.y1 - open->visible.y0 - toolbar_height;
		if (content && height < content->height * 2)
			height = content->height * 2;
		{
			os_box extent = { 0, -height, width, toolbar_height };
			wimp_set_extent(g->window, &extent);
		}
		g->data.browser.old_width = width;
		g->data.browser.old_height = height;
	}

	/* open toolbar, if present */
	if (!toolbar_height)
		return;

	state.w = g->data.browser.toolbar;
	wimp_get_window_state(&state);
	if (state.visible.x1 - state.visible.x0 !=
			g->data.browser.toolbar_width) {
		g->data.browser.toolbar_width = state.visible.x1 -
				state.visible.x0;
		ro_theme_resize_toolbar(g->data.browser.toolbar,
				g->data.browser.toolbar_width,
				state.visible.y1 - state.visible.y0);
	}
}


void ro_gui_throb(void)
{
  gui_window* g;
  float nowtime = (float) clock() / CLOCKS_PER_SEC;

  for (g = window_list; g != NULL; g = g->next)
  {
    if (g->type == GUI_BROWSER_WINDOW)
    {
      if ((g->data.browser.bw->flags & browser_TOOLBAR) != 0)
      {
        if (g->data.browser.bw->throbbing != 0)
        {
          if (nowtime > g->throbtime + 0.2)
          {
            g->throbtime = nowtime;
            g->throbber++;
            if (theme_throbs < g->throbber)
              g->throbber = 0;
            sprintf(g->throb_buf, "throbber%u", g->throbber);
            wimp_set_icon_state(g->data.browser.toolbar,
                ICON_TOOLBAR_THROBBER, 0, 0);
          }
        }
      }
    }
  }
}

gui_window* ro_lookup_gui_from_w(wimp_w window)
{
  gui_window* g;
  for (g = window_list; g != NULL; g = g->next)
  {
    if (g->type == GUI_BROWSER_WINDOW)
    {
      if (g->window == window)
      {
        return g;
      }
    }
  }
  return NULL;
}

gui_window* ro_lookup_gui_toolbar_from_w(wimp_w window)
{
  gui_window* g;

  for (g = window_list; g != NULL; g = g->next)
  {
    if (g->type == GUI_BROWSER_WINDOW)
    {
      if (g->data.browser.toolbar == window)
      {
        return g;
      }
    }
  }
  return NULL;
}


/**
 * Convert a wimp window handle to the owning gui_window structure.
 */
gui_window *ro_gui_window_lookup(wimp_w w)
{
	gui_window *g;

	for (g = window_list; g; g = g->next) {
		if (g->window == w)
			return g;
		else if (g->type == GUI_BROWSER_WINDOW &&
				g->data.browser.toolbar == w)
			return g;
	}
	return 0;
}


void ro_gui_window_mouse_at(wimp_pointer* pointer)
{
  int x,y;
  wimp_window_state state;
  gui_window* g;

  g = ro_lookup_gui_from_w(pointer->w);

  if (g == NULL)
    return;

  if (g->redraw_safety != SAFE)
  {
    fprintf(stderr, "mouse at UNSAFE\n");
    return;
  }

  state.w = pointer->w;
  wimp_get_window_state(&state);

  x = browser_x_units(window_x_units(pointer->pos.x, &state));
  y = browser_y_units(window_y_units(pointer->pos.y, &state));

  if (g->drag_status == drag_BROWSER_TEXT_SELECTION)
  {
    struct browser_action msg;
    msg.type = act_ALTER_SELECTION;
    msg.data.mouse.x = x;
    msg.data.mouse.y = y;
    browser_window_action(g->data.browser.bw, &msg);
  }

  if (g->type == GUI_BROWSER_WINDOW)
  {
    if (g->data.browser.bw->current_content != NULL)
    {
      struct browser_action msg;
      msg.type = act_MOUSE_AT;
      msg.data.mouse.x = x;
      msg.data.mouse.y = y;
      browser_window_action(g->data.browser.bw, &msg);
    }
  }
}

void ro_gui_toolbar_click(gui_window* g, wimp_pointer* pointer)
{
	switch (pointer->i) {
		case ICON_TOOLBAR_HISTORY:
			ro_gui_history_open(g->data.browser.bw,
					g->data.browser.bw->history_entry,
					pointer->pos.x - 200,
					pointer->pos.y + 100);
			break;
		case ICON_TOOLBAR_RELOAD:
			browser_window_open_location_historical(g->data.browser.bw,
					g->data.browser.bw->url, 0, 0);
			break;
	}
}


void ro_gui_window_click(gui_window* g, wimp_pointer* pointer)
{
  struct browser_action msg;
  int x,y;
  wimp_window_state state;

  if (g->redraw_safety != SAFE)
  {
    fprintf(stderr, "gui_window_click UNSAFE\n");
    return;
  }

  state.w = pointer->w;
  wimp_get_window_state(&state);

  if (g->type == GUI_BROWSER_WINDOW)
  {
    x = browser_x_units(window_x_units(pointer->pos.x, &state));
    y = browser_y_units(window_y_units(pointer->pos.y, &state));

    if (pointer->buttons == wimp_CLICK_MENU)
    {
      /* check for mouse gestures */
      ro_gui_mouse_action(g);
    }
    else if (g->data.browser.bw->current_content != NULL)
    {
      if (g->data.browser.bw->current_content->type == CONTENT_HTML)
      {
        if (pointer->buttons == wimp_CLICK_SELECT)
	{
		msg.type = act_MOUSE_CLICK;
        	msg.data.mouse.x = x;
		msg.data.mouse.y = y;
		msg.data.mouse.buttons = act_BUTTON_NORMAL;
		if (browser_window_action(g->data.browser.bw, &msg) == 1)
			return;
		msg.type = act_UNKNOWN;
	}
        if (pointer->buttons == wimp_CLICK_SELECT && g->data.browser.bw->current_content->data.html.text_selection.selected == 1)
          msg.type = act_CLEAR_SELECTION;
        else if (pointer->buttons == wimp_CLICK_ADJUST && g->data.browser.bw->current_content->data.html.text_selection.selected == 1)
                  msg.type = act_ALTER_SELECTION;
        else if (pointer->buttons == wimp_DRAG_SELECT ||
                 pointer->buttons == wimp_DRAG_ADJUST)
        {
          msg.type = act_START_NEW_SELECTION;
          if (pointer->buttons == wimp_DRAG_ADJUST && g->data.browser.bw->current_content->data.html.text_selection.selected == 1)
            msg.type = act_ALTER_SELECTION;

          ro_gui_start_selection(pointer, &state, g);
          g->drag_status = drag_BROWSER_TEXT_SELECTION;
        }
        msg.data.mouse.x = x;
        msg.data.mouse.y = y;
        if (msg.type != act_UNKNOWN)
          browser_window_action(g->data.browser.bw, &msg);

        if (pointer->buttons == wimp_CLICK_ADJUST && g->data.browser.bw->current_content->data.html.text_selection.selected == 1)
        {
          current_drag.data.selection.gui->data.browser.bw->current_content->data.html.text_selection.altering = alter_UNKNOWN;
        }

        if (pointer->buttons == wimp_CLICK_SELECT
            || pointer->buttons == wimp_CLICK_ADJUST)
        {
          if (pointer->buttons == wimp_CLICK_SELECT)
            msg.type = act_FOLLOW_LINK;
          else
            msg.type = act_FOLLOW_LINK_NEW_WINDOW;
          msg.data.mouse.x = x;
          msg.data.mouse.y = y;
          browser_window_action(g->data.browser.bw, &msg);
        }
      }
    }
  }
}


void gui_window_start_throbber(struct gui_window* g)
{
  g->throbtime = (float) (clock() + 0) / CLOCKS_PER_SEC;  /* workaround compiler warning */
  g->throbber = 0;
}

void gui_window_stop_throbber(gui_window* g)
{
  g->throbber = 0;
  sprintf(g->throb_buf, "throbber%u", g->throbber);
  wimp_set_icon_state(g->data.browser.toolbar, ICON_TOOLBAR_THROBBER, 0, 0);
}

void gui_window_place_caret(gui_window *g, int x, int y, int height)
{
	wimp_set_caret_position(g->window, -1,
			x * 2, -(y + height) * 2, height * 2, -1);
}


/**
 * Process Key_Pressed events in a browser window.
 */
bool ro_gui_window_keypress(gui_window *g, int key, bool toolbar)
{
	struct content *content = g->data.browser.bw->current_content;
	wimp_window_state state;
	int y;

	assert(g->type == GUI_BROWSER_WINDOW);

	/* First send the key to the browser window, eg. form fields. */
	if (!toolbar) {
		int c = key;
		/* Munge cursor keys into unused control chars */
		if (c == 396) c = 29;          /* Left */
		else if (c == 397) c = 28;     /* Right */
		else if (c == 398) c = 31;     /* Down */
		else if (c == 399) c = 30;     /* Up */
		if (c < 256)
			if (browser_window_key_press(g->data.browser.bw,
					(char) c))
				return true;
	}

	switch (key) {
		case wimp_KEY_F8:	/* View source. */
			ro_gui_view_source(content);
			return true;

		case wimp_KEY_F9:	/* Dump content for debugging. */
			switch (content->type) {
				case CONTENT_HTML:
					box_dump(content->data.html.layout->children, 0);
					break;
				case CONTENT_CSS:
					css_dump_stylesheet(content->data.css.css);
					break;
				default:
					break;
			}
			return true;

		case wimp_KEY_F10:	/* Dump cache for debugging. */
			cache_dump();
			return true;

		case wimp_KEY_CONTROL + wimp_KEY_F2:	/* Close window. */
			browser_window_destroy(g->data.browser.bw);
			return true;

		case wimp_KEY_RETURN:
			if (!toolbar)
				break;
			if (strcasecmp(g->url, "about:") == 0) {
				about_create();
				browser_window_open_location(g->data.browser.bw,
						"file:///%3CWimp$ScrapDir%3E/WWW/NetSurf/About");
			} else {
				browser_window_open_location(g->data.browser.bw, g->url);
			}
			return true;

		case wimp_KEY_UP:
		case wimp_KEY_DOWN:
		case wimp_KEY_PAGE_UP:
		case wimp_KEY_PAGE_DOWN:
		case wimp_KEY_CONTROL | wimp_KEY_UP:
		case wimp_KEY_CONTROL | wimp_KEY_DOWN:
			break;

		default:
			return false;
	}

	state.w = g->window;
	wimp_get_window_state(&state);
	y = state.visible.y1 - state.visible.y0 - 32;
	if (g->data.browser.bw->flags & browser_TOOLBAR)
		y -= ro_theme_toolbar_height();

	switch (key) {
		case wimp_KEY_UP:
			state.yscroll += 32;
			break;
		case wimp_KEY_DOWN:
			state.yscroll -= 32;
			break;
		case wimp_KEY_PAGE_UP:
			state.yscroll += y;
			break;
		case wimp_KEY_PAGE_DOWN:
			state.yscroll -= y;
			break;
		case wimp_KEY_CONTROL | wimp_KEY_UP:
			state.yscroll = 1000;
			break;
		case wimp_KEY_CONTROL | wimp_KEY_DOWN:
			state.yscroll = -0x10000000;
			break;
        }

	wimp_open_window((wimp_open *) &state);
	return true;
}


/**
 * Process Scroll_Request events.
 */
void ro_gui_scroll_request(wimp_scroll *scroll)
{
	int x, y;
	gui_window *g = ro_gui_window_lookup(scroll->w);

	if (!g || g->type != GUI_BROWSER_WINDOW)
		return;

	x = scroll->visible.x1 - scroll->visible.x0 - 32;
	y = scroll->visible.y1 - scroll->visible.y0 - 32;
	if (g->data.browser.bw->flags & browser_TOOLBAR)
		y -= ro_theme_toolbar_height();

	switch (scroll->xmin) {
		case wimp_SCROLL_PAGE_LEFT:
			scroll->xscroll -= x;
			break;
		case wimp_SCROLL_COLUMN_LEFT:
			scroll->xscroll -= 32;
			break;
		case wimp_SCROLL_COLUMN_RIGHT:
			scroll->xscroll += 32;
			break;
		case wimp_SCROLL_PAGE_RIGHT:
			scroll->xscroll += x;
			break;
		default:
			break;
	}

	switch (scroll->ymin) {
		case wimp_SCROLL_PAGE_UP:
			scroll->yscroll += y;
			break;
		case wimp_SCROLL_LINE_UP:
			scroll->yscroll += 32;
			break;
		case wimp_SCROLL_LINE_DOWN:
			scroll->yscroll -= 32;
			break;
		case wimp_SCROLL_PAGE_DOWN:
			scroll->yscroll -= y;
			break;
		default:
			break;
	}

	wimp_open_window((wimp_open *) scroll);
}
