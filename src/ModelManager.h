#ifndef MODELMANAGER_H
#define MODELMANAGER_H

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "Transform.h"

// Structure to hold model data
struct Model {
    std::vector<float> vertices;        // Vertex positions (x, y, z)
    std::vector<unsigned int> indices;  // Face indices
    glm::vec3 color;                   // Model color
    glm::vec3 position;                // Model position in world space
    std::string name;                  // Model name
    SubGroupType subGroupType;         // Sub-group assignment
    ParentGroupType parentGroupType;   // Parent group assignment
    
    // OpenGL buffer objects (will be set by renderer)
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    unsigned int EBO = 0;
    
    // Model statistics
    size_t vertexCount = 0;
    size_t triangleCount = 0;
};

// Class to manage loading and storing multiple OBJ models
class ModelManager
{
public:
    ModelManager();
    ~ModelManager();

    // Load all OBJ files from the specified directory
    bool loadAllModels(const std::string& directory);

    // Load a single OBJ file
    bool loadModel(const std::string& filePath, const std::string& modelName, const glm::vec3& color);

    // Load a single OBJ file at specific position
    bool loadModelAtPosition(const std::string& filePath, const std::string& modelName, 
                            const glm::vec3& color, const glm::vec3& position);

    // Generate sphere geometry
    bool generateSphere(float radius, int subdivisions, std::vector<float>& vertices, std::vector<unsigned int>& indices);

    // Get all loaded models
    const std::vector<Model>& getModels() const;

    // Get model count
    size_t getModelCount() const;

    // Get model by index
    const Model& getModel(size_t index) const;

    // Print model statistics
    void printModelStats() const;

    // Clear all models
    void clear();

    // Group assignment
    void assignModelGroups(TransformManager& transformManager);

private:
    std::vector<Model> models;

    // Predefined colors for models
    std::vector<glm::vec3> modelColors;

    // Initialize predefined colors
    void initializeColors();

    // Get file name without extension
    std::string getFileNameWithoutExtension(const std::string& filePath);

    // Check if file has .obj extension
    bool isObjFile(const std::string& fileName);

    // Get all .obj files in directory
    std::vector<std::string> getObjFilesInDirectory(const std::string& directory);

    // Get model position based on name
    glm::vec3 getModelPosition(const std::string& modelName);

    // Get model color based on name
    glm::vec3 getModelColor(const std::string& modelName);
};

#endif