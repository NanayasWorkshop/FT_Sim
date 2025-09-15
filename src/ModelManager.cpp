#include "ModelManager.h"
#include "ObjLoader.h"
#include <iostream>
#include <filesystem>
#include <algorithm>

ModelManager::ModelManager()
{
    initializeColors();
}

ModelManager::~ModelManager()
{
    clear();
}

bool ModelManager::loadAllModels(const std::string& directory)
{
    std::cout << "Loading models from directory: " << directory << std::endl;
    
    // Get all .obj files in the directory
    std::vector<std::string> objFiles = getObjFilesInDirectory(directory);
    
    if (objFiles.empty()) {
        std::cerr << "No .obj files found in directory: " << directory << std::endl;
        return false;
    }
    
    std::cout << "Found " << objFiles.size() << " .obj files" << std::endl;
    
    // Load each OBJ file with specific positioning and colors
    bool allLoaded = true;
    
    for (const std::string& filePath : objFiles) {
        std::string fileName = getFileNameWithoutExtension(filePath);
        
        // Skip stationary_negative here, we'll load it separately
        if (fileName == "stationary_negative") {
            continue;
        }
        
        // Determine position and color based on model name
        glm::vec3 position = getModelPosition(fileName);
        glm::vec3 color = getModelColor(fileName);
        
        if (!loadModelAtPosition(filePath, fileName, color, position)) {
            std::cerr << "Failed to load model: " << filePath << std::endl;
            allLoaded = false;
        }
    }
    
    // Load 3 copies of stationary_negative at group positions
    std::string stationaryPath = directory;
    if (!stationaryPath.empty() && stationaryPath.back() != '/' && stationaryPath.back() != '\\') {
        stationaryPath += "/";
    }
    stationaryPath += "stationary_negative.obj";
    
    // Load stationary_negative at A group position
    glm::vec3 posA = getModelPosition("A1_model");
    if (!loadModelAtPosition(stationaryPath, "stationary_negative_A", getModelColor("stationary_negative"), posA)) {
        std::cerr << "Failed to load stationary_negative at A position" << std::endl;
        allLoaded = false;
    }
    
    // Load stationary_negative at B group position
    glm::vec3 posB = getModelPosition("B1_model");
    if (!loadModelAtPosition(stationaryPath, "stationary_negative_B", getModelColor("stationary_negative"), posB)) {
        std::cerr << "Failed to load stationary_negative at B position" << std::endl;
        allLoaded = false;
    }
    
    // Load stationary_negative at C group position
    glm::vec3 posC = getModelPosition("C1_model");
    if (!loadModelAtPosition(stationaryPath, "stationary_negative_C", getModelColor("stationary_negative"), posC)) {
        std::cerr << "Failed to load stationary_negative at C position" << std::endl;
        allLoaded = false;
    }
    
    std::cout << "Successfully loaded " << models.size() << " models" << std::endl;
    printModelStats();
    
    return allLoaded && !models.empty();
}

bool ModelManager::loadModel(const std::string& filePath, const std::string& modelName, const glm::vec3& color)
{
    return loadModelAtPosition(filePath, modelName, color, glm::vec3(0.0f));
}

bool ModelManager::loadModelAtPosition(const std::string& filePath, const std::string& modelName, 
                                      const glm::vec3& color, const glm::vec3& position)
{
    Model model;
    model.name = modelName;
    model.color = color;
    model.position = position;
    model.subGroupType = SubGroupType::Individual; // Will be assigned later
    model.parentGroupType = ParentGroupType::Positiv; // Will be assigned later
    
    // Load OBJ file
    if (!ObjLoader::loadOBJ(filePath, model.vertices, model.indices, model.vertexCount, model.triangleCount)) {
        std::cerr << "Failed to load OBJ: " << filePath << std::endl;
        std::cerr << "Error: " << ObjLoader::getLastError() << std::endl;
        return false;
    }
    
    // Add to models list
    models.push_back(model);
    
    std::cout << "Loaded model '" << modelName << "' at position (" 
              << position.x << ", " << position.y << ", " << position.z 
              << ") with color (" << color.r << ", " << color.g << ", " << color.b
              << ") - " << model.vertexCount << " vertices and " 
              << model.triangleCount << " triangles" << std::endl;
    
    return true;
}

