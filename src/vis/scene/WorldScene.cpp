#include "vis/scene/WorldScene.hpp"

#include <cstdio>
#include <exception>

#include "vis/OBJLoader.hpp"

namespace {

static constexpr float kEarthWorldRadius = 10000.0f;

} // anonymous

namespace vis {

void WorldScene::init(Loader& loader,
                      GeoMapper& mapper,
                      const geo::GeoOrigin* origin)
{
    initSun_();
    initMissile_(loader);
    initEarth_(loader, mapper, origin);
    initGround_();

    if (origin && earthEntity_.model) {
        initLaunchMarker_(origin, mapper);
        initTerrainPatch_(loader, origin, mapper);
    }
}

void WorldScene::initSun_()
{
    sun_.position = glm::vec3(kEarthWorldRadius * 3.0f,
                              kEarthWorldRadius * 3.0f,
                              kEarthWorldRadius * 3.0f);
    sun_.color    = glm::vec3(1.0f, 1.0f, 1.0f);
}

void WorldScene::initMissile_(Loader& loader)
{
    try {
        RawModel missileRaw = OBJLoader::loadObjModel("tree", loader);

        ModelTexture missileTex{};
        missileTex.id              = loader.loadTexture("tree");
        missileTex.shineDamper     = 10.0f;
        missileTex.reflectivity    = 0.9f;
        missileTex.hasTransparency = false;
        missileTex.useFakeLighting = false;

        missileModel_  = TexturedModel{missileRaw, missileTex};
        missileEntity_ = Entity(&missileModel_,
                                glm::vec3(0.0f),
                                glm::vec3{90.0f, 0.0f, 0.0f},
                                1.0f);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "[WorldScene] Failed to load missile OBJ: %s\n", e.what());
        missileEntity_.model    = nullptr;
        missileEntity_.position = glm::vec3(0.0f);
        missileEntity_.rotation = glm::vec3(0.0f);
        missileEntity_.scale    = 1.0f;
    }
}

void WorldScene::initEarth_(Loader& loader,
                            GeoMapper& mapper,
                            const geo::GeoOrigin* origin)
{
    try {
        RawModel earthRaw = OBJLoader::loadObjModel("earth", loader);

        // Query the true model-space radius of the mesh.
        float earthModelRadius = OBJLoader::getModelRadius("earth");

        // Desired visual Earth radius in world units.
        float earthWorldRadius = kEarthWorldRadius;   // e.g. 10000.0f
        float earthScale       = earthWorldRadius / earthModelRadius;

        ModelTexture earthTex{};
        earthTex.id              = loader.loadTexture("8k_earth_daymap");
        earthTex.shineDamper     = 10.0f;
        earthTex.reflectivity    = 0.0f;
        earthTex.hasTransparency = false;
        earthTex.useFakeLighting = false;

        earthModel_  = TexturedModel{earthRaw, earthTex};
        earthEntity_ = Entity(&earthModel_,
                              glm::vec3(0.0f),
                              glm::vec3(0.0f),
                              earthScale);
        earthEnabled_ = true;

        // Tie GeoMapper's world + physical radii to THIS origin + mesh.
        if (origin) {
            mapper.configureFromOrigin(origin, earthWorldRadius);
        }
    } catch (const std::exception& e) {
        std::fprintf(stderr, "[WorldScene] Failed to load Earth OBJ/texture: %s\n", e.what());
        earthEntity_.model    = nullptr;
        earthEntity_.position = glm::vec3(0.0f);
        earthEntity_.rotation = glm::vec3(0.0f);
        earthEntity_.scale    = 1.0f;
        earthEnabled_         = false;
    }
}


void WorldScene::initLaunchMarker_(const geo::GeoOrigin* origin,
                                   GeoMapper& mapper)
{
    mapper.configureFromOrigin(origin, /*earthWorldRadius*/ mapper.earthRadius());

    glm::vec3 markerPos = mapper.enuToGlobe(glm::vec3(0.0f), 0.0f);

    launchMarkerEntity_.model    = nullptr;  // we draw a cross only
    launchMarkerEntity_.position = markerPos;
    launchMarkerEntity_.rotation = glm::vec3(0.0f);
    launchMarkerEntity_.scale    = 300.0f;

    launchMarkerPos_     = markerPos;
    haveLaunchMarkerPos_ = true;
}

void WorldScene::initGround_()
{
    groundEntity_.model    = nullptr;
    groundEntity_.position = glm::vec3(0.0f);
    groundEntity_.rotation = glm::vec3(0.0f);
    groundEntity_.scale    = 1.0f;
}

void WorldScene::initTerrainPatch_(Loader& loader,
                                   const geo::GeoOrigin* origin,
                                   GeoMapper& mapper)
{
    terrainEnabled_ = false;

    if (!origin || !mapper.isReady()) {
        return;
    }

    TerrainPatchConfig cfg;
    cfg.halfSizeMeters = 50000.0f; // matches your previous HALF_SIZE
    cfg.resolution     = 64;       // matches previous N
    cfg.patchOffset    = 0.0f;    // small offset above globe

    std::vector<float>         positions;
    std::vector<float>         texcoords;
    std::vector<float>         normals;
    std::vector<unsigned int>  indices;

    buildTerrainPatchMesh(*origin, mapper, cfg,
                          positions, texcoords, normals, indices);

    terrainRaw_ = loader.loadToVAO(
        positions,
        texcoords,
        normals,
        indices
    );

    ModelTexture terrainTex{};
    terrainTex.id              = loader.loadTexture("terrain_local");
    terrainTex.shineDamper     = 4.0f;
    terrainTex.reflectivity    = 0.0f;
    terrainTex.hasTransparency = false;
    terrainTex.useFakeLighting = false;

    terrainModel_  = TexturedModel{terrainRaw_, terrainTex};
    terrainEntity_ = Entity(&terrainModel_,
                            glm::vec3(0.0f),
                            glm::vec3(0.0f, 0.0f, 0.0f),
                            1.0f);

    terrainEnabled_ = true;
}

void WorldScene::updateMissile(const glm::vec3& missileWorld)
{
    if (missileEntity_.model) {
        missileEntity_.position = missileWorld;
        // Orientation from velocity can be added later here.
    }
}

void WorldScene::submit(MasterRenderer& renderer, const Camera& cam)
{
    if (earthEnabled_ && earthEntity_.model) {
        renderer.processEntity(earthEntity_);
    }

    if (terrainEnabled_ && terrainEntity_.model) {
        renderer.processEntity(terrainEntity_);
    }

    if (earthEnabled_ && launchMarkerEntity_.model) {
        renderer.processEntity(launchMarkerEntity_);
    }

    if (missileEntity_.model) {
        renderer.processEntity(missileEntity_);
    }

    if (groundEntity_.model) {
        renderer.processEntity(groundEntity_);
    }

    renderer.render(sun_, cam);
}

} // namespace vis
