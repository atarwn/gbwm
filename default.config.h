#ifndef CONFIG_H
#define CONFIG_H

#include <X11/Xlib.h>
#include <X11/keysym.h>

// Modifier
#define MOD Mod4Mask

// Grid layout
#define GRID_ROWS 4
#define GRID_COLS 7
static const char grid_chars[GRID_ROWS][GRID_COLS+1] = {
    "1234567",
    "qwertyu",
    "asdfghj",
    "zxcvbnm"
};

// Configuration
static const int padding = 8;
static const int border_width = 2;
static const char *col_border_normal = "#444444";
static const char *col_border_focused = "#4a90e2";
static const char *col_bg = "#000000";
static const char *col_fg = "#ffffff";
static const char *col_sel = "#4a90e2";
static const char *overlay_font = "LiberationMono:size=48";
static const char *root_bg = "#000000";

// Commands
static const char *termcmd[] = { "st", NULL };
static const char *menucmd[] = { "dmenu_run", NULL };
static const char *scrotcmd[] = { "scrot", NULL };

// Key bindings
static Key keys[] = {
    /* modifier         key              function         argument */
    { MOD,              XK_a,            enter_overlay,   {0} },
    { MOD,              XK_Return,       spawn,           {.v = termcmd} },
    { MOD,              XK_p,            spawn,           {.v = menucmd} },
    { 0,                XK_Print,        spawn,           {.v = scrotcmd} },
    { MOD,              XK_q,            killclient,      {0} },
    { MOD,              XK_f,            toggle_fullscreen, {0} },
    { MOD,              XK_Tab,          cycle_focus,     {0} },
    { MOD|ShiftMask,    XK_q,            quit,            {0} },
    
    // Workspaces
    { MOD,              XK_1,            switchws,        {.i = 0} },
    { MOD,              XK_2,            switchws,        {.i = 1} },
    { MOD,              XK_3,            switchws,        {.i = 2} },
    { MOD,              XK_4,            switchws,        {.i = 3} },
    { MOD,              XK_5,            switchws,        {.i = 4} },
    { MOD,              XK_6,            switchws,        {.i = 5} },
    { MOD,              XK_7,            switchws,        {.i = 6} },
    { MOD,              XK_8,            switchws,        {.i = 7} },
    { MOD,              XK_9,            switchws,        {.i = 8} },
    
    // Move window to workspace
    { MOD|ShiftMask,    XK_1,            movewin_to_ws,   {.i = 0} },
    { MOD|ShiftMask,    XK_2,            movewin_to_ws,   {.i = 1} },
    { MOD|ShiftMask,    XK_3,            movewin_to_ws,   {.i = 2} },
    { MOD|ShiftMask,    XK_4,            movewin_to_ws,   {.i = 3} },
    { MOD|ShiftMask,    XK_5,            movewin_to_ws,   {.i = 4} },
    { MOD|ShiftMask,    XK_6,            movewin_to_ws,   {.i = 5} },
    { MOD|ShiftMask,    XK_7,            movewin_to_ws,   {.i = 6} },
    { MOD|ShiftMask,    XK_8,            movewin_to_ws,   {.i = 7} },
    { MOD|ShiftMask,    XK_9,            movewin_to_ws,   {.i = 8} },
};

#endif /* CONFIG_H */
