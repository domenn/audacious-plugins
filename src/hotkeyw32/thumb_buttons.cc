

#include <windows.h>
#include <shobjidl.h>

#include <libaudcore/runtime.h>

#include <vector>

#include "src/hotkeyw32/res/resource.h"
#include "w32_common_headers.h"

ComObjectHolder ComObjectHolder::com_object_holder;

class IconLoader {
  static constexpr int NUM_ICONS = 4;
  static constexpr int INDEX_FIRST = IDI_ICON_PREV;

  HICON icons[NUM_ICONS]{};
  HMODULE hndl_dll_{nullptr};

  static int map_to_this_class_array(int windows_resource_id) {
    return windows_resource_id - INDEX_FIRST;
  }

  template<typename T1, typename T2, typename T3>
  HICON LoadOrWarn(T1 dll, T2 what, T3 ln) {
    auto l = LoadIconW(dll, what);
    if (!l) {
      AUDERR((std::to_string(ln) + " " + format_windows_error("LoadIconW")).c_str());
    }
    return l;
  }

  HMODULE get_dll() {
    if (!hndl_dll_) {
      hndl_dll_ = GetModuleHandleW(L"hotkeyw32.dll");
      if (!hndl_dll_) {
        AUDERR(format_windows_error("GetModuleHandleW(L\"hotkeyw32.dll\")").c_str());
      }
    }
    return hndl_dll_;
  }

  template<typename T1>
  HICON load_or_return(int windows_resource_id, T1 ln) {
    auto map_idx = map_to_this_class_array(windows_resource_id);
    if (!icons[map_idx]) {
      icons[map_idx] = LoadOrWarn(get_dll(), MAKEINTRESOURCEW(windows_resource_id), ln);
    }
    return icons[map_idx];
  }

 public:
  IconLoader() = default;
  IconLoader(const IconLoader &) = delete;
  IconLoader &operator=(const IconLoader &) = delete;
  IconLoader(IconLoader &&) = delete;
  IconLoader &operator=(IconLoader &&) = delete;

  ~IconLoader() {
    for (int i = 0; i < NUM_ICONS; ++i) {
      if (icons[i]) {
        DestroyIcon(icons[i]);
      }
    }
  }

  HICON prev_icon() {
    return load_or_return(IDI_ICON_PREV, __LINE__);
  }
  HICON next_icon() {
    return load_or_return(IDI_ICON_NEXT, __LINE__);
  }
  HICON play_pause_icon(ComObjectHolder::ButtonToRender state) {
    return load_or_return(
        state == ComObjectHolder::ButtonToRender::PLAY ? IDI_ICON_PLAY_AUD : IDI_ICON_PAUSE_AUD, __LINE__);
  }

};

const THUMBBUTTONMASK ComObjectHolder::dwMask = THB_ICON | THB_TOOLTIP | THB_FLAGS;

bool ComObjectHolder::set_type(ButtonToRender state) {
  auto returns = icon_to_render_ != state;
  icon_to_render_ = state;
  return returns;
}
void ComObjectHolder::create_buttons(HWND hwnd) {
  hwnd_ = hwnd;

  IconLoader loader;
  THUMBBUTTON btn[]{{
                        dwMask,
                        W_KEY_ID_PREV,
                        0,
                        loader.prev_icon(),
                        // nullptr,
                        L"Previous song",
                        THBF_ENABLED
                    },
                    {
                        dwMask,
                        W_KEY_ID_PLAY,
                        0,
                        loader.play_pause_icon(icon_to_render_),
                        //    nullptr,

                        L"Play/pause",
                        THBF_ENABLED
                    },
                    {
                        dwMask,
                        W_KEY_ID_NEXT,
                        0,
                        loader.next_icon(),
                        //  nullptr,

                        L"Next song",
                        THBF_ENABLED
                    }
  };

  auto ptbl = get_ptbl();
  if (!ptbl) {
    AUDDBG("Unable to create buttons. ComObject is not initialized. Try calling ComObjectHolder::create_buttons");
    return;
  }
  auto hr = ptbl->ThumbBarAddButtons(hwnd, ARRAYSIZE(btn), btn);
  if (hr != S_OK) {
    AUDERR("Unable to setup ThumbBar buttons due to WinApi error:\n    %s",
           format_windows_error("ThumbBarAddButtons").c_str());
  }
}

ITaskbarList3 *ComObjectHolder::get_ptbl() {
  if (!ptbl_) {
    HRESULT hr = CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&ptbl_));
    if (!SUCCEEDED(hr)) {
      AUDERR("Unable to setup ThumbBar buttons due to WinApi error:\n    %s",
             format_windows_error("CoCreateInstance").c_str());
      ptbl_ = nullptr;
    }
  }
  return ptbl_;
}

template<bool warn_if_fail>
void update_buttons_warn_failure(const char *func, int line) {}

#define UPDATE_BTN_WARN_LOG_ITEMS "Unable to update buttons. No window handle. Create with ComObjectHolder::create_buttons\n   Called from %s:%d", func, line

template<>
void update_buttons_warn_failure<false>(const char *func, int line) {
  AUDDBG(UPDATE_BTN_WARN_LOG_ITEMS);
}

template<>
void update_buttons_warn_failure<true>(const char *func, int line) {
  AUDWARN(UPDATE_BTN_WARN_LOG_ITEMS);
}

template<bool warn_if_fail>
void ComObjectHolder::update_buttons() {
  if (!get_ptbl()) {
    AUDDBG(
        "Unable to update buttons. ComObject is not initialized. Try calling ComObjectHolder::create_buttons. This may have had been intentional.");
    return;
  }
  if (!hwnd_) {
    update_buttons_warn_failure<warn_if_fail>(__FUNCTION__, __LINE__);
    return;
  }
  IconLoader loader;
  THUMBBUTTON thbButton{
      dwMask,
      W_KEY_ID_PLAY,
      0,
      loader.play_pause_icon(icon_to_render_),
      //    nullptr,

      L"Play/pause",
      THBF_ENABLED
  };

  auto hr = get_ptbl()->ThumbBarUpdateButtons(hwnd_, 1, &thbButton);
  if (hr != S_OK) {
    AUDERR("Unable to update ThumbBar buttons due to WinApi error:\n    %s",
           format_windows_error("ThumbBarAddButtons").c_str());
  }
}
void ComObjectHolder::release_resources() {
  if (ptbl_) {
    ptbl_->Release();
  }
  hwnd_ = nullptr;
}

template void ComObjectHolder::update_buttons<false>();
template void ComObjectHolder::update_buttons<true>();