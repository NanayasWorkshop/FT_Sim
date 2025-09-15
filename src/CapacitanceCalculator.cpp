#include "CapacitanceCalculator.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <algorithm>

// Constants
constexpr double FARADS_TO_PICOFARADS = 1e12;

// Static member initialization
const std::vector<std::string> CapacitanceCalculator::POSITIVE_MODEL_NAMES = {
    "A1_model", "A2_model", "B1_model", "B2_model", "C1_model", "C2_model"
};

CapacitanceCalculator::CapacitanceCalculator() 
    : device(nullptr), transformManager(nullptr)
{
}

CapacitanceCalculator::~CapacitanceCalculator()
{
    cleanup();
}

bool CapacitanceCalculator::initialize(const std::vector<Model>& models, TransformManager& transformMgr)
{
    std::cout << "Initializing CapacitanceCalculator..." << std::endl;
    
    allModels = models;
    transformManager = &transformMgr;
    
    // Setup Embree device
    if (!setupEmbreeDevice()) {
        std::cerr << "Failed to setup Embree device" << std::endl;
        return false;
    }
    
    // Setup model pairings
    setupModelPairings();
    
    // Extract transformed geometry
    if (!extractTransformedGeometry()) {
        std::cerr << "Failed to extract transformed geometry" << std::endl;
        return false;
    }
    
    // Create Embree scenes
    if (!createEmbreeScenes()) {
        std::cerr << "Failed to create Embree scenes" << std::endl;
        return false;
    }
    
    std::cout << "CapacitanceCalculator initialized successfully" << std::endl;
    printModelInfo();
    
    return true;
}

std::vector<CapacitanceResult> CapacitanceCalculator::calculateCapacitances()
{
    std::cout << "\nCalculating capacitances for all positive models..." << std::endl;
    
    std::vector<CapacitanceResult> results;
    
    for (const std::string& modelName : POSITIVE_MODEL_NAMES) {
        CapacitanceResult result = calculateSingleCapacitance(modelName);
        results.push_back(result);
        
        // Convert to picofarads for display
        double capacitancePF = result.capacitance * FARADS_TO_PICOFARADS;
        
        std::cout << "Model " << modelName << ": " << std::scientific << std::setprecision(3) 
                  << capacitancePF << " pF (" << result.hitCount << "/" 
                  << result.triangleCount << " hits)" << std::endl;
    }
    
    return results;
}

CapacitanceResult CapacitanceCalculator::calculateSingleCapacitance(const std::string& positiveModelName)
{
    CapacitanceResult result;
    result.modelName = positiveModelName;
    result.capacitance = 0.0;
    result.triangleCount = 0;
    result.hitCount = 0;
    result.averageDistance = 0.0;
    
    // Find the triangles for this model
    auto triangleIt = positiveTriangles.find(positiveModelName);
    if (triangleIt == positiveTriangles.end()) {
        std::cerr << "No triangles found for model: " << positiveModelName << std::endl;
        return result;
    }
    
    // Find the corresponding scene
    auto sceneIt = scenes.find(positiveModelName);
    if (sceneIt == scenes.end()) {
        std::cerr << "No scene found for model: " << positiveModelName << std::endl;
        return result;
    }
    
    const std::vector<Triangle>& triangles = triangleIt->second;
    RTCScene scene = sceneIt->second;
    
    result.triangleCount = triangles.size();
    
    double totalCapacitance = 0.0;
    double totalDistance = 0.0;
    size_t hitCount = 0;
    
    // Process each triangle
    for (const Triangle& triangle : triangles) {
        double contribution = shootRayAndCalculateContribution(triangle, scene);
        if (contribution > 0.0) {
            totalCapacitance += contribution;
            hitCount++;
            // Note: We'd need to modify shootRayAndCalculateContribution to return distance for averaging
        }
    }
    
    result.capacitance = totalCapacitance;
    result.hitCount = hitCount;
    result.averageDistance = hitCount > 0 ? totalDistance / hitCount : 0.0;
    
    return result;
}

