cmake_minimum_required(VERSION 3.20)
project(TrickLogic2 VERSION 0.1.0 LANGUAGES CXX)

# PortableKit brings the recompiler, the runtime and the system layer, and
# defines portablekit_add_game().
add_subdirectory(portablekit)

portablekit_add_game(TrickLogic2Native
    PROFILE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
    SOURCES host/tricklogic2_profile.cpp host/season1_import.cpp)
