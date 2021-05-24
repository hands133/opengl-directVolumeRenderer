cmake_minimum_required(VERSION 3.4.1)

project(tinyVR
        VERSION 0.0.1
        DESCRIPTION "A tiny renderer for volume rendering"
        LANGUAGES CXX)
    
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMakeTargets")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED OFF)
set(CMAKE_CXX_EXTENSIONS OFF)

set(ThirdLIB_BASE_DIR "include/vendor/")
set(INCLUDE_BASE_DIR "include/utility/")
set(SOURCE_BASE_DIR "src/")

# warp code in utility as a static library
set(SOURCES ${SOURCE_BASE_DIR}main.cpp)
aux_source_directory(${SOURCE_BASE_DIR}/utility/ SRC_LIST)
add_library(libVR STATIC ${SRC_LIST})

target_include_directories(libVR
    PUBLIC ${INCLUDE_BASE_DIR})

add_executable(tinyVR  ${SOURCE_BASE_DIR}main.cpp)


add_subdirectory(${ThirdLIB_BASE_DIR}glfw)
option(BUILD_STATIC_LIBS "Build shared libraries" On)
option(GLFW_BUILD_EXAMPLES "Build the GLFW example programs" OFF)
option(GLFW_BUILD_TESTS "Build the GLFW test programs" OFF)
option(GLFW_BUILD_DOCS "Build the GLFW documentation" OFF)
option(GLFW_INSTALL "Generate installation target" OFF)

add_subdirectory(${ThirdLIB_BASE_DIR}glm)

add_subdirectory(${ThirdLIB_BASE_DIR}glad)

target_link_libraries(tinyVR
    PRIVATE libVR)

target_link_libraries(libVR
    PUBLIC glfw
    PUBLIC glm_static
    PUBLIC glad)