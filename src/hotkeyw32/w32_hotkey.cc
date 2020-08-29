

#include <src/hotkey/api_hotkey.h>

#include <windows.h>
#include <shobjidl.h>

#include <string>
#include <vector>
#include <libaudcore/runtime.h>
#include <gtk/gtkeditable.h>
#include <glib.h>
#include <cstring>
#include <thread>
#include <gdk/gdkwin32.h>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <libaudcore/hook.h>
#include <libaudcore/i18n.h>
#include <src/hotkey/grab.h>

#include "w32_common_headers.h"
#include "windows_key_str_map.h"

bool window_hidden{false};
/**
 * Handle to window that has thumbbar buttons associated. Nullptr if it is not on display.
 */
HWND h_main_window{nullptr};
HWND handle_hotkey_association{nullptr};
GdkWindow *w32_add_filter_ptr{nullptr};
bool aud_evts_hooked{false};
auto win_registration_hotkey_id = W_FIRST_GLOBAL_KEY_ID;

GdkFilterReturn w32_evts_filter(GdkXEvent *gdk_xevent, GdkEvent *event, gpointer user_data);
void create_and_hook_thumbbar();

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

std::string format_windows_error(
    const std::string &str_lpszFunction) {
  wchar_t *errorText = nullptr;
  auto e = GetLastError();
  // https://stackoverflow.com/questions/455434/how-should-i-use-formatmessage-properly-in-c
  FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
                 nullptr,
                 e,
                 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                 reinterpret_cast<LPWSTR>(&errorText),  // output
                 0, // minimum size for output buffer
                 NULL);   // arguments - see note

  if (NULL != errorText) {
    Utf16CharArrConverter cvt(errorText);
    LocalFree(errorText);
    errorText = NULL;
    return str_lpszFunction + " exited with code " + std::to_string(e) + " : " + std::string(cvt);
  }
  return {};
}

struct MainWindowSearchFilterData {
  gpointer window_;
  void *function_ptr_;

  MainWindowSearchFilterData(gpointer window, void *functionPtr) : window_(window), function_ptr_(functionPtr) {
    //   // L_HOTKEY_FLOW("Constructor of weird struct.");
  }

  MainWindowSearchFilterData(const MainWindowSearchFilterData &) = delete;
  MainWindowSearchFilterData(MainWindowSearchFilterData &&) = delete;
  MainWindowSearchFilterData &operator=(const MainWindowSearchFilterData &) = delete;
  MainWindowSearchFilterData &operator=(MainWindowSearchFilterData &&) = delete;
  ~MainWindowSearchFilterData() = default;

};

struct WindowsWindow {
  HWND handle_{};
  std::string class_name_{};
  std::string win_header_{};

  WindowsWindow() = default;
  ~WindowsWindow() = default;
  WindowsWindow(const WindowsWindow &right) = delete;
  WindowsWindow &operator=(const WindowsWindow &right) = delete;
  WindowsWindow &operator=(WindowsWindow &&right) = default;
  WindowsWindow(WindowsWindow &&right) = default;

  WindowsWindow(HWND handle, std::string className, std::string winHeader)
      : handle_(handle), class_name_(std::move(className)), win_header_(std::move(winHeader)) {}

  static std::string translated_title() {
    // get "%s - <translatedAudacious>"
    std::string org = N_("%s - Audacious");
    // erase %s
    org.erase(0, 2);
    return org;
  }

  bool is_main_window() const {
    return class_name_ == "gdkWindowToplevel"
        && [](const std::string &title) {
          return title == N_("Audacious")
              || title.find(translated_title()) != std::string::npos
              || title == N_("Buffering ...");// msgid "%s - Audacious"
        }(win_header_)
//        [this]() {
//          // L_HOTKEY_FLOW("Will call GetParent for found MainWindow: " + static_cast<std::string>(*this));
//          auto pr = GetParent(handle_);
//          if (pr) {
//            auto pw = get_window_data(pr);
//            // L_HOTKEY_FLOW("Found parent: " + static_cast<std::string>(pw));
//            return pw.is_main_window();
//          } else {
//            // L_HOTKEY_FLOW("No parent for me. Win err: " + format_windows_error("GetParent"));
//            return true;
//          }
//        }()
        ;
  }

