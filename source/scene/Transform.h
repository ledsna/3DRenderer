#pragma once

#include <Eigen/Core>
#include <Eigen/Geometry>

namespace rasterizer {

struct Transform {
    Eigen::Vector3f position{0.0f, 0.0f, 0.0f};
    Eigen::Vector3f rotationEuler{0.0f, 0.0f, 0.0f}; // degrees: pitch (X), yaw (Y), roll (Z)
    Eigen::Vector3f scale{1.0f, 1.0f, 1.0f};

    Eigen::Matrix4f modelMatrix() const;
};

} 

