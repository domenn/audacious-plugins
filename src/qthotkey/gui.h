#ifndef _GUI_H_INCLUDED_
#define _GUI_H_INCLUDED_

#include <QtCore/QList>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QKeySequenceEdit>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

#include "plugin.h"

struct PluginPreferences;
class QSignalMapper;

namespace GlobalHotkeys
{

extern const PluginPreferences hotkey_prefs;

struct KeyControls
{
    QComboBox * combobox;
    QKeySequenceEdit * keytext;
    QToolButton * button;

    ~KeyControls();
};

class PrefWidget : public QWidget
{
public:
    explicit PrefWidget(QWidget * parent = nullptr);
    ~PrefWidget();

    static void * make_config_widget();
    static void ok_callback();
    static void destroy_callback();

    QVBoxLayout * main_widget_layout;

private:
    QLabel * information_pixmap;
    QLabel * information_label;
    QHBoxLayout * information_layout;
    QGroupBox * group_box;
    QGridLayout * group_box_layout;
    QLabel * action_label;
    QLabel * key_binding_label;
    QPushButton * add_button;
    QHBoxLayout * add_button_layout;

    QList<KeyControls *> controls_list;

    void add_event_control(const HotkeyConfiguration * hotkey);
};

const char * get_event_name(Event the_event);
} /* namespace GlobalHotkeys */

#endif