  operator std::string() const {
    if (!handle_) {
      return "INVALID_WINDOW";
    }
    std::string printer;
    printer.append(std::to_string(reinterpret_cast<long long> (handle_)))
        .append("(gdk:")
        .append(std::to_string(reinterpret_cast<long long> (gdk_win32_handle_table_lookup(handle_))))
        .append(") ")
        .append(class_name_).append(
            ": ").append(win_header_);
    return printer;
  }

  static WindowsWindow get_window_data(HWND pHwnd) {
    wchar_t char_buf[311];
    auto num_char = GetClassNameW(pHwnd, char_buf, _countof(char_buf));
    if (!num_char) {
      AUDDBG("WinApiError (code %ld): cannot get ClassName for %p.\n",
             GetLastError(), pHwnd);
      return {};
    }
    std::string w_class_name = Utf16CharArrConverter(char_buf);
    num_char = GetWindowTextW(pHwnd, char_buf, _countof(char_buf));
    if (!num_char) {
      AUDINFO("WinApiError (code %ld) getting WindowHeader. No header for %p. Ignoring this window.\n",
              GetLastError(), pHwnd);
      char_buf[0] = 0;
    }
    return {pHwnd, std::move(w_class_name), char_buf[0] ?
                                            Utf16CharArrConverter(char_buf) : std::string{"[NO_TITLE]"}};
  }
};

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
  // L_HOTKEY_FLOW("Win call.");
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

void unregister_global_keys() {
  while (win_registration_hotkey_id > W_FIRST_GLOBAL_KEY_ID) {
    if (!UnregisterHotKey(handle_hotkey_association, --win_registration_hotkey_id)) {
      AUDWARN(format_windows_error("UnregisterHotKey").c_str());
    }
  }
  handle_hotkey_association = nullptr;
}

void register_global_keys(HWND handle) {
  if (handle_hotkey_association) {
    AUDWARN("Hotkeys already registered. This should not've happened, probably a software bug.");
    return;
  }
  handle_hotkey_association = handle;
  // L_HOTKEY_FLOW("Registering for handle: " + std::to_string(reinterpret_cast<unsigned long long >(handle)));
  auto *hotkey = &(plugin_cfg.first);
  while (hotkey) {
    if (!RegisterHotKey(handle, win_registration_hotkey_id++, hotkey->mask, hotkey->key)) {
      AUDWARN(format_windows_error("RegisterHotKey").c_str());
    } else {
      AUDDBG("Registered a hotkey: %u", hotkey->key);
    }
    hotkey = hotkey->next;
  }
}

template<typename T>
std::string to_hex_string(T integer) {
  std::ostringstream os;
  os << "0x" << std::setw(4) << std::setfill('0') << std::hex << integer;
  return os.str();
}

#define CASE_WIN_PRINT(id) case id: return #id;
template<typename T>
std::string stringify_win_evt(T id) {
  switch (id) {
    CASE_WIN_PRINT(WM_DESTROY)
    CASE_WIN_PRINT(WM_CREATE)
    CASE_WIN_PRINT(WM_ACTIVATE)
    CASE_WIN_PRINT(WM_ENABLE)
    CASE_WIN_PRINT(WM_SHOWWINDOW)
    CASE_WIN_PRINT(WM_WINDOWPOSCHANGING)
    CASE_WIN_PRINT(WM_WINDOWPOSCHANGED)
    CASE_WIN_PRINT(WM_NCACTIVATE)
    CASE_WIN_PRINT(WM_NCPAINT)
    CASE_WIN_PRINT(WM_NCCREATE)
    CASE_WIN_PRINT(WM_ACTIVATEAPP)
    CASE_WIN_PRINT(WM_KILLFOCUS)
    CASE_WIN_PRINT(WM_SETFOCUS)
    CASE_WIN_PRINT(WM_IME_SETCONTEXT)
    CASE_WIN_PRINT(WM_IME_NOTIFY)
    CASE_WIN_PRINT(WM_GETICON)
    CASE_WIN_PRINT(WM_PAINT)
    CASE_WIN_PRINT(WM_SETCURSOR)
    CASE_WIN_PRINT(WM_MOUSEMOVE)
    CASE_WIN_PRINT(WM_NCMOUSEMOVE)
    CASE_WIN_PRINT(WM_MOUSEACTIVATE)
    CASE_WIN_PRINT(WM_NCLBUTTONDOWN)
    CASE_WIN_PRINT(WM_CAPTURECHANGED)
    CASE_WIN_PRINT(WM_SYSCOMMAND)
    CASE_WIN_PRINT(WM_CLOSE)
    CASE_WIN_PRINT(WM_MOVING)
    CASE_WIN_PRINT(WM_ERASEBKGND)
    CASE_WIN_PRINT(WM_EXITMENULOOP)
    CASE_WIN_PRINT(WM_NCMOUSELEAVE)
    CASE_WIN_PRINT(WM_NCHITTEST)
    CASE_WIN_PRINT(WM_EXITSIZEMOVE)
    CASE_WIN_PRINT(WM_ENTERSIZEMOVE)
    CASE_WIN_PRINT(WM_MOUSELEAVE)
    CASE_WIN_PRINT(WM_PARENTNOTIFY)
    CASE_WIN_PRINT(WM_NCCALCSIZE)
    CASE_WIN_PRINT(WM_GETMINMAXINFO)
    CASE_WIN_PRINT(WM_QUERYOPEN)
    default: return "Unknown msg: " + to_hex_string(id);
  }
}

