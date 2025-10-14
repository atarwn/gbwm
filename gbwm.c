#define _POSIX_C_SOURCE 200809L
/* gbwm - grid-based tiling window manager */
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xft/Xft.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>
#include <X11/extensions/XTest.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>

typedef struct Client Client;
struct Client {
	Window win;
	int x, y, w, h;
	int saved_x, saved_y, saved_w, saved_h;  // Saved position before fullscreen
	int isfullscreen;
	int workspace;
	Client *next;
};

typedef union {
	int i;
	unsigned int ui;
	float f;
	const void *v;
} Arg;

typedef struct {
	unsigned int mod;
	KeySym keysym;
	void (*func)(const Arg *);
	const Arg arg;
} Key;

static Display *dpy;
static Window root;
static Client *workspaces[9] = {NULL};  // 9 workspaces
static int current_ws = 0;
static Client *focused = NULL;
static int sw, sh;
static int overlay_mode = 0;
static char overlay_input[3] = {0};
static Window overlay_win = 0;
static GC gc;
static XftDraw *xftdraw = NULL;
static XftFont *font = NULL;
static XftColor xft_col_bg, xft_col_fg, xft_col_sel;
static unsigned long border_normal, border_focused;

// ICCCM atoms
static Atom wm_protocols, wm_delete_window, wm_state, wm_take_focus;

// Forward decls
static void arrange(void);
static void resize(Client *c, int x, int y, int w, int h);
static void focus(Client *c, int warp);
static void spawn(const Arg *arg);
static void killclient(const Arg *arg);
static void toggle_fullscreen(const Arg *arg);
static void enter_overlay(const Arg *arg);
static void process_overlay_input(void);
static void draw_overlay(void);
static void hide_overlay(void);
static void quit(const Arg *arg);
static void cycle_focus(const Arg *arg);
static void grabkeys(void);
static void setfullscreen(Client *c, int fullscreen);
static int sendevent(Client *c, Atom proto);
static void updateborder(Client *c);
static void find_next_free_cell(int *out_r, int *out_c);
static void switchws(const Arg *arg);
static void movewin_to_ws(const Arg *arg);
static void die(const char *fmt, ...);

#include "config.h"

// Event handlers
static void buttonpress(XEvent *e) {
	for (Client *c = workspaces[current_ws]; c; c = c->next)
		if (c->win == e->xbutton.subwindow) {
			focus(c, 1);
			break;
		}
}

static void clientmessage(XEvent *e) {
	XClientMessageEvent *cme = &e->xclient;
	Client *c;
	for (c = workspaces[current_ws]; c; c = c->next)
		if (c->win == cme->window)
			break;
	if (!c)
		return;

	if (cme->message_type == wm_state && cme->data.l[1] == (long)XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False)) {
		setfullscreen(c, cme->data.l[0] == 1 || (cme->data.l[0] == 2 && !c->isfullscreen));
	}
}

static void maprequest(XEvent *e) {
	XWindowAttributes wa;
	if (!XGetWindowAttributes(dpy, e->xmaprequest.window, &wa)) return;
	if (wa.override_redirect) return;

	Client *c = calloc(1, sizeof(Client));
	c->win = e->xmaprequest.window;
	c->workspace = current_ws;
	c->next = workspaces[current_ws];
	workspaces[current_ws] = c;

	// ICCCM setup
	XSetWindowBorderWidth(dpy, c->win, border_width);
	XSelectInput(dpy, c->win, EnterWindowMask | FocusChangeMask | PropertyChangeMask | StructureNotifyMask);

	// Set WM_STATE
	long data[] = { NormalState, None };
	XChangeProperty(dpy, c->win, wm_state, wm_state, 32, PropModeReplace, (unsigned char *)data, 2);

	XMapWindow(dpy, c->win);
	focus(c, 1);
	arrange();
}

static void removeclient(Window win) {
	Client *c, **prev;
	for (prev = &workspaces[current_ws]; (c = *prev); prev = &c->next) {
		if (c->win == win) {
			*prev = c->next;
			if (focused == c) {
				focused = workspaces[current_ws];
				if (focused)
					focus(focused, 1);
			}
			free(c);
			arrange();
			return;
		}
	}
}

static void unmapnotify(XEvent *e) {
	removeclient(e->xunmap.window);
}

static void destroynotify(XEvent *e) {
	removeclient(e->xdestroywindow.window);
}

