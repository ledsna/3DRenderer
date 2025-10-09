#pragma once

#include <vector>
#include <string>
#include <Eigen/Core>

#include "../assets/Mesh.h"
#include "Transform.h"
#include "Material.h"
#include "../utils/BoundingBox.h"

namespace rasterizer {

class Object {
public:
    std::string name;
    Mesh mesh;
    Transform transform;
    Material material;

    BoundingBox localBounds; // Object-space AABB

    void computeLocalBounds();
    
    BoundingBox getWorldBounds() const {
        return localBounds.transformed(transform.modelMatrix());
    }
    
    Eigen::Vector3f getBoundingSphereCenter() const {
        const Eigen::Vector4f localCenter = Eigen::Vector4f(localBounds.center().x(), 
                                                            localBounds.center().y(), 
                                                            localBounds.center().z(), 1.0f);
        const Eigen::Vector4f worldCenter = transform.modelMatrix() * localCenter;
        return worldCenter.head<3>();
    }
    
    float getBoundingSphereRadius() const {
        const float maxScale = std::max({std::abs(transform.scale.x()), 
                                        std::abs(transform.scale.y()), 
                                        std::abs(transform.scale.z())});
        return localBounds.radius() * maxScale;
    }
};

} 