//#define WIN_IF_IS_MSG(smth) reinterpret_cast<T>(smth) == w_msg
//#define WIN_IF_IS_MSG_OR(smth) WIN_IF_IS_MSG(smth) ||

int *counters{nullptr};

template<typename T>
void log_win_msg(T w_msg) {

  switch (w_msg) {
    case WM_NCMOUSELEAVE:
    case WM_SETCURSOR:
    case WM_MOUSEMOVE:
    case WM_MOVING:
    case WM_NCHITTEST:
    case WM_MOUSELEAVE:
    case WM_PAINT:
    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
    case WM_GETICON:
    case WM_IME_SETCONTEXT :
    case WM_IME_NOTIFY :
    case WM_SETFOCUS :
    case WM_SYSCOMMAND :
    case WM_CAPTURECHANGED :
    case WM_NCACTIVATE :
    case WM_ACTIVATE :
    case WM_ACTIVATEAPP :
    case WM_KILLFOCUS :
    case WM_NCMOUSEMOVE:return;
    default:
      if (!counters) {
        counters = new int[2000000]();
      }
      // L_HOTKEY_FLOW("Win msg: " + stringify_win_evt(w_msg) + " cnt: " + std::to_string(++counters[w_msg]));
      break;
  }
}

gboolean async_make_thumb_buttons(gpointer user_data) {
  create_and_hook_thumbbar();
  return G_SOURCE_REMOVE;
}

void register_buttons_with_main_window(const WindowsWindow &main_win) {
  // L_HOTKEY_FLOW("P3: \n     Setting up event loop with win: " + static_cast<std::string>(main_win));
  w32_add_filter_ptr = reinterpret_cast<GdkWindow *> (gdk_win32_handle_table_lookup(main_win.handle_));
  gdk_window_add_filter(w32_add_filter_ptr, w32_evts_filter, nullptr);
  register_global_keys(main_win.handle_);
}

/**
 * Main window created for the first time. Set h_main_window and make it possible for main event filter to
 * handle HIDE and SHOW events for main window.
 *
 * @param user_data HWND
 * @return GDK_FILTER_CONTINUE
 */
gboolean async_make_buttons_first_time(gpointer user_data) {
  h_main_window = reinterpret_cast<HWND>(user_data);
  register_buttons_with_main_window(WindowsWindow::get_window_data(h_main_window));
  return async_make_thumb_buttons(nullptr);
}

void main_window_created_from_statusbar(MainWindowSearchFilterData *const passed_data,
                                        WindowsWindow &created_main_window) {
  // When window fully shows, must add windows buttons.
  gdk_window_remove_filter(reinterpret_cast<GdkWindow *>(passed_data->window_),
                           reinterpret_cast <GdkFilterFunc> (passed_data->function_ptr_),
                           passed_data);
  ungrab_keys();
  release_filter();
  delete passed_data;
  g_idle_add(&async_make_buttons_first_time, created_main_window.handle_);
}

