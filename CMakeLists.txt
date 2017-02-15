cmake_minimum_required(VERSION 2.6)

# Download Chad Vernon's cgcmake package (https://github.com/chadmv/cgcmake/)
# and make sure your CMAKE_MODULES_PATH environment variable points at it.

set(CMAKE_MODULE_PATH "$ENV{CMAKE_MODULE_PATH}")
    
project(polySymmetry)   
    file(GLOB SOURCE_FILES "src/*.cpp" "src/*.h")
    find_package(Maya REQUIRED) 

    if (MAYA_VERSION STRLESS 2016)
        message(FATAL_ERROR "${PROJECT_NAME} must be compiled with Maya 2016 or newer")
    endif()

    include_directories(${MAYA_INCLUDE_DIR})
    link_directories(${MAYA_LIBRARY_DIR})

    add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})
    target_link_libraries(${PROJECT_NAME} ${MAYA_LIBRARIES})
    
    MAYA_PLUGIN(${PROJECT_NAME})

