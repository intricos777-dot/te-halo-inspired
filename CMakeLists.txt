cmake_minimum_required(VERSION 3.20)
project(te-halo-inspired VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(/home/sin/Projects/games/twilight-elysium ${CMAKE_BINARY_DIR}/twilight-elysium)

# Core game library
add_library(tehi-core STATIC
    src/core/game.cpp
    src/core/world.cpp
    src/core/game_state.cpp
)
target_link_libraries(tehi-core PUBLIC te-engine te-renderer te-asset tehi-player tehi-ui)
target_include_directories(tehi-core PUBLIC include src)

# Player controller
add_library(tehi-player STATIC
    src/player/controller.cpp
    src/player/camera.cpp
)
target_link_libraries(tehi-player PRIVATE tehi-core)

# Weapons
add_library(tehi-weapons STATIC
    src/weapons/weapon.cpp
    src/weapons/inventory.cpp
)
target_link_libraries(tehi-weapons PRIVATE tehi-core)

# Vehicles
add_library(tehi-vehicles STATIC
    src/vehicles/vehicle.cpp
)
target_link_libraries(tehi-vehicles PRIVATE tehi-core)

# AI
add_library(tehi-ai STATIC
    src/ai/bot.cpp
    src/ai/navmesh.cpp
    src/ai/enemy.cpp
)
target_link_libraries(tehi-ai PRIVATE tehi-core)

# Campaign / Co-op
add_library(tehi-campaign STATIC
    src/campaign/mission.cpp
    src/campaign/roster.cpp
    src/campaign/companion.cpp
)
target_include_directories(tehi-campaign PRIVATE src)
target_link_libraries(tehi-campaign PUBLIC tehi-core tehi-audio)

add_library(tehi-multiplayer STATIC
    src/multiplayer/coop.cpp
)
target_include_directories(tehi-multiplayer PRIVATE src)
target_link_libraries(tehi-multiplayer PRIVATE tehi-core tehi-campaign)

# Audio
add_library(tehi-audio STATIC
    src/audio/audio_system.cpp
    src/audio/music.cpp
    src/audio/transition.cpp
    src/audio/voice.cpp
)
target_include_directories(tehi-audio PUBLIC src)
target_link_libraries(tehi-audio PRIVATE tehi-core)

# World/maps
add_library(tehi-world STATIC
    src/world/map.cpp
    src/world/spawner.cpp
)
target_link_libraries(tehi-world PRIVATE tehi-core tehi-ai tehi-weapons)

# UI
add_library(tehi-ui STATIC
    src/ui/hud.cpp
)
target_link_libraries(tehi-ui PRIVATE tehi-core)

# Test runner
enable_testing()
add_executable(tehi-test src/core/main.cpp)
target_include_directories(tehi-test PRIVATE src)
target_link_libraries(tehi-test PRIVATE te-engine tehi-campaign tehi-ui tehi-audio tehi-weapons tehi-world tehi-player)
add_test(NAME tehi-core COMMAND tehi-test)
