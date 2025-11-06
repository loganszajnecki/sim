cmake_minimum_required(VERSION 3.20)
project(sam_sim LANGUAGES C CXX)

# -------------------------------
# Essentials
# -------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

option(ENABLE_ASAN "Enable Address/UB sanitizers in Debug builds" ON)
function(enable_sanitizers target)
  if(ENABLE_ASAN AND CMAKE_BUILD_TYPE STREQUAL "Debug" AND CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(${target} PRIVATE -O0 -g -fno-omit-frame-pointer -fsanitize=address,undefined)
    target_link_options(${target} PRIVATE -fsanitize=address,undefined)
  endif()
endfunction()

# -------------------------------
# Third-party: JSON
# -------------------------------
include(FetchContent)
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG        v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# -------------------------------
# Source layout
# -------------------------------
# Core/app sources (shared by both bins)
file(GLOB_RECURSE SAM_CORE_SRC
  "${PROJECT_SOURCE_DIR}/src/*.cpp"
  "${PROJECT_SOURCE_DIR}/src/sim/*.cpp"
  "${PROJECT_SOURCE_DIR}/src/models/*.cpp"
  "${PROJECT_SOURCE_DIR}/src/app/*.cpp"
)
# Filter out viewer/vis files from the shared list
list(FILTER SAM_CORE_SRC EXCLUDE REGEX ".*/src/vis/.*")

# Viewer sources only
file(GLOB_RECURSE SAM_VIS_SRC
  "${PROJECT_SOURCE_DIR}/src/vis/*.cpp"
)

# -------------------------------
# Headless binary (no GL deps)
# -------------------------------
add_executable(sam_sim_headless
  ${SAM_CORE_SRC}
  # main.cpp is part of src/, already in SAM_CORE_SRC
)
target_include_directories(sam_sim_headless PRIVATE
  ${PROJECT_SOURCE_DIR}/include
)
target_link_libraries(sam_sim_headless PRIVATE
  nlohmann_json::nlohmann_json
)
# (No APP_WITH_VIEWER macro; code will run headless fast path)
enable_sanitizers(sam_sim_headless)

# -------------------------------
# Interactive binary (with viewer)
# -------------------------------
# GL deps
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.4
)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# Suppress noisy warnings coming from GLFW's own sources
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
  if(TARGET glfw)
    target_compile_options(glfw PRIVATE
      -Wno-sign-compare
      -Wno-missing-field-initializers
      -Wno-unused-parameter
      -Wno-pedantic
    )
  endif()
endif()

find_package(OpenGL REQUIRED)

add_library(glad STATIC
  ${PROJECT_SOURCE_DIR}/third_party/glad/src/glad.c
)
target_include_directories(glad PUBLIC
  ${PROJECT_SOURCE_DIR}/third_party/glad/include
)

# Suppress pedantic warnings just for glad.c (object ptr -> function ptr casts)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_options(glad PRIVATE -Wno-pedantic)
endif()

add_executable(sam_sim
  ${SAM_CORE_SRC}
  ${SAM_VIS_SRC}
)
target_include_directories(sam_sim PRIVATE
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/third_party/glad/include
)
target_compile_definitions(sam_sim PRIVATE APP_WITH_VIEWER=1)

target_link_libraries(sam_sim PRIVATE
  nlohmann_json::nlohmann_json
  glad
  glfw
  OpenGL::GL
)
if(UNIX AND NOT APPLE)
  target_link_libraries(sam_sim PRIVATE dl pthread)
endif()
enable_sanitizers(sam_sim)
