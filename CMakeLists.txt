cmake_minimum_required(VERSION 3.15)
project (sam_sim_skeleton LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_compile_options(-Wall -Wextra -Wpedantic -Wconversion)

include(FetchContent)
FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(nlohmann_json)

include_directories(${PROJECT_SOURCE_DIR}/include)

file(GLOB SRC
    src/*.cpp
    src/sim/*.cpp
    src/modules/*.cpp
)

add_executable(sam_sim ${SRC})
target_link_libraries(sam_sim PRIVATE nlohmann_json::nlohmann_json)