cmake_minimum_required(VERSION 3.1.3)

project(flow)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_PREFIX_PATH "/opt/qt/5.4/gcc_64/lib/cmake") #Qt CMakeFiles location
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

find_package(Qt5 COMPONENTS Core Gui Xml Multimedia WebEngineWidgets)

set(SRCS main.cpp apicomponent.cpp mainwindow.cpp mediacomponent.cpp playerwidget.cpp)
qt5_wrap_ui(UI_SRCS mainwindow.ui playerwidget.ui)
qt5_add_resources(RC_SRC icons.qrc)

add_executable(flow ${SRCS} ${UI_SRCS} ${RC_SRC})

include_directories(${CMAKE_SOURCE_DIR})

target_link_libraries(flow Qt5::Core Qt5::Gui Qt5::Xml Qt5::Multimedia Qt5::WebEngineWidgets)