static void enternotify(XEvent *e) {
	if (e->xcrossing.mode != NotifyNormal || e->xcrossing.detail == NotifyInferior)
		return;
	for (Client *c = workspaces[current_ws]; c; c = c->next)
		if (c->win == e->xcrossing.window) {
			focus(c, 0);
			break;
		}
}

static void expose(XEvent *e) {
	if (e->xexpose.window == overlay_win && overlay_mode) {
		draw_overlay();
	}
}

static void keypress(XEvent *e) {
	if (overlay_mode) {
		KeySym k = XLookupKeysym(&e->xkey, 0);
		if (k == XK_Escape) {
			hide_overlay();
			return;
		}
		if (k == XK_BackSpace) {
			if (overlay_input[1] != 0) {
				overlay_input[1] = 0;
			} else if (overlay_input[0] != 0) {
				overlay_input[0] = 0;
			}
			draw_overlay();
			return;
		}

		char ch = 0;
		if (k >= '0' && k <= '9') ch = (char)k;
		else if (k >= 'a' && k <= 'z') ch = (char)k;
		else if (k >= 'A' && k <= 'Z') ch = (char)(k - 'A' + 'a');
		else return;

		int found = 0;
		for (int r = 0; r < GRID_ROWS && !found; r++)
			for (int c = 0; c < GRID_COLS && !found; c++)
				if (grid_chars[r][c] == ch) found = 1;
		if (!found) return;

		if (overlay_input[0] == 0) {
			overlay_input[0] = ch;
			draw_overlay();
		} else if (overlay_input[1] == 0) {
			overlay_input[1] = ch;
			draw_overlay();
			struct timespec ts = {
				.tv_sec = 0,
				.tv_nsec = 150000 * 1000
			};
			nanosleep(&ts, NULL);
			process_overlay_input();
			hide_overlay();
		}
		return;
	}

	KeySym keysym = XLookupKeysym(&e->xkey, 0);
	unsigned int state = e->xkey.state & ~(LockMask | Mod2Mask);

	for (unsigned int i = 0; i < sizeof(keys) / sizeof(Key); i++) {
		if (keysym == keys[i].keysym && state == keys[i].mod && keys[i].func) {
			keys[i].func(&keys[i].arg);
			return;
		}
	}
}

// Core logic
static void resize(Client *c, int x, int y, int w, int h) {
	c->x = x; c->y = y; c->w = w; c->h = h;
	XMoveResizeWindow(dpy, c->win, x, y, w - 2 * border_width, h - 2 * border_width);
}

static void updateborder(Client *c) {
	XSetWindowBorder(dpy, c->win, c == focused ? border_focused : border_normal);
}

static void focus(Client *c, int warp) {
	if (!c) return;

	Client *old = focused;
	focused = c;

	if (old && old != c)
		updateborder(old);

	updateborder(c);
	XRaiseWindow(dpy, c->win);
	XSetInputFocus(dpy, c->win, RevertToPointerRoot, CurrentTime);
	sendevent(c, wm_take_focus);

	if (warp && !c->isfullscreen) {
		int cursor_x = c->x + c->w - 16;
		int cursor_y = c->y + c->h - 16;
		if (cursor_x < 0) cursor_x = 0;
		if (cursor_y < 0) cursor_y = 0;
		if (cursor_x >= sw) cursor_x = sw - 1;
		if (cursor_y >= sh) cursor_y = sh - 1;

		XWarpPointer(dpy, None, root, 0, 0, 0, 0, cursor_x, cursor_y);
	}
}

static int is_cell_free(int r, int c, int cell_w, int cell_h) {
	int cell_x = padding + c * (cell_w + padding);
	int cell_y = padding + r * (cell_h + padding);

	// Check if any window overlaps with this cell
	for (Client *cl = workspaces[current_ws]; cl; cl = cl->next) {
		if (cl->isfullscreen) continue;

		// Check for overlap
		int cl_right = cl->x + cl->w;
		int cl_bottom = cl->y + cl->h;
		int cell_right = cell_x + cell_w;
		int cell_bottom = cell_y + cell_h;

		// If rectangles overlap
		if (!(cl_right <= cell_x || cl->x >= cell_right ||
			  cl_bottom <= cell_y || cl->y >= cell_bottom)) {
			return 0;  // Cell is occupied
		}
	}

	return 1;  // Cell is free
}

