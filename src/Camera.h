#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Default camera values
const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 2.5f;
const float SENSITIVITY = 0.1f;
const float ZOOM = 45.0f;

// Orbital camera class for CAD-style navigation
class Camera
{
public:
    // Camera attributes
    glm::vec3 Position;
    glm::vec3 Target;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;

    // Orbital camera parameters
    float Distance;
    float Yaw;
    float Pitch;
    
    // Camera options
    float MouseSensitivity;
    float Zoom;
    float MinDistance;
    float MaxDistance;

    // Constructor with vectors
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f), 
           glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f),
           glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f));

    // Returns the view matrix calculated using Euler Angles and LookAt Matrix
    glm::mat4 getViewMatrix();

    // Returns the projection matrix
    glm::mat4 getProjectionMatrix(float screenWidth, float screenHeight, 
                                  float nearPlane = 0.1f, float farPlane = 1000.0f);

    // Processes input received from a mouse input system (for orbital camera)
    void processMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);

    // Processes input received from a mouse scroll-wheel event
    void processMouseScroll(float yoffset);

    // Reset camera to default position
    void reset();

private:
    // Calculates the camera position based on orbital parameters
    void updateCameraPosition();

    // Updates camera vectors from the updated position
    void updateCameraVectors();
};

#endif