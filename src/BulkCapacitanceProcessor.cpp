#include "BulkCapacitanceProcessor.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <cfloat>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

BulkCapacitanceProcessor::BulkCapacitanceProcessor() : maxRows(0), currentStepRow(0), stepModeActive(false)
{
    resetCentroidStats();
}

BulkCapacitanceProcessor::~BulkCapacitanceProcessor()
{
}

bool BulkCapacitanceProcessor::initializeStepMode(const std::string& csvDirectory)
{
    std::cout << "Initializing step mode with CSV directory: " << csvDirectory << std::endl;
    
    // Load all individual sphere CSV files and combine into group data
    if (!loadGroupFromIndividualFiles(csvDirectory, "TAG", tagData)) {
        std::cerr << "Failed to load TAG group files" << std::endl;
        return false;
    }
    tagData.groupName = "TAG";
    
    if (!loadGroupFromIndividualFiles(csvDirectory, "TBG", tbgData)) {
        std::cerr << "Failed to load TBG group files" << std::endl;
        return false;
    }
    tbgData.groupName = "TBG";
    
    if (!loadGroupFromIndividualFiles(csvDirectory, "TCG", tcgData)) {
        std::cerr << "Failed to load TCG group files" << std::endl;
        return false;
    }
    tcgData.groupName = "TCG";
    
    // Find maximum number of rows
    maxRows = std::max({tagData.rows.size(), tbgData.rows.size(), tcgData.rows.size()});
    
    std::cout << "Loaded CSV files for step mode:" << std::endl;
    std::cout << "  TAG: " << tagData.rows.size() << " rows" << std::endl;
    std::cout << "  TBG: " << tbgData.rows.size() << " rows" << std::endl;
    std::cout << "  TCG: " << tcgData.rows.size() << " rows" << std::endl;
    std::cout << "  Max rows for stepping: " << maxRows << std::endl;
    
    currentStepRow = 0;
    stepModeActive = true;
    
    // Initialize centroid stats with original positions
    resetCentroidStats();
    
    return true;
}

bool BulkCapacitanceProcessor::stepToRow(size_t rowNumber, TransformManager& transformManager)
{
    if (!stepModeActive) {
        std::cerr << "Step mode not initialized" << std::endl;
        return false;
    }
    
    if (rowNumber >= maxRows) {
        std::cerr << "Row " << rowNumber << " out of range (max: " << maxRows - 1 << ")" << std::endl;
        return false;
    }
    
    currentStepRow = rowNumber;
    
    std::cout << "Stepping to row " << currentStepRow << std::endl;
    
    // Get resting positions for each group
    SpherePositions tagResting = getRestingPositions("TAG");
    SpherePositions tbgResting = getRestingPositions("TBG");
    SpherePositions tcgResting = getRestingPositions("TCG");
    
    // Reset transformations to default state
    resetTransformations(transformManager);
    
    // Apply transformations for this row
    if (currentStepRow < tagData.rows.size()) {
        // Calculate TAG transformation
        SpherePositions tagDeformed = addOffsets(tagResting, tagData.rows[currentStepRow].offsets);
        updateCentroidStats("TAG", tagDeformed);
        CoordinateSystem tagUVW = createCoordinateSystem(tagResting.A, tagResting.B, tagResting.C, 'A');
        CoordinateSystem tagIJK = createCoordinateSystem(tagDeformed.A, tagDeformed.B, tagDeformed.C, 'A');
        glm::mat4 tagTransform = calculateRigidBodyTransform(tagUVW, tagIJK);
        
        // Apply TAG transformation to TransformManager
        transformManager.applyCalculatedTransform("TAG", tagTransform);
    }
    
    if (currentStepRow < tbgData.rows.size()) {
        // Calculate TBG transformation
        SpherePositions tbgDeformed = addOffsets(tbgResting, tbgData.rows[currentStepRow].offsets);
        updateCentroidStats("TBG", tbgDeformed);
        CoordinateSystem tbgUVW = createCoordinateSystem(tbgResting.A, tbgResting.B, tbgResting.C, 'B');
        CoordinateSystem tbgIJK = createCoordinateSystem(tbgDeformed.A, tbgDeformed.B, tbgDeformed.C, 'B');
        glm::mat4 tbgTransform = calculateRigidBodyTransform(tbgUVW, tbgIJK);
        
        // Apply TBG transformation to TransformManager
        transformManager.applyCalculatedTransform("TBG", tbgTransform);
    }
    
    if (currentStepRow < tcgData.rows.size()) {
        // Calculate TCG transformation
        SpherePositions tcgDeformed = addOffsets(tcgResting, tcgData.rows[currentStepRow].offsets);
        updateCentroidStats("TCG", tcgDeformed);
        CoordinateSystem tcgUVW = createCoordinateSystem(tcgResting.A, tcgResting.B, tcgResting.C, 'C');
        CoordinateSystem tcgIJK = createCoordinateSystem(tcgDeformed.A, tcgDeformed.B, tcgDeformed.C, 'C');
        glm::mat4 tcgTransform = calculateRigidBodyTransform(tcgUVW, tcgIJK);
        
        // Apply TCG transformation to TransformManager
        transformManager.applyCalculatedTransform("TCG", tcgTransform);
    }
    
    return true;
}

