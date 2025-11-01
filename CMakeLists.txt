cmake_minimum_required(VERSION 3.20)
project(sam_sim LANGUAGES C CXX)

# 0. Essentials
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 1. Viewer switches
option(BUILD_VIEWER "Build OpenGL viewer" ON)
option(USE_FETCHCONTENT "Fetch third-party deps for viewer" ON)

# 2. Core sources
file(GLOB_RECURSE SIM_CORE_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/sim/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/modules/*.cpp"
)

# Exclude viewer sources from core (we’ll add them conditionally later)
list(FILTER SIM_CORE_SRC EXCLUDE REGEX ".*/src/vis/.*")

add_executable(sam_sim ${SIM_CORE_SRC})
target_include_directories(sam_sim PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 3. nlohmann_json via FetchContent (already in your project; ensure it’s here)
include(FetchContent)
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG        v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)
target_link_libraries(sam_sim PRIVATE nlohmann_json::nlohmann_json)

# 4. Viewer (stub wiring only in Phase 0)
if(BUILD_VIEWER)
  message(STATUS "Viewer ENABLED")

  include(FetchContent)

  # --- GLFW ---
  FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
  )
  set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(glfw)

  # --- GLAD (vendored, generated from web tool) ---
  add_library(glad STATIC
    ${CMAKE_SOURCE_DIR}/third_party/glad/src/glad.c
  )
  target_include_directories(glad PUBLIC
    ${CMAKE_SOURCE_DIR}/third_party/glad/include
  )

  # --- Viewer core (Renderer + Camera + Shader) as a library ---
  add_library(viewer_core STATIC
    ${CMAKE_SOURCE_DIR}/src/vis/Renderer.cpp
    ${CMAKE_SOURCE_DIR}/src/vis/Shader.cpp
    ${CMAKE_SOURCE_DIR}/src/vis/Camera.cpp
  )
  target_include_directories(viewer_core PUBLIC
    ${CMAKE_SOURCE_DIR}/include
  )

  find_package(OpenGL REQUIRED)

  target_link_libraries(viewer_core PUBLIC
    glad
    glfw
    OpenGL::GL
  )
  if(UNIX AND NOT APPLE)
    target_link_libraries(viewer_core PUBLIC dl pthread)
  endif()

  # --- Smoke app links viewer_core ---
  add_executable(sam_viewer_smoke
    ${CMAKE_SOURCE_DIR}/src/vis/main_viewer_smoke.cpp
  )
  target_link_libraries(sam_viewer_smoke PRIVATE viewer_core)

  # --- IMPORTANT: link sam_sim with viewer_core too ---
  target_link_libraries(sam_sim PRIVATE viewer_core)
endif()