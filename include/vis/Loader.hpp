#pragma once

#include <vector>
#include <string>
#include <glad/glad.h>
#include "vis/models/RawModel.hpp"

namespace vis {

class Loader 
{
public:
    Loader() = default;
    ~Loader();

    Loader(const Loader&) = delete;
    Loader& operator=(const Loader&) = delete;

    Loader(Loader&&) = default;
    Loader& operator=(Loader&&) = default;

    // positions: 3 floats per vertex
    // texCoords: 2 floats per vertex (may be empty)
    // normals:   3 floats per vertex (may be empty)
    // indices:   GL_UNSIGNED_INT index list
    RawModel loadToVAO(const std::vector<float>& positions,
                       const std::vector<float>& texCoords,
                       const std::vector<float>& normals,
                       const std::vector<unsigned int>& indices);
    GLuint loadTexture(const std::string& fileName);
    
    void cleanUp();

private:
    GLuint createVAO();
    void storeDataInAttributeList(GLuint attribIndex,
                                  GLint  componentCount,
                                  const std::vector<float>& data);
    void bindIndicesBuffer(const std::vector<unsigned int>& indices);
    void unbindVAO();

private:
    std::vector<GLuint> vaos_;
    std::vector<GLuint> vbos_;
    std::vector<GLuint> textures_;
};

} // namespace vis