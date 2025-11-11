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

include(FetchContent)

# -------------------------------
# Third-party: nlohmann_json
# -------------------------------
set(SAM_THIRD_PARTY_DIR "${PROJECT_SOURCE_DIR}/third_party")

option(SAM_USE_SYSTEM_NLOHMANN_JSON
       "Use system / package-manager nlohmann_json if available"
       ON)

set(SAM_NLOHMANN_JSON_INCLUDE_DIR "")
set(SAM_NLOHMANN_JSON_TARGET "")

if(SAM_USE_SYSTEM_NLOHMANN_JSON)
  # Try a config-mode package (vcpkg / Conan / distro).
  find_package(nlohmann_json 3.11 QUIET CONFIG)
endif()

if(nlohmann_json_FOUND)
  message(STATUS "Using system nlohmann_json")
  set(SAM_NLOHMANN_JSON_TARGET nlohmann_json::nlohmann_json)
else()
  message(STATUS "Using vendored nlohmann_json single-header")
  set(SAM_NLOHMANN_JSON_INCLUDE_DIR "${SAM_THIRD_PARTY_DIR}/nlohmann")
endif()

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
  "${PROJECT_SOURCE_DIR}/src/vis/entities/*.cpp"
  "${PROJECT_SOURCE_DIR}/src/vis/models/*.cpp"
  "${PROJECT_SOURCE_DIR}/src/vis/shaders/*.cpp"
  "${PROJECT_SOURCE_DIR}/third_party/stb_image/stb_image.cpp"
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
  ${SAM_NLOHMANN_JSON_INCLUDE_DIR}
)

if(SAM_NLOHMANN_JSON_TARGET)
  target_link_libraries(sam_sim_headless PRIVATE
    ${SAM_NLOHMANN_JSON_TARGET}
  )
endif()

# (No APP_WITH_VIEWER macro; code will run headless fast path)
enable_sanitizers(sam_sim_headless)

# -------------------------------
# Interactive binary (with viewer)
# -------------------------------
# GL deps (GLFW via FetchContent)
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

# glad (vendored)
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
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb_image
  ${SAM_NLOHMANN_JSON_INCLUDE_DIR}
)

target_compile_definitions(sam_sim PRIVATE APP_WITH_VIEWER=1)

if(SAM_NLOHMANN_JSON_TARGET)
  target_link_libraries(sam_sim PRIVATE
    ${SAM_NLOHMANN_JSON_TARGET}
  )
endif()

target_link_libraries(sam_sim PRIVATE
  glad
  glfw
  OpenGL::GL
)

if(UNIX AND NOT APPLE)
  target_link_libraries(sam_sim PRIVATE dl pthread)
endif()

enable_sanitizers(sam_sim)
