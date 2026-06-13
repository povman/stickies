cmake_minimum_required(VERSION 3.16)
project(Stickies VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

qt_add_resources(DICT_RESOURCES dictionaries.qrc)

add_executable(stickies
    main.cpp
    stickie_card.cpp
    stickie_manager.cpp
    spell_highlighter.cpp
    ${DICT_RESOURCES}
)

target_link_libraries(stickies PRIVATE Qt6::Core Qt6::Widgets)

# Correção ortográfica via Hunspell (opcional)
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(HUNSPELL QUIET hunspell)
endif()

if(HUNSPELL_FOUND)
    message(STATUS "Hunspell encontrado — correção ortográfica ativada")
    target_compile_definitions(stickies PRIVATE HAVE_HUNSPELL)
    target_include_directories(stickies PRIVATE ${HUNSPELL_INCLUDE_DIRS})
    target_link_libraries(stickies PRIVATE ${HUNSPELL_LIBRARIES})
else()
    message(STATUS "Hunspell não encontrado — instale hunspell para ativar correção ortográfica")
endif()
