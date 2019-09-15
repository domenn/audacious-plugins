#pragma once

#include <string>

constexpr auto W_KEY_ID_PLAY = 18771;
constexpr auto W_KEY_ID_PREV = 18772;
constexpr auto W_KEY_ID_NEXT = 18773;
constexpr auto W_FIRST_GLOBAL_KEY_ID = 18774;

std::string format_windows_error(
    const std::string &str_lpszFunction);

class ComObjectHolder {
 public:
  enum class ButtonToRender { PLAY, PAUSE };
  static ComObjectHolder com_object_holder;
 private:
  static const THUMBBUTTONMASK dwMask;

  ButtonToRender icon_to_render_{ButtonToRender::PLAY};
  ITaskbarList3 *ptbl_{nullptr};
  HWND hwnd_{nullptr};

  ITaskbarList3 *get_ptbl();

 public:
  void create_buttons(HWND hwnd);

  /**
   * WinApi internal requirements. Releases COM object.
   *
   * According to documentation: "The toolbar itself cannot be removed without re-creating the window itself".
   * Buttons will remain active until the application is restarted (or window recreated by using system icon plugin).
   */
  void release_resources();

  /**
 * Actually update buttons on screen (play / pause) according to internal state, controled with set_state function. Does nothing if buttons had not been loaded and setup correctly.
 * @tparam warn_if_fail Failure is logged. As Warning if true, as debug otherwise.
 */
  template<bool warn_if_fail = true>
  void update_buttons();

  /**
   * Update internal state (button) of this object (playing / not playing). Do not change any icons. Call update_buttons to change.
   * @param button_to_render To set.
   * @return True if button was changed, false otherwise.
   */
  bool set_type(ButtonToRender button_to_render);
};