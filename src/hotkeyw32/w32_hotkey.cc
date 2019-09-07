

#include <src/hotkey/api_hotkey.h>

#include <windows.h>

#include <string>
#include <libaudcore/runtime.h>
#include <gtk/gtkeditable.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <libaudcore/named_logger_macros.h>
#include <cstring>

#include "windows_key_str_map.h"

template<typename T>
std::string VirtualKeyCodeToStringMethod2(T virtualKey) {
  WCHAR name[128];
  static auto key_layout = GetKeyboardLayout(0);
  UINT scanCode = MapVirtualKeyEx(virtualKey, MAPVK_VK_TO_VSC, key_layout);
  LONG lParamValue = (scanCode << 16);
  int result = GetKeyNameTextW(lParamValue, name, 1024);
  if (result > 0) {
    auto *windows_cmd = reinterpret_cast<char *>(g_utf16_to_utf8(
        reinterpret_cast<const gunichar2 *> (name),
        -1, nullptr, nullptr, nullptr));
    std::string returning(windows_cmd);
    g_free(windows_cmd);
    return returning;
  } else {
    return {};
  }
}

void Hotkey::add_hotkey(HotkeyConfiguration **pphotkey, Hotkey::OS_KeySym keysym, int mask, int type, EVENT event) {
  L_HOTKEY_FLOW("Win call.");
  HotkeyConfiguration *photkey;
  if (keysym == 0) return;
  if (pphotkey == nullptr) return;
  photkey = *pphotkey;
  if (photkey == nullptr) return;
  // keycode = XKeysymToKeycode(GDK_DISPLAY_XDISPLAY(gdk_display_get_default()), keysym);
  // if (keycode == 0) return;
  if (photkey->key) {
    photkey->next = g_new(HotkeyConfiguration, 1);
    photkey = photkey->next;
    *pphotkey = photkey;
    photkey->next = nullptr;
  }
  photkey->key = (int) 42;
  photkey->mask = mask;
  photkey->event = event;
  photkey->type = type;
}

void Hotkey::key_to_string(int key, char **out_keytext) {
  // Special handling for most OEM keys - they may depend on language or keyboard?
  switch (key) {
    case VK_OEM_NEC_EQUAL:
    // case VK_OEM_FJ_JISHO: (note: same as EQUAL)
    case VK_OEM_FJ_MASSHOU:
    case VK_OEM_FJ_TOUROKU:
    case VK_OEM_FJ_LOYA:
    case VK_OEM_FJ_ROYA:
    case VK_OEM_1:
    case VK_OEM_2:
    case VK_OEM_3:
    case VK_OEM_4:
    case VK_OEM_5:
    case VK_OEM_6:
    case VK_OEM_7:
    case VK_OEM_8:
    case VK_OEM_AX:
    case VK_OEM_102:
    case VK_OEM_RESET:
    case VK_OEM_JUMP:
    case VK_OEM_PA1:
    case VK_OEM_PA2:
    case VK_OEM_PA3:
    case VK_OEM_WSCTRL:
    case VK_OEM_CUSEL:
    case VK_OEM_ATTN:
    case VK_OEM_FINISH:
    case VK_OEM_COPY:
    case VK_OEM_AUTO:
    case VK_OEM_ENLW:
    case VK_OEM_BACKTAB:
    case VK_OEM_CLEAR: {
      auto win_str = VirtualKeyCodeToStringMethod2(key);
      if (!win_str.empty()) {
        *out_keytext = g_strdup(win_str.c_str());
        return;
      }
    }
  }
  *out_keytext = g_strdup(WIN_VK_NAMES[key]);
}
