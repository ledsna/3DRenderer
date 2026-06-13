#pragma once

#include <Eigen/Core>
#include <cmath>

namespace rasterizer {

class MathUtils {
public:
    static Eigen::Vector3f createCameraBasis(const Eigen::Vector3f& forward, 
                                           const Eigen::Vector3f& up) {
        Eigen::Vector3f right = forward.cross(up).normalized();
        Eigen::Vector3f upVector = right.cross(forward);
        return right;
    }
    
    static Eigen::Vector3f createCameraUp(const Eigen::Vector3f& forward, 
                                        const Eigen::Vector3f& right) {
        return right.cross(forward);
    }
    
    static float tanHalfFov(float fovRadians) {
        return std::tan(fovRadians * 0.5f);
    }
    
    static Eigen::Vector3f sphericalToCartesian(float theta, float phi) {
        return Eigen::Vector3f(std::sin(phi) * std::cos(theta), 
                              std::cos(phi), 
                              std::sin(phi) * std::sin(theta)).normalized();
    }
    
    static Eigen::Vector3f createOrthogonalVector(const Eigen::Vector3f& direction) {
        if (std::abs(direction.x()) < 0.9f) {
            return direction.cross(Eigen::Vector3f(1.0f, 0.0f, 0.0f)).normalized();
        } else {
            return direction.cross(Eigen::Vector3f(0.0f, 1.0f, 0.0f)).normalized();
        }
    }
};

} 