void ModelManager::assignModelGroups(TransformManager& transformManager)
{
    std::cout << "Assigning models to hierarchical transformation groups..." << std::endl;
    
    for (Model& model : models) {
        // Assign sub-group
        model.subGroupType = transformManager.getModelSubGroup(model.name);
        
        // Assign parent group based on sub-group
        model.parentGroupType = transformManager.getSubGroupParent(model.subGroupType);
        
        std::cout << "Model '" << model.name << "' assigned to:" << std::endl;
        std::cout << "  Sub-group: " << transformManager.getSubGroupName(model.subGroupType) << std::endl;
        std::cout << "  Parent group: " << transformManager.getParentGroupName(model.parentGroupType) << std::endl;
    }
    
    std::cout << "Hierarchical group assignment complete." << std::endl;
}

const std::vector<Model>& ModelManager::getModels() const
{
    return models;
}

size_t ModelManager::getModelCount() const
{
    return models.size();
}

const Model& ModelManager::getModel(size_t index) const
{
    if (index >= models.size()) {
        throw std::out_of_range("Model index out of range");
    }
    return models[index];
}

void ModelManager::printModelStats() const
{
    if (models.empty()) {
        std::cout << "No models loaded." << std::endl;
        return;
    }
    
    size_t totalVertices = 0;
    size_t totalTriangles = 0;
    
    std::cout << "\n=== Model Statistics ===" << std::endl;
    for (size_t i = 0; i < models.size(); i++) {
        const Model& model = models[i];
        std::cout << i + 1 << ". " << model.name 
                  << " - Vertices: " << model.vertexCount 
                  << ", Triangles: " << model.triangleCount
                  << ", Color: (" << model.color.r << ", " << model.color.g << ", " << model.color.b << ")"
                  << ", Position: (" << model.position.x << ", " << model.position.y << ", " << model.position.z << ")"
                  << std::endl;
        
        totalVertices += model.vertexCount;
        totalTriangles += model.triangleCount;
    }
    
    std::cout << "Total: " << totalVertices << " vertices, " << totalTriangles << " triangles" << std::endl;
    std::cout << "========================\n" << std::endl;
}

void ModelManager::clear()
{
    models.clear();
}

void ModelManager::initializeColors()
{
    // Define a set of distinct colors for the models
    modelColors = {
        glm::vec3(1.0f, 0.0f, 0.0f),   // Red
        glm::vec3(0.0f, 1.0f, 0.0f),   // Green
        glm::vec3(0.0f, 0.0f, 1.0f),   // Blue
        glm::vec3(1.0f, 1.0f, 0.0f),   // Yellow
        glm::vec3(1.0f, 0.0f, 1.0f),   // Magenta
        glm::vec3(0.0f, 1.0f, 1.0f),   // Cyan
        glm::vec3(1.0f, 0.5f, 0.0f),   // Orange
        glm::vec3(0.5f, 0.0f, 1.0f),   // Purple
        glm::vec3(0.0f, 0.5f, 0.0f),   // Dark Green
        glm::vec3(0.8f, 0.8f, 0.8f)    // Light Gray
    };
}

std::string ModelManager::getFileNameWithoutExtension(const std::string& filePath)
{
    size_t lastSlash = filePath.find_last_of("/\\");
    size_t lastDot = filePath.find_last_of('.');
    
    size_t start = (lastSlash == std::string::npos) ? 0 : lastSlash + 1;
    size_t end = (lastDot == std::string::npos) ? filePath.length() : lastDot;
    
    if (end <= start) {
        return filePath;
    }
    
    return filePath.substr(start, end - start);
}

