cmake_minimum_required(VERSION 4.0)
project(Chip_8EMU)

set(CMAKE_CXX_STANDARD 20)

set(SFML_DIR "G:/c++/librerie/SFML-3.0.2/lib/cmake/SFML")

find_package(SFML 3.0 COMPONENTS Graphics Window System Audio REQUIRED)

add_executable(Chip_8EMU main.cpp)

target_link_libraries(Chip_8EMU
        SFML::Graphics
        SFML::Window
        SFML::System
        SFML::Audio
)
