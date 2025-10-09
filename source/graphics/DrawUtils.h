#pragma once

#include "Screen.h"
#include "../engine/Camera.h"
#include "Viewport.h"
#include "../utils/BoundingBoxUtils.h"
#include "../utils/EdgeFunctionUtils.h"
#include <Eigen/Core>
#include <algorithm>

namespace rasterizer {

class DrawUtils {
public:
    
    static void drawTriangle2D(Screen& screen,
                               int x0, int y0,
                               int x1, int y1,
                               int x2, int y2,
                               uint32_t color) {
        int minX, maxX, minY, maxY;
        BoundingBoxUtils::computeTriangleBounds(x0, y0, x1, y1, x2, y2,
                                              screen.width(), screen.height(),
                                              minX, maxX, minY, maxY);
        
        auto& colorBuffer = screen.colorBuffer();
        const size_t width = screen.width();
        
        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                if (EdgeFunctionUtils::pointInTriangle(x, y, x0, y0, x1, y1, x2, y2)) {
                    colorBuffer[y * width + x] = color;
                }
            }
        }
    }
    
    
    static void drawLine2D(Screen& screen,
                          int x0, int y0,
                          int x1, int y1,
                          uint32_t color,
                          int thickness = 1) {
        auto& colorBuffer = screen.colorBuffer();
        const int width = static_cast<int>(screen.width());
        const int height = static_cast<int>(screen.height());
        
        int dx = std::abs(x1 - x0);
        int dy = std::abs(y1 - y0);
        int sx = (x0 < x1) ? 1 : -1;
        int sy = (y0 < y1) ? 1 : -1;
        int err = dx - dy;
        
        int x = x0, y = y0;
        
        while (true) {
            for (int dy_off = -thickness; dy_off <= thickness; ++dy_off) {
                for (int dx_off = -thickness; dx_off <= thickness; ++dx_off) {
                    const int px = x + dx_off;
                    const int py = y + dy_off;
                    
                    if (px >= 0 && px < width && py >= 0 && py < height) {
                        colorBuffer[py * width + px] = color;
                    }
                }
            }
            
            if (x == x1 && y == y1) break;
            
            const int e2 = 2 * err;
            if (e2 > -dy) {
                err -= dy;
                x += sx;
            }
            if (e2 < dx) {
                err += dx;
                y += sy;
            }
        }
    }
    
    
    static bool worldToScreen(const Eigen::Vector3f& worldPos,
                             const Camera& camera,
                             int screenWidth,
                             int screenHeight,
                             int& outX,
                             int& outY) {
        const Eigen::Matrix4f VP = camera.projectionMatrix() * camera.viewMatrix();
        Eigen::Vector4f clip = VP * Eigen::Vector4f(worldPos.x(), worldPos.y(), worldPos.z(), 1.0f);
        
        if (clip.w() <= 0.0f) return false;
        
        clip /= clip.w();
        
        Viewport viewport(PixelWidth(static_cast<uint32_t>(screenWidth)), 
                          PixelHeight(static_cast<uint32_t>(screenHeight)));
        Eigen::Vector2f screen = viewport.ndcToScreen(clip.head<2>());
        
        outX = static_cast<int>(screen.x());
        outY = static_cast<int>(screen.y());
        
        return true;
    }
    
    static bool worldToScreenFloat(const Eigen::Vector4f& ndcPos,
                                  int screenWidth,
                                  int screenHeight,
                                  float& outX,
                                  float& outY) {
        if (ndcPos.w() <= 0.0f) return false;
        
        Eigen::Vector4f clip = ndcPos;
        clip /= clip.w();
        
        Viewport viewport(PixelWidth(static_cast<uint32_t>(screenWidth)), 
                          PixelHeight(static_cast<uint32_t>(screenHeight)));
        Eigen::Vector2f screen = viewport.ndcToScreen(clip.head<2>());
        
        outX = screen.x();
        outY = screen.y();
        
        return true;
    }
};

} 

