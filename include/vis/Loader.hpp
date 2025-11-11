#pragma once

#include <vector>
#include <string>

#include <glad/glad.h>

#include "vis/models/RawModel.hpp"

namespace vis {

/**
 * @brief GL resource loader for VAOs/VBOs and textures.
 *
 * Responsibilities:
 *  - Create VAOs + VBOs for mesh data (positions, texcoords, normals, indices).
 *  - Load 2D textures from disk using stb_image.
 *  - Track all created VAOs / VBOs / textures and delete them in cleanUp()
 *    or in the destructor.
 *
 * Requirements:
 *  - All public methods that touch OpenGL (loadToVAO, loadTexture, cleanUp)
 *    must be called only when a valid OpenGL context is current.
 *
 * RAII:
 *  - Destructor calls cleanUp() so all GL objects owned by this loader
 *    are deleted before the GL context is destroyed.
 */
class Loader 
{
public:
    Loader() = default;
    ~Loader();

    Loader(const Loader&)            = delete;
    Loader& operator=(const Loader&) = delete;

    // We do NOT allow moving, because the default move would copy the
    // vectors of GLuints and cause double-deletion on destruction.
    Loader(Loader&&)            = delete;
    Loader& operator=(Loader&&) = delete;

    /**
     * @brief Upload mesh data into a VAO with attached VBOs.
     *
     * positions: 3 floats per vertex (mandatory, attrib location 0)
     * texCoords: 2 floats per vertex (optional, attrib location 1)
     * normals:   3 floats per vertex (optional, attrib location 2)
     * indices:   GL_UNSIGNED_INT index list (mandatory)
     */
    RawModel loadToVAO(const std::vector<float>& positions,
                       const std::vector<float>& texCoords,
                       const std::vector<float>& normals,
                       const std::vector<unsigned int>& indices);

    /**
     * @brief Load a 2D texture from ../res/<fileName>.png.
     *
     * @param fileName  Base file name (without extension or directory).
     * @return          GL texture object name (GLuint).
     *
     * Throws std::runtime_error on load failure.
     */
    GLuint loadTexture(const std::string& fileName);
    
    /**
     * @brief Delete all VAOs, VBOs, and textures created by this loader.
     *
     * Safe to call multiple times; subsequent calls are no-ops.
     * Must be called only while a valid GL context is current.
     */
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