GdkFilterReturn main_window_missing_filter(GdkXEvent *gdk_xevent, GdkEvent *event,
                                           gpointer user_data) {
  auto msg = reinterpret_cast<MSG *>(gdk_xevent);
  if (msg->message == WM_SHOWWINDOW && msg->wParam) {
    auto win_data = WindowsWindow::get_window_data(msg->hwnd);
    if (win_data.is_main_window()) {
      main_window_created_from_statusbar(reinterpret_cast<MainWindowSearchFilterData *> (user_data), win_data);
    }
  }
  return GDK_FILTER_CONTINUE;
}

GdkFilterReturn w32_evts_filter(GdkXEvent *gdk_xevent, GdkEvent *event,
                                gpointer user_data) {
  auto msg = reinterpret_cast<MSG *>(gdk_xevent);
  // log_win_msg(msg->message);
  if (msg->message == WM_SHOWWINDOW) {
    if (window_hidden && msg->wParam && msg->hwnd == h_main_window) {
      // set false: Prevent accidentally calling g_idle_add again
      window_hidden = false;
      g_idle_add(&async_make_thumb_buttons, nullptr);
    }
    // L_HOTKEY_FLOW("WM_SHOWWINDOW for HWND " + std::to_string(reinterpret_cast<intptr_t >( msg->hwnd)));
  } else if (msg->message == WM_COMMAND) {
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
    auto k_id = LOWORD(msg->wParam);
    // L_HOTKEY_FLOW("Global hotkey: " + std::to_string(k_id));
// Max 20 HKs
    if (k_id >=
        W_FIRST_GLOBAL_KEY_ID && k_id < W_FIRST_GLOBAL_KEY_ID
        + 20) {
      auto idx = k_id - W_FIRST_GLOBAL_KEY_ID;
      auto *config = &plugin_cfg.first;
      while (idx--) {
        config = config->next;
      }
      if (
          handle_keyevent(config
                              ->event)) {
        return
            GDK_FILTER_REMOVE;
      }
    }
  } else if (msg->message == WM_CLOSE) {
    if (h_main_window == msg->hwnd) {
      window_hidden = true;
      ComObjectHolder::com_object_holder.
          release_resources();
    }
  }
  return
      GDK_FILTER_CONTINUE;
}
struct EnumWindowsCallbackArgs {
  explicit EnumWindowsCallbackArgs(DWORD p) : pid(p) {}
  const DWORD pid;
  std::vector<WindowsWindow> handles;

  void add_window(HWND pHwnd) {
    handles.emplace_back(WindowsWindow::get_window_data(pHwnd));
  }
};

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

std::string print_wins(const std::vector<WindowsWindow> &wins) {
  std::string printer("Aud has " + std::to_string(wins.size()) + " windows:");
  for (auto &w : wins) {
    printer.append(w);
  }
  return printer;
}

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
#ifndef NDEBUG
  AUDDBG ("Aud has %zu windows: %s", args.handles.size(), print_wins(args.handles).c_str());
#endif
  return std::move(args.handles);
}

enum class WinSearchResult {
  NONE,
  MAIN_WINDOW,
  TASKBAR
};

std::pair<std::vector<WindowsWindow>::const_iterator, WinSearchResult> find_message_receiver_window
    (const std::vector<WindowsWindow> &ws) {
  auto main_win =
      std::find_if(ws.begin(),
                   ws.end(),
                   [](const WindowsWindow &itm) { return itm.is_main_window(); });
  if (main_win == ws.end()) {
    AUDDBG("Win API did not find the main window. It will seek taskbar icon.");
  } else {
    return std::make_pair(main_win, WinSearchResult::MAIN_WINDOW);
  }
  main_win = std::find_if(ws.begin(),
                          ws.end(),
                          [](const WindowsWindow &itm) { return itm.class_name_ == "gtkstatusicon-observer"; });
  if (main_win == ws.end()) {
    return std::make_pair(main_win, WinSearchResult::NONE);
  }
  return std::make_pair(main_win, WinSearchResult::TASKBAR);
}
gboolean window_created_callback(gpointer user_data) {
  AUDDBG("Window created. Do real stuff.");
  auto ws = get_this_app_windows();
  std::vector<WindowsWindow>::const_iterator main_win;
  WinSearchResult search_type;
  std::tie(main_win, search_type) = find_message_receiver_window(ws);
  if (search_type == WinSearchResult::MAIN_WINDOW) {
    window_hidden = false;
    h_main_window = main_win->handle_;
    // L_HOTKEY_FLOW("P1: Handle is: " + std::to_string(reinterpret_cast<unsigned long long >(main_win->handle_)));
    create_and_hook_thumbbar();
    // L_HOTKEY_FLOW("P2: Handle is: " + std::to_string(reinterpret_cast<unsigned long long >(main_win->handle_)));
  } else if (search_type == WinSearchResult::TASKBAR) {
    window_hidden = true;
    // We did not find main window .. add a filter that finds it once it comes up.
    auto *cb_data = new MainWindowSearchFilterData(gdk_win32_handle_table_lookup(main_win->handle_),
                                                   reinterpret_cast<void *>( &main_window_missing_filter));
    gdk_window_add_filter((GdkWindow *) cb_data->window_, &main_window_missing_filter,
                          cb_data);
  } else {
    AUDERR("Win API did not find the window to receive messages. Global hotkeys are disabled!");
    return G_SOURCE_REMOVE;
  }
  register_buttons_with_main_window(*main_win);
  return G_SOURCE_REMOVE;
}