size_t BulkCapacitanceProcessor::getCurrentRow() const
{
    return currentStepRow;
}

size_t BulkCapacitanceProcessor::getMaxRows() const
{
    return maxRows;
}

void BulkCapacitanceProcessor::printCurrentRowInfo() const
{
    if (!stepModeActive) {
        std::cout << "Step mode not active" << std::endl;
        return;
    }
    
    std::cout << "=== ROW " << currentStepRow << " INFO ===" << std::endl;
    
    if (currentStepRow < tagData.rows.size()) {
        const auto& tagOffsets = tagData.rows[currentStepRow].offsets;
        std::cout << "TAG offsets: A(" << tagOffsets.A.x << "," << tagOffsets.A.y << "," << tagOffsets.A.z 
                  << ") B(" << tagOffsets.B.x << "," << tagOffsets.B.y << "," << tagOffsets.B.z 
                  << ") C(" << tagOffsets.C.x << "," << tagOffsets.C.y << "," << tagOffsets.C.z << ")" << std::endl;
        std::cout << "TAG centroid: (" << std::fixed << std::setprecision(3) 
                  << tagCentroidStats.currentPosition.x << "," 
                  << tagCentroidStats.currentPosition.y << "," 
                  << tagCentroidStats.currentPosition.z << ")" << std::endl;
    }
    
    if (currentStepRow < tbgData.rows.size()) {
        const auto& tbgOffsets = tbgData.rows[currentStepRow].offsets;
        std::cout << "TBG offsets: A(" << tbgOffsets.A.x << "," << tbgOffsets.A.y << "," << tbgOffsets.A.z 
                  << ") B(" << tbgOffsets.B.x << "," << tbgOffsets.B.y << "," << tbgOffsets.B.z 
                  << ") C(" << tbgOffsets.C.x << "," << tbgOffsets.C.y << "," << tbgOffsets.C.z << ")" << std::endl;
        std::cout << "TBG centroid: (" << std::fixed << std::setprecision(3) 
                  << tbgCentroidStats.currentPosition.x << "," 
                  << tbgCentroidStats.currentPosition.y << "," 
                  << tbgCentroidStats.currentPosition.z << ")" << std::endl;
    }
    
    if (currentStepRow < tcgData.rows.size()) {
        const auto& tcgOffsets = tcgData.rows[currentStepRow].offsets;
        std::cout << "TCG offsets: A(" << tcgOffsets.A.x << "," << tcgOffsets.A.y << "," << tcgOffsets.A.z 
                  << ") B(" << tcgOffsets.B.x << "," << tcgOffsets.B.y << "," << tcgOffsets.B.z 
                  << ") C(" << tcgOffsets.C.x << "," << tcgOffsets.C.y << "," << tcgOffsets.C.z << ")" << std::endl;
        std::cout << "TCG centroid: (" << std::fixed << std::setprecision(3) 
                  << tcgCentroidStats.currentPosition.x << "," 
                  << tcgCentroidStats.currentPosition.y << "," 
                  << tcgCentroidStats.currentPosition.z << ")" << std::endl;
    }
    
    std::cout << "====================" << std::endl;
}

void BulkCapacitanceProcessor::printDetailedDebugInfo(size_t row, TransformManager& transformManager)
{
    // Removed detailed debug printing - only show for row 0 or when specifically needed
    if (row == 0) {
        std::cout << "=== DEBUG INFO FOR ROW " << row << " ===" << std::endl;
        std::cout << "Transform flags: TAG=" << (transformManager.enableTag ? "ON" : "OFF") 
                  << " TBG=" << (transformManager.enableTbg ? "ON" : "OFF")
                  << " TCG=" << (transformManager.enableTcg ? "ON" : "OFF") << std::endl;
        std::cout << "===============================" << std::endl;
    }
}

