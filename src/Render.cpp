#include "Render.h"
#include <glad/glad.h>
#include <iostream>
#include <fstream>
#include <sstream>

Render::Render() : shaderProgram(0), axesVAO(0), axesVBO(0), axesInitialized(false)
{
}

Render::~Render()
{
    cleanup();
}

bool Render::initialize(const std::vector<Model>& models)
{
    std::cout << "Initializing renderer..." << std::endl;
    
    // Load and compile shaders
    if (!loadShaders()) {
        std::cerr << "Failed to load shaders" << std::endl;
        return false;
    }
    
    // Setup coordinate axes
    setupCoordinateAxes();
    
    // Copy models and set up their buffers
    renderModels = models;
    for (size_t i = 0; i < renderModels.size(); i++) {
        setupModelBuffers(renderModels[i]);
    }
    
    std::cout << "Renderer initialized successfully" << std::endl;
    return true;
}

void Render::render(const glm::mat4& view, const glm::mat4& projection, 
                   TransformManager& transformManager, bool wireframe)
{
    // Use shader program
    glUseProgram(shaderProgram);
    
    // Set matrices
    int viewLoc = glGetUniformLocation(shaderProgram, "view");
    int projLoc = glGetUniformLocation(shaderProgram, "projection");
    int modelLoc = glGetUniformLocation(shaderProgram, "model");
    int colorLoc = glGetUniformLocation(shaderProgram, "color");
    
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projection[0][0]);
    
    // Set wireframe mode
    if (wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glLineWidth(2.0f); // Make lines thicker and more visible
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    
    // Render coordinate axes first
    renderCoordinateAxes(view, projection);
    
    // Render each model with proper transformation order
    for (const Model& model : renderModels) {
        if (model.VAO == 0) continue; // Skip if not properly initialized
        
        // Get combined transformation matrix (includes all positioning and transformations)
        glm::mat4 finalModelMatrix = transformManager.getCombinedTransform(model.name);
        
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &finalModelMatrix[0][0]);
        
        // Set model color (darker for wireframe, but not too dark)
        glm::vec3 renderColor = wireframe ? getDarkerColor(model.color) : model.color;
        glUniform3fv(colorLoc, 1, &renderColor[0]);
        
        // Bind and draw
        glBindVertexArray(model.VAO);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(model.indices.size()), GL_UNSIGNED_INT, 0);
    }
    
    glBindVertexArray(0);
    
    // Reset line width
    if (wireframe) {
        glLineWidth(1.0f);
    }
}

void Render::cleanup()
{
    // Clean up model buffers
    for (Model& model : renderModels) {
        cleanupModelBuffers(model);
    }
    
    // Clean up coordinate axes
    cleanupCoordinateAxes();
    
    // Clean up shader program
    if (shaderProgram != 0) {
        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
    }
}

glm::vec3 Render::getDarkerColor(const glm::vec3& color)
{
    // Make colors darker but not too dark for wireframe mode (multiply by 0.8 instead of 0.6)
    glm::vec3 darkerColor = color * 0.8f;
    
    // Ensure minimum brightness so wireframes are always visible
    float minBrightness = 0.3f;
    if (darkerColor.r < minBrightness && darkerColor.g < minBrightness && darkerColor.b < minBrightness) {
        // If too dark, boost the dominant color channel
        if (color.r >= color.g && color.r >= color.b) {
            darkerColor.r = std::max(darkerColor.r, minBrightness);
        } else if (color.g >= color.r && color.g >= color.b) {
            darkerColor.g = std::max(darkerColor.g, minBrightness);
        } else {
            darkerColor.b = std::max(darkerColor.b, minBrightness);
        }
    }
    
    return darkerColor;
}

glm::mat4 Render::calculateFinalTransform(const glm::mat4& view, const glm::mat4& projection,
                                         const glm::mat4& groupTransform, const glm::vec3& modelPosition)
{
    // Calculate the complete transformation chain
    glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), modelPosition);
    return projection * view * groupTransform * modelMatrix;
}

bool Render::loadShaders()
{
    // Try to load shaders from files first
    std::string vertexSource = loadShaderFromFile("shaders/vertex.glsl");
    std::string fragmentSource = loadShaderFromFile("shaders/fragment.glsl");
    
    // If file loading failed, use default shaders
    if (vertexSource.empty()) {
        std::cout << "Using default vertex shader" << std::endl;
        vertexSource = getDefaultVertexShader();
    }
    
    if (fragmentSource.empty()) {
        std::cout << "Using default fragment shader" << std::endl;
        fragmentSource = getDefaultFragmentShader();
    }
    
    // Create shader program
    shaderProgram = createShaderProgram(vertexSource, fragmentSource);
    
    return shaderProgram != 0;
}

unsigned int Render::compileShader(const std::string& source, unsigned int type)
{
    unsigned int shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);
    
    checkShaderCompileErrors(shader, (type == GL_VERTEX_SHADER) ? "VERTEX" : "FRAGMENT");
    
    return shader;
}

