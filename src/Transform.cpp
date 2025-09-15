#include "Transform.h"
#include <iostream>
#include <cmath>

TransformManager::TransformManager()
{
    initializeDefaultTransforms();
    initializeSampleTransforms();
}

TransformManager::~TransformManager()
{
}

void TransformManager::setParentGroupTransform(ParentGroupType group, const glm::mat4& transform)
{
    parentGroupTransforms[group] = transform;
}

glm::mat4 TransformManager::getParentGroupTransform(ParentGroupType group) const
{
    auto it = parentGroupTransforms.find(group);
    if (it != parentGroupTransforms.end()) {
        return it->second;
    }
    return createIdentityTransform();
}

void TransformManager::setSubGroupTransform(SubGroupType group, const glm::mat4& transform)
{
    subGroupTransforms[group] = transform;
}

glm::mat4 TransformManager::getSubGroupTransform(SubGroupType group) const
{
    auto it = subGroupTransforms.find(group);
    if (it != subGroupTransforms.end()) {
        return it->second;
    }
    return createIdentityTransform();
}

SubGroupType TransformManager::getModelSubGroup(const std::string& modelName) const
{
    // TAG group: A1, A2, and TAG spheres
    if (modelName == "A1_model" || modelName == "A2_model" || 
        modelName == "TAG_A" || modelName == "TAG_B" || modelName == "TAG_C") {
        return SubGroupType::TAG;
    }
    
    // TBG group: B1, B2, and TBG spheres
    if (modelName == "B1_model" || modelName == "B2_model" || 
        modelName == "TBG_A" || modelName == "TBG_B" || modelName == "TBG_C") {
        return SubGroupType::TBG;
    }
    
    // TCG group: C1, C2, and TCG spheres
    if (modelName == "C1_model" || modelName == "C2_model" || 
        modelName == "TCG_A" || modelName == "TCG_B" || modelName == "TCG_C") {
        return SubGroupType::TCG;
    }
    
    // Negativ group: all stationary_negative variants
    if (modelName == "stationary_negative_A" || 
        modelName == "stationary_negative_B" || 
        modelName == "stationary_negative_C") {
        return SubGroupType::Negativ;
    }
    
    // Default: individual (no group transformation)
    return SubGroupType::Individual;
}

ParentGroupType TransformManager::getSubGroupParent(SubGroupType subGroup) const
{
    switch (subGroup) {
        case SubGroupType::TAG:
        case SubGroupType::TBG:
        case SubGroupType::TCG:
            return ParentGroupType::Positiv;
            
        case SubGroupType::Negativ:
            return ParentGroupType::Negativ;
            
        case SubGroupType::Individual:
        default:
            return ParentGroupType::Positiv; // Default to Positiv
    }
}