bool BulkCapacitanceProcessor::processCSVFiles(const std::string& csvDirectory, 
                                             CapacitanceCalculator& capacitanceCalculator,
                                             TransformManager& transformManager)
{
    std::cout << "Starting bulk capacitance processing..." << std::endl;
    
    // Reset centroid statistics
    resetCentroidStats();
    
    // Load all individual sphere CSV files and combine into group data
    if (!loadGroupFromIndividualFiles(csvDirectory, "TAG", tagData)) {
        std::cerr << "Failed to load TAG group files" << std::endl;
        return false;
    }
    tagData.groupName = "TAG";
    
    if (!loadGroupFromIndividualFiles(csvDirectory, "TBG", tbgData)) {
        std::cerr << "Failed to load TBG group files" << std::endl;
        return false;
    }
    tbgData.groupName = "TBG";
    
    if (!loadGroupFromIndividualFiles(csvDirectory, "TCG", tcgData)) {
        std::cerr << "Failed to load TCG group files" << std::endl;
        return false;
    }
    tcgData.groupName = "TCG";
    
    // Find maximum number of rows
    maxRows = std::max({tagData.rows.size(), tbgData.rows.size(), tcgData.rows.size()});
    
    std::cout << "Loaded CSV files:" << std::endl;
    std::cout << "  TAG: " << tagData.rows.size() << " rows" << std::endl;
    std::cout << "  TBG: " << tbgData.rows.size() << " rows" << std::endl;
    std::cout << "  TCG: " << tcgData.rows.size() << " rows" << std::endl;
    std::cout << "  Processing " << maxRows << " rows total" << std::endl;
    
    // Storage for all results
    std::vector<std::vector<CapacitanceResult>> allResults;
    
    // Process each row
    for (size_t row = 0; row < maxRows; row++) {
        // Get resting positions for each group
        SpherePositions tagResting = getRestingPositions("TAG");
        SpherePositions tbgResting = getRestingPositions("TBG");
        SpherePositions tcgResting = getRestingPositions("TCG");
        
        // Reset transformations to default state
        resetTransformations(transformManager);
        
        // Apply transformations for this row
        if (row < tagData.rows.size()) {
            // Calculate TAG transformation
            SpherePositions tagDeformed = addOffsets(tagResting, tagData.rows[row].offsets);
            updateCentroidStats("TAG", tagDeformed);
            CoordinateSystem tagUVW = createCoordinateSystem(tagResting.A, tagResting.B, tagResting.C, 'A');
            CoordinateSystem tagIJK = createCoordinateSystem(tagDeformed.A, tagDeformed.B, tagDeformed.C, 'A');
            glm::mat4 tagTransform = calculateRigidBodyTransform(tagUVW, tagIJK);
            
            // Apply TAG transformation to TransformManager
            transformManager.applyCalculatedTransform("TAG", tagTransform);
        }
        
        if (row < tbgData.rows.size()) {
            // Calculate TBG transformation
            SpherePositions tbgDeformed = addOffsets(tbgResting, tbgData.rows[row].offsets);
            updateCentroidStats("TBG", tbgDeformed);
            CoordinateSystem tbgUVW = createCoordinateSystem(tbgResting.A, tbgResting.B, tbgResting.C, 'B');
            CoordinateSystem tbgIJK = createCoordinateSystem(tbgDeformed.A, tbgDeformed.B, tbgDeformed.C, 'B');
            glm::mat4 tbgTransform = calculateRigidBodyTransform(tbgUVW, tbgIJK);
            
            // Apply TBG transformation to TransformManager
            transformManager.applyCalculatedTransform("TBG", tbgTransform);
        }
        
        if (row < tcgData.rows.size()) {
            // Calculate TCG transformation
            SpherePositions tcgDeformed = addOffsets(tcgResting, tcgData.rows[row].offsets);
            updateCentroidStats("TCG", tcgDeformed);
            CoordinateSystem tcgUVW = createCoordinateSystem(tcgResting.A, tcgResting.B, tcgResting.C, 'C');
            CoordinateSystem tcgIJK = createCoordinateSystem(tcgDeformed.A, tcgDeformed.B, tcgDeformed.C, 'C');
            glm::mat4 tcgTransform = calculateRigidBodyTransform(tcgUVW, tcgIJK);
            
            // Apply TCG transformation to TransformManager
            transformManager.applyCalculatedTransform("TCG", tcgTransform);
        }
        
        // Refresh geometry with new transforms
        capacitanceCalculator.refreshGeometry();
        
        // Calculate capacitance for this configuration
        std::vector<CapacitanceResult> results = capacitanceCalculator.calculateCapacitances();
        allResults.push_back(results);
        
        // Print progress every 50 rows or for important milestones
        if ((row + 1) % 50 == 0 || row == 0 || (row + 1) == maxRows) {
            std::cout << "Processed row " << (row + 1) << "/" << maxRows << std::endl;
        }
    }
    
    // Save results to CSV
    std::string outputPath = csvDirectory + "/capacitance_results.csv";
    if (!saveResults(allResults, outputPath)) {
        std::cerr << "Failed to save results" << std::endl;
        return false;
    }
    
    std::cout << "Bulk processing complete. Results saved to: " << outputPath << std::endl;
    
    // Print centroid movement statistics
    printCentroidStats();
    
    return true;
}

