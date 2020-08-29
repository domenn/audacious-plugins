/*
 *  This file is part of audacious-hotkey plugin for audacious
 *
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

#include <gtk/gtk.h>
#include <libaudcore/drct.h>
#include <libaudcore/hook.h>
#include <libaudcore/interface.h>
#include <libaudcore/runtime.h>

const PreferencesWidget GlobalHotkeys::widgets[] = {
    WidgetCustomGTK(GlobalHotkeys::create_gui)};

const PluginPreferences GlobalHotkeys::prefs = {
    {GlobalHotkeys::widgets},
    nullptr, // init
    GlobalHotkeys::ok_callback,
    GlobalHotkeys::destroy_callback};

EXPORT GlobalHotkeys aud_plugin_instance;

const char GlobalHotkeys::about[] =
    N_("Global Hotkey Plugin\n"
       "Control the player with global key combinations or multimedia keys.\n\n"
       "Copyright (C) 2007-2008 Sascha Hlusiak <contact@saschahlusiak.de>\n\n"
       "Contributors include:\n"
       "Copyright (C) 2006-2007 Vladimir Paskov <vlado.paskov@gmail.com>\n"
       "Copyright (C) 2000-2002 Ville Syrjälä <syrjala@sci.fi>,\n"
       " Bryn Davies <curious@ihug.com.au>,\n"
       " Jonathan A. Davis <davis@jdhouse.org>,\n"
       " Jeremy Tan <nsx@nsx.homeip.net>");

/*
 * plugin activated
 */
bool GlobalHotkeys::init()
{
    if (!gtk_init_check(nullptr, nullptr))
    {
        AUDERR("GTK+ initialization failed.\n");
        return false;
    }
    return true;
}

/* handle keys */
gboolean handle_keyevent(EVENT event)
{
    int current_volume, old_volume;
    static int volume_static = 0;
    gboolean mute;

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

    /* mute the playback */
    if (event == EVENT_MUTE)
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

    /* decrease volume */
    if (event == EVENT_VOL_DOWN)
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

    /* increase volume */
    if (event == EVENT_VOL_UP)
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

    /* play */
    if (event == EVENT_PLAY)
    {
        aud_drct_play();
        return true;
    }

    /* pause */
    if (event == EVENT_PAUSE)
    {
        aud_drct_play_pause();
        return true;
    }

    /* stop */
    if (event == EVENT_STOP)
    {
        aud_drct_stop();
        return true;
    }

    /* prev track */
    if (event == EVENT_PREV_TRACK)
    {
        aud_drct_pl_prev();
        return true;
    }

    /* next track */
    if (event == EVENT_NEXT_TRACK)
    {
        aud_drct_pl_next();
        return true;
    }

    /* forward */
    if (event == EVENT_FORWARD)
    {
        aud_drct_seek(aud_drct_get_time() + aud_get_int("step_size") * 1000);
        return true;
    }

    /* backward */
    if (event == EVENT_BACKWARD)
    {
        aud_drct_seek(aud_drct_get_time() - aud_get_int("step_size") * 1000);
        return true;
    }

    /* Open Jump-To-File dialog */
    if (event == EVENT_JUMP_TO_FILE && !aud_get_headless_mode())
    {
        aud_ui_show_jump_to_song();
        return true;
    }

    /* Toggle Windows */
    if (event == EVENT_TOGGLE_WIN && !aud_get_headless_mode())
    {
        aud_ui_show(!aud_ui_is_shown());
        return true;
    }

    /* Show OSD through AOSD plugin*/
    if (event == EVENT_SHOW_AOSD)
    {
        hook_call("aosd toggle", nullptr);
        return true;
    }

    if (event == EVENT_TOGGLE_REPEAT)
    {
        aud_toggle_bool("repeat");
        return true;
    }

    if (event == EVENT_TOGGLE_SHUFFLE)
    {
        aud_toggle_bool("shuffle");
        return true;
    }

    if (event == EVENT_TOGGLE_STOP)
    {
        aud_toggle_bool("stop_after_current_song");
        return true;
    }

    if (event == EVENT_RAISE)
    {
        aud_ui_show(true);
        return true;
    }

    return false;
}

/* save plugin configuration */
void save_config() {}

void GlobalHotkeys::cleanup() {}
void GlobalHotkeys::ok_callback() {}
void GlobalHotkeys::destroy_callback() {}
