cmake_minimum_required(VERSION 3.27)
project(VulkanMagnets)

set(CMAKE_CXX_STANDARD 23)



add_executable(VulkanMagnets main.cpp)

find_package(Vulkan REQUIRED)
target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan)


include_directories("/opt/homebrew/Cellar/glm/1.0.0/include/")
find_package(glm CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} glm::glm)

if(WIN32)
    set(glfw3_DIR "C:/Program Files (x86)/GLFW/lib/cmake/glfw3")
elseif(APPLE)
    set(glfw3_DIR "/opt/homebrew/Cellar/glfw/3.3.9/lib/cmake/glfw3")
endif()
find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} glfw)