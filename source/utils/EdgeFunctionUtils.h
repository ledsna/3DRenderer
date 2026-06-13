#pragma once

#include <Eigen/Core>

namespace rasterizer {

class EdgeFunctionUtils {
public:
    static int edgeFunction(int ax, int ay, int bx, int by, int px, int py) {
        return (px - ax) * (by - ay) - (py - ay) * (bx - ax);
    }
    
    static float edgeFunction(const Eigen::Vector2f& point, 
                            const Eigen::Vector2f& edgeStart, 
                            const Eigen::Vector2f& edgeEnd) {
        return (point.x() - edgeStart.x()) * (edgeEnd.y() - edgeStart.y()) - 
               (point.y() - edgeStart.y()) * (edgeEnd.x() - edgeStart.x());
    }
    
    static bool pointInTriangle(int px, int py, int ax, int ay, int bx, int by, int cx, int cy) {
        const int d1 = edgeFunction(px, py, ax, ay, bx, by);
        const int d2 = edgeFunction(px, py, bx, by, cx, cy);
        const int d3 = edgeFunction(px, py, cx, cy, ax, ay);
        
        const bool hasNeg = (d1 < 0) || (d2 < 0) || (d3 < 0);
        const bool hasPos = (d1 > 0) || (d2 > 0) || (d3 > 0);
        
        return !(hasNeg && hasPos);
    }
    
    static bool pointInTriangle(const Eigen::Vector2f& point,
                              const Eigen::Vector2f& a, const Eigen::Vector2f& b, const Eigen::Vector2f& c) {
        const float d1 = edgeFunction(point, a, b);
        const float d2 = edgeFunction(point, b, c);
        const float d3 = edgeFunction(point, c, a);
        
        const bool hasNeg = (d1 < 0.0f) || (d2 < 0.0f) || (d3 < 0.0f);
        const bool hasPos = (d1 > 0.0f) || (d2 > 0.0f) || (d3 > 0.0f);
        
        return !(hasNeg && hasPos);
    }
};

} 
