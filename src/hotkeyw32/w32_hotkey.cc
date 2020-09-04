

#include <src/hotkey/api_hotkey.h>

#include <shobjidl.h>
#include <windows.h>

#include <cassert>
#include <cstring>
#include <gdk/gdkwin32.h>
#include <glib.h>
#include <gtk/gtkeditable.h>
#include <iomanip>
#include <libaudcore/runtime.h>
#include <src/hotkey/grab.h>
#include <sstream>

#include "windows_key_str_map.h"
#include "windows_window.h"

constexpr auto W_KEY_ID_PLAY = 18771;
constexpr auto W_KEY_ID_PREV = 18772;
constexpr auto W_KEY_ID_NEXT = 18773;
constexpr auto W_FIRST_GLOBAL_KEY_ID = 18774;

bool system_up_and_running{false};

template<typename T>
std::string VirtualKeyCodeToStringMethod2(T virtualKey)
{
    wchar_t name[128];
    static auto key_layout = GetKeyboardLayout(0);
    UINT scanCode = MapVirtualKeyEx(virtualKey, MAPVK_VK_TO_VSC, key_layout);
    LONG lParamValue = (scanCode << 16);
    int result = GetKeyNameTextW(lParamValue, name, sizeof name);
    if (result > 0)
    {
        auto * windows_cmd = reinterpret_cast<char *>(
            g_utf16_to_utf8(reinterpret_cast<const gunichar2 *>(name), -1,
                            nullptr, nullptr, nullptr));
        std::string returning(windows_cmd);
        g_free(windows_cmd);
        return returning;
    }
    else
    {
        return {};
    }
}

void Hotkey::add_hotkey(HotkeyConfiguration ** pphotkey,
                        Hotkey::OS_KeySym keysym, int mask, int type,
                        EVENT event)
{
    AUDDBG("lHotkeyFlow:Win call.");
    HotkeyConfiguration * photkey;
    if (keysym == 0)
        return;
    if (pphotkey == nullptr)
        return;
    photkey = *pphotkey;
    if (photkey == nullptr)
        return;
    // keycode =
    // XKeysymToKeycode(GDK_DISPLAY_XDISPLAY(gdk_display_get_default()),
    // keysym); if (keycode == 0) return;
    if (photkey->key)
    {
        photkey->next = g_new(HotkeyConfiguration, 1);
        photkey = photkey->next;
        *pphotkey = photkey;
        photkey->next = nullptr;
    }
    photkey->key = (int)42;
    photkey->mask = mask;
    photkey->event = event;
    photkey->type = type;
}

void register_global_keys(HWND handle)
{
    auto * hotkey = &(plugin_cfg.first);
    auto _id = W_FIRST_GLOBAL_KEY_ID;
    while (hotkey)
    {
        RegisterHotKey(handle, _id++, hotkey->mask, hotkey->key);
        hotkey = hotkey->next;
    }
}

// void assign(wchar_t *ptr_first_element, const wchar_t *text) {
//  int loc = 0;
//  while (text[loc]) {
//    ptr_first_element[loc] = text[loc];
//    ++loc;
//  }
//  ptr_first_element[loc] = 0;
//}

HRESULT AddThumbarButtons(HWND hwnd)
{
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

    THUMBBUTTON btn[]{
        {dwMask, W_KEY_ID_PREV, 0, nullptr, L"Previous song", THBF_ENABLED},
        {dwMask, W_KEY_ID_PLAY, 0, nullptr, L"Play/pause", THBF_ENABLED},
        {dwMask, W_KEY_ID_NEXT, 0, nullptr, L"Next song", THBF_ENABLED}};

    ITaskbarList3 * ptbl;
    HRESULT hr = CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&ptbl));

    if (SUCCEEDED(hr))
    {
        // Declare the image list that contains the button images.
        // hr = ptbl->ThumbBarSetImageList(hwnd, himl);

        if (SUCCEEDED(hr))
        {
            // Attach the toolbar to the thumbnail.
            hr = ptbl->ThumbBarAddButtons(hwnd, ARRAYSIZE(btn), btn);
        }
        ptbl->Release();
    }
    return hr;
}