/**
 * Callback that is hooked.
 * @param started_playing If 1, then playback has started. Otherwise, it has ended.
 */
void hk_icon_callback(void *, void *started_playing) {
  auto playback_active = reinterpret_cast<intptr_t >(started_playing);
  if (ComObjectHolder::com_object_holder.set_type(
      playback_active ? ComObjectHolder::ButtonToRender::PAUSE : ComObjectHolder::ButtonToRender::PLAY
  )) {
    ComObjectHolder::com_object_holder.update_buttons<false>();
  }
}

void ungrab_keys() {
  if (aud_evts_hooked) {
    hook_dissociate("playback begin", hk_icon_callback, reinterpret_cast<void *>(1));
    hook_dissociate("playback stop", hk_icon_callback, reinterpret_cast<void *>(0));
    hook_dissociate("playback pause", hk_icon_callback, reinterpret_cast<void *>(0));
    hook_dissociate("playback unpause", hk_icon_callback, reinterpret_cast<void *>(1));
    aud_evts_hooked = false;
  }
  unregister_global_keys();

  ComObjectHolder::com_object_holder.release_resources();
}

void release_filter() {
  if (!w32_add_filter_ptr) { return; }
  gdk_window_remove_filter(w32_add_filter_ptr, w32_evts_filter, nullptr);
  w32_add_filter_ptr = nullptr;
  window_hidden = false;
  h_main_window = nullptr;
}

/*
If paused, sends playback unpause (when playback starts) or pause (obvious).
If stopped, calls playback begin.
At end of playlist or stop button calls playback stop.
  */



void grab_keys() {
  g_idle_add(&window_created_callback, nullptr);
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

void create_and_hook_thumbbar() {
  ComObjectHolder::com_object_holder.create_buttons(h_main_window);
  if (!aud_evts_hooked) {
    hook_associate("playback begin", hk_icon_callback, reinterpret_cast<void *>(1));
    hook_associate("playback stop", hk_icon_callback, reinterpret_cast<void *>(0));
    hook_associate("playback pause", hk_icon_callback, reinterpret_cast<void *>(0));
    hook_associate("playback unpause", hk_icon_callback, reinterpret_cast<void *>(1));
    aud_evts_hooked = true;
  }
}

// TODO: [unregister_global_keys]: UnregisterHotKey exited with code 1400 : Invalid window handle.
  // Always happens at exit. Looks like window is already dead.
// TODO: Rewrite .. unified show/destroy .. statusIcon or not. Must always work.
//   Problem 1: closing and opening window doesn't add buttons.
//   Problem 2: closing and opening sometimes crashes. Other times, shows .. and other times hides. Difference if displayed at start or not (status only).
//      Might be better idea to simply disable all support for statusicon .. Since it doesn't grab global hotkeys anyway.
//      Just have grab and ungrab .. if already grabbed, do nothing.
// TODO: Modifiers are incorrectly written in Settings. But appear to work.
// TODO: Configuring global keys requires audacious restart. How is it on Linux? Maybe needs improvments.

// OPTIONS:
/*
StatusIcon enabled, window shows
StatusIcon enabled, window HIDDEN
StatusIcon disabled


 */

// UTIL1: spam print windows.
//  std::thread *spawned = new std::thread([]() {
//    while (true) {
//      get_and_print_app_windows();
//      std::this_thread::sleep_for(std::chrono::seconds(2));
//    }
//  });