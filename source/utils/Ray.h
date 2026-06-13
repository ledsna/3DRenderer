#pragma once

#include <Eigen/Core>
#include <optional>
#include <limits>

namespace rasterizer {

struct Ray {
    Eigen::Vector3f origin;
    Eigen::Vector3f direction;  // Should be normalized
    
    Ray(const Eigen::Vector3f& o, const Eigen::Vector3f& d) 
        : origin(o), direction(d.normalized()) {}
    
    Eigen::Vector3f at(float t) const {
        return origin + direction * t;
    }
};

// Ray-AABB (Axis-Aligned Bounding Box) intersection
// Returns distance to intersection, or nullopt if no hit
inline std::optional<float> intersectAABB(const Ray& ray, 
                                          const Eigen::Vector3f& boxMin, 
                                          const Eigen::Vector3f& boxMax) {
    float tMin = 0.0f;
    float tMax = std::numeric_limits<float>::max();
    
    for (int i = 0; i < 3; ++i) {
        if (std::abs(ray.direction[i]) < 1e-8f) {
            // Ray is parallel to slab, check if origin is within bounds
            if (ray.origin[i] < boxMin[i] || ray.origin[i] > boxMax[i]) {
                return std::nullopt;
            }
        } else {
            float t1 = (boxMin[i] - ray.origin[i]) / ray.direction[i];
            float t2 = (boxMax[i] - ray.origin[i]) / ray.direction[i];
            
            if (t1 > t2) std::swap(t1, t2);
            
            tMin = std::max(tMin, t1);
            tMax = std::min(tMax, t2);
            
            if (tMin > tMax) {
                return std::nullopt;
            }
        }
    }
    
    return tMin > 0.0f ? std::optional(tMin) : std::nullopt;
}

// Ray-Triangle (Möller-Trumbore) intersection
// Returns distance to intersection, or nullopt if no hit
inline std::optional<float> intersectTriangle(const Ray& ray,
                                              const Eigen::Vector3f& v0,
                                              const Eigen::Vector3f& v1,
                                              const Eigen::Vector3f& v2) {
    const Eigen::Vector3f edge1 = v1 - v0;
    const Eigen::Vector3f edge2 = v2 - v0;
    const Eigen::Vector3f h = ray.direction.cross(edge2);
    const float a = edge1.dot(h);
    
    // Ray is parallel to triangle
    if (std::abs(a) < 1e-8f) {
        return std::nullopt;
    }
    
    const float f = 1.0f / a;
    const Eigen::Vector3f s = ray.origin - v0;
    const float u = f * s.dot(h);
    
    if (u < 0.0f || u > 1.0f) {
        return std::nullopt;
    }
    
    const Eigen::Vector3f q = s.cross(edge1);
    const float v = f * ray.direction.dot(q);
    
    if (v < 0.0f || u + v > 1.0f) {
        return std::nullopt;
    }
    
    const float t = f * edge2.dot(q);
    
    if (t > 1e-8f) {
        return std::optional(t);
    }
    
    return std::nullopt;
}

} 

