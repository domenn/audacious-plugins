#ifndef _AUD_PLUGINS_HOTKEYW32_WINDOWS_WINDOW_H_INCLUDED_
#define _AUD_PLUGINS_HOTKEYW32_WINDOWS_WINDOW_H_INCLUDED_

#include <gdk/gdkwin32.h>
#include <string>
#include <vector>

enum class AudaciousWindowKind
{
    NONE,
    MAIN_WINDOW,
    TASKBAR_WITH_WINHANDLE_AND_GDKHANDLE,
    GTKSTATUSICON_OBSERVER,
    UNKNOWN
};

struct WindowsWindow
{
    WindowsWindow() = default;
    ~WindowsWindow() = default;
    WindowsWindow(const WindowsWindow & right) = delete;
    WindowsWindow & operator=(const WindowsWindow & right) = delete;
    WindowsWindow & operator=(WindowsWindow && right) = default;
    WindowsWindow(WindowsWindow && right) = default;

    WindowsWindow(HWND handle, std::string className, std::string winHeader,                  AudaciousWindowKind kind);
    static std::string translated_title();
    bool is_main_window() const;
    explicit operator std::string() const;
    static WindowsWindow get_window_data(HWND pHwnd);
    static WindowsWindow find_message_receiver_window();

    operator bool() const; // NOLINT(google-explicit-constructor)
    // clang-format off
    void unref();
    WindowsWindow& operator=(std::nullptr_t dummy) { unref(); return *this; }
    // clang-format on

    HWND handle() const { return handle_; }
    const std::string & class_name() const { return class_name_; }
    const std::string & win_header() const { return win_header_; }
    AudaciousWindowKind kind() const { return kind_; }
    GdkWindow * gdk_window() const;

    const char * kind_string();

private:
    HWND handle_{};
    std::string class_name_{};
    std::string win_header_{};
    AudaciousWindowKind kind_{};
    mutable GdkWindow * gdk_window_{};
};

struct MainWindowSearchFilterData
{
    gpointer window_;
    void * function_ptr_;

    MainWindowSearchFilterData(gpointer window, void * functionPtr)
        : window_(window), function_ptr_(functionPtr)
    {
    }

    MainWindowSearchFilterData(const MainWindowSearchFilterData &) = delete;
    MainWindowSearchFilterData(MainWindowSearchFilterData &&) = delete;
    MainWindowSearchFilterData &
    operator=(const MainWindowSearchFilterData &) = delete;
    MainWindowSearchFilterData &
    operator=(MainWindowSearchFilterData &&) = delete;
    ~MainWindowSearchFilterData() = default;
};

std::vector<WindowsWindow> get_this_app_windows();

#endif