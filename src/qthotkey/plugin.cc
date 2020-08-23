/*
 *  This file is part of audacious-hotkey plugin for audacious
 *
 *  Copyright (C) 2020 i.Dark_Templar <darktemplar@dark-templar-archives.net>
 *  Copyright (c) 2007 - 2008  Sascha Hlusiak <contact@saschahlusiak.de>
 *  Name: plugin.c
 *  Description: plugin.c
 *
 *  Part of this code is from itouch-ctrl plugin.
 *  Authors of itouch-ctrl are listed below:
 *
 *  Copyright (c) 2006 - 2007 Vladimir Paskov <vlado.paskov@gmail.com>
 *
 *  Part of this code are from xmms-itouch plugin.
 *  Authors of xmms-itouch are listed below:
 *
 *  Copyright (C) 2000-2002 Ville Syrjälä <syrjala@sci.fi>
 *                         Bryn Davies <curious@ihug.com.au>
 *                         Jonathan A. Davis <davis@jdhouse.org>
 *                         Jeremy Tan <nsx@nsx.homeip.net>
 *
 *  audacious-hotkey is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  audacious-hotkey is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with audacious-hotkey; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA.
 */

#include "plugin.h"
#include "gui.h"

#include "qhotkey.h"
#include <QtCore/QAbstractNativeEventFilter>
#include <QtCore/QCoreApplication>
#include <QtCore/QString>
#include <QtCore/QTimer>

#include <libaudcore/drct.h>
#include <libaudcore/hook.h>
#include <libaudcore/i18n.h>
#include <libaudcore/interface.h>
#include <libaudcore/plugin.h>
#include <libaudcore/runtime.h>

#include <libaudqt/libaudqt.h>

#include <stdlib.h>

namespace GlobalHotkeys
{

class GlobalHotkeys : public GeneralPlugin, public QAbstractNativeEventFilter
{
public:
    static const char about[];

    static constexpr PluginInfo info = {N_("Global Hotkeys"), PACKAGE, about,
                                        &hotkey_prefs, PluginQtOnly};

    GlobalHotkeys();