void BulkCapacitanceProcessor::updateCentroidStats(const std::string& groupName, const SpherePositions& currentPositions)
{
    // Calculate current circumcenter
    glm::vec3 currentCentroid = calculateCircumcenter(currentPositions.A, currentPositions.B, currentPositions.C);
    
    CentroidStats* stats = nullptr;
    if (groupName == "TAG") {
        stats = &tagCentroidStats;
    } else if (groupName == "TBG") {
        stats = &tbgCentroidStats;
    } else if (groupName == "TCG") {
        stats = &tcgCentroidStats;
    } else {
        return;
    }
    
    // Update current position
    stats->currentPosition = currentCentroid;
    
    // Update min/max tracking
    stats->minPosition.x = std::min(stats->minPosition.x, currentCentroid.x);
    stats->minPosition.y = std::min(stats->minPosition.y, currentCentroid.y);
    stats->minPosition.z = std::min(stats->minPosition.z, currentCentroid.z);
    
    stats->maxPosition.x = std::max(stats->maxPosition.x, currentCentroid.x);
    stats->maxPosition.y = std::max(stats->maxPosition.y, currentCentroid.y);
    stats->maxPosition.z = std::max(stats->maxPosition.z, currentCentroid.z);
    
    // Calculate and update bounding sphere radius
    calculateBoundingSphere(*stats);
}

void BulkCapacitanceProcessor::calculateBoundingSphere(CentroidStats& stats)
{
    // Calculate distance from original position to current position
    glm::vec3 displacement = stats.currentPosition - stats.originalPosition;
    float currentDistance = glm::length(displacement);
    
    // Update bounding sphere radius if current distance is larger
    stats.boundingSphereRadius = std::max(stats.boundingSphereRadius, currentDistance);
}