static void find_next_free_cell(int *out_r, int *out_c) {
	int cell_w = (sw - padding * (GRID_COLS + 1)) / GRID_COLS;
	int cell_h = (sh - padding * (GRID_ROWS + 1)) / GRID_ROWS;

	// First pass: look for any completely free cell
	for (int r = 0; r < GRID_ROWS; r++) {
		for (int c = 0; c < GRID_COLS; c++) {
			if (is_cell_free(r, c, cell_w, cell_h)) {
				*out_r = r;
				*out_c = c;
				return;
			}
		}
	}

	// Second pass: no free space found, check top-left for 1x1 windows
	int cell_x = padding;
	int cell_y = padding;

	for (Client *cl = workspaces[current_ws]; cl; cl = cl->next) {
		if (cl->isfullscreen) continue;
		if (cl->x == cell_x && cl->y == cell_y &&
			cl->w == cell_w && cl->h == cell_h) {
			// Found a 1x1 window at top-left, find next free cell
			for (int r = 0; r < GRID_ROWS; r++) {
				for (int c = 0; c < GRID_COLS; c++) {
					int check_x = padding + c * (cell_w + padding);
					int check_y = padding + r * (cell_h + padding);

					int found_1x1 = 0;
					for (Client *check = workspaces[current_ws]; check; check = check->next) {
						if (check->isfullscreen) continue;
						if (check->x == check_x && check->y == check_y &&
							check->w == cell_w && check->h == cell_h) {
							found_1x1 = 1;
							break;
						}
					}

					if (!found_1x1) {
						*out_r = r;
						*out_c = c;
						return;
					}
				}
			}
			break;
		}
	}

	// Fallback to top-left
	*out_r = 0;
	*out_c = 0;
}

static void arrange(void) {
	if (!workspaces[current_ws]) return;

	if (!focused) focused = workspaces[current_ws];

	if (focused->isfullscreen) {
		return;
	}

	// Default window location - find next free cell
	int cell_w = (sw - padding * (GRID_COLS + 1)) / GRID_COLS;
	int cell_h = (sh - padding * (GRID_ROWS + 1)) / GRID_ROWS;

	if (focused->w == 0 || focused->h == 0) {
		int r, c;
		find_next_free_cell(&r, &c);
		int x = padding + c * (cell_w + padding);
		int y = padding + r * (cell_h + padding);
		resize(focused, x, y, cell_w, cell_h);
	}

	// Update all borders
	for (Client *c = workspaces[current_ws]; c; c = c->next)
		updateborder(c);
}

static void draw_overlay(void) {
	if (!overlay_win) return;

	XClearWindow(dpy, overlay_win);

	int cell_w = (sw - padding * (GRID_COLS + 1)) / GRID_COLS;
	int cell_h = (sh - padding * (GRID_ROWS + 1)) / GRID_ROWS;

	int r1 = -1, c1 = -1, r2 = -1, c2 = -1;
	if (overlay_input[0]) {
		for (int r = 0; r < GRID_ROWS; r++)
			for (int c = 0; c < GRID_COLS; c++)
				if (grid_chars[r][c] == overlay_input[0]) { r1 = r; c1 = c; }
	}
	if (overlay_input[1]) {
		for (int r = 0; r < GRID_ROWS; r++)
			for (int c = 0; c < GRID_COLS; c++)
				if (grid_chars[r][c] == overlay_input[1]) { r2 = r; c2 = c; }
	}

	for (int r = 0; r < GRID_ROWS; r++) {
		for (int c = 0; c < GRID_COLS; c++) {
			int x = padding + c * (cell_w + padding);
			int y = padding + r * (cell_h + padding);

			int is_selected = 0;
			if (r1 >= 0 && c1 >= 0) {
				if (r2 >= 0 && c2 >= 0) {
					int min_r = r1 < r2 ? r1 : r2;
					int max_r = r1 > r2 ? r1 : r2;
					int min_c = c1 < c2 ? c1 : c2;
					int max_c = c1 > c2 ? c1 : c2;
					if (r >= min_r && r <= max_r && c >= min_c && c <= max_c)
						is_selected = 1;
				} else if (r == r1 && c == c1) {
					is_selected = 1;
				}
			}

			if (is_selected) {
				XSetForeground(dpy, gc, xft_col_sel.pixel);
				XFillRectangle(dpy, overlay_win, gc, x, y, cell_w, cell_h);
			}

			XSetForeground(dpy, gc, xft_col_fg.pixel);
			XDrawRectangle(dpy, overlay_win, gc, x, y, cell_w, cell_h);

			if (font && xftdraw) {
				char txt[2] = {grid_chars[r][c], 0};
				XGlyphInfo extents;
				XftTextExtentsUtf8(dpy, font, (FcChar8*)txt, strlen(txt), &extents);

				int tx = x + (cell_w - extents.width) / 2;
				int ty = y + (cell_h - extents.height) / 2 + extents.y;

				XftDrawStringUtf8(xftdraw, &xft_col_fg, font, tx, ty,
								(FcChar8*)txt, strlen(txt));
			}
		}
	}

	if (overlay_input[0] || overlay_input[1]) {
		char status[64];
		snprintf(status, sizeof(status), "Input: %c%c",
				overlay_input[0] ? overlay_input[0] : ' ',
				overlay_input[1] ? overlay_input[1] : ' ');

		if (font && xftdraw) {
			XftDrawStringUtf8(xftdraw, &xft_col_fg, font, 20, sh - 20,
							(FcChar8*)status, strlen(status));
		}
	}

	XFlush(dpy);
}

