#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Camera.h"
#include "ModelManager.h"
#include "Render.h"
#include "Transform.h"
#include "CapacitanceCalculator.h"  // NEW

// Window settings
const unsigned int WINDOW_WIDTH = 1200;
const unsigned int WINDOW_HEIGHT = 800;
const char* WINDOW_TITLE = "OBJ Viewer - FT_Sim with Group Transformations & Capacitance";

// Global objects
Camera* camera = nullptr;
ModelManager* modelManager = nullptr;
Render* renderer = nullptr;
TransformManager* transformManager = nullptr;
CapacitanceCalculator* capacitanceCalculator = nullptr;  // NEW

// Input state
bool wireframeMode = false;
bool firstMouse = true;
double lastX = WINDOW_WIDTH / 2.0;
double lastY = WINDOW_HEIGHT / 2.0;

// Forward declarations
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void processInput(GLFWwindow* window);

int main()
{
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Configure GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4); // Enable MSAA

    // Create window
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (window == NULL) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Set callbacks
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);

    // Capture mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    // Load OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Print OpenGL version
    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE); // Enable MSAA

    // Initialize components
    try {
        camera = new Camera(glm::vec3(10.0f, 10.0f, 10.0f)); // Isometric starting position
        renderer = new Render();
        modelManager = new ModelManager();
        transformManager = new TransformManager();
        capacitanceCalculator = new CapacitanceCalculator();  // NEW

        // Load all OBJ models
        if (!modelManager->loadAllModels("models/")) {
            std::cerr << "Failed to load models" << std::endl;
            return -1;
        }

        // Assign models to transformation groups
        modelManager->assignModelGroups(*transformManager);

        // Initialize renderer with loaded models
        if (!renderer->initialize(modelManager->getModels())) {
            std::cerr << "Failed to initialize renderer" << std::endl;
            return -1;
        }

        // Initialize capacitance calculator
        if (!capacitanceCalculator->initialize(modelManager->getModels(), *transformManager)) {
            std::cerr << "Failed to initialize capacitance calculator" << std::endl;
            return -1;
        }

        // Print transformation info
        transformManager->printGroupTransforms();

        std::cout << "Successfully loaded " << modelManager->getModelCount() << " models" << std::endl;
        std::cout << "\nControls:" << std::endl;
        std::cout << "- Mouse drag: Rotate camera" << std::endl;
        std::cout << "- Mouse wheel: Zoom in/out" << std::endl;
        std::cout << "- SPACE: Toggle wireframe/solid mode" << std::endl;
        std::cout << "- C: Calculate capacitance" << std::endl;  // NEW
        std::cout << "- ESC: Exit" << std::endl;
        std::cout << "\nGroup Transformations Applied:" << std::endl;
        std::cout << "- TAG (A1, A2): 15° X-axis rotation + 2mm Y translation" << std::endl;
        std::cout << "- TBG (B1, B2): 15° Y-axis rotation + 3mm Z translation" << std::endl;
        std::cout << "- TCG (C1, C2): 15° Z-axis rotation + 2mm X translation" << std::endl;
        std::cout << "- Negativ: No transformation (identity)" << std::endl;

        // Calculate initial capacitance
        std::cout << "\nCalculating initial capacitance..." << std::endl;
        std::vector<CapacitanceResult> results = capacitanceCalculator->calculateCapacitances();
        capacitanceCalculator->printResults(results);

    } catch (const std::exception& e) {
        std::cerr << "Initialization error: " << e.what() << std::endl;
        return -1;
    }

    // Render loop
    while (!glfwWindowShouldClose(window)) {
        // Process input
        processInput(window);

        // Clear screen
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Get matrices
        glm::mat4 view = camera->getViewMatrix();
        glm::mat4 projection = camera->getProjectionMatrix(WINDOW_WIDTH, WINDOW_HEIGHT);

        // Render all models with group transformations
        renderer->render(view, projection, *transformManager, wireframeMode);

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    delete camera;
    delete modelManager;
    delete renderer;
    delete transformManager;
    delete capacitanceCalculator;  // NEW

    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    // Only process mouse movement if left button is pressed
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        float xoffset = static_cast<float>(xpos - lastX);
        float yoffset = static_cast<float>(lastY - ypos); // Reversed since y-coordinates go from bottom to top

        camera->processMouseMovement(xoffset, yoffset);
    }

    lastX = xpos;
    lastY = ypos;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera->processMouseScroll(static_cast<float>(yoffset));
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_ESCAPE:
                glfwSetWindowShouldClose(window, true);
                break;
            case GLFW_KEY_SPACE:
                wireframeMode = !wireframeMode;
                std::cout << "Wireframe mode: " << (wireframeMode ? "ON" : "OFF") << std::endl;
                break;
            case GLFW_KEY_C:  // NEW: Calculate capacitance
                if (capacitanceCalculator) {
                    std::cout << "\nRecalculating capacitance..." << std::endl;
                    std::vector<CapacitanceResult> results = capacitanceCalculator->calculateCapacitances();
                    capacitanceCalculator->printResults(results);
                }
                break;
        }
    }
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}