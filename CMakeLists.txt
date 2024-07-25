cmake_minimum_required(VERSION 3.12.0)

project("MangoesInTahiti") # Just one more big score bro, I swear.

# Source files
add_executable(${PROJECT_NAME} "src/main.cpp" "src/vksys.cpp")

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)

add_subdirectory("external/glfw")

target_include_directories(${PROJECT_NAME} PUBLIC "include" PUBLIC "$ENV{VULKAN_SDK}/Include" PUBLIC "external/glfw/include")

target_link_directories(${PROJECT_NAME} PRIVATE "$ENV{VULKAN_SDK}/Lib")

target_link_libraries(${PROJECT_NAME} vulkan-1 glfw)