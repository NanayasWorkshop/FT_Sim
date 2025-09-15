#include "Camera.h"
#include <algorithm>
#include <iostream>

// Constructor
Camera::Camera(glm::vec3 position, glm::vec3 target, glm::vec3 up)
    : Target(target)
    , WorldUp(up)
    , MouseSensitivity(SENSITIVITY)
    , Zoom(ZOOM)
    , MinDistance(1.0f)
    , MaxDistance(100.0f)
{
    // Calculate initial orbital parameters
    glm::vec3 direction = position - target;
    Distance = glm::length(direction);
    
    // Calculate yaw and pitch from initial position
    direction = glm::normalize(direction);
    Yaw = atan2(direction.x, direction.z) * 180.0f / 3.14159f;
    Pitch = asin(direction.y) * 180.0f / 3.14159f;
    
    // Set initial position
    Position = position;
    
    // Update camera vectors
    updateCameraVectors();
}

// Returns the view matrix
glm::mat4 Camera::getViewMatrix()
{
    return glm::lookAt(Position, Target, Up);
}

// Returns the projection matrix
glm::mat4 Camera::getProjectionMatrix(float screenWidth, float screenHeight, float nearPlane, float farPlane)
{
    float aspect = screenWidth / screenHeight;
    return glm::perspective(glm::radians(Zoom), aspect, nearPlane, farPlane);
}

// Process mouse movement for orbital camera
void Camera::processMouseMovement(float xoffset, float yoffset, bool constrainPitch)
{
    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;

    Yaw += xoffset;
    Pitch += yoffset;

    // Constrain pitch to avoid gimbal lock
    if (constrainPitch)
    {
        if (Pitch > 89.0f)
            Pitch = 89.0f;
        if (Pitch < -89.0f)
            Pitch = -89.0f;
    }

    // Update camera position and vectors
    updateCameraPosition();
    updateCameraVectors();
}

// Process mouse scroll for zoom
void Camera::processMouseScroll(float yoffset)
{
    Distance -= yoffset * 0.5f; // Zoom speed
    
    // Constrain zoom distance
    if (Distance < MinDistance)
        Distance = MinDistance;
    if (Distance > MaxDistance)
        Distance = MaxDistance;
    
    // Update camera position
    updateCameraPosition();
    updateCameraVectors();
}

// Reset camera to default position
void Camera::reset()
{
    Distance = 15.0f;
    Yaw = -45.0f;
    Pitch = 35.0f; // Isometric-like angle
    Target = glm::vec3(0.0f, 0.0f, 0.0f);
    
    updateCameraPosition();
    updateCameraVectors();
}

// Calculate camera position based on orbital parameters
void Camera::updateCameraPosition()
{
    // Convert spherical coordinates to cartesian
    float yawRad = glm::radians(Yaw);
    float pitchRad = glm::radians(Pitch);
    
    float x = Distance * cos(pitchRad) * sin(yawRad);
    float y = Distance * sin(pitchRad);
    float z = Distance * cos(pitchRad) * cos(yawRad);
    
    Position = Target + glm::vec3(x, y, z);
}

// Update camera vectors
void Camera::updateCameraVectors()
{
    // Calculate the front vector (from camera to target)
    glm::vec3 front = glm::normalize(Target - Position);
    
    // Calculate the right vector
    Right = glm::normalize(glm::cross(front, WorldUp));
    
    // Calculate the up vector
    Up = glm::normalize(glm::cross(Right, front));
}