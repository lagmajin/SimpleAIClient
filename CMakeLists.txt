cmake_minimum_required(VERSION 3.16)
project(lagmajinnoqt6 VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)

qt_standard_project_setup()

qt_add_executable(lagmajinnoqt6
    src/main.cpp
    src/mainwindow.cpp
    src/apiclient.cpp
)

target_link_libraries(lagmajinnoqt6 PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
)

target_include_directories(lagmajinnoqt6 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
