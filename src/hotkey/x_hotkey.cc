#include <X11/X.h>
#include <X11/XKBlib.h>
#include <gdk/gdkx.h>
#include <gtk/gtkentry.h>

#include "api_hotkey.h"

void Hotkey::add_hotkey(HotkeyConfiguration ** pphotkey, OS_KeySym keysym,
                        int mask, int type, EVENT event)
{
    KeyCode keycode;
    HotkeyConfiguration * photkey;
    if (keysym == 0)
        return;
    if (pphotkey == nullptr)
        return;
    photkey = *pphotkey;
    if (photkey == nullptr)
        return;
    keycode = XKeysymToKeycode(GDK_DISPLAY_XDISPLAY(gdk_display_get_default()),
                               keysym);
    if (keycode == 0)
        return;
    if (photkey->key)
    {
        photkey->next = g_new(HotkeyConfiguration, 1);
        photkey = photkey->next;
        *pphotkey = photkey;
        photkey->next = nullptr;
    }
    photkey->key = (int)keycode;
    photkey->mask = mask;
    photkey->event = event;
    photkey->type = type;
}

void Hotkey::key_to_string(int key, char ** out_keytext)
{
    KeySym keysym;
    keysym = XkbKeycodeToKeysym(GDK_DISPLAY_XDISPLAY(gdk_display_get_default()),
                                key, 0, 0);
    if (keysym == 0 || keysym == NoSymbol)
    {
        *out_keytext = g_strdup_printf("#%d", key);
    }
    else
    {
        *out_keytext = g_strdup(XKeysymToString(keysym));
    }
}

void Hotkey::create_human_readable_keytext(const char* const keytext, int key, int mask, char&* text)
{
    static const constexpr unsigned int modifiers[] = {
        HK_CONTROL_MASK, HK_SHIFT_MASK, HK_MOD1_ALT_MASK, HK_MOD2_MASK,
        HK_MOD3_MASK,    HK_MOD4_MASK,  HK_MOD5_MASK};
    static const constexpr char * const modifier_string[] = {
        "Control", "Shift", "Alt", "Mod2", "Mod3", "Super", "Mod5"};
    const char * strings[9];
    int i, j;
    for (i = 0, j = 0; j < 7; j++)
    {
        if (mask & modifiers[j])
            strings[i++] = modifier_string[j];
    }
    if (key != 0)
        strings[i++] = keytext;
    strings[i] = nullptr;

    text = g_strjoinv(" + ", (char **)strings);
}