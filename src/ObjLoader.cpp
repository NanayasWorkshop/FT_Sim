#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#include "ObjLoader.h"
#include <iostream>

// Static member initialization
std::string ObjLoader::lastError = "";

bool ObjLoader::loadOBJ(const std::string& filePath,
                        std::vector<float>& vertices,
                        std::vector<unsigned int>& indices,
                        size_t& vertexCount,
                        size_t& triangleCount)
{
    // Clear output vectors
    vertices.clear();
    indices.clear();
    vertexCount = 0;
    triangleCount = 0;
    lastError = "";

    // TinyObjLoader structures
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    // Load the OBJ file
    bool success = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filePath.c_str());

    // Handle errors (but suppress warnings about missing material files)
    if (!err.empty()) {
        lastError = "Error loading " + filePath + ": " + err;
        std::cerr << lastError << std::endl;
        return false;
    }

    if (!success) {
        lastError = "Failed to load " + filePath;
        return false;
    }

    if (shapes.empty()) {
        lastError = "No shapes found in " + filePath;
        return false;
    }

    // Process all shapes into a single mesh
    size_t indexOffset = 0;
    
    for (size_t s = 0; s < shapes.size(); s++) {
        const auto& mesh = shapes[s].mesh;
        
        // Process each face
        for (size_t f = 0; f < mesh.num_face_vertices.size(); f++) {
            size_t fv = mesh.num_face_vertices[f];
            
            // We only handle triangulated faces (should be 3 vertices per face)
            if (fv != 3) {
                indexOffset += fv;
                continue;
            }
            
            // Process the 3 vertices of this triangle
            for (size_t v = 0; v < fv; v++) {
                tinyobj::index_t idx = mesh.indices[indexOffset + v];
                
                // Get vertex position
                float vx = attrib.vertices[3 * idx.vertex_index + 0];
                float vy = attrib.vertices[3 * idx.vertex_index + 1];
                float vz = attrib.vertices[3 * idx.vertex_index + 2];
                
                // Add vertex to our array
                vertices.push_back(vx);
                vertices.push_back(vy);
                vertices.push_back(vz);
                
                // Add index (simple sequential indexing since we're creating a flat array)
                indices.push_back(static_cast<unsigned int>(indices.size()));
            }
            
            indexOffset += fv;
            triangleCount++;
        }
    }
    
    vertexCount = vertices.size() / 3;
    
    if (vertices.empty()) {
        lastError = "No vertex data found in " + filePath;
        return false;
    }
    
    return true;
}

std::string ObjLoader::getLastError()
{
    return lastError;
}