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

# Include directories
include_directories(include)
include_directories(src)

# Source files
set(SOURCES
    main.cpp
    src/Camera.cpp
    src/ObjLoader.cpp
    src/ModelManager.cpp
    src/Render.cpp
    src/Transform.cpp
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME} 
    glfw
    glad::glad
    glm::glm
    OpenGL::GL
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
endif()

# Make sure we can find shader files
configure_file(${CMAKE_SOURCE_DIR}/shaders/vertex.glsl 
               ${CMAKE_BINARY_DIR}/shaders/vertex.glsl COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/shaders/fragment.glsl 
               ${CMAKE_BINARY_DIR}/shaders/fragment.glsl COPYONLY)

# Copy models directory to build directory
file(COPY ${CMAKE_SOURCE_DIR}/models/ 
     DESTINATION ${CMAKE_BINARY_DIR}/models/)