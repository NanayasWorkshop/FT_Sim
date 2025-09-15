#ifndef RENDER_H
#define RENDER_H

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "ModelManager.h"
#include "Transform.h"

// OpenGL renderer class
class Render
{
public:
    Render();
    ~Render();

    // Initialize renderer with models
    bool initialize(const std::vector<Model>& models);

    // Render all models with group transformations
    void render(const glm::mat4& view, const glm::mat4& projection, 
                TransformManager& transformManager, bool wireframe = false);

    // Cleanup resources
    void cleanup();

private:
    // Shader program ID
    unsigned int shaderProgram;
    
    // Model data for rendering
    std::vector<Model> renderModels;

    // Coordinate axes
    unsigned int axesVAO, axesVBO;
    bool axesInitialized;

    // Shader compilation and linking
    bool loadShaders();
    unsigned int compileShader(const std::string& source, unsigned int type);
    unsigned int createShaderProgram(const std::string& vertexSource, const std::string& fragmentSource);
    
    // Buffer management
    void setupModelBuffers(Model& model);
    void cleanupModelBuffers(Model& model);
    
    // Coordinate axes
    void setupCoordinateAxes();
    void renderCoordinateAxes(const glm::mat4& view, const glm::mat4& projection);
    void cleanupCoordinateAxes();
    
    // Color utility
    glm::vec3 getDarkerColor(const glm::vec3& color);
    
    // Transform calculation
    glm::mat4 calculateFinalTransform(const glm::mat4& view, const glm::mat4& projection,
                                     const glm::mat4& groupTransform, const glm::vec3& modelPosition);
    
    // Utility functions
    std::string loadShaderFromFile(const std::string& filePath);
    void checkShaderCompileErrors(unsigned int shader, const std::string& type);
    void checkProgramLinkErrors(unsigned int program);
    
    // Default shader sources (fallback if files not found)
    std::string getDefaultVertexShader();
    std::string getDefaultFragmentShader();
};

#endif