void CapacitanceCalculator::printResults(const std::vector<CapacitanceResult>& results) const
{
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "CAPACITANCE CALCULATION RESULTS" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    
    double totalCapacitance = 0.0;
    
    for (const auto& result : results) {
        // Convert to picofarads for display
        double capacitancePF = result.capacitance * FARADS_TO_PICOFARADS;
        
        std::cout << std::left << std::setw(12) << result.modelName << ": ";
        std::cout << std::fixed << std::setprecision(5) << std::setw(12) << capacitancePF << " pF";
        std::cout << " (Hits: " << std::setw(4) << result.hitCount << "/" << std::setw(4) << result.triangleCount << ")";
        
        if (result.triangleCount > 0) {
            double hitRate = 100.0 * result.hitCount / result.triangleCount;
            std::cout << " [" << std::fixed << std::setprecision(1) << hitRate << "%]";
        }
        
        std::cout << std::endl;
        totalCapacitance += result.capacitance;
    }
    
    // Convert total to picofarads for display
    double totalCapacitancePF = totalCapacitance * FARADS_TO_PICOFARADS;
    
    std::cout << std::string(80, '-') << std::endl;
    std::cout << std::left << std::setw(12) << "TOTAL" << ": ";
    std::cout << std::fixed << std::setprecision(5) << totalCapacitancePF << " pF" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
}

bool CapacitanceCalculator::setupEmbreeDevice()
{
    device = rtcNewDevice(nullptr);
    if (!device) {
        std::cerr << "Failed to create Embree device" << std::endl;
        return false;
    }
    
    // Set error handler
    rtcSetDeviceErrorFunction(device, [](void* userPtr, RTCError error, const char* str) {
        std::cerr << "Embree error " << error << ": " << str << std::endl;
    }, nullptr);
    
    std::cout << "Embree device created successfully" << std::endl;
    return true;
}

void CapacitanceCalculator::setupModelPairings()
{
    // Setup the positive -> negative model pairings
    modelPairings["A1_model"] = "stationary_negative_A";
    modelPairings["A2_model"] = "stationary_negative_A";
    modelPairings["B1_model"] = "stationary_negative_B";
    modelPairings["B2_model"] = "stationary_negative_B";
    modelPairings["C1_model"] = "stationary_negative_C";
    modelPairings["C2_model"] = "stationary_negative_C";
    
    std::cout << "Model pairings configured:" << std::endl;
    for (const auto& pair : modelPairings) {
        std::cout << "  " << pair.first << " -> " << pair.second << std::endl;
    }
}

bool CapacitanceCalculator::extractTransformedGeometry()
{
    std::cout << "Extracting transformed geometry..." << std::endl;
    
    // Extract positive model triangles
    for (const std::string& modelName : POSITIVE_MODEL_NAMES) {
        // Find the model
        auto modelIt = std::find_if(allModels.begin(), allModels.end(),
                                   [&modelName](const Model& m) { return m.name == modelName; });
        
        if (modelIt == allModels.end()) {
            std::cerr << "Model not found: " << modelName << std::endl;
            return false;
        }
        
        // Get transformation matrix
        glm::mat4 transform = transformManager->getCombinedTransform(modelName);
        
        // Extract triangles
        std::vector<Triangle> triangles = extractTrianglesFromModel(*modelIt, transform);
        positiveTriangles[modelName] = triangles;
        
        std::cout << "  " << modelName << ": " << triangles.size() << " triangles" << std::endl;
    }
    
    return true;
}