static void enter_overlay(const Arg *arg) {
	if (!focused) return;

	overlay_mode = 1;
	memset(overlay_input, 0, sizeof(overlay_input));

	if (!overlay_win) {
		XSetWindowAttributes wa = {
			.override_redirect = True,
			.background_pixel = xft_col_bg.pixel,
			.event_mask = ExposureMask | KeyPressMask
		};
		overlay_win = XCreateWindow(dpy, root, 0, 0, sw, sh, 0,
			CopyFromParent, InputOutput, CopyFromParent,
			CWOverrideRedirect | CWBackPixel | CWEventMask, &wa);

		gc = XCreateGC(dpy, overlay_win, 0, NULL);

		Visual *visual = DefaultVisual(dpy, DefaultScreen(dpy));
		Colormap cmap = DefaultColormap(dpy, DefaultScreen(dpy));
		xftdraw = XftDrawCreate(dpy, overlay_win, visual, cmap);

		unsigned long opacity = (unsigned long)(0.85 * 0xffffffff);
		Atom atom = XInternAtom(dpy, "_NET_WM_WINDOW_OPACITY", False);
		XChangeProperty(dpy, overlay_win, atom, XA_CARDINAL, 32,
					   PropModeReplace, (unsigned char *)&opacity, 1);
	}

	XMapRaised(dpy, overlay_win);
	XSetInputFocus(dpy, overlay_win, RevertToPointerRoot, CurrentTime);
	draw_overlay();
}

static void hide_overlay(void) {
	overlay_mode = 0;
	memset(overlay_input, 0, sizeof(overlay_input));
	if (overlay_win) {
		XUnmapWindow(dpy, overlay_win);
	}
	if (focused) {
		XSetInputFocus(dpy, focused->win, RevertToPointerRoot, CurrentTime);
	}
}

static void process_overlay_input(void) {
	if (!focused || overlay_input[0] == 0 || overlay_input[1] == 0) return;

	int r1 = -1, c1 = -1, r2 = -1, c2 = -1;
	for (int r = 0; r < GRID_ROWS; r++) {
		for (int c = 0; c < GRID_COLS; c++) {
			if (grid_chars[r][c] == overlay_input[0]) { r1 = r; c1 = c; }
			if (grid_chars[r][c] == overlay_input[1]) { r2 = r; c2 = c; }
		}
	}
	if (r1 == -1 || r2 == -1) return;

	if (r1 > r2) { int t = r1; r1 = r2; r2 = t; }
	if (c1 > c2) { int t = c1; c1 = c2; c2 = t; }

	int cols_span = c2 - c1 + 1;
	int rows_span = r2 - r1 + 1;

	int cell_w = (sw - padding * (GRID_COLS + 1)) / GRID_COLS;
	int cell_h = (sh - padding * (GRID_ROWS + 1)) / GRID_ROWS;

	int x = padding + c1 * (cell_w + padding);
	int y = padding + r1 * (cell_h + padding);
	int w = cols_span * cell_w + (cols_span - 1) * padding;
	int h = rows_span * cell_h + (rows_span - 1) * padding;

	resize(focused, x, y, w, h);
	if (focused) focus(focused, 1);
}