glm::mat4 TransformManager::getCombinedTransform(const std::string& modelName) const
{
    // Get sub group for this model
    SubGroupType subGroup = getModelSubGroup(modelName);
    
    // Get parent group for this sub group
    ParentGroupType parentGroup = getSubGroupParent(subGroup);
    
    // Step 1: Get the model's final world position
    glm::vec3 modelWorldPos = getModelWorldPosition(modelName);
    
    // Step 2: Start with model positioned in world
    glm::mat4 finalTransform = glm::translate(glm::mat4(1.0f), modelWorldPos);
    
    // Step 3: Apply sub-group transformations around the group center
    if (enableTag && subGroup == SubGroupType::TAG) {
        // All TAG objects rotate around TAG center (0, 24.85, 0)
        glm::vec3 tagCenter = glm::vec3(0.0f, 24.85f, 0.0f);
        finalTransform = glm::translate(glm::mat4(1.0f), tagCenter) * 
                        tagRotation * tagTranslation * 
                        glm::translate(glm::mat4(1.0f), -tagCenter) * 
                        glm::translate(glm::mat4(1.0f), modelWorldPos);
    }
    else if (enableTbg && subGroup == SubGroupType::TBG) {
        // All TBG objects rotate around TBG center
        float radius = 24.85f;
        float angle = -30.0f * 3.14159f / 180.0f;
        glm::vec3 tbgCenter = glm::vec3(radius * cos(angle), radius * sin(angle), 0.0f);
        finalTransform = glm::translate(glm::mat4(1.0f), tbgCenter) * 
                        tbgRotation * tbgTranslation * 
                        glm::translate(glm::mat4(1.0f), -tbgCenter) * 
                        glm::translate(glm::mat4(1.0f), modelWorldPos);
    }
    else if (enableTcg && subGroup == SubGroupType::TCG) {
        // All TCG objects rotate around TCG center
        float radius = 24.85f;
        float angle = -150.0f * 3.14159f / 180.0f;
        glm::vec3 tcgCenter = glm::vec3(radius * cos(angle), radius * sin(angle), 0.0f);
        finalTransform = glm::translate(glm::mat4(1.0f), tcgCenter) * 
                        tcgRotation * tcgTranslation * 
                        glm::translate(glm::mat4(1.0f), -tcgCenter) * 
                        glm::translate(glm::mat4(1.0f), modelWorldPos);
    }
    
    // Step 4: Apply Positiv group transform if enabled (around world center)
    if (enablePositiv && parentGroup == ParentGroupType::Positiv) {
        finalTransform = positivTranslation * positivRotation * finalTransform;
    }
    
    return finalTransform;
}

glm::vec3 TransformManager::getModelWorldPosition(const std::string& modelName) const
{
    // Return the original world positions for models
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
    // TAG spheres - relative to TAG center (0, 24.85, 0)
    else if (modelName == "TAG_A") {
        // TAG_A: 4mm straight down from TAG center
        return glm::vec3(0.0f, radius - 4.0f, 0.0f);
    }
    else if (modelName == "TAG_B") {
        // TAG_B: 4mm at diagonal +Y+X from TAG center
        float offset = 4.0f / sqrt(2.0f); // 2.83mm in each direction
        return glm::vec3(offset, radius + offset, 0.0f);
    }
    else if (modelName == "TAG_C") {
        // TAG_C: 4mm at diagonal +Y-X from TAG center
        float offset = 4.0f / sqrt(2.0f); // 2.83mm in each direction
        return glm::vec3(-offset, radius + offset, 0.0f);
    }
    // TBG spheres - relative to TBG center (21.51, -12.425, 0)
    else if (modelName == "TBG_A") {
        // TBG_A: 4mm at diagonal +Y-X from TBG center (rotated pattern)
        float angle = -30.0f * 3.14159f / 180.0f;
        float tbgX = radius * cos(angle);
        float tbgY = radius * sin(angle);
        float offset = 4.0f / sqrt(2.0f); // 2.83mm in each direction
        return glm::vec3(tbgX - offset, tbgY + offset, 0.0f);
    }
    else if (modelName == "TBG_B") {
        // TBG_B: 4mm straight down from TBG center (rotated pattern)
        float angle = -30.0f * 3.14159f / 180.0f;
        float tbgX = radius * cos(angle);
        float tbgY = radius * sin(angle);
        return glm::vec3(tbgX, tbgY - 4.0f, 0.0f);
    }
    else if (modelName == "TBG_C") {
        // TBG_C: 4mm at diagonal +Y+X from TBG center (rotated pattern)
        float angle = -30.0f * 3.14159f / 180.0f;
        float tbgX = radius * cos(angle);
        float tbgY = radius * sin(angle);
        float offset = 4.0f / sqrt(2.0f); // 2.83mm in each direction
        return glm::vec3(tbgX + offset, tbgY + offset, 0.0f);
    }
    // TCG spheres - relative to TCG center (-21.51, -12.425, 0)
    else if (modelName == "TCG_A") {
        // TCG_A: 4mm at diagonal +Y+X from TCG center (rotated pattern)
        float angle = -150.0f * 3.14159f / 180.0f;
        float tcgX = radius * cos(angle);
        float tcgY = radius * sin(angle);
        float offset = 4.0f / sqrt(2.0f); // 2.83mm in each direction
        return glm::vec3(tcgX + offset, tcgY + offset, 0.0f);
    }
    else if (modelName == "TCG_B") {
        // TCG_B: 4mm at diagonal +Y-X from TCG center (rotated pattern)
        float angle = -150.0f * 3.14159f / 180.0f;
        float tcgX = radius * cos(angle);
        float tcgY = radius * sin(angle);
        float offset = 4.0f / sqrt(2.0f); // 2.83mm in each direction
        return glm::vec3(tcgX - offset, tcgY + offset, 0.0f);
    }
    else if (modelName == "TCG_C") {
        // TCG_C: 4mm straight down from TCG center (rotated pattern)
        float angle = -150.0f * 3.14159f / 180.0f;
        float tcgX = radius * cos(angle);
        float tcgY = radius * sin(angle);
        return glm::vec3(tcgX, tcgY - 4.0f, 0.0f);
    }
    else if (modelName == "stationary_negative_A") {
        return glm::vec3(0.0f, radius, 0.0f); // Same as A group
    }
    else if (modelName == "stationary_negative_B") {
        float angle = -30.0f * 3.14159f / 180.0f;
        float x = radius * cos(angle);
        float y = radius * sin(angle);
        return glm::vec3(x, y, 0.0f); // Same as B group
    }
    else if (modelName == "stationary_negative_C") {
        float angle = -150.0f * 3.14159f / 180.0f;
        float x = radius * cos(angle);
        float y = radius * sin(angle);
        return glm::vec3(x, y, 0.0f); // Same as C group
    }
    
    // Default position
    return glm::vec3(0.0f, 0.0f, 0.0f);
}

