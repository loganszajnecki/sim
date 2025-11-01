cmake_minimum_required(VERSION 3.20)
project(sam_sim LANGUAGES C CXX)

# 0) Essentials
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1) Options
option(BUILD_VIEWER "Build OpenGL viewer (Renderer, Shader, Camera)" ON)
option(USE_FETCHCONTENT "Fetch third-party deps" ON)

# 2) Dependencies (one FetchContent block)
include(FetchContent)

# nlohmann_json (always needed)
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG        v3.11.3
)

# GLFW (needed even headless because main uses glfwGetTime())
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.4
)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)

# Materialize always-needed deps
FetchContent_MakeAvailable(nlohmann_json glfw)

# 3) Core sources
file(GLOB_RECURSE SIM_CORE_SRC
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/sim/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/modules/*.cpp"
)
# Exclude vis sources from sam_sim for now (viewer built separately)
list(FILTER SIM_CORE_SRC EXCLUDE REGEX ".*/src/vis/.*")

add_executable(sam_sim ${SIM_CORE_SRC})

target_include_directories(sam_sim PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include/app   # scaffolded in Phase 0
)

target_link_libraries(sam_sim PRIVATE nlohmann_json::nlohmann_json glfw)

# 4) Viewer (only when BUILD_VIEWER=ON)
if(BUILD_VIEWER)
  message(STATUS "Viewer ENABLED")

  # GLAD (vendored)
  add_library(glad STATIC
    ${PROJECT_SOURCE_DIR}/third_party/glad/src/glad.c
  )
  target_include_directories(glad PUBLIC
    ${PROJECT_SOURCE_DIR}/third_party/glad/include
  )

  # Viewer core (Renderer + Shader + Camera + TelemetryBus)
  add_library(viewer_core STATIC
    ${PROJECT_SOURCE_DIR}/src/vis/Renderer.cpp
    ${PROJECT_SOURCE_DIR}/src/vis/Shader.cpp
    ${PROJECT_SOURCE_DIR}/src/vis/Camera.cpp
    ${PROJECT_SOURCE_DIR}/src/vis/TelemetryBus.cpp
  )
  target_include_directories(viewer_core PUBLIC
    ${PROJECT_SOURCE_DIR}/include
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

  # Smoke app for viewer_core
  add_executable(sam_viewer_smoke
    ${PROJECT_SOURCE_DIR}/src/vis/main_viewer_smoke.cpp
  )
  target_link_libraries(sam_viewer_smoke PRIVATE viewer_core)

  # Main sim also links viewer_core
  target_link_libraries(sam_sim PRIVATE viewer_core)
else()
  message(STATUS "Viewer DISABLED")
endif()