GdkFilterReturn w32_evts_filter(GdkXEvent * gdk_xevent, GdkEvent * event,
                                gpointer user_data)
{
    auto msg = reinterpret_cast<MSG *>(gdk_xevent);
    // log_win_msg(msg->message);
    if (msg->message == WM_SHOWWINDOW)
    {
        AUDINFO("WinApiMsg WM_SHOWWINDOW for %p", msg->hwnd);
    }
    else if (msg->message == WM_COMMAND)
    {
        auto buttonId = static_cast<short>(msg->wParam & 0xFFFF);
        AUDDBG("Clicked button with ID: %d", buttonId);
        switch (buttonId)
        {
        case W_KEY_ID_PLAY:
            handle_keyevent(EVENT_PAUSE);
            break;
        case W_KEY_ID_NEXT:
            handle_keyevent(EVENT_NEXT_TRACK);
            break;
        case W_KEY_ID_PREV:
            handle_keyevent(EVENT_PREV_TRACK);
            break;
        }
        return GDK_FILTER_REMOVE;
    }
    else if (msg->message == WM_HOTKEY)
    {
        auto k_id = LOWORD(msg->wParam);
        AUDDBG("Global hotkey: %du", k_id);
        // Max 20 HKs
        if (k_id >= W_FIRST_GLOBAL_KEY_ID && k_id < W_FIRST_GLOBAL_KEY_ID + 20)
        {
            auto idx = k_id - W_FIRST_GLOBAL_KEY_ID;
            auto * config = &plugin_cfg.first;
            while (idx--)
            {
                config = config->next;
            }
            if (handle_keyevent(config->event))
            {
                return GDK_FILTER_REMOVE;
            }
        }
    }
    else if (msg->message == WM_CLOSE)
    {
    }
    return GDK_FILTER_CONTINUE;
}

void register_hotkeys_with_available_window(WindowsWindow message_receiving_window)
{
    auto* gdkwin = message_receiving_window.gdk_window();
    if(!gdkwin){
        AUDINFO("HWND doesn't have associated gdk window. Cannot setup global hotkeys.");
        return;
    }
    gdk_window_add_filter(gdkwin,
                          w32_evts_filter, nullptr);
    register_global_keys(message_receiving_window.handle());
}

void release_filter()
{
//    if (!message_receiving_window)
//    {
//        return;
//    }
//    gdk_window_remove_filter(message_receiving_window.gdk_window(),
//                             w32_evts_filter, nullptr);
//    message_receiving_window = nullptr;
}
//
// void main_window_created_from_statusbar(
//    MainWindowSearchFilterData * const passed_data,
//    WindowsWindow & created_main_window)
//{
//    // When window fully shows, must add windows buttons.
//    gdk_window_remove_filter(
//        reinterpret_cast<GdkWindow *>(passed_data->window_),
//        reinterpret_cast<GdkFilterFunc>(passed_data->function_ptr_),
//        passed_data);
//    ungrab_keys();
//    release_filter();
//    delete passed_data;
//}
//
// GdkFilterReturn main_window_missing_filter(GdkXEvent * gdk_xevent,
//                                           GdkEvent * event, gpointer
//                                           user_data)
//{
//    auto msg = reinterpret_cast<MSG *>(gdk_xevent);
//    if (msg->message == WM_SHOWWINDOW && msg->wParam)
//    {
//        auto win_data = WindowsWindow::get_window_data(msg->hwnd);
//        if (win_data.is_main_window())
//        {
//            main_window_created_from_statusbar(
//                reinterpret_cast<MainWindowSearchFilterData *>(user_data),
//                win_data);
//        }
//    }
//    return GDK_FILTER_CONTINUE;
//}

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
gboolean window_created_callback(gpointer user_data)
{
    AUDDBG("Window created. Do real stuff.");
    system_up_and_running = true;
    grab_keys();
    return G_SOURCE_REMOVE;
}

void win_init()
{
    AUDDBG("lHotkeyFlow:Win .. important call.");
    g_idle_add(&window_created_callback, nullptr);
}

void Hotkey::key_to_string(int key, char ** out_keytext)
{
    // Special handling for most OEM keys - they may depend on language or
    // keyboard?
    switch (key)
    {
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
    case VK_OEM_CLEAR:
    {
        auto win_str = VirtualKeyCodeToStringMethod2(key);
        if (!win_str.empty())
        {
            *out_keytext = g_strdup(win_str.c_str());
            return;
        }
    }
    }
    *out_keytext = g_strdup(WIN_VK_NAMES[key]);
}

