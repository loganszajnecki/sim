#include "vis/Loader.hpp"
#include <stb_image.h>
#include <stdexcept>
#include <glm/glm.hpp>

namespace vis {

Loader::~Loader()
{
    cleanUp();
}

GLuint Loader::createVAO() 
{
    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    vaos_.push_back(vao);
    return vao;
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

GLuint Loader::loadTexture(const std::string& fileName)
{
    // Expects res/<fileName>.png... TODO: cleanup paths
    std::string path = "../res/" + fileName + ".png";

    int width = 0, height = 0, channels = 0;
    //stbi_set_flip_vertically_on_load(true);

    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 4);
    if (!data) {
        throw std::runtime_error("Failed to load texture: " + path); // TODO: cleanup exceptions and errors
    }

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA8,
                 width,
                 height,
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 data);

    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);

    textures_.push_back(tex);
    return tex;
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

void Loader::cleanUp() {
    for (auto vao : vaos_) {
        glDeleteVertexArrays(1, &vao);
    }
    for (auto vbo : vbos_) {
        glDeleteBuffers(1, &vbo);
    }
    for (auto tex : textures_) {
        glDeleteTextures(1, &tex);
    }
    vaos_.clear();
    vbos_.clear();
    textures_.clear();
}

} // namespace vis