bool CapacitanceCalculator::createEmbreeScenes()
{
    std::cout << "Creating Embree scenes..." << std::endl;
    
    for (const auto& pairing : modelPairings) {
        const std::string& positiveModel = pairing.first;
        const std::string& negativeModel = pairing.second;
        
        // Create scene
        RTCScene scene = rtcNewScene(device);
        
        // Find negative model
        auto negModelIt = std::find_if(allModels.begin(), allModels.end(),
                                      [&negativeModel](const Model& m) { return m.name == negativeModel; });
        
        if (negModelIt == allModels.end()) {
            std::cerr << "Negative model not found: " << negativeModel << std::endl;
            return false;
        }
        
        // Get transformation for negative model
        glm::mat4 negTransform = transformManager->getCombinedTransform(negativeModel);
        
        // Create geometry for negative model
        RTCGeometry geom = createEmbreeGeometry(*negModelIt, negTransform);
        if (!geom) {
            std::cerr << "Failed to create geometry for: " << negativeModel << std::endl;
            return false;
        }
        
        // Attach geometry to scene
        rtcAttachGeometry(scene, geom);
        rtcReleaseGeometry(geom);
        
        // Commit scene
        rtcCommitScene(scene);
        
        scenes[positiveModel] = scene;
        std::cout << "  Created scene for " << positiveModel << " -> " << negativeModel << std::endl;
    }
    
    return true;
}

std::vector<Triangle> CapacitanceCalculator::extractTrianglesFromModel(const Model& model, const glm::mat4& transform)
{
    std::vector<Triangle> triangles;
    
    // Process triangles (assuming indices represent triangles)
    for (size_t i = 0; i < model.indices.size(); i += 3) {
        if (i + 2 >= model.indices.size()) break;
        
        // Get vertex indices
        unsigned int idx0 = model.indices[i];
        unsigned int idx1 = model.indices[i + 1];
        unsigned int idx2 = model.indices[i + 2];
        
        // Get vertices (assuming 3 floats per vertex)
        if ((idx0 + 1) * 3 > model.vertices.size() ||
            (idx1 + 1) * 3 > model.vertices.size() ||
            (idx2 + 1) * 3 > model.vertices.size()) {
            continue; // Skip invalid indices
        }
        
        glm::vec3 v0(model.vertices[idx0 * 3], model.vertices[idx0 * 3 + 1], model.vertices[idx0 * 3 + 2]);
        glm::vec3 v1(model.vertices[idx1 * 3], model.vertices[idx1 * 3 + 1], model.vertices[idx1 * 3 + 2]);
        glm::vec3 v2(model.vertices[idx2 * 3], model.vertices[idx2 * 3 + 1], model.vertices[idx2 * 3 + 2]);
        
        // Apply transformation
        v0 = applyTransform(v0, transform);
        v1 = applyTransform(v1, transform);
        v2 = applyTransform(v2, transform);
        
        // Create triangle
        Triangle triangle;
        triangle.v0 = v0;
        triangle.v1 = v1;
        triangle.v2 = v2;
        triangle.center = (v0 + v1 + v2) / 3.0f;
        triangle.normal = calculateTriangleNormal(v0, v1, v2);
        triangle.area = calculateTriangleArea(v0, v1, v2);
        
        triangles.push_back(triangle);
    }
    
    return triangles;
}

RTCGeometry CapacitanceCalculator::createEmbreeGeometry(const Model& model, const glm::mat4& transform)
{
    RTCGeometry geom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);
    
    // Count valid triangles
    size_t triangleCount = model.indices.size() / 3;
    size_t vertexCount = model.vertices.size() / 3;
    
    // Allocate vertex buffer
    float* vertices = (float*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, 3 * sizeof(float), vertexCount);
    
    // Copy and transform vertices
    for (size_t i = 0; i < vertexCount; i++) {
        glm::vec3 v(model.vertices[i * 3], model.vertices[i * 3 + 1], model.vertices[i * 3 + 2]);
        v = applyTransform(v, transform);
        
        vertices[i * 3] = v.x;
        vertices[i * 3 + 1] = v.y;
        vertices[i * 3 + 2] = v.z;
    }
    
    // Allocate index buffer
    unsigned int* indices = (unsigned int*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, 3 * sizeof(unsigned int), triangleCount);
    
    // Copy indices
    for (size_t i = 0; i < model.indices.size(); i++) {
        indices[i] = model.indices[i];
    }
    
    rtcCommitGeometry(geom);
    return geom;
}