void BulkCapacitanceProcessor::printCentroidStats() const
{
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "CENTROID MOVEMENT STATISTICS" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    // TAG Group
    std::cout << "\nTAG Group:" << std::endl;
    std::cout << "  Original: (" << std::fixed << std::setprecision(3) 
              << tagCentroidStats.originalPosition.x << ", " 
              << tagCentroidStats.originalPosition.y << ", " 
              << tagCentroidStats.originalPosition.z << ")" << std::endl;
    std::cout << "  Current:  (" << std::fixed << std::setprecision(3) 
              << tagCentroidStats.currentPosition.x << ", " 
              << tagCentroidStats.currentPosition.y << ", " 
              << tagCentroidStats.currentPosition.z << ")" << std::endl;
    std::cout << "  Range X:  " << std::fixed << std::setprecision(3) 
              << tagCentroidStats.minPosition.x << " to " << tagCentroidStats.maxPosition.x 
              << " mm (span: " << (tagCentroidStats.maxPosition.x - tagCentroidStats.minPosition.x) << " mm)" << std::endl;
    std::cout << "  Range Y:  " << std::fixed << std::setprecision(3) 
              << tagCentroidStats.minPosition.y << " to " << tagCentroidStats.maxPosition.y 
              << " mm (span: " << (tagCentroidStats.maxPosition.y - tagCentroidStats.minPosition.y) << " mm)" << std::endl;
    std::cout << "  Range Z:  " << std::fixed << std::setprecision(3) 
              << tagCentroidStats.minPosition.z << " to " << tagCentroidStats.maxPosition.z 
              << " mm (span: " << (tagCentroidStats.maxPosition.z - tagCentroidStats.minPosition.z) << " mm)" << std::endl;
    std::cout << "  Bounding sphere radius: " << std::fixed << std::setprecision(3) 
              << tagCentroidStats.boundingSphereRadius << " mm" << std::endl;
    
    // TBG Group
    std::cout << "\nTBG Group:" << std::endl;
    std::cout << "  Original: (" << std::fixed << std::setprecision(3) 
              << tbgCentroidStats.originalPosition.x << ", " 
              << tbgCentroidStats.originalPosition.y << ", " 
              << tbgCentroidStats.originalPosition.z << ")" << std::endl;
    std::cout << "  Current:  (" << std::fixed << std::setprecision(3) 
              << tbgCentroidStats.currentPosition.x << ", " 
              << tbgCentroidStats.currentPosition.y << ", " 
              << tbgCentroidStats.currentPosition.z << ")" << std::endl;
    std::cout << "  Range X:  " << std::fixed << std::setprecision(3) 
              << tbgCentroidStats.minPosition.x << " to " << tbgCentroidStats.maxPosition.x 
              << " mm (span: " << (tbgCentroidStats.maxPosition.x - tbgCentroidStats.minPosition.x) << " mm)" << std::endl;
    std::cout << "  Range Y:  " << std::fixed << std::setprecision(3) 
              << tbgCentroidStats.minPosition.y << " to " << tbgCentroidStats.maxPosition.y 
              << " mm (span: " << (tbgCentroidStats.maxPosition.y - tbgCentroidStats.minPosition.y) << " mm)" << std::endl;
    std::cout << "  Range Z:  " << std::fixed << std::setprecision(3) 
              << tbgCentroidStats.minPosition.z << " to " << tbgCentroidStats.maxPosition.z 
              << " mm (span: " << (tbgCentroidStats.maxPosition.z - tbgCentroidStats.minPosition.z) << " mm)" << std::endl;
    std::cout << "  Bounding sphere radius: " << std::fixed << std::setprecision(3) 
              << tbgCentroidStats.boundingSphereRadius << " mm" << std::endl;
    
    // TCG Group
    std::cout << "\nTCG Group:" << std::endl;
    std::cout << "  Original: (" << std::fixed << std::setprecision(3) 
              << tcgCentroidStats.originalPosition.x << ", " 
              << tcgCentroidStats.originalPosition.y << ", " 
              << tcgCentroidStats.originalPosition.z << ")" << std::endl;
    std::cout << "  Current:  (" << std::fixed << std::setprecision(3) 
              << tcgCentroidStats.currentPosition.x << ", " 
              << tcgCentroidStats.currentPosition.y << ", " 
              << tcgCentroidStats.currentPosition.z << ")" << std::endl;
    std::cout << "  Range X:  " << std::fixed << std::setprecision(3) 
              << tcgCentroidStats.minPosition.x << " to " << tcgCentroidStats.maxPosition.x 
              << " mm (span: " << (tcgCentroidStats.maxPosition.x - tcgCentroidStats.minPosition.x) << " mm)" << std::endl;
    std::cout << "  Range Y:  " << std::fixed << std::setprecision(3) 
              << tcgCentroidStats.minPosition.y << " to " << tcgCentroidStats.maxPosition.y 
              << " mm (span: " << (tcgCentroidStats.maxPosition.y - tcgCentroidStats.minPosition.y) << " mm)" << std::endl;
    std::cout << "  Range Z:  " << std::fixed << std::setprecision(3) 
              << tcgCentroidStats.minPosition.z << " to " << tcgCentroidStats.maxPosition.z 
              << " mm (span: " << (tcgCentroidStats.maxPosition.z - tcgCentroidStats.minPosition.z) << " mm)" << std::endl;
    std::cout << "  Bounding sphere radius: " << std::fixed << std::setprecision(3) 
              << tcgCentroidStats.boundingSphereRadius << " mm" << std::endl;
    
    std::cout << "\n" << std::string(60, '=') << std::endl;
}

void BulkCapacitanceProcessor::resetCentroidStats()
{
    // Get original resting positions for each group
    SpherePositions tagResting = getRestingPositions("TAG");
    SpherePositions tbgResting = getRestingPositions("TBG");
    SpherePositions tcgResting = getRestingPositions("TCG");
    
    // Calculate original circumcenters
    tagCentroidStats.originalPosition = calculateCircumcenter(tagResting.A, tagResting.B, tagResting.C);
    tbgCentroidStats.originalPosition = calculateCircumcenter(tbgResting.A, tbgResting.B, tbgResting.C);
    tcgCentroidStats.originalPosition = calculateCircumcenter(tcgResting.A, tcgResting.B, tcgResting.C);
    
    // Reset current positions to original
    tagCentroidStats.currentPosition = tagCentroidStats.originalPosition;
    tbgCentroidStats.currentPosition = tbgCentroidStats.originalPosition;
    tcgCentroidStats.currentPosition = tcgCentroidStats.originalPosition;
    
    // Reset min/max tracking
    tagCentroidStats.minPosition = glm::vec3(FLT_MAX);
    tagCentroidStats.maxPosition = glm::vec3(-FLT_MAX);
    tagCentroidStats.boundingSphereRadius = 0.0f;
    
    tbgCentroidStats.minPosition = glm::vec3(FLT_MAX);
    tbgCentroidStats.maxPosition = glm::vec3(-FLT_MAX);
    tbgCentroidStats.boundingSphereRadius = 0.0f;
    
    tcgCentroidStats.minPosition = glm::vec3(FLT_MAX);
    tcgCentroidStats.maxPosition = glm::vec3(-FLT_MAX);
    tcgCentroidStats.boundingSphereRadius = 0.0f;
}

