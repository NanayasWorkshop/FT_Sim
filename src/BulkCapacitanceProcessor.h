#ifndef BULKCAPACITANCEPROCESSOR_H
#define BULKCAPACITANCEPROCESSOR_H

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "CapacitanceCalculator.h"
#include "Transform.h"

// Structure to hold sphere position data for one row
struct SpherePositions {
    glm::vec3 A, B, C;  // Positions of the three spheres
};

// Structure to hold one row of CSV data for a group
struct GroupRowData {
    SpherePositions offsets;  // Relative offsets from resting positions
};

// Structure to hold all CSV data for one group
struct GroupCSVData {
    std::vector<GroupRowData> rows;
    std::string groupName;
};

// Structure to hold coordinate system (UVW or IJK)
struct CoordinateSystem {
    glm::vec3 origin;  // Center point
    glm::vec3 U, V, W; // Basis vectors
};

class BulkCapacitanceProcessor
{
public:
    BulkCapacitanceProcessor();
    ~BulkCapacitanceProcessor();

    // Original bulk processing function
    bool processCSVFiles(const std::string& csvDirectory, 
                        CapacitanceCalculator& capacitanceCalculator,
                        TransformManager& transformManager);

    // NEW: Step mode functions
    bool initializeStepMode(const std::string& csvDirectory);
    bool stepToRow(size_t rowNumber, TransformManager& transformManager);
    size_t getCurrentRow() const;
    size_t getMaxRows() const;
    void printCurrentRowInfo() const;

private:
    // NEW: Individual file loading methods
    bool loadGroupFromIndividualFiles(const std::string& csvDirectory, const std::string& groupName, GroupCSVData& groupData);
    bool loadIndividualSphereFile(const std::string& filePath, std::vector<glm::vec3>& sphereOffsets);
    bool parseIndividualSphereRow(const std::string& line, glm::vec3& offset);

    // Original CSV loading methods (kept for compatibility)
    bool loadCSVFile(const std::string& filePath, GroupCSVData& groupData);
    bool parseCSVRow(const std::string& line, GroupRowData& rowData);
    std::vector<std::string> splitCSVLine(const std::string& line);

    // Geometry calculations
    glm::vec3 calculateCircumcenter(const glm::vec3& A, const glm::vec3& B, const glm::vec3& C);
    CoordinateSystem createCoordinateSystem(const glm::vec3& A, const glm::vec3& B, const glm::vec3& C, 
                                           char referencePoint); // 'A', 'B', or 'C'
    
    // Rigid body transformation
    glm::mat4 calculateRigidBodyTransform(const CoordinateSystem& from, const CoordinateSystem& to);
    
    // Sphere position management
    SpherePositions getRestingPositions(const std::string& groupName);
    SpherePositions addOffsets(const SpherePositions& resting, const SpherePositions& offsets);
    
    // Output
    bool saveResults(const std::vector<std::vector<CapacitanceResult>>& allResults, 
                    const std::string& outputPath);
    
    // Helper functions
    void resetTransformations(TransformManager& transformManager);
    void printDetailedDebugInfo(size_t row, TransformManager& transformManager);
    std::string trim(const std::string& str);
    
    // Data storage
    GroupCSVData tagData, tbgData, tcgData;
    size_t maxRows;
    
    // NEW: Step mode state
    size_t currentStepRow;
    bool stepModeActive;
};

#endif