double CapacitanceCalculator::shootRayAndCalculateContribution(const Triangle& triangle, RTCScene scene)
{
    // Shoot ray in both directions along normal
    double totalContribution = 0.0;
    
    for (int direction = -1; direction <= 1; direction += 2) { // -1 and +1
        RTCRayHit rayhit;
        rayhit.ray.org_x = triangle.center.x;
        rayhit.ray.org_y = triangle.center.y;
        rayhit.ray.org_z = triangle.center.z;
        rayhit.ray.dir_x = triangle.normal.x * direction;
        rayhit.ray.dir_y = triangle.normal.y * direction;
        rayhit.ray.dir_z = triangle.normal.z * direction;
        rayhit.ray.tnear = 0.0f;
        rayhit.ray.tfar = MAX_RAY_DISTANCE;
        rayhit.ray.mask = -1;
        rayhit.ray.flags = 0;
        rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
        rayhit.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;
        
        // Intersect ray with scene
        rtcIntersect1(scene, &rayhit);
        
        // Check if we hit something
        if (rayhit.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
            float distance = rayhit.ray.tfar; // Distance to hit point in mm
            
            // Calculate capacitance contribution: C = ε₀ * εᵣ * A / d
            // Convert area from mm² to m²: multiply by 1e-6
            // Distance is already in mm, convert to m: multiply by 1e-3
            double areaInM2 = triangle.area * 1e-6;
            double distanceInM = distance * 1e-3;
            
            if (distanceInM > 0.0) {
                double contribution = EPSILON_0 * GLYCERIN_RELATIVE_PERMITTIVITY * areaInM2 / distanceInM;
                totalContribution += contribution;
            }
        }
    }
    
    return totalContribution;
}

glm::vec3 CapacitanceCalculator::calculateTriangleNormal(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2)
{
    glm::vec3 edge1 = v1 - v0;
    glm::vec3 edge2 = v2 - v0;
    glm::vec3 normal = glm::cross(edge1, edge2);
    return glm::length(normal) > 0.0f ? glm::normalize(normal) : glm::vec3(0.0f, 0.0f, 1.0f);
}

float CapacitanceCalculator::calculateTriangleArea(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2)
{
    glm::vec3 edge1 = v1 - v0;
    glm::vec3 edge2 = v2 - v0;
    return 0.5f * glm::length(glm::cross(edge1, edge2));
}

glm::vec3 CapacitanceCalculator::applyTransform(const glm::vec3& vertex, const glm::mat4& transform)
{
    glm::vec4 transformed = transform * glm::vec4(vertex, 1.0f);
    return glm::vec3(transformed.x, transformed.y, transformed.z);
}

void CapacitanceCalculator::printModelInfo() const
{
    std::cout << "\nModel Information:" << std::endl;
    std::cout << "Positive models: " << positiveTriangles.size() << std::endl;
    for (const auto& pair : positiveTriangles) {
        std::cout << "  " << pair.first << ": " << pair.second.size() << " triangles" << std::endl;
    }
    std::cout << "Embree scenes: " << scenes.size() << std::endl;
}

void CapacitanceCalculator::cleanup()
{
    // Release scenes
    for (auto& pair : scenes) {
        if (pair.second) {
            rtcReleaseScene(pair.second);
        }
    }
    scenes.clear();
    
    // Release device
    if (device) {
        rtcReleaseDevice(device);
        device = nullptr;
    }
    
    // Clear data
    positiveTriangles.clear();
    negativeGeoms.clear();
    modelPairings.clear();
}