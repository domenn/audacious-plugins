


#include "../hotkey/grab.h"

#include <gtk/gtk.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <libaudcore/named_logger_macros.h>

#include "../hotkey/plugin.h"

void grab_keys (){
  L_HOTKEY_FLOW("w_grab: grab_keys");
}
void ungrab_keys (){
  L_HOTKEY_FLOW("w_grab: ungrab_keys");
}