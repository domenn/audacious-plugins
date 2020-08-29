/*
 *  This file is part of audacious-hotkey plugin for audacious
 *
 *  Copyright (c) 2007 - 2008  Sascha Hlusiak <contact@saschahlusiak.de>
 *  Name: gui.c
 *  Description: gui.c
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA.
 */

#include <libaudcore/i18n.h>
#include <libaudcore/preferences.h>

#include <libaudgui/libaudgui-gtk.h>

#include <gdk/gdkkeysyms-compat.h>
#include <gtk/gtk.h>
#include <libaudcore/runtime.h>
#include <vector>

#include "grab.h"
#include "gui.h"
#include "plugin.h"

struct KeyControls
{
    GtkWidget * keytext;
    GtkWidget * grid;
    GtkWidget * button;
    GtkWidget * combobox;
};

static const char * event_desc[EVENT_MAX] = {
    [EVENT_PREV_TRACK] = N_("Previous track"),
    [EVENT_PLAY] = N_("Play"),
    [EVENT_PAUSE] = N_("Pause/Resume"),
    [EVENT_STOP] = N_("Stop"),
    [EVENT_NEXT_TRACK] = N_("Next track"),
    [EVENT_FORWARD] = N_("Step forward"),
    [EVENT_BACKWARD] = N_("Step backward"),
    [EVENT_MUTE] = N_("Mute"),
    [EVENT_VOL_UP] = N_("Volume up"),
    [EVENT_VOL_DOWN] = N_("Volume down"),
    [EVENT_JUMP_TO_FILE] = N_("Jump to file"),
    [EVENT_TOGGLE_WIN] = N_("Toggle player window(s)"),
    [EVENT_SHOW_AOSD] = N_("Show On-Screen-Display"),
    [EVENT_TOGGLE_REPEAT] = N_("Toggle repeat"),
    [EVENT_TOGGLE_SHUFFLE] = N_("Toggle shuffle"),
    [EVENT_TOGGLE_STOP] = N_("Toggle stop after current"),
    [EVENT_RAISE] = N_("Raise player window(s)")};

std::vector<KeyControls> gui_controls{};

KeyControls * add_event_controls(GtkWidget * grid, int row)
{

    KeyControls * controls = &gui_controls.emplace_back();

    controls->combobox = gtk_combo_box_text_new();
    for (int i = 0; i < EVENT_MAX; i++)
    {
        gtk_combo_box_text_append_text((GtkComboBoxText *)controls->combobox,
                                       _(event_desc[i]));
    }
    // gtk_combo_box_set_active(GTK_COMBO_BOX(controls->combobox),
    // controls->hotkey.event);
    gtk_table_attach_defaults(GTK_TABLE(grid), controls->combobox, 0, 1, row,
                              row + 1);

    controls->keytext = gtk_entry_new();
    gtk_table_attach_defaults(GTK_TABLE(grid), controls->keytext, 1, 2, row,
                              row + 1);
    gtk_editable_set_editable(GTK_EDITABLE(controls->keytext), false);

    // set_keytext(controls->keytext, controls->hotkey.key,
    // controls->hotkey.mask, controls->hotkey.type);
    g_signal_connect(
        (void *)controls->keytext, "key_press_event",
        G_CALLBACK(+[](GtkWidget * widget, GdkEventButton * event,
                       void * user_data) -> gboolean { AUDDBG("KeyPressEvent"); return true;}),
        controls);
    g_signal_connect(
        (void *)controls->keytext, "key_release_event",
        G_CALLBACK(+[](GtkWidget * widget, GdkEventButton * event,
                       void * user_data) -> gboolean { AUDDBG("KeyReleaseEvent"); return true;}),
        controls);
    g_signal_connect(
        (void *)controls->keytext, "button_press_event",
        G_CALLBACK(+[](GtkWidget * widget, GdkEventButton * event,
                       void * user_data) -> gboolean { AUDDBG("BtnPressEvt"); return true;}),
        controls);
    g_signal_connect(
        (void *)controls->keytext, "scroll_event",
        G_CALLBACK(+[](GtkWidget * widget, GdkEventButton * event,
                       void * user_data) -> gboolean { AUDDBG("ScrollEvt"); return true; }),
        controls);

    controls->button = gtk_button_new();
    gtk_button_set_image(
        GTK_BUTTON(controls->button),
        gtk_image_new_from_icon_name("edit-delete", GTK_ICON_SIZE_BUTTON));
    gtk_table_attach_defaults(GTK_TABLE(grid), controls->button, 2, 3, row,
                              row + 1);
//    g_signal_connect(G_OBJECT(controls->button), "clicked",
//                     G_CALLBACK(clear_keyboard), controls);

    gtk_widget_grab_focus(GTK_WIDGET(controls->keytext));
    return controls;
}