bool BulkCapacitanceProcessor::loadGroupFromIndividualFiles(const std::string& csvDirectory, const std::string& groupName, GroupCSVData& groupData)
{
    // Map group names to file prefixes
    std::string prefix;
    if (groupName == "TAG") {
        prefix = "A";
    } else if (groupName == "TBG") {
        prefix = "B";
    } else if (groupName == "TCG") {
        prefix = "C";
    } else {
        std::cerr << "Unknown group name: " << groupName << std::endl;
        return false;
    }
    
    // Load individual sphere files
    std::vector<std::vector<glm::vec3>> sphereData(3); // A, B, C spheres
    
    // Load sphere A
    std::string fileA = csvDirectory + "/" + prefix + "A1Def.csv";
    if (!loadIndividualSphereFile(fileA, sphereData[0])) {
        std::cerr << "Failed to load " << fileA << std::endl;
        return false;
    }
    
    // Load sphere B
    std::string fileB = csvDirectory + "/" + prefix + "B1Def.csv";
    if (!loadIndividualSphereFile(fileB, sphereData[1])) {
        std::cerr << "Failed to load " << fileB << std::endl;
        return false;
    }
    
    // Load sphere C
    std::string fileC = csvDirectory + "/" + prefix + "C1Def.csv";
    if (!loadIndividualSphereFile(fileC, sphereData[2])) {
        std::cerr << "Failed to load " << fileC << std::endl;
        return false;
    }
    
    // Find minimum row count
    size_t minRows = std::min({sphereData[0].size(), sphereData[1].size(), sphereData[2].size()});
    
    // Combine into group data
    groupData.rows.clear();
    for (size_t row = 0; row < minRows; row++) {
        GroupRowData rowData;
        rowData.offsets.A = sphereData[0][row];
        rowData.offsets.B = sphereData[1][row];
        rowData.offsets.C = sphereData[2][row];
        groupData.rows.push_back(rowData);
    }
    
    std::cout << "Loaded " << groupName << " group: " << minRows << " rows from individual files" << std::endl;
    
    return !groupData.rows.empty();
}

bool BulkCapacitanceProcessor::loadIndividualSphereFile(const std::string& filePath, std::vector<glm::vec3>& sphereOffsets)
{
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Cannot open file: " << filePath << std::endl;
        return false;
    }
    
    std::string line;
    bool firstLine = true;
    
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty()) continue;
        
        // Skip header row
        if (firstLine) {
            firstLine = false;
            continue;
        }
        
        glm::vec3 offset;
        if (parseIndividualSphereRow(line, offset)) {
            sphereOffsets.push_back(offset);
        }
    }
    
    file.close();
    return !sphereOffsets.empty();
}

bool BulkCapacitanceProcessor::parseIndividualSphereRow(const std::string& line, glm::vec3& offset)
{
    std::vector<std::string> tokens = splitCSVLine(line);
    
    if (tokens.size() < 3) {
        std::cerr << "Invalid CSV row: expected 3 columns (UX,UY,UZ), got " << tokens.size() << std::endl;
        return false;
    }
    
    try {
        // Parse UX, UY, UZ and convert from meters to mm
        offset.x = std::stof(tokens[0]) * 1000.0f;
        offset.y = std::stof(tokens[1]) * 1000.0f;
        offset.z = std::stof(tokens[2]) * 1000.0f;
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error parsing CSV row: " << e.what() << std::endl;
        return false;
    }
}

bool BulkCapacitanceProcessor::loadCSVFile(const std::string& filePath, GroupCSVData& groupData)
{
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Cannot open file: " << filePath << std::endl;
        return false;
    }
    
    std::string line;
    bool firstLine = true;
    
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty()) continue;
        
        // Skip header row
        if (firstLine) {
            firstLine = false;
            continue;
        }
        
        GroupRowData rowData;
        if (parseCSVRow(line, rowData)) {
            groupData.rows.push_back(rowData);
        }
    }
    
    file.close();
    return !groupData.rows.empty();
}

