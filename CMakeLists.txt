cmake_minimum_required(VERSION 3.12.0)

project("MangoesInTahiti") # Just one more big score bro, I swear.

# Source files
add_executable(${PROJECT_NAME} "src/main.cpp" "src/vulkan.cpp" "src/window.cpp" "src/render.cpp")

# List of all shaders
set(SHADER_SOURCES
    "shader.vert"
    "shader.frag"
)

if (NOT EXISTS "${CMAKE_BINARY_DIR}/shaders")
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/shaders")
endif()

# Compile all shaders using glslc upon each build
set(SHADER_TARGET_INDEX 0)
foreach(SHADER ${SHADER_SOURCES})
    add_custom_target("shader_build_${SHADER_TARGET_INDEX}"
        COMMAND glslc "${PROJECT_SOURCE_DIR}/shaders/${SHADER}" -o "${CMAKE_BINARY_DIR}/shaders/${SHADER}.spv"
        COMMENT "Compile shader module ${SHADER}"
    )
    add_dependencies(${PROJECT_NAME} "shader_build_${SHADER_TARGET_INDEX}")
    math(EXPR SHADER_TARGET_INDEX "${SHADER_TARGET_INDEX} + 1")
endforeach()

# C++20 build
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)

# External dependencies
add_subdirectory("external/glfw")
target_include_directories(${PROJECT_NAME} PUBLIC "include" PUBLIC "$ENV{VULKAN_SDK}/Include" PUBLIC "external/glfw/include")
target_link_directories(${PROJECT_NAME} PRIVATE "$ENV{VULKAN_SDK}/Lib")
target_link_libraries(${PROJECT_NAME} vulkan-1 glfw)