void add_callback(GtkWidget * widget, void * data)
{
    add_event_controls(controls, controls->grid, count, &temphotkey);
    gtk_widget_show_all(GTK_WIDGET(controls->grid));
}

void * GlobalHotkeys::create_gui()
{

    KeyControls * current_controls;
    GtkWidget *main_vbox, *hbox;
    GtkWidget * alignment;
    GtkWidget * frame;
    GtkWidget * label;
    GtkWidget * image;
    GtkWidget * grid;
    GtkWidget *button_box, *button_add;

    main_vbox = gtk_vbox_new(false, 4);

    alignment = gtk_alignment_new(0.5, 0.5, 1, 1);
    gtk_box_pack_start(GTK_BOX(main_vbox), alignment, false, true, 0);
    gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 4, 0, 0, 0);
    hbox = gtk_hbox_new(false, 2);
    gtk_container_add(GTK_CONTAINER(alignment), hbox);
    image = gtk_image_new_from_icon_name("dialog-information",
                                         GTK_ICON_SIZE_DIALOG);
    gtk_box_pack_start(GTK_BOX(hbox), image, false, true, 0);
    label = gtk_label_new(_("Press a key combination inside a text field.\nYou "
                            "can also bind mouse buttons."));
    gtk_box_pack_start(GTK_BOX(hbox), label, true, true, 0);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

    label = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(label), _("Hotkeys:"));
    frame = gtk_frame_new(nullptr);
    gtk_frame_set_label_widget(GTK_FRAME(frame), label);
    gtk_box_pack_start(GTK_BOX(main_vbox), frame, true, true, 0);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
    alignment = gtk_alignment_new(0, 0, 1, 0);
    gtk_container_add(GTK_CONTAINER(frame), alignment);
    gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 3, 3, 3, 3);

    grid = gtk_table_new(0, 0, false);
    gtk_table_set_col_spacings(GTK_TABLE(grid), 2);
    gtk_container_add(GTK_CONTAINER(alignment), grid);

    label = gtk_label_new(nullptr);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
    gtk_misc_set_alignment(GTK_MISC(label), 0.5, 0.5);
    gtk_label_set_markup(GTK_LABEL(label), _("<b>Action:</b>"));
    gtk_table_attach_defaults(GTK_TABLE(grid), label, 0, 1, 0, 1);

    label = gtk_label_new(nullptr);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
    gtk_misc_set_alignment(GTK_MISC(label), 0.5, 0.5);
    gtk_label_set_markup(GTK_LABEL(label), _("<b>Key Binding:</b>"));
    gtk_table_attach_defaults(GTK_TABLE(grid), label, 1, 2, 0, 1);

    hbox = gtk_hbox_new(false, 0);
    gtk_box_pack_start(GTK_BOX(main_vbox), hbox, false, true, 0);

    button_box = gtk_hbutton_box_new();
    gtk_box_pack_start(GTK_BOX(hbox), button_box, false, true, 0);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(button_box), GTK_BUTTONBOX_START);
    gtk_box_set_spacing(GTK_BOX(button_box), 4);

    button_add = audgui_button_new(_("_Add"), "list-add", nullptr, nullptr);
    gtk_container_add(GTK_CONTAINER(button_box), button_add);
    g_signal_connect(G_OBJECT(button_add), "clicked", G_CALLBACK(add_callback),
                     nullptr);

    return main_vbox;
}