std::string TransformManager::getParentGroupName(ParentGroupType group) const
{
    switch (group) {
        case ParentGroupType::Positiv: return "Positiv";
        case ParentGroupType::Negativ: return "Negativ";
        default: return "Unknown";
    }
}

std::string TransformManager::getSubGroupName(SubGroupType group) const
{
    switch (group) {
        case SubGroupType::TAG: return "TAG";
        case SubGroupType::TBG: return "TBG";
        case SubGroupType::TCG: return "TCG";
        case SubGroupType::Negativ: return "Negativ";
        case SubGroupType::Individual: return "Individual";
        default: return "Unknown";
    }
}

void TransformManager::initializeDefaultTransforms()
{
    // Initialize parent groups with identity matrices
    parentGroupTransforms[ParentGroupType::Positiv] = createIdentityTransform();
    parentGroupTransforms[ParentGroupType::Negativ] = createIdentityTransform();
    
    // Initialize sub groups with identity matrices
    subGroupTransforms[SubGroupType::TAG] = createIdentityTransform();
    subGroupTransforms[SubGroupType::TBG] = createIdentityTransform();
    subGroupTransforms[SubGroupType::TCG] = createIdentityTransform();
    subGroupTransforms[SubGroupType::Negativ] = createIdentityTransform();
    subGroupTransforms[SubGroupType::Individual] = createIdentityTransform();
    
    // Initialize boolean flags
    enablePositiv = true;  // Start with all groups disabled
    enableTag = false;
    enableTbg = false;
    enableTcg = false;
    
    // Initialize all transformation values to zero (default state)
    // Positiv group
    positivRotationX = 0.0f;    // radians
    positivRotationY = 0.0f;    // radians
    positivRotationZ = 0.0f;    // radians
    positivTranslationX = 0.0f; // mm
    positivTranslationY = 0.0f; // mm
    positivTranslationZ = 0.0f; // mm
    
    // TAG group
    tagRotationX = 0.0f;        // radians
    tagRotationY = 0.0f;        // radians
    tagRotationZ = 0.0f;        // radians
    tagTranslationX = 0.0f;     // mm
    tagTranslationY = 0.0f;     // mm
    tagTranslationZ = 0.0f;     // mm
    
    // TBG group
    tbgRotationX = 0.0f;        // radians
    tbgRotationY = 0.0f;        // radians
    tbgRotationZ = 0.0f;        // radians
    tbgTranslationX = 0.0f;     // mm
    tbgTranslationY = 0.0f;     // mm
    tbgTranslationZ = 0.0f;     // mm
    
    // TCG group
    tcgRotationX = 0.0f;        // radians
    tcgRotationY = 0.0f;        // radians
    tcgRotationZ = 0.0f;        // radians
    tcgTranslationX = 0.0f;     // mm
    tcgTranslationY = 0.0f;     // mm
    tcgTranslationZ = 0.0f;     // mm
}

