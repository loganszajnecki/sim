#include "vis/Loader.hpp"
#include <glm/glm.hpp>

namespace vis {

Loader::~Loader()
{
    cleanUp();
}

RawModel Loader::loadToVAO(const std::vector<float>& positions,
                           const std::vector<float>& texCoords,
                           const std::vector<float>& normals,
                           const std::vector<unsigned int>& indices)
{
    GLuint vao = createVAO();
    bindIndicesBuffer(indices);

    // positions: location 0, 3 floats
    storeDataInAttributeList(0, 3, positions);

    // texture coords: location 1, 2 floats (if provided)
    if (!texCoords.empty()) {
        storeDataInAttributeList(1, 2, texCoords);
    }

    // normals: location 2, 3 floats (if provided)
    if (!normals.empty()) {
        storeDataInAttributeList(2, 3, normals);
    }

    unbindVAO();

    RawModel model;
    model.vao = vao;
    model.indexCount = static_cast<GLsizei>(indices.size());
    return model;
}


void Loader::cleanUp() 
{
    for (auto vao : vaos_) {
        glDeleteVertexArrays(1, &vao);
    }
    for (auto vbo : vbos_) {
        glDeleteBuffers(1, &vbo);
    }
    vaos_.clear();
    vbos_.clear();
}

GLuint Loader::createVAO() 
{
    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    vaos_.push_back(vao);
    return vao;
}


void Loader::storeDataInAttributeList(GLuint attribIndex,
                                      GLint  componentCount,
                                      const std::vector<float>& data)
{
    if (data.empty()) return;

    GLuint vbo = 0;
    glGenBuffers(1, &vbo);
    vbos_.push_back(vbo);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 data.size() * sizeof(float),
                 data.data(),
                 GL_STATIC_DRAW);

    glEnableVertexAttribArray(attribIndex);
    glVertexAttribPointer(attribIndex,
                          componentCount,
                          GL_FLOAT,
                          GL_FALSE,
                          0,
                          (void*)0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Loader::bindIndicesBuffer(const std::vector<unsigned int>& indices) 
{
    if (indices.empty()) return;

    GLuint vbo = 0;
    glGenBuffers(1, &vbo);
    vbos_.push_back(vbo);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 indices.size() * sizeof(unsigned int),
                 indices.data(),
                 GL_STATIC_DRAW);
}

void Loader::unbindVAO() 
{
    glBindVertexArray(0);
}

} // namespace vis