    bool init() override;
    void cleanup() override;

private:
    bool nativeEventFilter(const QByteArray & eventType, void * message,
                           long * result) override;
};

constexpr PluginInfo GlobalHotkeys::info;

/* global vars */
static PluginConfig plugin_cfg;

static int grabbed = 0;
static unsigned int numlock_mask = 0;
static unsigned int scrolllock_mask = 0;
static unsigned int capslock_mask = 0;

const char GlobalHotkeys::about[] =
    N_("Global Hotkey Plugin\n"
       "Control the player with global key combinations or multimedia keys.\n\n"
       "Copyright (C) 2020 i.Dark_Templar "
       "<darktemplar@dark-templar-archives.net>\n"
       "Copyright (C) 2007-2008 Sascha Hlusiak <contact@saschahlusiak.de>\n\n"
       "Contributors include:\n"
       "Copyright (C) 2006-2007 Vladimir Paskov <vlado.paskov@gmail.com>\n"
       "Copyright (C) 2000-2002 Ville Syrjälä <syrjala@sci.fi>,\n"
       " Bryn Davies <curious@ihug.com.au>,\n"
       " Jonathan A. Davis <davis@jdhouse.org>,\n"
       " Jeremy Tan <nsx@nsx.homeip.net>");

PluginConfig * get_config() { return &plugin_cfg; }

/* handle keys */
bool handle_keyevent(Event event)
{
    int current_volume, old_volume;
    static int volume_static = 0;
    bool mute;
    AUDINFO("Handling the %s", get_event_name(event));
    /* get current volume */
    current_volume = aud_drct_get_volume_main();
    old_volume = current_volume;

    if (current_volume)
    {
        /* volume is not mute */
        mute = false;
    }
    else
    {
        /* volume is mute */
        mute = true;
    }

    switch (event)
    {
    /* mute the playback */
    case Event::Mute:
    {
        if (!mute)
        {
            volume_static = current_volume;
            aud_drct_set_volume_main(0);
            mute = true;
        }
        else
        {
            aud_drct_set_volume_main(volume_static);
            mute = false;
        }

        return true;
    }
    break;

    /* decrease volume */
    case Event::VolumeDown:
    {
        if (mute)
        {
            current_volume = old_volume;
            old_volume = 0;
            mute = false;
        }

        if ((current_volume -= aud_get_int("volume_delta")) < 0)
        {
            current_volume = 0;
        }

        if (current_volume != old_volume)
        {
            aud_drct_set_volume_main(current_volume);
        }

        old_volume = current_volume;
        return true;
    }
    break;

    /* increase volume */
    case Event::VolumeUp:
    {
        if (mute)
        {
            current_volume = old_volume;
            old_volume = 0;
            mute = false;
        }

        if ((current_volume += aud_get_int("volume_delta")) > 100)
        {
            current_volume = 100;
        }

        if (current_volume != old_volume)
        {
            aud_drct_set_volume_main(current_volume);
        }

        old_volume = current_volume;
        return true;
    }
    break;

    /* play */
    case Event::Play:
    {
        aud_drct_play();
        return true;
    }
    break;

    /* pause */
    case Event::Pause:
    {
        aud_drct_play_pause();
        return true;
    }
    break;

    /* stop */
    case Event::Stop:
    {
        aud_drct_stop();
        return true;
    }
    break;

    /* prev track */
    case Event::PrevTrack:
    {
        aud_drct_pl_prev();
        return true;
    }
    break;

    /* next track */
    case Event::NextTrack:
    {
        aud_drct_pl_next();
        return true;
    }
    break;

    /* forward */
    case Event::Forward:
    {
        aud_drct_seek(aud_drct_get_time() + aud_get_int("step_size") * 1000);
        return true;
    }
    break;

    /* backward */
    case Event::Backward:
    {
        aud_drct_seek(aud_drct_get_time() - aud_get_int("step_size") * 1000);
        return true;
    }
    break;

    /* Open Jump-To-File dialog */
    case Event::JumpToFile:
        if (!aud_get_headless_mode())
        {
            aud_ui_show_jump_to_song();
            return true;
        }
        break;

    /* Toggle Windows */
    case Event::ToggleWindow:
        if (!aud_get_headless_mode())
        {
            aud_ui_show(!aud_ui_is_shown());
            return true;
        }
        break;

    /* Show OSD through AOSD plugin*/
    case Event::ShowAOSD:
    {
        hook_call("aosd toggle", nullptr);
        return true;
    }
    break;

    case Event::ToggleRepeat:
    {
        aud_toggle_bool("repeat");
        return true;
    }
    break;

    case Event::ToggleShuffle:
    {
        aud_toggle_bool("shuffle");
        return true;
    }
    break;

    case Event::ToggleStop:
    {
        aud_toggle_bool("stop_after_current_song");
        return true;
    }
    break;

    case Event::Raise:
    {
        aud_ui_show(true);
        return true;
    }
    break;

    /* silence warning */
    case Event::Max:
        break;
    }

    return false;
}

void add_hotkey(QList<HotkeyConfiguration> & hotkeys_list, Qt::Key key,
                Qt::KeyboardModifiers modifiers, Event key_event)
{
    hotkeys_list.push_back({new QHotkey(key, modifiers), key_event});
}

void add_hotkey(QList<HotkeyConfiguration> & hotkeys_list, QKeySequence seq,
                Event key_event)
{
    hotkeys_list.push_back({new QHotkey(), key_event});

    hotkeys_list.back().q_hotkey->setShortcut(std::move(seq));
}

void load_defaults()
{
    add_hotkey(plugin_cfg.hotkeys_list, Qt::Key_P,
               Qt::KeyboardModifier::AltModifier |
                   Qt::KeyboardModifier::ShiftModifier,
               Event::PrevTrack);
    //	add_hotkey(plugin_cfg.hotkeys_list, XF86XK_AudioPlay, 0, Event::Play);
    //	add_hotkey(plugin_cfg.hotkeys_list, XF86XK_AudioPause, 0, Event::Pause);
    //	add_hotkey(plugin_cfg.hotkeys_list, XF86XK_AudioStop, 0, Event::Stop);
    //	add_hotkey(plugin_cfg.hotkeys_list, XF86XK_AudioNext, 0,
    //Event::NextTrack); 	add_hotkey(plugin_cfg.hotkeys_list, XF86XK_AudioMute,
    //0, Event::Mute); 	add_hotkey(plugin_cfg.hotkeys_list,
    //XF86XK_AudioRaiseVolume, 0, Event::VolumeUp);
    //	add_hotkey(plugin_cfg.hotkeys_list, XF86XK_AudioLowerVolume, 0,
    //Event::VolumeDown);
}

/* load plugin configuration */
void load_config()
{
    int max = aud_get_int("globalHotkey", "NumHotkeys");
    if (max == 0)
    {
        load_defaults();
    }
    else
    {
        for (int i = 0; i < max; ++i)
        {
            HotkeyConfiguration hotkey;
            hotkey.q_hotkey = new QHotkey(
                static_cast<Qt::Key>(aud_get_int(
                    "globalHotkey", QString::fromLatin1("Hotkey_%1_key")
                                        .arg(i)
                                        .toLocal8Bit()
                                        .data())),
                static_cast<Qt::KeyboardModifiers>(aud_get_int(
                    "globalHotkey", QString::fromLatin1("Hotkey_%1_mask")
                                        .arg(i)
                                        .toLocal8Bit()
                                        .data())));

            hotkey.event = static_cast<Event>(aud_get_int(
                "globalHotkey", QString::fromLatin1("Hotkey_%1_event")
                                    .arg(i)
                                    .toLocal8Bit()
                                    .data()));

            plugin_cfg.hotkeys_list.push_back(hotkey);
        }
    }
}

/* save plugin configuration */
void save_config()
{
    int max = 0;

    for (const auto & hotkey : plugin_cfg.hotkeys_list)
    {
        if (hotkey.q_hotkey != nullptr)
        {
            aud_set_int("globalHotkey",
                        QString::fromLatin1("Hotkey_%1_key")
                            .arg(max)
                            .toLocal8Bit()
                            .data(),
                        hotkey.q_hotkey->keyCode());
            aud_set_int("globalHotkey",
                        QString::fromLatin1("Hotkey_%1_mask")
                            .arg(max)
                            .toLocal8Bit()
                            .data(),
                        hotkey.q_hotkey->modifiers());
            aud_set_int("globalHotkey",
                        QString::fromLatin1("Hotkey_%1_event")
                            .arg(max)
                            .toLocal8Bit()
                            .data(),
                        static_cast<int>(hotkey.event));
            ++max;
        }
    }

    aud_set_int("globalHotkey", "NumHotkeys", max);
}

std::vector<QMetaObject::Connection> connections;

void grab_keys()
{
    PluginConfig * plugin_cfg = get_config();

    for (auto & hk : plugin_cfg->hotkeys_list)
    {
        connections.emplace_back(QObject::connect(
            hk.q_hotkey, &QHotkey::activated,
            [capture_event{hk.event}]() { handle_keyevent(capture_event); }));
        AUDINFO("Registering %s and %s to %s",
                QKeySequence(hk.q_hotkey->keyCode())
                    .toString()
                    .toStdString()
                    .c_str(),
                QKeySequence(hk.q_hotkey->modifiers())
                    .toString()
                    .toStdString()
                    .c_str(),
                get_event_name(hk.event));
        hk.q_hotkey->setRegistered(true);
    }
}
void ungrab_keys()
{
    for (auto & c : connections)
    {
        QObject::disconnect(c);
    }
    connections.clear();
    PluginConfig * plugin_cfg = get_config();

    for (auto & hk : plugin_cfg->hotkeys_list)
    {
        hk.q_hotkey->setRegistered(false);
    }
}

GlobalHotkeys::GlobalHotkeys() : GeneralPlugin(info, false) {}

bool GlobalHotkeys::init()
{
    audqt::init();

    load_config();
    grab_keys();
    QCoreApplication::instance()->installNativeEventFilter(this);

    return true;
}

void GlobalHotkeys::cleanup()
{
    QCoreApplication::instance()->removeNativeEventFilter(this);
    ungrab_keys();
    plugin_cfg.hotkeys_list.clear();

    audqt::cleanup();
}

bool GlobalHotkeys::nativeEventFilter(const QByteArray & eventType,
                                      void * message, long * result)
{
    //	Q_UNUSED(eventType);
    //	Q_UNUSED(result);
    //
    //	if (!grabbed)
    //	{
    //		return false;
    //	}
    //
    //	xcb_generic_event_t *e = static_cast<xcb_generic_event_t*>(message);
    //
    //	if (e->response_type != XCB_KEY_PRESS)
    //	{
    //		return false;
    //	}
    //
    //	xcb_key_press_event_t *ke = (xcb_key_press_event_t*)e;
    //
    //	for (const auto &hotkey: plugin_cfg.hotkeys_list)
    //	{
    //		if ((hotkey.key == ke->detail)
    //			&& (hotkey.mask == (ke->state & ~(scrolllock_mask | numlock_mask |
    //capslock_mask))))
    //		{
    //			if (handle_keyevent(hotkey.event))
    //			{
    //				return true;
    //			}
    //		}
    //	}

    return false;
}

} /* namespace GlobalHotkeys */

EXPORT GlobalHotkeys::GlobalHotkeys aud_plugin_instance;
