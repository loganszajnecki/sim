#include "vis/OBJLoader.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <filesystem>
#include <unordered_map>

#include <glm/glm.hpp>

namespace {

// model radii cache
std::unordered_map<std::string, float> g_modelRadii;

// simple whitespace split
std::vector<std::string> splitWS(const std::string& s)
{
    std::istringstream iss(s);
    std::vector<std::string> out;
    std::string tok;
    while (iss >> tok) out.push_back(tok);
    return out;
}

// Key for unique vertex: OBJ lets v / vt / vn indices be independent
struct VertexKey {
    int v  = -1;
    int vt = -1;
    int vn = -1;
};

struct VertexKeyHash {
    std::size_t operator()(const VertexKey& k) const noexcept {
        // simple mixing of 3 ints
        std::size_t h = 1469598103934665603ull;
        h ^= std::size_t(k.v + 1);  h *= 1099511628211ull;
        h ^= std::size_t(k.vt + 1); h *= 1099511628211ull;
        h ^= std::size_t(k.vn + 1); h *= 1099511628211ull;
        return h;
    }
};

struct VertexKeyEq {
    bool operator()(const VertexKey& a, const VertexKey& b) const noexcept {
        return a.v == b.v && a.vt == b.vt && a.vn == b.vn;
    }
};

// Parse a single "v/vt/vn" reference (vt or vn can be missing)
void parseVertexRef(const std::string& ref, int& vIdx, int& vtIdx, int& vnIdx)
{
    vIdx  = -1;
    vtIdx = -1;
    vnIdx = -1;

    std::string a, b, c;
    {
        std::stringstream ss(ref);
        std::getline(ss, a, '/');
        std::getline(ss, b, '/');
        std::getline(ss, c, '/');
    }

    auto parseIndex = [](const std::string& s) -> int {
        if (s.empty()) return -1;
        return std::stoi(s); // still 1-based here
    };

    vIdx  = parseIndex(a);
    vtIdx = parseIndex(b);
    vnIdx = parseIndex(c);
}

} // anonymous namespace

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

RawModel OBJLoader::loadObjModel(const std::string& fileName,
                                 Loader& loader)
{
    namespace fs = std::filesystem;

    fs::path objPath = fs::path("../res") / (fileName + ".obj");

    std::ifstream in(objPath);
    if (!in) {
        throw std::runtime_error("Failed to open OBJ file: " + objPath.string());
    }

    // Raw OBJ attribute arrays
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> texcoords;
    std::vector<glm::vec3> normals;

    // Final packed arrays (one entry per *unique* (v,vt,vn) triple)
    std::vector<float> posArray;
    std::vector<float> texArray;
    std::vector<float> normArray;
    std::vector<unsigned int> indices;

    // Map (v,vt,vn) -> index in packed arrays
    std::unordered_map<VertexKey, unsigned int, VertexKeyHash, VertexKeyEq> vertexMap;

    auto getOrCreateVertexIndex = [&](int vIdx1, int vtIdx1, int vnIdx1) -> unsigned int {
        // OBJ indices are 1-based. We only handle positive indices here.
        if (vIdx1 <= 0 || vIdx1 > (int)vertices.size()) {
            throw std::runtime_error("OBJ: invalid vertex index");
        }

        VertexKey key;
        key.v  = vIdx1;
        key.vt = vtIdx1;
        key.vn = vnIdx1;

        auto it = vertexMap.find(key);
        if (it != vertexMap.end()) {
            return it->second;
        }

        // Create new packed vertex
        int vIdx = vIdx1 - 1; // 0-based
        glm::vec3 pos = vertices[vIdx];

        glm::vec2 uv(0.0f);
        if (vtIdx1 > 0 && vtIdx1 <= (int)texcoords.size()) {
            uv = texcoords[vtIdx1 - 1];
        }

        glm::vec3 n(0.0f, 1.0f, 0.0f);
        if (vnIdx1 > 0 && vnIdx1 <= (int)normals.size()) {
            n = normals[vnIdx1 - 1];
        }

        unsigned int newIndex = static_cast<unsigned int>(posArray.size() / 3);
        vertexMap[key] = newIndex;

        // positions
        posArray.push_back(pos.x);
        posArray.push_back(pos.y);
        posArray.push_back(pos.z);

        // texcoords (flip V like before)
        texArray.push_back(uv.x);
        texArray.push_back(1.0f - uv.y);

        // normals
        normArray.push_back(n.x);
        normArray.push_back(n.y);
        normArray.push_back(n.z);

        return newIndex;
    };

    std::string line;

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
            auto tokens = splitWS(line);
            if (tokens.size() < 4) continue; // need at least a triangle

            // collect vertex refs (strings like "v/vt/vn")
            std::vector<std::string> refs;
            for (size_t i = 1; i < tokens.size(); ++i) {
                refs.push_back(tokens[i]);
            }

            // Triangulate as fan: (0, i, i+1)
            for (size_t i = 1; i + 1 < refs.size(); ++i) {
                int v0, vt0, vn0;
                int v1, vt1, vn1;
                int v2, vt2, vn2;

                parseVertexRef(refs[0], v0, vt0, vn0);
                parseVertexRef(refs[i], v1, vt1, vn1);
                parseVertexRef(refs[i+1], v2, vt2, vn2);

                unsigned int i0 = getOrCreateVertexIndex(v0, vt0, vn0);
                unsigned int i1 = getOrCreateVertexIndex(v1, vt1, vn1);
                unsigned int i2 = getOrCreateVertexIndex(v2, vt2, vn2);

                indices.push_back(i0);
                indices.push_back(i1);
                indices.push_back(i2);
            }
        }
    }

    // Compute model-space radius for spherical meshes (unchanged)
    float maxR2 = 0.0f;
    for (const auto& v : vertices) {
        float r2 = glm::dot(v, v);
        if (r2 > maxR2) maxR2 = r2;
    }
    float modelRadius = std::sqrt(maxR2);
    g_modelRadii[fileName] = modelRadius;

    // Create RawModel via Loader
    return loader.loadToVAO(posArray,
                            texArray,
                            normArray,
                            indices);
}

float OBJLoader::getModelRadius(const std::string& fileName)
{
    auto it = g_modelRadii.find(fileName);
    if (it == g_modelRadii.end()) {
        return 1.0f;
    }
    return it->second;
}


} // namespace vis