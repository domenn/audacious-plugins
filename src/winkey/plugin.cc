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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#include <libaudcore/i18n.h>
#include <libaudcore/plugin.h>
#include <libaudcore/runtime.h>


#include <windows.h>
#include <vector>

class WindowsHotkeys : public GeneralPlugin {
public:
    static constexpr PluginInfo info = {
            N_("A Useless Plugin"),
            PACKAGE
    };

    constexpr WindowsHotkeys() : GeneralPlugin(info, true) {}

    bool init();
    void cleanup();
};

EXPORT WindowsHotkeys
aud_plugin_instance;

struct EnumWindowsCallbackArgs {
    EnumWindowsCallbackArgs(DWORD p) : pid(p) {}

    const DWORD pid;
    std::vector<HWND> handles;
};

static BOOL CALLBACK EnumWindowsCallback(HWND hnd, LPARAM lParam) {
    EnumWindowsCallbackArgs *args = (EnumWindowsCallbackArgs *) lParam;

    DWORD windowPID;
    (void) ::GetWindowThreadProcessId(hnd, &windowPID);
    if (windowPID == args->pid) {
        args->handles.push_back(hnd);
    }

    return TRUE;
}

std::vector<HWND> getToplevelWindows() {
    EnumWindowsCallbackArgs args(::GetCurrentProcessId());
    if (::EnumWindows(&EnumWindowsCallback, (LPARAM) &args) == FALSE) {
        // XXX Log error here
        return std::vector<HWND>();
    }
    return args.handles;
}

bool WindowsHotkeys::init() {
    auto wnd = getToplevelWindows();
    AUDWARN ("Got some windows: %d", wnd.size());
    return true;
}

void WindowsHotkeys::cleanup() {
}