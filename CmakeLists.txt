cmake_minimum_required(VERSION 3.14)
project(network_project C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Set source directory
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# First try to find system-installed libuv
find_package(PkgConfig)
if(PkgConfig_FOUND)
    pkg_check_modules(LIBUV QUIET libuv)
endif()

# If not found, download and build libuv
if(NOT LIBUV_FOUND)
    include(FetchContent)
    
    FetchContent_Declare(
        libuv
        GIT_REPOSITORY https://github.com/libuv/libuv.git
        GIT_TAG v1.48.0
    )
    
    FetchContent_GetProperties(libuv)
    if(NOT libuv_POPULATED)
        FetchContent_Populate(libuv)
        add_subdirectory(${libuv_SOURCE_DIR} ${libuv_BINARY_DIR} EXCLUDE_FROM_ALL)
    endif()
    
    set(LIBUV_INCLUDE_DIRS ${libuv_SOURCE_DIR}/include)
    set(LIBUV_LIBRARIES uv_a)
endif()

# Add executables with path to source files
add_executable(server ${SRC_DIR}/server.c)
add_executable(client ${SRC_DIR}/client.c)

# Include directories
target_include_directories(server PRIVATE ${LIBUV_INCLUDE_DIRS})
target_include_directories(client PRIVATE ${LIBUV_INCLUDE_DIRS})

# Link libraries
target_link_libraries(server PRIVATE ${LIBUV_LIBRARIES})
target_link_libraries(client PRIVATE ${LIBUV_LIBRARIES})