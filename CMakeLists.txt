cmake_minimum_required(VERSION 3.14)
project(SimpleAF__AudaciousPlugins)
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
# PKG_CHECK_MODULES(QT5 REQUIRED Qt5Core Qt5Widgets)
message(STATUS "gtk2 at: ${GTK2_INCLUDE_DIRS}\n and qt at ${QT5_INCLUDE_DIRS}")

PKG_CHECK_MODULES(AUDACIOUS REQUIRED audacious)
# PKG_CHECK_MODULES(QT5 REQUIRED Qt5Core Qt5Widgets)
message(STATUS "a at: ${AUDACIOUS_INCLUDE_DIRS}\n AUDACIOUS_LIBRARIES ${AUDACIOUS_LIBRARIES}\n AUDACIOUS__LINK_LIBRARIES
 ${AUDACIOUS__LINK_LIBRARIES}\n AUDACIOUS_LIBRARY_DIRS ${AUDACIOUS_LIBRARY_DIRS}\n AUDACIOUS_LDFLAGS_OTHER ${AUDACIOUS_LDFLAGS_OTHER}\n AUDACIOUS_LIBRARIES ${AUDACIOUS_LIBRARIES}\n  ")

find_package(Qt5 COMPONENTS Core Widgets REQUIRED)

# message(WARNING qt5 "inc ${Qt5_INCLUDE_DIR}")

include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/dbus ${GTK2_INCLUDE_DIRS} C:/aud/include C:/Qt/5.12.9/mingw73_32/include
    )

file(GLOB_RECURSE aud_srcs "./src/*.cpp" "./src/*.hpp" "./src/*.c" "./src/*.cc" "./src/*.h")

add_library(qthotkey SHARED ${aud_srcs})

target_link_directories(qthotkey PRIVATE ${AUDACIOUS_LIBRARY_DIRS})

target_link_libraries(qthotkey libaudtag audcore audqt Qt5::Core Qt5::Widgets)
set_target_properties(qthotkey PROPERTIES OUTPUT_NAME qthotkey )

#get_target_property(qthotkeyDLL qthotkey OUTPUT_NAME)
#message(STATUS "making LIBRARY_OUTPUT_NAME ${qthotkeyDLL}")

install(TARGETS qthotkey RUNTIME DESTINATION lib/audacious/General)


#<XXX>_LINK_LIBRARIES
#the libraries and their absolute paths
#
#<XXX>_LIBRARY_DIRS
#the paths of the libraries (without the ‘-L’)
#
#<XXX>_LDFLAGS
#all required linker flags
#
#<XXX>_LDFLAGS_OTHER
#all other linker flags
#
#<XXX>_INCLUDE_DIRS
#the ‘-I’ preprocessor flags (without the ‘-I’)
#
#<XXX>_CFLAGS
#all required cflags
#
#<XXX>_CFLAGS_OTHER
#the other compiler flags
#
#All but <XXX>_FOUND may be a ;-list if the associated variable returned from pkg-config has multiple values.