unsigned int Render::createShaderProgram(const std::string& vertexSource, const std::string& fragmentSource)
{
    unsigned int vertexShader = compileShader(vertexSource, GL_VERTEX_SHADER);
    unsigned int fragmentShader = compileShader(fragmentSource, GL_FRAGMENT_SHADER);
    
    if (vertexShader == 0 || fragmentShader == 0) {
        return 0;
    }
    
    unsigned int program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    
    checkProgramLinkErrors(program);
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return program;
}

void Render::setupModelBuffers(Model& model)
{
    // Generate buffers
    glGenVertexArrays(1, &model.VAO);
    glGenBuffers(1, &model.VBO);
    glGenBuffers(1, &model.EBO);
    
    // Bind VAO
    glBindVertexArray(model.VAO);
    
    // Bind and fill VBO
    glBindBuffer(GL_ARRAY_BUFFER, model.VBO);
    glBufferData(GL_ARRAY_BUFFER, model.vertices.size() * sizeof(float), model.vertices.data(), GL_STATIC_DRAW);
    
    // Bind and fill EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, model.indices.size() * sizeof(unsigned int), model.indices.data(), GL_STATIC_DRAW);
    
    // Set vertex attributes (position only)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindVertexArray(0);
}

void Render::cleanupModelBuffers(Model& model)
{
    if (model.VAO != 0) {
        glDeleteVertexArrays(1, &model.VAO);
        model.VAO = 0;
    }
    if (model.VBO != 0) {
        glDeleteBuffers(1, &model.VBO);
        model.VBO = 0;
    }
    if (model.EBO != 0) {
        glDeleteBuffers(1, &model.EBO);
        model.EBO = 0;
    }
}

std::string Render::loadShaderFromFile(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cout << "Could not open shader file: " << filePath << std::endl;
        return "";
    }
    
    std::stringstream stream;
    stream << file.rdbuf();
    file.close();
    
    return stream.str();
}

void Render::checkShaderCompileErrors(unsigned int shader, const std::string& type)
{
    int success;
    char infoLog[1024];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 1024, NULL, infoLog);
        std::cerr << "Shader compilation error (" << type << "): " << infoLog << std::endl;
    }
}

void Render::checkProgramLinkErrors(unsigned int program)
{
    int success;
    char infoLog[1024];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 1024, NULL, infoLog);
        std::cerr << "Program linking error: " << infoLog << std::endl;
    }
}

std::string Render::getDefaultVertexShader()
{
    return R"(#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";
}

std::string Render::getDefaultFragmentShader()
{
    return R"(#version 330 core
out vec4 FragColor;

uniform vec3 color;

void main()
{
    FragColor = vec4(color, 1.0);
}
)";
}

void Render::setupCoordinateAxes()
{
    // Define coordinate axes (short thick ones, 10 units length each)
    float axisLength = 10.0f;
    float axesVertices[] = {
        // X-axis (Red)
        0.0f, 0.0f, 0.0f,  axisLength, 0.0f, 0.0f,
        // Y-axis (Green)  
        0.0f, 0.0f, 0.0f,  0.0f, axisLength, 0.0f,
        // Z-axis (Blue)
        0.0f, 0.0f, 0.0f,  0.0f, 0.0f, axisLength
    };
    
    glGenVertexArrays(1, &axesVAO);
    glGenBuffers(1, &axesVBO);
    
    glBindVertexArray(axesVAO);
    glBindBuffer(GL_ARRAY_BUFFER, axesVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(axesVertices), axesVertices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindVertexArray(0);
    axesInitialized = true;
}

void Render::renderCoordinateAxes(const glm::mat4& view, const glm::mat4& projection)
{
    if (!axesInitialized) return;
    
    // Get uniform locations
    int modelLoc = glGetUniformLocation(shaderProgram, "model");
    int colorLoc = glGetUniformLocation(shaderProgram, "color");
    
    // Set model matrix to identity (axes at origin)
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &modelMatrix[0][0]);
    
    // Temporarily set line mode and increase line width
    GLenum currentPolygonMode[2];
    glGetIntegerv(GL_POLYGON_MODE, (GLint*)currentPolygonMode);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glLineWidth(3.0f);
    
    glBindVertexArray(axesVAO);
    
    // Draw X-axis (Red)
    glUniform3f(colorLoc, 1.0f, 0.0f, 0.0f);
    glDrawArrays(GL_LINES, 0, 2);
    
    // Draw Y-axis (Green)
    glUniform3f(colorLoc, 0.0f, 1.0f, 0.0f);
    glDrawArrays(GL_LINES, 2, 2);
    
    // Draw Z-axis (Blue)
    glUniform3f(colorLoc, 0.0f, 0.0f, 1.0f);
    glDrawArrays(GL_LINES, 4, 2);
    
    glBindVertexArray(0);
    
    // Restore previous polygon mode and line width
    glPolygonMode(GL_FRONT_AND_BACK, currentPolygonMode[0]);
    glLineWidth(1.0f);
}

void Render::cleanupCoordinateAxes()
{
    if (axesVAO != 0) {
        glDeleteVertexArrays(1, &axesVAO);
        axesVAO = 0;
    }
    if (axesVBO != 0) {
        glDeleteBuffers(1, &axesVBO);
        axesVBO = 0;
    }
    axesInitialized = false;
}