bool BulkCapacitanceProcessor::parseCSVRow(const std::string& line, GroupRowData& rowData)
{
    std::vector<std::string> tokens = splitCSVLine(line);
    
    if (tokens.size() < 9) {
        std::cerr << "Invalid CSV row: expected 9 columns, got " << tokens.size() << std::endl;
        return false;
    }
    
    try {
        // Parse A, B, C positions (convert from meters to mm)
        rowData.offsets.A.x = std::stof(tokens[0]) * 1000.0f;
        rowData.offsets.A.y = std::stof(tokens[1]) * 1000.0f;
        rowData.offsets.A.z = std::stof(tokens[2]) * 1000.0f;
        
        rowData.offsets.B.x = std::stof(tokens[3]) * 1000.0f;
        rowData.offsets.B.y = std::stof(tokens[4]) * 1000.0f;
        rowData.offsets.B.z = std::stof(tokens[5]) * 1000.0f;
        
        rowData.offsets.C.x = std::stof(tokens[6]) * 1000.0f;
        rowData.offsets.C.y = std::stof(tokens[7]) * 1000.0f;
        rowData.offsets.C.z = std::stof(tokens[8]) * 1000.0f;
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error parsing CSV row: " << e.what() << std::endl;
        return false;
    }
}

std::vector<std::string> BulkCapacitanceProcessor::splitCSVLine(const std::string& line)
{
    std::vector<std::string> tokens;
    std::stringstream ss(line);
    std::string token;
    
    while (std::getline(ss, token, ',')) {
        tokens.push_back(trim(token));
    }
    
    return tokens;
}

glm::vec3 BulkCapacitanceProcessor::calculateCircumcenter(const glm::vec3& A, const glm::vec3& B, const glm::vec3& C)
{
    // Vector differences
    glm::vec3 AB = B - A;
    glm::vec3 AC = C - A;
    
    // Cross product for normal vector
    glm::vec3 normal = glm::cross(AB, AC);
    float normalLengthSq = glm::dot(normal, normal);
    
    // Check for collinear points
    if (normalLengthSq < 1e-10f) {
        // Points are collinear, return centroid
        return (A + B + C) / 3.0f;
    }
    
    // Calculate circumcenter using the standard formula
    float AB_lengthSq = glm::dot(AB, AB);
    float AC_lengthSq = glm::dot(AC, AC);
    
    glm::vec3 numerator = glm::cross((AC_lengthSq * AB - AB_lengthSq * AC), normal);
    glm::vec3 circumcenter = A + numerator / (2.0f * normalLengthSq);
    
    return circumcenter;
}

CoordinateSystem BulkCapacitanceProcessor::createCoordinateSystem(const glm::vec3& A, const glm::vec3& B, const glm::vec3& C, char referencePoint)
{
    CoordinateSystem coord;
    
    // Calculate circumcenter as origin
    coord.origin = calculateCircumcenter(A, B, C);
    
    // Calculate W axis (normal to plane ABC)
    glm::vec3 AB = B - A;
    glm::vec3 AC = C - A;
    
    glm::vec3 crossProduct = glm::cross(AB, AC);
    coord.W = glm::normalize(crossProduct);
    
    // Calculate V axis (inverted line from center to reference point)
    glm::vec3 referencePos;
    switch (referencePoint) {
        case 'A': referencePos = A; break;
        case 'B': referencePos = B; break;
        case 'C': referencePos = C; break;
        default: referencePos = A; break;
    }
    
    glm::vec3 centerToRef = referencePos - coord.origin;
    coord.V = -glm::normalize(centerToRef);  // Inverted
    
    // Calculate U axis (cross product of V and W)
    glm::vec3 VxW = glm::cross(coord.V, coord.W);
    coord.U = glm::normalize(VxW);
    
    return coord;
}

glm::mat4 BulkCapacitanceProcessor::calculateRigidBodyTransform(const CoordinateSystem& from, const CoordinateSystem& to)
{
    // Create transformation matrix from UVW coordinate system to IJK coordinate system
    
    // Create matrix for "from" coordinate system (UVW)
    glm::mat4 fromMatrix(1.0f);
    fromMatrix[0] = glm::vec4(from.U, 0.0f);
    fromMatrix[1] = glm::vec4(from.V, 0.0f);
    fromMatrix[2] = glm::vec4(from.W, 0.0f);
    fromMatrix[3] = glm::vec4(from.origin, 1.0f);
    
    // Create matrix for "to" coordinate system (IJK)
    glm::mat4 toMatrix(1.0f);
    toMatrix[0] = glm::vec4(to.U, 0.0f);
    toMatrix[1] = glm::vec4(to.V, 0.0f);
    toMatrix[2] = glm::vec4(to.W, 0.0f);
    toMatrix[3] = glm::vec4(to.origin, 1.0f);

    // Calculate transformation: to * inverse(from)
    return toMatrix * glm::inverse(fromMatrix);
}

