#ifndef _PLUGIN_H_INCLUDED_
#define _PLUGIN_H_INCLUDED_

#include <QtCore/QList>
#include <QtCore>
#include <functional>

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

    HotkeyConfiguration(QHotkey * qHotkey, Event event);

    static void clear_configured_hotkeys();

    static const QList<HotkeyConfiguration> & get_configured_hotkeys()
    {
        return hotkeys_list;
    }

    static void replace(std::function<void(QList<HotkeyConfiguration> &)> p_function);
private:
    static QList<HotkeyConfiguration> hotkeys_list;
};

void load_config();
void save_config();

bool handle_keyevent(Event event);

void grab_keys();
void ungrab_keys();

void add_hotkey(QList<HotkeyConfiguration> & hotkeys_list, Qt::Key key,
                QFlags<Qt::KeyboardModifier> modifiers, Event key_event);
void add_hotkey(QList<HotkeyConfiguration> & hotkeys_list, QKeySequence seq,
                Event key_event);

} /* namespace GlobalHotkeys */

#endif
