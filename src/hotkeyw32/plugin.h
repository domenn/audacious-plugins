#ifndef _PLUGIN_H_INCLUDED_
#define _PLUGIN_H_INCLUDED_

#include <glib.h>
#include <libaudcore/i18n.h>
#include <libaudcore/plugin.h>
#include <libaudcore/preferences.h>

#define TYPE_KEY 0
#define TYPE_MOUSE 1


class GlobalHotkeys : public GeneralPlugin
{
public:
    static const PluginPreferences prefs;
    static const PreferencesWidget widgets[];
    static const char about[];
    static void ok_callback();
    static void destroy_callback();
    static void * create_gui();

    static constexpr PluginInfo info = {N_("Global Hotkeys"), PACKAGE, about,
                                        &prefs, PluginGLibOnly};

    constexpr GlobalHotkeys() : GeneralPlugin(info, false) {}

    bool init() override;
    void cleanup() override;
};


typedef enum {
    EVENT_PREV_TRACK = 0,
    EVENT_PLAY,
    EVENT_PAUSE,
    EVENT_STOP,
    EVENT_NEXT_TRACK,

    EVENT_FORWARD,
    EVENT_BACKWARD,
    EVENT_MUTE,
    EVENT_VOL_UP,
    EVENT_VOL_DOWN,
    EVENT_JUMP_TO_FILE,
    EVENT_TOGGLE_WIN,
    EVENT_SHOW_AOSD,

    EVENT_TOGGLE_REPEAT,
    EVENT_TOGGLE_SHUFFLE,
    EVENT_TOGGLE_STOP,

    EVENT_RAISE,

    EVENT_MAX
} EVENT;


void load_config ();
void save_config ();
gboolean handle_keyevent(EVENT event);

#endif
