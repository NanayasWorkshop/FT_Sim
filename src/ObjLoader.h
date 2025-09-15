#ifndef OBJLOADER_H
#define OBJLOADER_H

#include <vector>
#include <string>

// Simple OBJ loader using tinyobjloader
class ObjLoader
{
public:
    // Load OBJ file and return vertex data
    static bool loadOBJ(const std::string& filePath,
                        std::vector<float>& vertices,
                        std::vector<unsigned int>& indices,
                        size_t& vertexCount,
                        size_t& triangleCount);

    // Get last error message
    static std::string getLastError();

private:
    static std::string lastError;
    
    // Helper function to process tinyobj data
    static void processTinyObjData(const void* attrib, const void* shapes, 
                                  std::vector<float>& vertices, 
                                  std::vector<unsigned int>& indices,
                                  size_t& vertexCount,
                                  size_t& triangleCount);
};

#endif