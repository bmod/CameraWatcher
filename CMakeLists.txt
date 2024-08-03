cmake_minimum_required(VERSION 3.18)
project(CameraWatcher)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

include(FetchContent)

FetchContent_Declare(gphoto2
        GIT_REPOSITORY https://github.com/gphoto/libgphoto2.git
        GIT_TAG v2.5.31)
FetchContent_MakeAvailable(gphoto2)

find_package(Qt5 COMPONENTS
        Core
        Gui
        Widgets
        REQUIRED)

file(GLOB_RECURSE SRC src/*.cpp)
file(GLOB_RECURSE RES res/*.qrc)

add_executable(${PROJECT_NAME} ${SRC} ${RES})

target_link_libraries(${PROJECT_NAME}
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        udev
        gphoto2
)
