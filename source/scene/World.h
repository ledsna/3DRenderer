#pragma once

#include <vector>
#include <memory>
#include "Object.h"
#include "Material.h"

namespace rasterizer {

class ShadowMap;

class World {
public:
    std::vector<Object> objects;
    AmbientLight ambient;
    std::vector<DirectionalLight> dirLights;
    std::vector<PointLight> pointLights;
    
    // one per dir light
    std::vector<std::unique_ptr<ShadowMap>> shadowMaps;
};

} 
