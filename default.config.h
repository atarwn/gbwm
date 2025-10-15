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
static const char col_border_normal[] = "#5a3f47";
static const char col_border_focused[] = "#ffb4a9";
static const char col_bg[] = "#1f1a1b";
static const char col_fg[] = "#ebe0e1";
static const char col_sel[] = "#5a3f47";
static const char overlay_font[] = "LiberationMono:size=48";
static const char dmenu_font[]   = "LiberationMono:size=12"; // You can cut-out this line if you don't use dmenu
static const char root_bg[] = "#1f1a1b";

// Commands
static const char *termcmd[] = { "st", NULL };
static const char *menucmd[] = {
    "dmenu_run",
    "-fn", dmenu_font,
    "-nb", col_bg,
    "-nf", col_fg,
    "-sb", col_sel,
    "-sf", col_border_focused,
    NULL
};
// static const char *upbrightness[]   = { "brightnessctl", "set", "10%+", NULL };
// static const char *downbrightness[] = { "brightnessctl", "set", "10%-", NULL };
// static const char *incvol[] = {"wpctl", "set-volume", "@DEFAULT_AUDIO_SINK@", "5%+", NULL};
// static const char *decvol[] = {"wpctl", "set-volume", "@DEFAULT_AUDIO_SINK@", "5%-", NULL};
// static const char *mutevol[] = {"wpctl", "set-mute", "@DEFAULT_AUDIO_SINK@", "toggle", NULL};
static const char *scrotcmd[] = { "/bin/sh", "-c", "scrot ~/Pictures/Screenshots/$(date +%Y.%m.%d_%H.%M).png", NULL };
// static const char *scrotselcmd[] = { "/bin/sh", "-c", "scrot -s ~/Pictures/Screenshots/$(date +%Y.%m.%d_%H.%M).png", NULL };

// Key bindings
static Key keys[] = {
    /* modifier         key              function         argument */
    { MOD,              XK_a,            enter_overlay,   {0} },
    { MOD,              XK_Return,       spawn,           {.v = termcmd} },
    { MOD,              XK_p,            spawn,           {.v = menucmd} },
    // { 0,            XF86XK_MonBrightnessUp,    spawn,     {.v = upbrightness } },
    // { 0,            XF86XK_MonBrightnessDown,  spawn,     {.v = downbrightness } },
    // { 0,            XF86XK_AudioLowerVolume,   spawn,     {.v = decvol} },	
    // { 0,            XF86XK_AudioRaiseVolume,   spawn,     {.v = incvol} },
    // { 0,            XF86XK_AudioMute,          spawn,     {.v = mutevol} },
    { 0,                XK_Print,        spawn,           {.v = scrotcmd} },
    // { 0|ShiftMask,      XK_Print,        spawn,           {.v = scrotselcmd} },
    { MOD,              XK_q,            killclient,      {0} },
    { MOD,              XK_f,            toggle_fullscreen, {0} },
    { MOD,              XK_Tab,          cycle_focus,     {0} },
    { MOD|ShiftMask, XK_Tab,          cycle_focus_backward, {0} },
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