// Workspace functions
static void switchws(const Arg *arg) {
	int ws = arg->i;
	if (ws < 0 || ws >= 9 || ws == current_ws) return;
	
	current_ws = ws;
	
	// Hide all windows from all workspaces
	for (int i = 0; i < 9; i++) {
		for (Client *c = workspaces[i]; c; c = c->next) {
			XUnmapWindow(dpy, c->win);
		}
	}
	
	// Show current workspace windows
	for (Client *c = workspaces[current_ws]; c; c = c->next) {
		XMapWindow(dpy, c->win);
	}
	
	focused = workspaces[current_ws];
	if (focused) focus(focused, 1);
	arrange();
}

static void movewin_to_ws(const Arg *arg) {
	int ws = arg->i;
	if (!focused || ws < 0 || ws >= 9 || ws == current_ws) return;
	
	Client *moving = focused;
	
	// Remove from current workspace
	Client **prev;
	for (prev = &workspaces[current_ws]; *prev; prev = &(*prev)->next) {
		if (*prev == moving) {
			*prev = moving->next;
			break;
		}
	}
	
	// Add to target workspace
	moving->workspace = ws;
	moving->next = workspaces[ws];
	workspaces[ws] = moving;
	moving->isfullscreen = 0;  // Reset fullscreen state
	
	// Hide the window we just moved
	XUnmapWindow(dpy, moving->win);
	
	// Update focus to next available window in current workspace
	focused = workspaces[current_ws];
	if (focused) {
		focus(focused, 0);
	} else {
		focused = NULL;
	}
	
	// Re-arrange current workspace
	arrange();
}

// Action functions
static int sendevent(Client *c, Atom proto) {
	int n;
	Atom *protocols;
	int exists = 0;
	XEvent ev;

	if (XGetWMProtocols(dpy, c->win, &protocols, &n)) {
		while (!exists && n--)
			exists = protocols[n] == proto;
		XFree(protocols);
	}
	if (exists) {
		ev.type = ClientMessage;
		ev.xclient.window = c->win;
		ev.xclient.message_type = wm_protocols;
		ev.xclient.format = 32;
		ev.xclient.data.l[0] = proto;
		ev.xclient.data.l[1] = CurrentTime;
		XSendEvent(dpy, c->win, False, NoEventMask, &ev);
	}
	return exists;
}

static void killclient(const Arg *arg) {
	if (!focused) return;
	if (!sendevent(focused, wm_delete_window)) {
		XGrabServer(dpy);
		XSetCloseDownMode(dpy, DestroyAll);
		XKillClient(dpy, focused->win);
		XSync(dpy, False);
		XUngrabServer(dpy);
	}
}

static void setfullscreen(Client *c, int fullscreen) {
	if (!c) return;

	if (fullscreen && !c->isfullscreen) {
		// Save current position before going fullscreen
		c->saved_x = c->x;
		c->saved_y = c->y;
		c->saved_w = c->w;
		c->saved_h = c->h;

		c->isfullscreen = 1;

		// Remove border and set to full screen
		XSetWindowBorderWidth(dpy, c->win, 0);
		resize(c, 0, 0, sw, sh);
		XRaiseWindow(dpy, c->win);

	} else if (!fullscreen && c->isfullscreen) {
		// Restore saved position
		c->isfullscreen = 0;

		// Restore border
		XSetWindowBorderWidth(dpy, c->win, border_width);

		// Restore original position
		resize(c, c->saved_x, c->saved_y, c->saved_w, c->saved_h);
	}

	// Update _NET_WM_STATE
	XEvent ev;
	ev.type = ClientMessage;
	ev.xclient.window = c->win;
	ev.xclient.message_type = XInternAtom(dpy, "_NET_WM_STATE", False);
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = fullscreen ? 1 : 0;
	ev.xclient.data.l[1] = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
	ev.xclient.data.l[2] = 0;
	XSendEvent(dpy, root, False, SubstructureNotifyMask | SubstructureRedirectMask, &ev);
}

static void toggle_fullscreen(const Arg *arg) {
	if (!focused) return;
	setfullscreen(focused, !focused->isfullscreen);
}

static void spawn(const Arg *arg) {
	if (fork() == 0) {
		if (dpy)
			close(ConnectionNumber(dpy));
		setsid();
		execvp(((char **)arg->v)[0], (char **)arg->v);
		die("execvp %s failed", ((char **)arg->v)[0]);
	}
}

static void quit(const Arg *arg) {
	exit(0);
}

static void cycle_focus(const Arg *arg) {
	if (!workspaces[current_ws]) return;

	if (!focused) {
		focus(workspaces[current_ws], 1);
		return;
	}

	Client *next = focused->next;
	if (!next) next = workspaces[current_ws];

	focus(next, 1);
}