void TransformManager::initializeSampleTransforms()
{
    // Set actual transformation values for your application
    
    // Positiv group
    positivRotationX = 0.0f;    // radians
    positivRotationY = 0.30f;    // radians
    positivRotationZ = 0.0f;    // radians
    positivTranslationX = 0.0f; // mm
    positivTranslationY = 0.0f; // mm
    positivTranslationZ = 0.0f; // mm
    
    // TAG group
    tagRotationX = 0.0f;        // radians
    tagRotationY = 0.20f;        // radians
    tagRotationZ = 0.20f;        // radians
    tagTranslationX = 0.0f;     // mm
    tagTranslationY = 0.0f;     // mm
    tagTranslationZ = 0.0f;     // mm
    
    // TBG group
    tbgRotationX = 0.0f;        // radians
    tbgRotationY = 0.0f;        // radians
    tbgRotationZ = 0.0f;        // radians
    tbgTranslationX = 0.0f;     // mm
    tbgTranslationY = 0.0f;     // mm
    tbgTranslationZ = 0.0f;     // mm
    
    // TCG group
    tcgRotationX = 0.0f;        // radians
    tcgRotationY = 0.0f;        // radians
    tcgRotationZ = 0.0f;        // radians
    tcgTranslationX = 0.0f;     // mm
    tcgTranslationY = 0.0f;     // mm
    tcgTranslationZ = 0.0f;     // mm
    
    // Build transformation matrices from explicit values
    buildTransformationMatrices();
    
    std::cout << "Transformation values set:" << std::endl;
    std::cout << "- Positiv: RotX=" << positivRotationX << " rad, TransY=" << positivTranslationY << " mm [" << (enablePositiv ? "ENABLED" : "DISABLED") << "]" << std::endl;
    std::cout << "- TAG: All zero [" << (enableTag ? "ENABLED" : "DISABLED") << "]" << std::endl;
    std::cout << "- TBG: All zero [" << (enableTbg ? "ENABLED" : "DISABLED") << "]" << std::endl;
    std::cout << "- TCG: All zero [" << (enableTcg ? "ENABLED" : "DISABLED") << "]" << std::endl;
}

