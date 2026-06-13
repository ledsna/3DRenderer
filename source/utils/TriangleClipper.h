#pragma once

#include <Eigen/Core>
#include <vector>
#include <array>
#include <functional>

namespace rasterizer {

struct ClipVertex {
    Eigen::Vector4f clipPos;
    Eigen::Vector3f worldPos;
    Eigen::Vector3f worldNormal;
    
    static ClipVertex lerp(const ClipVertex& a, const ClipVertex& b, float t) {
        ClipVertex result;
        result.clipPos = a.clipPos + t * (b.clipPos - a.clipPos);
        result.worldPos = a.worldPos + t * (b.worldPos - a.worldPos);
        result.worldNormal = (a.worldNormal + t * (b.worldNormal - a.worldNormal)).normalized();
        return result;
    }
};

class TriangleClipper {
public:
    using PlaneFunc = std::function<float(const Eigen::Vector4f&)>;
    
    static std::vector<std::array<ClipVertex, 3>> clipTriangle(
        const ClipVertex& v0,
        const ClipVertex& v1,
        const ClipVertex& v2
    ) {
        std::vector<ClipVertex> polygon = {v0, v1, v2};
        
        
        polygon = clipAgainstPlane(polygon, [](const Eigen::Vector4f& v) { return v.x() + v.w(); });
        if (polygon.empty()) return {};
        
        polygon = clipAgainstPlane(polygon, [](const Eigen::Vector4f& v) { return -v.x() + v.w(); });
        if (polygon.empty()) return {};
        
        polygon = clipAgainstPlane(polygon, [](const Eigen::Vector4f& v) { return v.y() + v.w(); });
        if (polygon.empty()) return {};
        
        polygon = clipAgainstPlane(polygon, [](const Eigen::Vector4f& v) { return -v.y() + v.w(); });
        if (polygon.empty()) return {};
        
        polygon = clipAgainstPlane(polygon, [](const Eigen::Vector4f& v) { return v.z() + v.w(); });
        if (polygon.empty()) return {};
        
        polygon = clipAgainstPlane(polygon, [](const Eigen::Vector4f& v) { return -v.z() + v.w(); });
        if (polygon.empty()) return {};
        
        return triangulate(polygon);
    }
    
private:
    static std::vector<ClipVertex> clipAgainstPlane(
        const std::vector<ClipVertex>& vertices,
        const PlaneFunc& planeDist
    ) {
        if (vertices.size() < 3) return {};
        
        std::vector<ClipVertex> output;
        output.reserve(vertices.size() + 1); // funny
        
        for (size_t i = 0; i < vertices.size(); ++i) {
            const ClipVertex& current = vertices[i];
            const ClipVertex& next = vertices[(i + 1) % vertices.size()];
            
            const float currentDist = planeDist(current.clipPos);
            const float nextDist = planeDist(next.clipPos);
            
            const bool currentInside = (currentDist >= 0.0f);
            const bool nextInside = (nextDist >= 0.0f);
            
            if (currentInside && nextInside) {
                output.push_back(next);
            } else if (currentInside && !nextInside) {
                const float t = currentDist / (currentDist - nextDist);
                output.push_back(ClipVertex::lerp(current, next, t));
            } else if (!currentInside && nextInside) {
                const float t = currentDist / (currentDist - nextDist);
                output.push_back(ClipVertex::lerp(current, next, t));
                output.push_back(next);
            }
        }
        
        return output;
    }
    
    static std::vector<std::array<ClipVertex, 3>> triangulate(const std::vector<ClipVertex>& polygon) {
        if (polygon.size() < 3) return {};
        
        std::vector<std::array<ClipVertex, 3>> triangles;
        triangles.reserve(polygon.size() - 2);
        
        // Fan triangulation from vertex 0
        for (size_t i = 1; i + 1 < polygon.size(); ++i) {
            triangles.push_back({polygon[0], polygon[i], polygon[i + 1]});
        }
        
        return triangles;
    }
};

} 