char * Hotkey::create_human_readable_keytext(const char * const keytext,
                                             int key, int mask)
{
    const auto w_mask = static_cast<UINT>(mask);
    std::ostringstream produced;
    // "Control", "Shift", "Alt", "Win"
    if (w_mask & static_cast<UINT>(MOD_CONTROL))
    {
        produced << "Control + ";
    }
    if (w_mask & static_cast<UINT>(MOD_SHIFT))
    {
        produced << "Shift + ";
    }
    if (w_mask & static_cast<UINT>(MOD_ALT))
    {
        produced << "Alt + ";
    }
    if (w_mask & static_cast<UINT>(MOD_WIN))
    {
        produced << "Win + ";
    }
    produced << keytext;

    AUDDBG(produced.str().c_str());
    return g_strdup(produced.str().c_str());
}

void grab_keys()
{
    if (!system_up_and_running)
    {
        return;
    }
    AUDDBG("Grabbing ...");

    auto message_receiving_window = WindowsWindow::find_message_receiver_window();
    AUDDBG("Will hook together the window of kind %s", message_receiving_window.kind_string());
    if (message_receiving_window.kind() == AudaciousWindowKind::NONE ||
        message_receiving_window.kind() == AudaciousWindowKind::UNKNOWN)
    {
        AUDERR("Win API did not find the window to receive messages. Global "
               "hotkeys are disabled!");
        message_receiving_window = nullptr;
        return;
    }
    register_hotkeys_with_available_window(std::move(message_receiving_window));
}
void ungrab_keys()
{
//    AUDDBG("Releasing ...");
//    if (message_receiving_window)
//    {
//        auto * hotkey = &(plugin_cfg.first);
//        auto _id = W_FIRST_GLOBAL_KEY_ID;
//        while (hotkey)
//        {
//            UnregisterHotKey(message_receiving_window.handle(), _id++);
//            hotkey = hotkey->next;
//        }
//    }
    release_filter();
}


#include "windows_window.h"

#include <algorithm>
#include <libaudcore/i18n.h>
#include <libaudcore/runtime.h>
#include <vector>

class Utf16CharArrConverter
{
    gchar * converted{nullptr};

public:
    Utf16CharArrConverter(const wchar_t * const input_w_str)
        : converted(reinterpret_cast<char *>(
                        g_utf16_to_utf8(reinterpret_cast<const gunichar2 *>(input_w_str),
                                        -1, nullptr, nullptr, nullptr)))
    {
    }
    ~Utf16CharArrConverter()
    {
        if (converted)
        {
            g_free(converted);
        }
    };
    operator std::string()
    { // NOLINT(google-explicit-constructor)
        return std::string(converted);
    }
};

std::string print_wins(const std::vector<WindowsWindow> & wins)
{
    std::string printer("Aud has " + std::to_string(wins.size()) + " windows:");
    for (auto & w : wins)
    {
        printer.append("\n   ").append(static_cast<std::string>(w));
    }
    return printer;
}

WindowsWindow::WindowsWindow(HWND handle, std::string className,
                             std::string winHeader, AudaciousWindowKind kind)
    : handle_(handle), class_name_(std::move(className)),
      win_header_(std::move(winHeader)), kind_(kind)
{
}
std::string WindowsWindow::translated_title()
{
    // get "%s - <translatedAudacious>"
    std::string org = N_("%s - Audacious");
    // erase %s
    org.erase(0, 2);
    return org;
}
bool WindowsWindow::is_main_window() const
{
    return class_name_ == "gdkWindowToplevel" && [](const std::string & title) {
      return title == N_("Audacious") ||
          title.find(translated_title()) != std::string::npos ||
          title == N_("Buffering ..."); // msgid "%s - Audacious"
    }(win_header_);
}
WindowsWindow::operator std::string() const
{
    if (!handle_)
    {
        return "INVALID_WINDOW";
    }
    std::string printer;
    printer.append(std::to_string(reinterpret_cast<long long>(handle_)))
        .append("(gdk:")
        .append(std::to_string(reinterpret_cast<long long>(
                                   gdk_win32_handle_table_lookup(handle_))))
        .append(") ")
        .append(class_name_)
        .append(": ")
        .append(win_header_);
    return printer;
}
WindowsWindow WindowsWindow::get_window_data(HWND pHwnd)
{
    wchar_t char_buf[311];
    auto num_char = GetClassNameW(pHwnd, char_buf, _countof(char_buf));
    if (!num_char)
    {
        AUDDBG("WinApiError (code %ld): cannot get ClassName for %p.\n",
               GetLastError(), pHwnd);
        return {};
    }
    std::string w_class_name = Utf16CharArrConverter(char_buf);
    num_char = GetWindowTextW(pHwnd, char_buf, _countof(char_buf));
    if (!num_char)
    {
        AUDINFO("WinApiError (code %ld) getting WindowHeader. No header "
                "for %p. Ignoring this window.\n",
                GetLastError(), pHwnd);
        char_buf[0] = 0;
    }
    return {pHwnd, std::move(w_class_name),
            char_buf[0] ? Utf16CharArrConverter(char_buf)
                        : std::string{"[NO_TITLE]"},
            AudaciousWindowKind::UNKNOWN};
}

