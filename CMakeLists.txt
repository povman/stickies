cmake_minimum_required(VERSION 3.16)
project(Stickies VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

add_executable(stickies
    main.cpp
    stickie_card.cpp
    stickie_manager.cpp
)

target_link_libraries(stickies
    Qt6::Core
    Qt6::Widgets
)
