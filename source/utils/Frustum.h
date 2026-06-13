#pragma once

#include <Eigen/Core>
#include <array>
#include "BoundingBox.h"

namespace rasterizer {

class Frustum {
public:
    enum Plane {
        Left = 0,
        Right = 1,
        Bottom = 2,
        Top = 3,
        Near = 4,
        Far = 5
    };
    
    Frustum() = default;
    
    static Frustum fromViewProjection(const Eigen::Matrix4f& vp) {
        Frustum f;
        
        const Eigen::Vector4f r0 = vp.row(0).transpose();
        const Eigen::Vector4f r1 = vp.row(1).transpose();
        const Eigen::Vector4f r2 = vp.row(2).transpose();
        const Eigen::Vector4f r3 = vp.row(3).transpose();
        
        f.planes_[Left]   = normalizePlane(r3 + r0);
        f.planes_[Right]  = normalizePlane(r3 - r0);
        f.planes_[Bottom] = normalizePlane(r3 + r1);
        f.planes_[Top]    = normalizePlane(r3 - r1);
        f.planes_[Near]   = normalizePlane(r3 + r2);
        f.planes_[Far]    = normalizePlane(r3 - r2);
        
        return f;
    }
    
    bool intersectsSphere(const Eigen::Vector3f& center, float radius) const {
        for (const auto& plane : planes_) {
            const float distance = plane.head<3>().dot(center) + plane.w();
            if (distance < -radius) {
                return false;
            }
        }
        return true;
    }
    
    bool intersectsAABB(const BoundingBox& box) const {
        const Eigen::Vector3f center = box.center();
        const Eigen::Vector3f extents = box.extents();
        
        for (const auto& plane : planes_) {
            const Eigen::Vector3f normal = plane.head<3>();
            const float d = plane.w();
            
            const float r = extents.x() * std::abs(normal.x()) +
                           extents.y() * std::abs(normal.y()) +
                           extents.z() * std::abs(normal.z());
            
            const float distance = normal.dot(center) + d;
            
            if (distance < -r) {
                return false;
            }
        }
        return true;
    }
    
    const Eigen::Vector4f& getPlane(Plane p) const { return planes_[p]; }

private:
    static Eigen::Vector4f normalizePlane(const Eigen::Vector4f& plane) {
        const float length = plane.head<3>().norm();
        return (length > 0.0f) ? (plane / length) : plane;
    }
    
    std::array<Eigen::Vector4f, 6> planes_;
};

} 

