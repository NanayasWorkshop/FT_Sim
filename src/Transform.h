#ifndef TRANSFORM_H
#define TRANSFORM_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <map>
#include <string>

// Parent group types
enum class ParentGroupType { 
    Positiv,    // Contains TAG, TBG, TCG
    Negativ     // Contains stationary_negative models
};

// Sub group types
enum class SubGroupType { 
    TAG,        // A1, A2 models
    TBG,        // B1, B2 models  
    TCG,        // C1, C2 models
    Negativ,    // stationary_negative models
    Individual  // No group transformation
};

// Transform manager class with proper transformation order
class TransformManager
{
public:
    TransformManager();
    ~TransformManager();

    // Parent group transformation management
    void setParentGroupTransform(ParentGroupType group, const glm::mat4& transform);
    glm::mat4 getParentGroupTransform(ParentGroupType group) const;
    
    // Sub group transformation management
    void setSubGroupTransform(SubGroupType group, const glm::mat4& transform);
    glm::mat4 getSubGroupTransform(SubGroupType group) const;
    
    // Model group assignment
    SubGroupType getModelSubGroup(const std::string& modelName) const;
    ParentGroupType getSubGroupParent(SubGroupType subGroup) const;
    
    // Get combined transformation for a model (handles different transformation orders)
    glm::mat4 getCombinedTransform(const std::string& modelName) const;
    
    // Get model's original world position
    glm::vec3 getModelWorldPosition(const std::string& modelName) const;
    
    // Name utilities
    std::string getParentGroupName(ParentGroupType group) const;
    std::string getSubGroupName(SubGroupType group) const;
    
    // Initialization
    void initializeDefaultTransforms();
    void initializeSampleTransforms();
    void buildTransformationMatrices();
    
    // Debug info
    void printGroupTransforms() const;

    // NEW: Methods for applying calculated transformation matrices
    void applyCalculatedTransform(const std::string& groupName, const glm::mat4& transform);
    void setGroupTransformMatrix(SubGroupType group, const glm::mat4& transform);
    
    // NEW: Matrix decomposition helpers
    void decomposeTransformMatrix(const glm::mat4& transform, 
                                 glm::vec3& translation, 
                                 glm::vec3& rotation, 
                                 glm::vec3& scale);
    
    // NEW: Apply decomposed transformation to specific group
    void applyDecomposedTransform(SubGroupType group, 
                                 const glm::vec3& translation, 
                                 const glm::vec3& rotation);

    // Boolean flags for enabling/disabling transformations
    bool enablePositiv;
    bool enableTag;
    bool enableTbg;
    bool enableTcg;

    // Separated transformation matrices
    glm::mat4 positivRotation;
    glm::mat4 positivTranslation;
    glm::mat4 tagRotation;
    glm::mat4 tagTranslation;
    glm::mat4 tbgRotation;
    glm::mat4 tbgTranslation;
    glm::mat4 tcgRotation;
    glm::mat4 tcgTranslation;

    // Explicit transformation values - Positiv group
    float positivRotationX;     // radians
    float positivRotationY;     // radians
    float positivRotationZ;     // radians
    float positivTranslationX;  // mm
    float positivTranslationY;  // mm
    float positivTranslationZ;  // mm
    
    // Explicit transformation values - TAG group
    float tagRotationX;         // radians
    float tagRotationY;         // radians
    float tagRotationZ;         // radians
    float tagTranslationX;      // mm
    float tagTranslationY;      // mm
    float tagTranslationZ;      // mm
    
    // Explicit transformation values - TBG group
    float tbgRotationX;         // radians
    float tbgRotationY;         // radians
    float tbgRotationZ;         // radians
    float tbgTranslationX;      // mm
    float tbgTranslationY;      // mm
    float tbgTranslationZ;      // mm
    
    // Explicit transformation values - TCG group
    float tcgRotationX;         // radians
    float tcgRotationY;         // radians
    float tcgRotationZ;         // radians
    float tcgTranslationX;      // mm
    float tcgTranslationY;      // mm
    float tcgTranslationZ;      // mm

private:
    std::map<ParentGroupType, glm::mat4> parentGroupTransforms;
    std::map<SubGroupType, glm::mat4> subGroupTransforms;
    
    // Helper methods
    glm::mat4 createIdentityTransform() const;
    glm::mat4 createInversePositionMatrix(const glm::vec3& position) const;

    // NEW: Helper methods for matrix decomposition
    glm::vec3 extractTranslation(const glm::mat4& matrix);
    glm::vec3 extractRotation(const glm::mat4& matrix);
    glm::vec3 extractScale(const glm::mat4& matrix);
};

#endif