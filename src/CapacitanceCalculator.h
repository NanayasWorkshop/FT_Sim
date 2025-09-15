#ifndef CAPACITANCECALCULATOR_H
#define CAPACITANCECALCULATOR_H

#include <vector>
#include <string>
#include <map>
#include <glm/glm.hpp>
#include <embree4/rtcore.h>
#include "ModelManager.h"
#include "Transform.h"

// Physical constants
constexpr double EPSILON_0 = 8.854e-12; // F/m (vacuum permittivity)
constexpr double GLYCERIN_RELATIVE_PERMITTIVITY = 42.28;
constexpr float MAX_RAY_DISTANCE = 2.0f; // mm

// Structure to hold capacitance calculation results
struct CapacitanceResult {
    std::string modelName;
    double capacitance;           // Farads
    size_t triangleCount;         // Number of triangles processed
    size_t hitCount;             // Number of successful ray hits
    double averageDistance;       // Average hit distance in mm
};

// Triangle data for ray shooting
struct Triangle {
    glm::vec3 v0, v1, v2;        // Triangle vertices
    glm::vec3 center;            // Triangle center
    glm::vec3 normal;            // Surface normal
    float area;                  // Triangle area
};

class CapacitanceCalculator
{
public:
    CapacitanceCalculator();
    ~CapacitanceCalculator();

    // Initialize with models and transformation manager
    bool initialize(const std::vector<Model>& models, TransformManager& transformManager);

    // Calculate capacitance for all 6 positive models
    std::vector<CapacitanceResult> calculateCapacitances();

    // Calculate capacitance for a specific positive model
    CapacitanceResult calculateSingleCapacitance(const std::string& positiveModelName);

    // Print results in formatted way
    void printResults(const std::vector<CapacitanceResult>& results) const;

    // Refresh geometry with current transformations
    void refreshGeometry();

    // Cleanup resources
    void cleanup();

private:
    // Embree objects
    RTCDevice device;
    std::map<std::string, RTCScene> scenes;           // One scene per positive-negative pair
    std::map<std::string, RTCGeometry> negativeGeoms; // Negative geometries

    // Processed model data
    std::map<std::string, std::vector<Triangle>> positiveTriangles;
    std::map<std::string, std::string> modelPairings; // positive -> negative mapping

    // Model data storage
    std::vector<Model> allModels;
    TransformManager* transformManager;

    // Initialization helpers
    bool setupEmbreeDevice();
    bool extractTransformedGeometry();
    bool createEmbreeScenes();
    void setupModelPairings();

    // Geometry processing
    std::vector<Triangle> extractTrianglesFromModel(const Model& model, const glm::mat4& transform);
    RTCGeometry createEmbreeGeometry(const Model& model, const glm::mat4& transform);

    // Ray shooting and calculation
    CapacitanceResult calculateModelCapacitance(const std::string& positiveModelName);
    double shootRayAndCalculateContribution(const Triangle& triangle, RTCScene scene);
    
    // Utility functions
    glm::vec3 calculateTriangleNormal(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2);
    float calculateTriangleArea(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2);
    glm::vec3 applyTransform(const glm::vec3& vertex, const glm::mat4& transform);
    
    // Debug helpers
    void printModelInfo() const;
    void printSceneInfo() const;

    // Constants
    static constexpr size_t POSITIVE_MODEL_COUNT = 6;
    static const std::vector<std::string> POSITIVE_MODEL_NAMES;
};

#endif