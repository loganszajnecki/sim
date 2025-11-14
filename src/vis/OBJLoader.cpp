#include "vis/OBJLoader.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <filesystem>

#include <glm/glm.hpp>

namespace vis {

// static helpers
static std::vector<std::string> splitWS(const std::string& s)
{
    std::istringstream iss(s);
    std::vector<std::string> out;
    std::string tok;
    while (iss >> tok) out.push_back(tok);
    return out;
}

static void processVertexRef(
    const std::string& ref,
    std::vector<unsigned int>& indices,
    const std::vector<glm::vec2>& texcoords,
    const std::vector<glm::vec3>& normals,
    std::vector<float>& texArray,
    std::vector<float>& normArray)
{
    // ref format: vIndex/vtIndex/vnIndex
    // We must handle vt or vn missing (v//vn).
    int vIdx = -1, vtIdx = -1, vnIdx = -1;

    std::string a, b, c;
    {
        std::stringstream ss(ref);
        std::getline(ss, a, '/');
        std::getline(ss, b, '/');
        std::getline(ss, c, '/');
    }
    if (!a.empty()) vIdx  = std::stoi(a) - 1;
    if (!b.empty()) vtIdx = std::stoi(b) - 1;
    if (!c.empty()) vnIdx = std::stoi(c) - 1;

    if (vIdx < 0) return; // invalid

    indices.push_back(static_cast<unsigned int>(vIdx));

    // texture
    if (vtIdx >= 0 && vtIdx < (int)texcoords.size() && !texArray.empty()) {
        glm::vec2 uv = texcoords[vtIdx];
        size_t base = static_cast<size_t>(vIdx) * 2;
        if (base + 1 < texArray.size()) {
            texArray[base + 0] = uv.x;
            texArray[base + 1] = 1.0f - uv.y; // V flip
        }
    }

    // normal
    if (vnIdx >= 0 && vnIdx < (int)normals.size() && !normArray.empty()) {
        glm::vec3 n = normals[vnIdx];
        size_t base = static_cast<size_t>(vIdx) * 3;
        if (base + 2 < normArray.size()) {
            normArray[base + 0] = n.x;
            normArray[base + 1] = n.y;
            normArray[base + 2] = n.z;
        }
    }
}

RawModel OBJLoader::loadObjModel(const std::string& fileName,
                                 Loader& loader)
{
    namespace fs = std::filesystem;

    fs::path objPath = fs::path("../res") / (fileName + ".obj");

    std::ifstream in(objPath);
    if (!in) {
        throw std::runtime_error("Failed to open OBJ file: " + objPath.string());
    }

    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> texcoords;
    std::vector<glm::vec3> normals;
    std::vector<unsigned int> indices;

    std::vector<float> texArray;
    std::vector<float> normArray;

    std::string line;
    bool seenFaces = false;

    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;

        if (line.rfind("v ", 0) == 0) {
            auto t = splitWS(line);
            if (t.size() < 4) continue;
            float x = std::stof(t[1]);
            float y = std::stof(t[2]);
            float z = std::stof(t[3]);
            vertices.emplace_back(x, y, z);
        } else if (line.rfind("vt ", 0) == 0) {
            auto t = splitWS(line);
            if (t.size() < 3) continue;
            float u = std::stof(t[1]);
            float v = std::stof(t[2]);
            texcoords.emplace_back(u, v);
        } else if (line.rfind("vn ", 0) == 0) {
            auto t = splitWS(line);
            if (t.size() < 4) continue;
            float nx = std::stof(t[1]);
            float ny = std::stof(t[2]);
            float nz = std::stof(t[3]);
            normals.emplace_back(nx, ny, nz);
        } else if (line.rfind("f ", 0) == 0) {
            if (!seenFaces) {
                seenFaces = true;
                size_t vCount = vertices.size();
                texArray.assign(vCount * 2, 0.0f);
                normArray.assign(vCount * 3, 0.0f);
            }

            auto tokens = splitWS(line);
            if (tokens.size() < 4) continue; // need at least a triangle

            // collect vertex refs
            std::vector<std::string> refs;
            for (size_t i = 1; i < tokens.size(); ++i) {
                refs.push_back(tokens[i]);
            }

            // triangulate fan: (0, i, i+1)
            for (size_t i = 1; i + 1 < refs.size(); ++i) {
                processVertexRef(refs[0], indices, texcoords, normals, texArray, normArray);
                processVertexRef(refs[i], indices, texcoords, normals, texArray, normArray);
                processVertexRef(refs[i+1], indices, texcoords, normals, texArray, normArray);
            }
        }
    }

    // Flatten vertex positions
    std::vector<float> posArray(vertices.size() * 3);
    for (size_t i = 0; i < vertices.size(); ++i) {
        const auto& v = vertices[i];
        size_t base = i * 3;
        posArray[base + 0] = v.x;
        posArray[base + 1] = v.y;
        posArray[base + 2] = v.z;
    }

    // Create RawModel via Loader
    return loader.loadToVAO(posArray,
                            texArray,    // may be empty if no vt
                            normArray,   // may be empty if no vn
                            indices);

}


} // namespace vis