SpherePositions BulkCapacitanceProcessor::getRestingPositions(const std::string& groupName)
{
    SpherePositions positions;
    float radius = 24.85f;
    
    if (groupName == "TAG") {
        // TAG group positions
        positions.A = glm::vec3(0.0f, radius - 4.0f, 0.0f);  // TAG_A
        float offset = 4.0f / sqrt(2.0f);
        positions.B = glm::vec3(offset, radius + offset, 0.0f);   // TAG_B
        positions.C = glm::vec3(-offset, radius + offset, 0.0f);  // TAG_C
    }
    else if (groupName == "TBG") {
        // TBG group positions
        float angle = -30.0f * M_PI / 180.0f;
        float tbgX = radius * cos(angle);
        float tbgY = radius * sin(angle);
        float offset = 4.0f / sqrt(2.0f);
        
        positions.A = glm::vec3(tbgX - offset, tbgY + offset, 0.0f);  // TBG_A
        positions.B = glm::vec3(tbgX, tbgY - 4.0f, 0.0f);           // TBG_B
        positions.C = glm::vec3(tbgX + offset, tbgY + offset, 0.0f); // TBG_C
    }
    else if (groupName == "TCG") {
        // TCG group positions
        float angle = -150.0f * M_PI / 180.0f;
        float tcgX = radius * cos(angle);
        float tcgY = radius * sin(angle);
        float offset = 4.0f / sqrt(2.0f);
        
        positions.A = glm::vec3(tcgX + offset, tcgY + offset, 0.0f); // TCG_A
        positions.B = glm::vec3(tcgX - offset, tcgY + offset, 0.0f); // TCG_B
        positions.C = glm::vec3(tcgX, tcgY - 4.0f, 0.0f);           // TCG_C
    }
    
    return positions;
}

SpherePositions BulkCapacitanceProcessor::addOffsets(const SpherePositions& resting, const SpherePositions& offsets)
{
    SpherePositions result;
    result.A = resting.A + offsets.A;
    result.B = resting.B + offsets.B;
    result.C = resting.C + offsets.C;
    return result;
}

bool BulkCapacitanceProcessor::saveResults(const std::vector<std::vector<CapacitanceResult>>& allResults, const std::string& outputPath)
{
    std::ofstream file(outputPath);
    if (!file.is_open()) {
        std::cerr << "Cannot create output file: " << outputPath << std::endl;
        return false;
    }
    
    // Write header
    file << "Row,A1_Capacitance_pF,A2_Capacitance_pF,B1_Capacitance_pF,B2_Capacitance_pF,C1_Capacitance_pF,C2_Capacitance_pF,Total_Capacitance_pF\n";
    
    // Write data
    for (size_t i = 0; i < allResults.size(); i++) {
        const auto& results = allResults[i];
        
        file << (i + 1);  // Row number (1-based)
        
        double totalCapacitance = 0.0;
        for (const auto& result : results) {
            double capacitancePF = result.capacitance * 1e12;  // Convert to picofarads
            file << "," << std::fixed << std::setprecision(5) << capacitancePF;
            totalCapacitance += result.capacitance;
        }
        
        // Add total capacitance
        double totalPF = totalCapacitance * 1e12;
        file << "," << std::fixed << std::setprecision(5) << totalPF << "\n";
    }
    
    file.close();
    return true;
}

void BulkCapacitanceProcessor::resetTransformations(TransformManager& transformManager)
{
    // Reset all transformations to default values
    transformManager.enablePositiv = false;
    transformManager.enableTag = false;
    transformManager.enableTbg = false;
    transformManager.enableTcg = false;
    
    // Reset transformation values to zero
    transformManager.tagRotationX = 0.0f;
    transformManager.tagRotationY = 0.0f;
    transformManager.tagRotationZ = 0.0f;
    transformManager.tagTranslationX = 0.0f;
    transformManager.tagTranslationY = 0.0f;
    transformManager.tagTranslationZ = 0.0f;
    
    transformManager.tbgRotationX = 0.0f;
    transformManager.tbgRotationY = 0.0f;
    transformManager.tbgRotationZ = 0.0f;
    transformManager.tbgTranslationX = 0.0f;
    transformManager.tbgTranslationY = 0.0f;
    transformManager.tbgTranslationZ = 0.0f;
    
    transformManager.tcgRotationX = 0.0f;
    transformManager.tcgRotationY = 0.0f;
    transformManager.tcgRotationZ = 0.0f;
    transformManager.tcgTranslationX = 0.0f;
    transformManager.tcgTranslationY = 0.0f;
    transformManager.tcgTranslationZ = 0.0f;
    
    // Rebuild transformation matrices
    transformManager.buildTransformationMatrices();
}

std::string BulkCapacitanceProcessor::trim(const std::string& str)
{
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}