struct EnumWindowsCallbackArgs
{
    explicit EnumWindowsCallbackArgs(DWORD p) : pid(p) {}
    const DWORD pid;
    std::vector<WindowsWindow> handles;

    void add_window(HWND pHwnd)
    {
        handles.emplace_back(WindowsWindow::get_window_data(pHwnd));
    }
};

static BOOL CALLBACK EnumWindowsCallback(HWND hnd, LPARAM lParam)
{
    // Get pointer to created callback object for storing data.
    auto * args = reinterpret_cast<EnumWindowsCallbackArgs *>(lParam);
    // Callback result are all windows (not only my program). I filter by PID /
    // thread ID.
    DWORD windowPID;
    GetWindowThreadProcessId(hnd, &windowPID);
    // Compare to the one we have stored in our callbaack structure.
    if (windowPID == args->pid)
    {
        args->add_window(hnd);
    }
    return TRUE;
}

std::vector<WindowsWindow> get_this_app_windows()
{
    // Create object that will hold a result.
    EnumWindowsCallbackArgs args(GetCurrentProcessId());
    // AUDERR("Testing getlasterror: %ld\n.", GetLastError());
    // Call EnumWindows and pass pointer to function and created callback
    // structure.
    if (EnumWindows(&EnumWindowsCallback, (LPARAM)&args) == FALSE)
    {
        // If the call fails, return empty vector
        AUDERR("WinApiError (code %ld). Cannot get windows, hotkey plugin "
               "won't work.\n",
               GetLastError());
        return std::vector<WindowsWindow>{};
    }
#ifndef NDEBUG
    AUDDBG("Aud has %zu windows: %s", args.handles.size(),
           print_wins(args.handles).c_str());
#endif
    return std::move(args.handles);
}
WindowsWindow WindowsWindow::find_message_receiver_window()
{
    AUDWARN("Call a function AFTER");
    auto ws = get_this_app_windows();
    auto main_win = std::find_if(ws.begin(), ws.end(), [](WindowsWindow & itm) {
      return itm.is_main_window();
    });
    if (main_win != ws.end())
    {
        main_win->kind_ = AudaciousWindowKind::MAIN_WINDOW;
        return std::move(*main_win);
    }
    main_win =
        std::find_if(ws.begin(), ws.end(), [](const WindowsWindow & itm) {
          return itm.gdk_window() != nullptr;
        });
    if (main_win != ws.end())
    {
        main_win->kind_ =
            AudaciousWindowKind::TASKBAR_WITH_WINHANDLE_AND_GDKHANDLE;
        return std::move(*main_win);
    }
    main_win =
        std::find_if(ws.begin(), ws.end(), [](const WindowsWindow & itm) {
          return itm.class_name_ == "gtkstatusicon-observer";
        });
    if (main_win == ws.end())
    {
        AUDDBG("No proper window found. Giving up.");
        return {nullptr, {}, {}, AudaciousWindowKind::NONE};
    }
    main_win->kind_ = AudaciousWindowKind::GTKSTATUSICON_OBSERVER;
    return std::move(*main_win);
}
WindowsWindow::operator bool() const { return handle_; }
GdkWindow * WindowsWindow::gdk_window() const
{
    if (!gdk_window_)
    {
        gdk_window_ = reinterpret_cast<GdkWindow *>(
            gdk_win32_handle_table_lookup(handle_));
    }
    return gdk_window_;
}
void WindowsWindow::unref()
{
    handle_ = nullptr;
    gdk_window_ = nullptr;
}

#define ENUM_KIND_CASE(item)                                                   \
    case item:                                                                 \
        return #item;

const char * WindowsWindow::kind_string()
{
    switch (kind_)
    {
    ENUM_KIND_CASE(AudaciousWindowKind::MAIN_WINDOW)
    ENUM_KIND_CASE(AudaciousWindowKind::UNKNOWN)
    ENUM_KIND_CASE(AudaciousWindowKind::NONE)
    ENUM_KIND_CASE(AudaciousWindowKind::GTKSTATUSICON_OBSERVER)
    ENUM_KIND_CASE(
        AudaciousWindowKind::TASKBAR_WITH_WINHANDLE_AND_GDKHANDLE)
    }
}
