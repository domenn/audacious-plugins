PLUGIN = hotkeyw32${PLUGIN_SUFFIX}

SRCS = ../hotkey/gui.cc ../hotkey/plugin.cc w32_hotkey.cc ../hotkey/hotkey_api_common.cc \
    w32_grab.cc ../hotkey/grab.cc w32_resources_icons.cc res/thumb_icons_map.rc

include ../../buildsys.mk
include ../../extra.mk

plugindir := ${plugindir}/${GENERAL_PLUGIN_DIR}

LD = ${CXX}
CFLAGS += ${PLUGIN_CFLAGS}
CPPFLAGS += ${PLUGIN_CPPFLAGS} ${GTK_CFLAGS} ${GLIB_CFLAGS} -I../.. -I..
LIBS += ${GLIB_LIBS} ${GTK_LIBS} -laudgui -lComctl32