#pragma once

#include <Eigen/Core>
#include <algorithm>
#include <cmath>

namespace rasterizer {

class BoundingBoxUtils {
public:
    template<typename T>
    static void computeTriangleBounds(T x0, T y0, T x1, T y1, T x2, T y2,
                                    int screenWidth, int screenHeight,
                                    int& minX, int& maxX, int& minY, int& maxY) {
        minX = std::max(0, static_cast<int>(std::floor(std::min({x0, x1, x2}))));
        maxX = std::min(screenWidth - 1, static_cast<int>(std::ceil(std::max({x0, x1, x2}))));
        minY = std::max(0, static_cast<int>(std::floor(std::min({y0, y1, y2}))));
        maxY = std::min(screenHeight - 1, static_cast<int>(std::ceil(std::max({y0, y1, y2}))));
    }
    
    static void computeTriangleBounds(const Eigen::Vector2f& a, const Eigen::Vector2f& b, const Eigen::Vector2f& c,
                                    int screenWidth, int screenHeight,
                                    int& minX, int& maxX, int& minY, int& maxY) {
        minX = std::max(0, static_cast<int>(std::floor(std::min({a.x(), b.x(), c.x()}))));
        maxX = std::min(screenWidth - 1, static_cast<int>(std::ceil(std::max({a.x(), b.x(), c.x()}))));
        minY = std::max(0, static_cast<int>(std::floor(std::min({a.y(), b.y(), c.y()}))));
        maxY = std::min(screenHeight - 1, static_cast<int>(std::ceil(std::max({a.y(), b.y(), c.y()}))));
    }
};

} 
