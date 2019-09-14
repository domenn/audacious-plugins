
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
#include <algorithm>
#include <iomanip>
#include <sstream>
#ifndef NDEBUG
#include <cassert>
#endif

#include "w32_resources_icons.hpp"
#include "src/hotkeyw32/res/resource.h"
#include "w32_common_headers.hpp"

template<typename T1, typename T2, typename T3>
HICON LoadOrWarn(T1 dll, T2 what, T3 ln) {
  auto l = LoadIconW(dll, what);
  if (!l) {
    AUDERR((std::to_string(ln) + " " + format_windows_error("LoadIconW")).c_str());
  }
  return l;
}

std::vector<HICON> load_icons() {
  auto hndl = GetModuleHandleW(L"hotkeyw32.dll");

  if (!hndl) {
    AUDERR(format_windows_error("GetModuleHandleW(L\"hotkeyw32.dll\")").c_str());
  }

  return {LoadOrWarn(hndl, MAKEINTRESOURCEW(IDI_ICON_PREV), __LINE__),
          LoadOrWarn(hndl, MAKEINTRESOURCEW(IDI_ICON_NEXT), __LINE__),
          LoadOrWarn(hndl, MAKEINTRESOURCEW(IDI_ICON_PAUSE_AUD), __LINE__)
  };
}

HIMAGELIST make_icons_for_buttons() {
  // MAKEINTRESOURCE(IDI_ICON_PREV);
//    MAKEINTRESOURCE(IDI_ICON_MID);
//    MAKEINTRESOURCE(IDI_ICON_NEXT);
  HIMAGELIST himl = ImageList_Create(16, 16, 0, 0, 3);
  auto hndl = GetModuleHandleW(L"hotkeyw32.dll");
  if (!hndl) {
    AUDERR(format_windows_error("GetModuleHandleW(L\"hotkeyw32.dll\")").c_str());
  }

  auto icons = load_icons();

  auto added = ImageList_ReplaceIcon(himl, -1, icons[0]);
  if (added == -1) {
    AUDERR(format_windows_error("ImageList_AddIcon(himl, h_icon_prev").c_str());
  }
  added = ImageList_ReplaceIcon(himl, -1, icons[1]);
  if (added == -1) {
    AUDERR(format_windows_error("ImageList_AddIcon(himl, h_icon_p").c_str());
  }
  added = ImageList_ReplaceIcon(himl, -1, icons[2]);
  if (added == -1) {
    AUDERR(format_windows_error("ImageList_AddIcon(himl, h_icon_n").c_str());
  }
//    ImageList_AddIcon(himl, hItem1);
//    ImageList_AddIcon(himl, hItem2);

  return himl;
}