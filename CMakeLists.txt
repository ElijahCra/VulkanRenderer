cmake_minimum_required(VERSION 3.27)
project(VulkanMagnets)

set(CMAKE_CXX_STANDARD 23)



add_executable(VulkanMagnets main.cpp
        main.cpp)

find_package(Vulkan REQUIRED)
target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan)


include_directories("/opt/homebrew/Cellar/glm/1.0.0/include/")
find_package(glm CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} glm::glm)

include_directories("/opt/homebrew/Cellar/glfw/3.3.9/include/GLFW")
find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} glfw)

