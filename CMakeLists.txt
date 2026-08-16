cmake_minimum_required(VERSION 3.13)
project(CenterDot)

# version number
set (CENTER_DOT_VERSION_MAJOR 1)
set (CENTER_DOT_VERSION_MINOR 1)
set (CENTER_DOT_VERSION_REVISION 0)

set(CMAKE_CXX_STANDARD 14)

configure_file (
    "${PROJECT_SOURCE_DIR}/src/version_number.h.in"
    "${PROJECT_SOURCE_DIR}/src/version_number.h"
)

# set directory containing Qt includes and libraries
# pass -DCMAKE_PREFIX_PATH=<qt-dir>/lib/cmake on the command line to override
if(WIN32 AND NOT CMAKE_PREFIX_PATH)
    set(CMAKE_PREFIX_PATH "C:\\Dev\\Qt\\5.12.2\\mingw73_64\\lib\\cmake") # To be modified pointing to Qt CMake libs dir
endif()

# call find_package for required Qt packages
find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Gui REQUIRED)

set(CMAKE_AUTOMOC ON) # Handle the Qt MOC (Meta object code) generation automatically
set(CMAKE_AUTOUIC ON) # Handle the Qt UIC (Forms) code generation automatically
set(CMAKE_AUTORCC ON) # Handle the Qt RCC (Resources) code generation automatically

# the .ui files live next to the sources, tell AUTOUIC where to look
set(CMAKE_AUTOUIC_SEARCH_PATHS ${PROJECT_SOURCE_DIR}/src)

# compiler flags
# note: option -D__NO_INLINE__ necessary due to MinGW bug, can maybe removed later
#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -mwindows -D__NO_INLINE__")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")
if(NOT MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
endif()

# source files to be compiled
set(SOURCE_FILES
    src/main.cpp
    src/crosshairrenderer.cpp
    src/crosshairrenderer.h
    src/mainwindow.cpp
    src/mainwindow.h
    src/settingsdata.h
    src/settingsdialog.cpp
    src/settingsdialog.h
    src/settings.ui
    src/resources.qrc
    src/version_number.h
    src/version_number.h.in
    src/winkeyboardhook.cpp
    src/winkeyboardhook.h)

if(WIN32)
    list(APPEND SOURCE_FILES src/centerdot.rc)
endif()

# set exectuable name, WIN32 keeps the GUI app from opening a console window
add_executable(${CMAKE_PROJECT_NAME} WIN32 ${SOURCE_FILES})

# link against Qt libraries
target_link_libraries(${CMAKE_PROJECT_NAME} Qt5::Core Qt5::Widgets Qt5::Gui)
