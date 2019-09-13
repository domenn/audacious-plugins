

#include <src/hotkey/api_hotkey.h>

#include <windows.h>
#include <shobjidl.h>

#include <string>
#include <vector>
#include <libaudcore/runtime.h>
#include <gtk/gtkeditable.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <libaudcore/named_logger_macros.h>
#include <cstring>
#include <thread>
#include <gdk/gdkwin32.h>

#include "windows_key_str_map.h"

constexpr auto W_KEY_ID_PLAY = 18771;
constexpr auto W_KEY_ID_PREV = 18772;
constexpr auto W_KEY_ID_NEXT = 18773;
constexpr auto W_FIRST_GLOBAL_KEY_ID = 18774;

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

void register_global_keys(HWND handle) {
  auto *hotkey = &(plugin_cfg.first);
  auto _id = W_FIRST_GLOBAL_KEY_ID;
  while (hotkey) {
    RegisterHotKey(handle, _id++, hotkey->mask, hotkey->key);
    hotkey = hotkey->next;
  }
}

//void assign(wchar_t *ptr_first_element, const wchar_t *text) {
//  int loc = 0;
//  while (text[loc]) {
//    ptr_first_element[loc] = text[loc];
//    ++loc;
//  }
//  ptr_first_element[loc] = 0;
//}

HRESULT AddThumbarButtons(HWND hwnd) {
  // Define an array of two buttons. These buttons provide images through an
  // image list and also provide tooltips.
  THUMBBUTTONMASK dwMask = THB_BITMAP | THB_TOOLTIP | THB_FLAGS;

  /*
    THUMBBUTTONMASK dwMask;
    UINT iId;
    UINT iBitmap;
    HICON hIcon;
    WCHAR szTip[260];
    THUMBBUTTONFLAGS dwFlags;
    */


  THUMBBUTTON btn[]{{
                        dwMask,
                        W_KEY_ID_PREV,
                        0,
                        nullptr,
                        L"Previous song",
                        THBF_ENABLED
                    },
                    {
                        dwMask,
                        W_KEY_ID_PLAY,
                        0,
                        nullptr,
                        L"Play/pause",
                        THBF_ENABLED
                    },
                    {
                        dwMask,
                        W_KEY_ID_NEXT,
                        0,
                        nullptr,
                        L"Next song",
                        THBF_ENABLED
                    }
  };

  ITaskbarList3 *ptbl;
  HRESULT hr = CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(&ptbl));

  if (SUCCEEDED(hr)) {
    // Declare the image list that contains the button images.
    // hr = ptbl->ThumbBarSetImageList(hwnd, himl);

    if (SUCCEEDED(hr)) {
      // Attach the toolbar to the thumbnail.
      hr = ptbl->ThumbBarAddButtons(hwnd, ARRAYSIZE(btn), btn);
    }
    ptbl->Release();
  }
  return hr;
}

GdkFilterReturn buttons_evts_filter(GdkXEvent *gdk_xevent, GdkEvent *event,
                                    gpointer user_data) {
  auto msg = reinterpret_cast<MSG *>(gdk_xevent);
  if (msg->message == WM_COMMAND) {
    auto buttonId = static_cast<short>(msg->wParam & 0xFFFF);
    AUDDBG("Clicked button with ID: %d", buttonId);
    switch (buttonId) {
      case W_KEY_ID_PLAY:handle_keyevent(EVENT_PAUSE);
        break;
      case W_KEY_ID_NEXT :handle_keyevent(EVENT_NEXT_TRACK);
        break;
      case W_KEY_ID_PREV:handle_keyevent(EVENT_PREV_TRACK);
        break;
    }
    return GDK_FILTER_REMOVE;
  } else if (msg->message == WM_HOTKEY) {
    auto k_id = static_cast<short>(msg->wParam & 0xFFFF);
    L_HOTKEY_FLOW("Global hotkey: " + std::to_string(k_id));
    // Max 20 HKs
    if (k_id >= W_FIRST_GLOBAL_KEY_ID && k_id < W_FIRST_GLOBAL_KEY_ID + 20) {
      auto idx = k_id - W_FIRST_GLOBAL_KEY_ID;
      auto *config = &plugin_cfg.first;
      while (idx--) {
        config = config->next;
      }
      if (handle_keyevent(config->event)) {
        return GDK_FILTER_REMOVE;
      }
    }
  } else if (msg->message == WM_CLOSE) {
    L_AUD_TESTING("W32 close event.");
    // TODO unregister all that we added. If there is toolbar version ... open it.
  }
  return GDK_FILTER_CONTINUE;
}

class Utf16CharArrConverter {
  gchar *converted{nullptr};
 public:
  Utf16CharArrConverter(const wchar_t *const input_w_str) :
      converted(reinterpret_cast<char *>(g_utf16_to_utf8(
          reinterpret_cast<const gunichar2 *> (input_w_str),
          -1, nullptr, nullptr, nullptr))) {
  }
  ~Utf16CharArrConverter() {
    if (converted) {
      g_free(converted);
    }
  };
  operator std::string() {
    return std::string(converted);
  }
};

struct WindowsWindow {
  HWND handle_;
  std::string class_name_;
  std::string win_header_;

  WindowsWindow(const HWND handle, std::string className, std::string winHeader)
      : handle_(handle), class_name_(std::move(className)), win_header_(std::move(winHeader)) {}
};

