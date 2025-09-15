cmake_minimum_required(VERSION 3.16)
project(FT_Sim)

# Set C++ standard (required for tinyobjloader and filesystem)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find vcpkg packages
find_package(glfw3 REQUIRED)
find_package(glad REQUIRED)
find_package(glm REQUIRED)
find_package(OpenGL REQUIRED)

# Embree setup for manual installation
set(EMBREE_ROOT_DIR "C:/embree")
set(EMBREE_INCLUDE_DIRS "${EMBREE_ROOT_DIR}/include")
set(EMBREE_LIBRARIES "${EMBREE_ROOT_DIR}/lib/embree4.lib")

# Check if Embree files exist
if(NOT EXISTS "${EMBREE_INCLUDE_DIRS}/embree4/rtcore.h")
    message(FATAL_ERROR "Embree headers not found at ${EMBREE_INCLUDE_DIRS}")
endif()

if(NOT EXISTS "${EMBREE_LIBRARIES}")
    message(FATAL_ERROR "Embree library not found at ${EMBREE_LIBRARIES}")
endif()

# Include directories
include_directories(include)
include_directories(src)
include_directories(${EMBREE_INCLUDE_DIRS})

# Source files
set(SOURCES
    main.cpp
    src/Camera.cpp
    src/ObjLoader.cpp
    src/ModelManager.cpp
    src/Render.cpp
    src/Transform.cpp
    src/CapacitanceCalculator.cpp
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME} 
    glfw
    glad::glad
    glm::glm
    OpenGL::GL
    ${EMBREE_LIBRARIES}
)

# Windows specific settings
if(WIN32)
    # Copy DLLs to output directory if needed
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set_target_properties(${PROJECT_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Debug)
    else()
        set_target_properties(${PROJECT_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Release)
    endif()
    
    # Copy Embree DLL to output directory
    if(EXISTS "${EMBREE_ROOT_DIR}/bin/embree4.dll")
        configure_file("${EMBREE_ROOT_DIR}/bin/embree4.dll" 
                       "${CMAKE_BINARY_DIR}/embree4.dll" COPYONLY)
    endif()
endif()

# Make sure we can find shader files
configure_file(${CMAKE_SOURCE_DIR}/shaders/vertex.glsl 
               ${CMAKE_BINARY_DIR}/shaders/vertex.glsl COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/shaders/fragment.glsl 
               ${CMAKE_BINARY_DIR}/shaders/fragment.glsl COPYONLY)

# Copy models directory to build directory
file(COPY ${CMAKE_SOURCE_DIR}/models/ 
     DESTINATION ${CMAKE_BINARY_DIR}/models/)