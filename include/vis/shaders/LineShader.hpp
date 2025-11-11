#pragma once

#include "vis/shaders/ShaderProgram.hpp"
#include <glm/glm.hpp>

namespace vis {

/**
 * @brief Minimal shader for rendering colored lines.
 *
 * Expected GLSL interface:
 *
 *   // Vertex shader:
 *   layout (location = 0) in vec3 position;
 *   uniform mat4 uVP;   // view-projection matrix
 *
 *   // Fragment shader:
 *   uniform vec3 uColor;
 *
 *   out vec4 fragColor;
 *
 *   void main() {
 *       gl_Position = uVP * vec4(position, 1.0);
 *       fragColor   = vec4(uColor, 1.0);
 *   }
 *
 * Responsibilities:
 *  - Bind attribute location 0 to "position".
 *  - Fetch uniform locations for uVP and uColor.
 *  - Provide helpers to load VP matrix and color.
 *
 * RAII:
 *  - Underlying GL program lifetime is managed by ShaderProgram.
 *    LineShader only configures attribute bindings and uniform locations.
 */
class LineShader : public ShaderProgram
{
public:
    /**
     * @brief Construct a line shader from vertex/fragment shader file paths.
     *
     * @param vertPath  Path to vertex shader source file.
     * @param fragPath  Path to fragment shader source file.
     *
     * The base ShaderProgram is responsible for compiling and owning the
     * shader program object. LineShader then:
     *   1) Binds vertex attributes.
     *   2) Links/validates the program.
     *   3) Queries uniform locations.
     */
    LineShader(const std::string& vertPath, const std::string& fragPath);

    /// Load the combined view-projection matrix into uniform uVP.
    void loadVP(const glm::mat4& vp);

    /// Load the RGB line color into uniform uColor.
    void loadColor(const glm::vec3& rgb);

protected:
    /// Bind vertex attribute locations (called from ctor).
    void bindAttributes() override;

    /// Query all uniform locations (called from ctor).
    void getAllUniformLocations() override;

private:
    GLint loc_vp_    = -1;  ///< Location of uVP uniform.
    GLint loc_color_ = -1;  ///< Location of uColor uniform.
};

} // namespace vis