struct EnumWindowsCallbackArgs {
  wchar_t char_buf[311];
  explicit EnumWindowsCallbackArgs(DWORD p) : pid(p) {}
  const DWORD pid;
  std::vector<WindowsWindow> handles;
  void add_window(HWND pHwnd) {
    auto num_char = GetClassNameW(pHwnd, char_buf, _countof(char_buf));
    if (!num_char) {
      AUDERR("WinApiError (code %ld) getting ClassName. Cannot get windows, hotkey plugin may not work.\n",
             GetLastError());
      return;
    }
    std::string w_class_name = Utf16CharArrConverter(char_buf);
    num_char = GetWindowTextW(pHwnd, char_buf, _countof(char_buf));
    if (!num_char) {
      AUDERR("WinApiError (code %ld) getting WindowHeader. Cannot get windows, hotkey plugin may not work.\n",
             GetLastError());
      char_buf[0] = 0;
      // return;
    }
    handles.emplace_back(pHwnd, std::move(w_class_name),
                         char_buf[0] ?
                         Utf16CharArrConverter(char_buf) : std::string{"[NO_TITLE]"}
    );
  }
};

void get_and_print_app_windows();
static BOOL CALLBACK EnumWindowsCallback(HWND hnd, LPARAM lParam) {
  // Get pointer to created callback object for storing data.
  auto *args = reinterpret_cast<EnumWindowsCallbackArgs *> ( lParam);
  // Callback result are all windows (not only my program). I filter by PID / thread ID.
  DWORD windowPID;
  GetWindowThreadProcessId(hnd, &windowPID);
  // Compare to the one we have stored in our callbaack structure.
  if (windowPID == args->pid) {
    args->add_window(hnd);
  }
  return TRUE;
}
/*
   gdkWindowTempShadow: audacious.exe
   gdkWindowTempShadow: audacious.exe
   gdkWindowTemp: audacious.exe
   gtkstatusicon-observer: [NO_TITLE]
   gdkWindowToplevel: Audacious Settings
   gdkWindowToplevel: OtherListens - JoeRoganLegacy - 00033_Joe_Rogan_Experience_98_-_Daryl_Wright_Brian_Whitaker (2:08:36) - Audacious
   GdkClipboardNotification: [NO_TITLE]
   MSCTFIME UI: MSCTFIME UI
   IME: Default IME
12.09.2019 07:59:07.032 [ERROR] w32_hotkey.cc:220 [add_window]: WinApiError (code 0) getting WindowHeader. Cannot get windows, hotkey plugin may not work.
12.09.2019 07:59:07.033 [ERROR] w32_hotkey.cc:220 [add_window]: WinApiError (code 0) getting WindowHeader. Cannot get windows, hotkey plugin may not work.
12.09.2019 07:59:07.038 [WARNING] w32_hotkey.cc:292 [get_and_print_app_windows]: hotkey_flow - windows:
12.09.2019 07:59:07.038 [WARNING] w32_hotkey.cc:297 [get_and_print_app_windows]: hotkey_flow - Aud has 9 windows:
   gdkWindowTempShadow: audacious.exe
   gdkWindowTempShadow: audacious.exe
   gdkWindowTemp: audacious.exe
   gtkstatusicon-observer: [NO_TITLE]
   gdkWindowToplevel: Audacious Settings
   gdkWindowToplevel: OtherListens - JoeRoganLegacy - 00033_Joe_Rogan_Experience_98_-_Daryl_Wright_Brian_Whitaker (2:08:36) - Audacious
   GdkClipboardNotification: [NO_TITLE]
   MSCTFIME UI: MSCTFIME UI
   IME: Default IME
 */

std::vector<WindowsWindow> get_this_app_windows() {
  // Create object that will hold a result.
  EnumWindowsCallbackArgs args(GetCurrentProcessId());
  // AUDERR("Testing getlasterror: %ld\n.", GetLastError());
  // Call EnumWindows and pass pointer to function and created callback structure.
  if (EnumWindows(&EnumWindowsCallback, (LPARAM) &args) == FALSE) {
    // If the call fails, return empty vector
    AUDERR("WinApiError (code %ld). Cannot get windows, hotkey plugin won't work.\n", GetLastError());
    return std::vector<WindowsWindow>{};
  }
  return args.handles;
}

//void loop_function<>

/**
 * GSourceFunc:
 * @user_data: data passed to the function, set when the source was
 *     created with one of the above functions
 *
 * Specifies the type of function passed to g_timeout_add(),
 * g_timeout_add_full(), g_idle_add(), and g_idle_add_full().
 *
 * Returns: %FALSE if the source should be removed. #G_SOURCE_CONTINUE and
 * #G_SOURCE_REMOVE are more memorable names for the return value.
 */
gboolean window_created_callback(gpointer user_data) {
  L_HOTKEY_FLOW("Window created. Do real stuff.");
  get_and_print_app_windows();

//  std::thread *spawned = new std::thread([]() {
//    while (true) {
//      get_and_print_app_windows();
//      std::this_thread::sleep_for(std::chrono::seconds(2));
//    }
//  });

  //
//  AddThumbarButtons(the_hwnd);
//  auto gdkwin = gdk_win32_handle_table_lookup(the_hwnd);
//  gdk_window_add_filter((GdkWindow *) gdkwin, buttons_evts_filter,
//                        nullptr);
//  register_global_keys(the_hwnd);
  return G_SOURCE_REMOVE;
}
void get_and_print_app_windows() {
  auto wins = get_this_app_windows();
  L_HOTKEY_FLOW("windows: ");
  std::string printer;
  for (auto &w : wins) {
    printer.append("\n   ").append(w.class_name_).append(": ").append(w.win_header_);
  }
  L_HOTKEY_FLOW("Aud has " + std::to_string(wins.size()) + " windows: " + printer);
}

void win_init() {
  L_HOTKEY_FLOW("Win .. important call.");
  g_idle_add(&window_created_callback,
             nullptr);
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