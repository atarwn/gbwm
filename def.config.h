/* gbwm config.h - example configuration with multi-monitor support */

#define VERSION "1.1"

/* appearance */
static const unsigned int border_width = 2;
static const unsigned int padding = 10;

/* colors */
static const char col_background[]    = "#1e1e2e";
static const char col_foreground[]    = "#cdd6f4";
static const char col_selection[]     = "#89b4fa";
static const char col_border_normal[] = "#313244";
static const char col_border_focused[]= "#89b4fa";
static const char root_bg[]           = "#1e1e2e";

/* font for overlay */
static const char overlay_font[] = "monospace:size=24:antialias=true";

/* grid layout */
#define GRID_ROWS 3
#define GRID_COLS 4

/* grid character labels (row-major order) */
static const char grid_chars[GRID_ROWS][GRID_COLS] = {
	{'q', 'w', 'e', 'r'},
	{'a', 's', 'd', 'f'},
	{'z', 'x', 'c', 'v'}
};

/* modifier key - Mod4Mask is Super/Windows key */
#define MODKEY Mod4Mask

/* helper macros for launching applications */
#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }

/* commands */
// static const char *termcmd[]  = { "st", NULL };
// static const char *menucmd[]  = { "dmenu_run", NULL };
static const char *termcmd[]  = { "alacritty", NULL };
static const char *menucmd[]  = { "rofi", "-show", "drun", NULL };

/* key bindings */
static Key keys[] = {
	/* modifier          key              function           argument */
	
	/* application launching */
	{ MODKEY,            XK_Return,       spawn,             {.v = termcmd } },
	{ MODKEY,            XK_p,            spawn,             {.v = menucmd } },
	
	/* window management */
	{ MODKEY,            XK_q,            killclient,        {0} },
	{ MODKEY,            XK_f,            toggle_fullscreen, {0} },
	{ MODKEY,            XK_space,        enter_overlay,     {0} },
	
	/* focus cycling */
	{ MODKEY,            XK_j,            cycle_focus,       {0} },
	{ MODKEY,            XK_k,            cycle_focus_backward, {0} },
	{ MODKEY,            XK_Tab,          cycle_focus,       {0} },
	{ MODKEY|ShiftMask,  XK_Tab,          cycle_focus_backward, {0} },
	
	/* workspace switching */
	{ MODKEY,            XK_1,            switchws,          {.i = 0} },
	{ MODKEY,            XK_2,            switchws,          {.i = 1} },
	{ MODKEY,            XK_3,            switchws,          {.i = 2} },
	{ MODKEY,            XK_4,            switchws,          {.i = 3} },
	{ MODKEY,            XK_5,            switchws,          {.i = 4} },
	{ MODKEY,            XK_6,            switchws,          {.i = 5} },
	{ MODKEY,            XK_7,            switchws,          {.i = 6} },
	{ MODKEY,            XK_8,            switchws,          {.i = 7} },
	{ MODKEY,            XK_9,            switchws,          {.i = 8} },
	
	/* move window to workspace */
	{ MODKEY|ShiftMask,  XK_1,            movewin_to_ws,     {.i = 0} },
	{ MODKEY|ShiftMask,  XK_2,            movewin_to_ws,     {.i = 1} },
	{ MODKEY|ShiftMask,  XK_3,            movewin_to_ws,     {.i = 2} },
	{ MODKEY|ShiftMask,  XK_4,            movewin_to_ws,     {.i = 3} },
	{ MODKEY|ShiftMask,  XK_5,            movewin_to_ws,     {.i = 4} },
	{ MODKEY|ShiftMask,  XK_6,            movewin_to_ws,     {.i = 5} },
	{ MODKEY|ShiftMask,  XK_7,            movewin_to_ws,     {.i = 6} },
	{ MODKEY|ShiftMask,  XK_8,            movewin_to_ws,     {.i = 7} },
	{ MODKEY|ShiftMask,  XK_9,            movewin_to_ws,     {.i = 8} },
	
	/* multi-monitor support */
	{ MODKEY,            XK_comma,        focus_monitor,     {.i = -1} },  // focus previous monitor
	{ MODKEY,            XK_period,       focus_monitor,     {.i = +1} },  // focus next monitor
	{ MODKEY|ShiftMask,  XK_comma,        movewin_to_monitor, {.i = -1} }, // move window to previous monitor
	{ MODKEY|ShiftMask,  XK_period,       movewin_to_monitor, {.i = +1} }, // move window to next monitor
	
	/* quit */
	{ MODKEY|ShiftMask,  XK_BackSpace,    quit,              {0} },
};