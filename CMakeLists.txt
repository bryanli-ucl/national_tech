cmake_minimum_required(VERSION 3.28)

project(national_technology VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)


add_library(glad STATIC 
    external/glad/src/glad.c
)
target_include_directories(glad PUBLIC 
    external/glad/include
)

# add source files
file(GLOB_RECURSE SOURCES 
    "src/*.cpp"
    "src/*.hpp"
    "src/*.c"
    "src/*.h"
)

include_directories(external)
include_directories(src)

add_executable(${PROJECT_NAME} ${SOURCES})

# link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    glad
    glm::glm
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/resources $<TARGET_FILE_DIR:${PROJECT_NAME}>/resources
)

target_compile_options(${PROJECT_NAME} PRIVATE 
    -Wall 
    -Wextra 
    -pedantic
)