void TransformManager::buildTransformationMatrices()
{
    // Build separated rotation and translation matrices for Positiv group
    positivRotation = glm::rotate(glm::mat4(1.0f), positivRotationX, glm::vec3(1.0f, 0.0f, 0.0f)) *
                      glm::rotate(glm::mat4(1.0f), positivRotationY, glm::vec3(0.0f, 1.0f, 0.0f)) *
                      glm::rotate(glm::mat4(1.0f), positivRotationZ, glm::vec3(0.0f, 0.0f, 1.0f));
    
    positivTranslation = glm::translate(glm::mat4(1.0f), glm::vec3(positivTranslationX, positivTranslationY, positivTranslationZ));
    
    // Build separated rotation and translation matrices for TAG group
    tagRotation = glm::rotate(glm::mat4(1.0f), tagRotationX, glm::vec3(1.0f, 0.0f, 0.0f)) *
                  glm::rotate(glm::mat4(1.0f), tagRotationY, glm::vec3(0.0f, 1.0f, 0.0f)) *
                  glm::rotate(glm::mat4(1.0f), tagRotationZ, glm::vec3(0.0f, 0.0f, 1.0f));
    
    tagTranslation = glm::translate(glm::mat4(1.0f), glm::vec3(tagTranslationX, tagTranslationY, tagTranslationZ));
    
    // Build separated rotation and translation matrices for TBG group
    tbgRotation = glm::rotate(glm::mat4(1.0f), tbgRotationX, glm::vec3(1.0f, 0.0f, 0.0f)) *
                  glm::rotate(glm::mat4(1.0f), tbgRotationY, glm::vec3(0.0f, 1.0f, 0.0f)) *
                  glm::rotate(glm::mat4(1.0f), tbgRotationZ, glm::vec3(0.0f, 0.0f, 1.0f));
    
    tbgTranslation = glm::translate(glm::mat4(1.0f), glm::vec3(tbgTranslationX, tbgTranslationY, tbgTranslationZ));
    
    // Build separated rotation and translation matrices for TCG group
    tcgRotation = glm::rotate(glm::mat4(1.0f), tcgRotationX, glm::vec3(1.0f, 0.0f, 0.0f)) *
                  glm::rotate(glm::mat4(1.0f), tcgRotationY, glm::vec3(0.0f, 1.0f, 0.0f)) *
                  glm::rotate(glm::mat4(1.0f), tcgRotationZ, glm::vec3(0.0f, 0.0f, 1.0f));
    
    tcgTranslation = glm::translate(glm::mat4(1.0f), glm::vec3(tcgTranslationX, tcgTranslationY, tcgTranslationZ));
}

void TransformManager::printGroupTransforms() const
{
    std::cout << "\n=== Transformation Values ===" << std::endl;
    
    std::cout << "\nPositiv Group [" << (enablePositiv ? "ENABLED" : "DISABLED") << "]:" << std::endl;
    std::cout << "  Rotation: X=" << positivRotationX << " Y=" << positivRotationY << " Z=" << positivRotationZ << " (radians)" << std::endl;
    std::cout << "  Translation: X=" << positivTranslationX << " Y=" << positivTranslationY << " Z=" << positivTranslationZ << " (mm)" << std::endl;
    
    std::cout << "\nTAG Group [" << (enableTag ? "ENABLED" : "DISABLED") << "]:" << std::endl;
    std::cout << "  Rotation: X=" << tagRotationX << " Y=" << tagRotationY << " Z=" << tagRotationZ << " (radians)" << std::endl;
    std::cout << "  Translation: X=" << tagTranslationX << " Y=" << tagTranslationY << " Z=" << tagTranslationZ << " (mm)" << std::endl;
    
    std::cout << "\nTBG Group [" << (enableTbg ? "ENABLED" : "DISABLED") << "]:" << std::endl;
    std::cout << "  Rotation: X=" << tbgRotationX << " Y=" << tbgRotationY << " Z=" << tbgRotationZ << " (radians)" << std::endl;
    std::cout << "  Translation: X=" << tbgTranslationX << " Y=" << tbgTranslationY << " Z=" << tbgTranslationZ << " (mm)" << std::endl;
    
    std::cout << "\nTCG Group [" << (enableTcg ? "ENABLED" : "DISABLED") << "]:" << std::endl;
    std::cout << "  Rotation: X=" << tcgRotationX << " Y=" << tcgRotationY << " Z=" << tcgRotationZ << " (radians)" << std::endl;
    std::cout << "  Translation: X=" << tcgTranslationX << " Y=" << tcgTranslationY << " Z=" << tcgTranslationZ << " (mm)" << std::endl;
    
    std::cout << "==============================" << std::endl;
}

glm::mat4 TransformManager::createIdentityTransform() const
{
    return glm::mat4(1.0f);
}

glm::mat4 TransformManager::createInversePositionMatrix(const glm::vec3& position) const
{
    return glm::translate(glm::mat4(1.0f), -position);
}