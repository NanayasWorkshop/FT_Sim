#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Camera.h"
#include "ModelManager.h"
#include "Render.h"
#include "Transform.h"
#include "CapacitanceCalculator.h"
#include "BulkCapacitanceProcessor.h"

// Window settings
const unsigned int WINDOW_WIDTH = 1200;
const unsigned int WINDOW_HEIGHT = 800;
const char* WINDOW_TITLE = "OBJ Viewer - FT_Sim with Step Mode Debug";

// Global objects
Camera* camera = nullptr;
ModelManager* modelManager = nullptr;
Render* renderer = nullptr;
TransformManager* transformManager = nullptr;
CapacitanceCalculator* capacitanceCalculator = nullptr;
BulkCapacitanceProcessor* bulkProcessor = nullptr;

// Input state
bool wireframeMode = false;
bool firstMouse = true;
double lastX = WINDOW_WIDTH / 2.0;
double lastY = WINDOW_HEIGHT / 2.0;

// Step mode state
bool stepMode = false;
size_t currentRow = 0;
size_t maxRows = 0;
bool stepModeInitialized = false;

// Forward declarations
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void processInput(GLFWwindow* window);
bool runBulkCapacitanceProcessing();
bool initializeStepMode();
void stepToRow(size_t row);
void printStepModeInfo();

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
        capacitanceCalculator = new CapacitanceCalculator();
        bulkProcessor = new BulkCapacitanceProcessor();

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
        std::cout << "- C: Calculate single capacitance" << std::endl;
        std::cout << "- S: Initialize step mode" << std::endl;
        std::cout << "- N: Next row (step mode)" << std::endl;
        std::cout << "- P: Previous row (step mode)" << std::endl;
        std::cout << "- B: Run bulk capacitance processing from CSV files" << std::endl;
        std::cout << "- ESC: Exit" << std::endl;

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
    delete capacitanceCalculator;
    delete bulkProcessor;

    glfwTerminate();
    return 0;
}

bool initializeStepMode()
{
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "INITIALIZING STEP MODE" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    std::string csvDirectory = "csv_data";
    
    if (!bulkProcessor->initializeStepMode(csvDirectory)) {
        std::cerr << "Failed to initialize step mode" << std::endl;
        return false;
    }
    
    maxRows = bulkProcessor->getMaxRows();
    currentRow = 0;
    stepModeInitialized = true;
    stepMode = true;
    
    std::cout << "Step mode initialized with " << maxRows << " rows" << std::endl;
    std::cout << "Starting at row 0 (resting positions)" << std::endl;
    
    // Set to resting positions initially
    stepToRow(0);
    
    return true;
}

void stepToRow(size_t row)
{
    if (!stepModeInitialized || !bulkProcessor) {
        std::cerr << "Step mode not initialized" << std::endl;
        return;
    }
    
    if (row >= maxRows) {
        std::cerr << "Row " << row << " out of range (max: " << maxRows - 1 << ")" << std::endl;
        return;
    }
    
    currentRow = row;
    
    std::cout << "\n" << std::string(40, '-') << std::endl;
    std::cout << "STEPPING TO ROW " << currentRow << "/" << (maxRows - 1) << std::endl;
    std::cout << std::string(40, '-') << std::endl;
    
    if (!bulkProcessor->stepToRow(currentRow, *transformManager)) {
        std::cerr << "Failed to step to row " << currentRow << std::endl;
        return;
    }
    
    bulkProcessor->printCurrentRowInfo();
    
    std::cout << "Row " << currentRow << " applied successfully" << std::endl;
}

void printStepModeInfo()
{
    if (!stepMode) {
        std::cout << "Step mode not active" << std::endl;
        return;
    }
    
    std::cout << "\n=== STEP MODE STATUS ===" << std::endl;
    std::cout << "Current row: " << currentRow << "/" << (maxRows - 1) << std::endl;
    std::cout << "Total rows: " << maxRows << std::endl;
    std::cout << "Initialized: " << (stepModeInitialized ? "YES" : "NO") << std::endl;
    std::cout << "========================" << std::endl;
}

bool runBulkCapacitanceProcessing()
{
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "STARTING BULK CAPACITANCE PROCESSING" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    std::string csvDirectory = "csv_data";
    
    if (!bulkProcessor->processCSVFiles(csvDirectory, *capacitanceCalculator, *transformManager)) {
        std::cerr << "Bulk processing failed" << std::endl;
        return false;
    }
    
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "BULK CAPACITANCE PROCESSING COMPLETED" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    return true;
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
            case GLFW_KEY_C:  // Single capacitance calculation
                if (capacitanceCalculator) {
                    std::cout << "\nCalculating single capacitance..." << std::endl;
                    std::vector<CapacitanceResult> results = capacitanceCalculator->calculateCapacitances();
                    capacitanceCalculator->printResults(results);
                }
                break;
            case GLFW_KEY_S:  // Initialize step mode
                std::cout << "\nInitializing step mode..." << std::endl;
                initializeStepMode();
                break;
            case GLFW_KEY_N:  // Next row in step mode
                if (stepMode && stepModeInitialized) {
                    if (currentRow < maxRows - 1) {
                        stepToRow(currentRow + 1);
                    } else {
                        std::cout << "Already at last row (" << currentRow << ")" << std::endl;
                    }
                } else {
                    std::cout << "Step mode not active. Press 'S' to initialize." << std::endl;
                }
                break;
            case GLFW_KEY_P:  // Previous row in step mode
                if (stepMode && stepModeInitialized) {
                    if (currentRow > 0) {
                        stepToRow(currentRow - 1);
                    } else {
                        std::cout << "Already at first row (0)" << std::endl;
                    }
                } else {
                    std::cout << "Step mode not active. Press 'S' to initialize." << std::endl;
                }
                break;
            case GLFW_KEY_B:  // Bulk capacitance processing
                std::cout << "\nStarting bulk capacitance processing..." << std::endl;
                runBulkCapacitanceProcessing();
                break;
        }
    }
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}