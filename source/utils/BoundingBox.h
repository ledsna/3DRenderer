#pragma once

#include <Eigen/Core>
#include <algorithm>
#include <vector>

namespace rasterizer {

class BoundingBox {
public:
    BoundingBox() : min_(Eigen::Vector3f::Zero()), max_(Eigen::Vector3f::Zero()) {}
    
    BoundingBox(const Eigen::Vector3f& min, const Eigen::Vector3f& max)
        : min_(min), max_(max) {}
    
    static BoundingBox fromPoints(const std::vector<Eigen::Vector3f>& points) {
        if (points.empty()) return BoundingBox();
        
        Eigen::Vector3f min = points[0];
        Eigen::Vector3f max = points[0];
        
        for (const auto& p : points) {
            min = min.cwiseMin(p);
            max = max.cwiseMax(p);
        }
        
        return BoundingBox(min, max);
    }
    
    const Eigen::Vector3f& min() const { return min_; }
    const Eigen::Vector3f& max() const { return max_; }
    
    Eigen::Vector3f center() const { return (min_ + max_) * 0.5f; }
    Eigen::Vector3f extents() const { return (max_ - min_) * 0.5f; }
    Eigen::Vector3f size() const { return max_ - min_; }
    
    float radius() const { return extents().norm(); }
    
    BoundingBox transformed(const Eigen::Matrix4f& transform) const {
        const Eigen::Vector3f corners[8] = {
            {min_.x(), min_.y(), min_.z()},
            {max_.x(), min_.y(), min_.z()},
            {min_.x(), max_.y(), min_.z()},
            {max_.x(), max_.y(), min_.z()},
            {min_.x(), min_.y(), max_.z()},
            {max_.x(), min_.y(), max_.z()},
            {min_.x(), max_.y(), max_.z()},
            {max_.x(), max_.y(), max_.z()}
        };
        
        std::vector<Eigen::Vector3f> transformedCorners;
        transformedCorners.reserve(8);
        
        for (const auto& corner : corners) {
            const Eigen::Vector4f h = transform * Eigen::Vector4f(corner.x(), corner.y(), corner.z(), 1.0f);
            transformedCorners.emplace_back(h.head<3>() / h.w());
        }
        
        return fromPoints(transformedCorners);
    }
    
    void expand(const BoundingBox& other) {
        min_ = min_.cwiseMin(other.min_);
        max_ = max_.cwiseMax(other.max_);
    }
    
    void expand(const Eigen::Vector3f& point) {
        min_ = min_.cwiseMin(point);
        max_ = max_.cwiseMax(point);
    }
    
    bool contains(const Eigen::Vector3f& point) const {
        return (point.x() >= min_.x() && point.x() <= max_.x() &&
                point.y() >= min_.y() && point.y() <= max_.y() &&
                point.z() >= min_.z() && point.z() <= max_.z());
    }
    
    bool intersects(const BoundingBox& other) const {
        return (min_.x() <= other.max_.x() && max_.x() >= other.min_.x() &&
                min_.y() <= other.max_.y() && max_.y() >= other.min_.y() &&
                min_.z() <= other.max_.z() && max_.z() >= other.min_.z());
    }

private:
    Eigen::Vector3f min_;
    Eigen::Vector3f max_;
};

} 

