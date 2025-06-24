cmake_minimum_required(VERSION 3.10)
project(RaylibBulletDemo)

set(CMAKE_CXX_STANDARD 17)

find_package(raylib REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(BULLET REQUIRED bullet)

include_directories(${BULLET_INCLUDE_DIRS})
link_directories(${BULLET_LIBRARY_DIRS})

add_executable(demo main.cpp)

target_link_libraries(demo raylib ${BULLET_LIBRARIES})

# macOS specific: link frameworks
if(APPLE)
    target_link_libraries(demo "-framework IOKit" "-framework Cocoa" "-framework OpenGL" "-framework CoreVideo" "-framework CoreGraphics")
endif()