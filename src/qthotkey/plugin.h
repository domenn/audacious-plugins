#ifndef _PLUGIN_H_INCLUDED_
#define _PLUGIN_H_INCLUDED_

#include <QtCore/QList>
#include <QtCore>

class QHotkey;

namespace GlobalHotkeys
{

/*
 * Values in this enum must not be skipped and must start with 0,
 * otherwise event_desc in gui.cc and it's use must be updated.
 * All values except for Event::Max must be present in event_desc map.
 */
enum class Event
{
    PrevTrack = 0,
    Play,
    Pause,
    Stop,
    NextTrack,

    Forward,
    Backward,
    Mute,
    VolumeUp,
    VolumeDown,
    JumpToFile,
    ToggleWindow,
    ShowAOSD,

    ToggleRepeat,
    ToggleShuffle,
    ToggleStop,

    Raise,

    Max
};

struct HotkeyConfiguration
{
    QHotkey * q_hotkey;
    Event event;
};

struct PluginConfig
{
    /* keyboard */
    QList<HotkeyConfiguration> hotkeys_list;
};

void load_config();
void save_config();
PluginConfig * get_config();
bool handle_keyevent(Event event);

void grab_keys();
void ungrab_keys();

void add_hotkey(QList<HotkeyConfiguration> & hotkeys_list, Qt::Key key,
                QFlags<Qt::KeyboardModifier> modifiers, Event key_event);
void add_hotkey(QList<HotkeyConfiguration> & hotkeys_list, QKeySequence seq,
                Event key_event);

} /* namespace GlobalHotkeys */

#endif