static void grabkeys(void) {
	XUngrabKey(dpy, AnyKey, AnyModifier, root);
	for (unsigned int i = 0; i < sizeof(keys) / sizeof(Key); i++) {
		KeyCode code = XKeysymToKeycode(dpy, keys[i].keysym);
		if (code) {
			XGrabKey(dpy, code, keys[i].mod, root, True,
					 GrabModeAsync, GrabModeAsync);
			XGrabKey(dpy, code, keys[i].mod | Mod2Mask, root, True,
					 GrabModeAsync, GrabModeAsync);
		}
	}
}

static void sigchld(int s) {
	(void)s;
	while (waitpid(-1, NULL, WNOHANG) > 0);
}

int xerror_handler(Display *dpy, XErrorEvent *ee) {
	return 0;
}

static void setup_colors(void) {
	Visual *visual = DefaultVisual(dpy, DefaultScreen(dpy));
	Colormap cmap = DefaultColormap(dpy, DefaultScreen(dpy));

	XftColorAllocName(dpy, visual, cmap, col_bg, &xft_col_bg);
	XftColorAllocName(dpy, visual, cmap, col_fg, &xft_col_fg);
	XftColorAllocName(dpy, visual, cmap, col_sel, &xft_col_sel);

	font = XftFontOpenName(dpy, DefaultScreen(dpy), overlay_font);

	// Allocate border colors
	XColor color;
	XParseColor(dpy, cmap, col_border_normal, &color);
	XAllocColor(dpy, cmap, &color);
	border_normal = color.pixel;

	XParseColor(dpy, cmap, col_border_focused, &color);
	XAllocColor(dpy, cmap, &color);
	border_focused = color.pixel;
}

static void setup_icccm(void) {
	wm_protocols = XInternAtom(dpy, "WM_PROTOCOLS", False);
	wm_delete_window = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	wm_state = XInternAtom(dpy, "WM_STATE", False);
	wm_take_focus = XInternAtom(dpy, "WM_TAKE_FOCUS", False);
}

static void setrootbackground(void) {
	Colormap cmap = DefaultColormap(dpy, DefaultScreen(dpy));
	XColor color;

	if (XParseColor(dpy, cmap, root_bg, &color) &&
		XAllocColor(dpy, cmap, &color)) {
		XSetWindowBackground(dpy, root, color.pixel);
		XClearWindow(dpy, root);
	}
}

void die(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fputc('\n', stderr);
	exit(1);
}

int main(int argc, char *argv[]) {
	if (argc == 2 && !strcmp("-v", argv[1]))
		die("gbwm v"VERSION);
	else if (argc != 1)
		die("Usage: gbwm [-v]");
	if (!getenv("DISPLAY"))
		die("DISPLAY environment variable not set");
	if (!(dpy = XOpenDisplay(NULL)))
		die("cannot open X11 display (is X running?)");

	signal(SIGCHLD, sigchld);
	if (!(dpy = XOpenDisplay(NULL))) {
		fprintf(stderr, "eowm: cannot open display\n");
		exit(1);
	}
	XSetErrorHandler(xerror_handler);

	sw = DisplayWidth(dpy, DefaultScreen(dpy));
	sh = DisplayHeight(dpy, DefaultScreen(dpy));
	root = RootWindow(dpy, DefaultScreen(dpy));
	Cursor cursor = XCreateFontCursor(dpy, XC_left_ptr);
	XDefineCursor(dpy, root, cursor);

	setup_colors();
	setrootbackground();
	setup_icccm();

	XSelectInput(dpy, root,
		SubstructureRedirectMask | SubstructureNotifyMask |
		EnterWindowMask | LeaveWindowMask | FocusChangeMask);

	grabkeys();

	XEvent ev;
	while (1) {
		XNextEvent(dpy, &ev);
		switch (ev.type) {
			case ButtonPress: buttonpress(&ev); break;
			case ClientMessage: clientmessage(&ev); break;
			case MapRequest: maprequest(&ev); break;
			case UnmapNotify: unmapnotify(&ev); break;
			case DestroyNotify: destroynotify(&ev); break;
			case EnterNotify: enternotify(&ev); break;
			case KeyPress: keypress(&ev); break;
			case Expose: expose(&ev); break;
		}
	}

	XCloseDisplay(dpy);
	return 0;
}