bool ModelManager::isObjFile(const std::string& fileName)
{
    if (fileName.length() < 4) return false;
    
    std::string extension = fileName.substr(fileName.length() - 4);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    return extension == ".obj";
}

std::vector<std::string> ModelManager::getObjFilesInDirectory(const std::string& directory)
{
    std::vector<std::string> objFiles;
    
    try {
        // Use filesystem to iterate through directory
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                std::string fileName = entry.path().filename().string();
                if (isObjFile(fileName)) {
                    objFiles.push_back(entry.path().string());
                }
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
        
        // Fallback: try to manually list known files
        std::vector<std::string> knownFiles = {
            "A1_model.obj", "A2_model.obj", "B1_model.obj", "B2_model.obj",
            "C1_model.obj", "C2_model.obj", "stationary_negative.obj"
        };
        
        for (const std::string& file : knownFiles) {
            std::string fullPath = directory;
            if (!fullPath.empty() && fullPath.back() != '/' && fullPath.back() != '\\') {
                fullPath += "/";
            }
            fullPath += file;
            objFiles.push_back(fullPath);
        }
        
        std::cout << "Using fallback file list with " << objFiles.size() << " files" << std::endl;
    }
    
    // Sort the files for consistent loading order
    std::sort(objFiles.begin(), objFiles.end());
    
    return objFiles;
}

glm::vec3 ModelManager::getModelPosition(const std::string& modelName)
{
    // Radius in mm
    float radius = 24.85f;
    
    // Calculate positions for 120° spacing with correct quadrants
    if (modelName == "A1_model" || modelName == "A2_model") {
        // A1 and A2 at top (90° or Y+)
        return glm::vec3(0.0f, radius, 0.0f);
    }
    else if (modelName == "B1_model" || modelName == "B2_model") {
        // B1 and B2 at bottom-right (-30° or 330°)
        float angle = -30.0f * 3.14159f / 180.0f; // Convert to radians
        float x = radius * cos(angle);
        float y = radius * sin(angle);
        return glm::vec3(x, y, 0.0f);
    }
    else if (modelName == "C1_model" || modelName == "C2_model") {
        // C1 and C2 at bottom-left (-150° or 210°)
        float angle = -150.0f * 3.14159f / 180.0f; // Convert to radians
        float x = radius * cos(angle);
        float y = radius * sin(angle);
        return glm::vec3(x, y, 0.0f);
    }
    else if (modelName == "stationary_negative") {
        // Center position
        return glm::vec3(0.0f, 0.0f, 0.0f);
    }
    
    // Default position
    return glm::vec3(0.0f, 0.0f, 0.0f);
}

glm::vec3 ModelManager::getModelColor(const std::string& modelName)
{
    if (modelName == "A1_model") {
        return glm::vec3(1.0f, 0.0f, 1.0f); // Magenta
    }
    else if (modelName == "A2_model") {
        return glm::vec3(0.0f, 1.0f, 1.0f); // Cyan
    }
    else if (modelName == "B1_model") {
        return glm::vec3(0.0f, 1.0f, 0.0f); // Green
    }
    else if (modelName == "B2_model") {
        return glm::vec3(1.0f, 1.0f, 0.0f); // Yellow
    }
    else if (modelName == "C1_model") {
        return glm::vec3(1.0f, 0.0f, 0.0f); // Red
    }
    else if (modelName == "C2_model") {
        return glm::vec3(0.0f, 0.0f, 1.0f); // Blue
    }
    else if (modelName == "stationary_negative" || 
             modelName == "stationary_negative_A" ||
             modelName == "stationary_negative_B" ||
             modelName == "stationary_negative_C") {
        return glm::vec3(0.7f, 0.7f, 0.7f); // Light gray
    }
    
    // Default color
    return glm::vec3(0.8f, 0.8f, 0.8f);
}