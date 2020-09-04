cmake_minimum_required(VERSION 3.14)
project(AudaciousPlugins)
set(CMAKE_CXX_STANDARD 17)
set (CMAKE_VERBOSE_MAKEFILE true)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -include config.h")

#
#include_directories(audacious)
#include_directories(audacious/src)
#include_directories(audacious-plugins/src)
#include_directories(C:\\GTK\\include\\glib-2.0)
#
#file(GLOB_RECURSE aud_srcs "./src/*.cpp" "./src/*.hpp" "./src/*.c" "./src/*.cc" "./src/*.h")
#
#add_executable(audacious ${aud_srcs})


# set(PKG_CONFIG_PATH /C/GTK/lib/pkgconfig)
# set(ENV{PKG_CONFIG_PATH} "C:/GTK/lib/pkgconfig;C:/libs/lib/pkgconfig")


find_package(PkgConfig REQUIRED)
PKG_CHECK_MODULES(GTK2 REQUIRED glib-2.0 gtk+-2.0)
PKG_CHECK_MODULES(QT5 REQUIRED Qt5Core Qt5Widgets)
# PKG_CHECK_MODULES(AUDACIOUS REQUIRED audacious)
message(STATUS "gtk2 at: ${GTK2_INCLUDE_DIRS}\n and qt at ${QT5_INCLUDE_DIRS}")

find_package(Qt5 COMPONENTS Core Widgets REQUIRED)
include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/dbus ${GTK2_INCLUDE_DIRS} C:/aud/include C:/Qt/5.12.9/mingw73_32/include
    )

file(GLOB_RECURSE aud_srcs "./src/*.cpp" "./src/*.hpp" "./src/*.c" "./src/*.cc" "./src/*.h")

add_library(audaciousPlugins ${aud_srcs})
target_link_libraries(audaciousPlugins Qt5::Core Qt5::Widgets)


# THUMBBAR ADDED. Currently off.
#set(hotkeyw32src ./src/hotkey/gui.cc ./src/hotkey/plugin.cc ./src/hotkeyw32/w32_hotkey.cc ./src/hotkey/hotkey_api_common.cc
#    ./src/hotkeyw32/res/thumb_icons_map.rc ./src/hotkeyw32/thumb_buttons.cc
#    ../audacious-plugins_simpleAF/src/thirdparty/d_custom_logger.cc
#    )


set(hotkeyw32src ./src/hotkey/gui.cc ./src/hotkey/grab.cc  ./src/hotkey/plugin.cc ./src/hotkeyw32/w32_hotkey.cc ./src/hotkeyw32/w32_grab.cc ./src/hotkey/hotkey_api_common.cc
    ../audacious-plugins_simpleAF/src/thirdparty/d_custom_logger.cc
    )


add_library(hotkeyw32 SHARED ${hotkeyw32src})
target_link_libraries(hotkeyw32 audtag audcore audgui ${GTK2_LIBRARIES} Qt5::Core )
target_compile_definitions(hotkeyw32 PRIVATE BUILT_FROM_CMAKE)

#include_directories (${GTK2_INCLUDE_DIRS})
#link_directories (${GTK2_LIBRARY_DIRS})
#add_executable (gtk-test main.c)
#add_definitions (${GTK2_CFLAGS_OTHER})
#target_link_libraries (gtk-test ${GTK2_LIBRARIES})

# MY EXPERIMENTAL CONFIG: ./configure --prefix=/C/audgtk CPPFLAGS="-DDEBUG" CXXFLAGS="-O0 -g3"  CFLAGS="-O0 -g3" --enable-gtk=yes --enable-qt=no --enable-hotkeyw32=yes && make -j20 && make install


# different cmake:
#    find_package(PkgConfig REQUIRED)
#    PKG_CHECK_MODULES(GTK2 REQUIRED glib-2.0 gtk+-2.0)
#    # PKG_CHECK_MODULES(QT5 REQUIRED Qt5Core Qt5Widgets)
#    message(STATUS "gtk2 at: ${GTK2_INCLUDE_DIRS}\n and qt at ${QT5_INCLUDE_DIRS}")
#
#    find_package(Qt5 COMPONENTS Core Widgets REQUIRED)
#
#    # message(WARNING qt5 "inc ${Qt5_INCLUDE_DIR}")
#
#    include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/dbus ${GTK2_INCLUDE_DIRS} C:/aud/include C:/Qt/5.12.9/mingw73_32/include
#        )
#
#    file(GLOB_RECURSE aud_srcs "./src/*.cpp" "./src/*.hpp" "./src/*.c" "./src/*.cc" "./src/*.h")
#
#    add_library(audaciousPlugins ${aud_srcs})
#    target_link_libraries(audaciousPlugins Qt5::Core Qt5::Widgets)