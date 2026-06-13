#include "SelectionHighlight.h"
#include <algorithm>
#include "DrawUtils.h"
#include "../utils/BoundingBoxUtils.h"
#include "../utils/EdgeFunctionUtils.h"

namespace rasterizer {

void SelectionHighlight::drawOutline(Screen& screen, 
                                     const Object& object, 
                                     const Camera& camera,
                                     const Rgba& outlineColor,
                                     int thickness) {
    auto& colorBuffer = screen.colorBuffer();
    const size_t width = screen.width();
    const size_t height = screen.height();
    
    // Create mask of object pixels
    std::vector<bool> objectMask = createObjectMask(object, camera, width, height);
    
    // find edge pixels and draw outline
    for (int y = 0; y < static_cast<int>(height); ++y) {
        for (int x = 0; x < static_cast<int>(width); ++x) {
            const size_t idx = y * width + x;
            
            if (!objectMask[idx]) continue;
            
            // Check if current pixel is on the edge
            bool isEdge = false;
            for (int dy = -thickness; dy <= thickness && !isEdge; ++dy) {
                for (int dx = -thickness; dx <= thickness && !isEdge; ++dx) {
                    const int nx = x + dx;
                    const int ny = y + dy;
                    
                    if (nx >= 0 && nx < static_cast<int>(width) && 
                        ny >= 0 && ny < static_cast<int>(height)) {
                        if (!objectMask[ny * width + nx]) {
                            isEdge = true;
                        }
                    }
                }
            }
            
            if (isEdge) {
                colorBuffer[idx] = outlineColor.value;
            }
        }
    }
}

std::vector<bool> SelectionHighlight::createObjectMask(const Object& object,
                                                        const Camera& camera,
                                                        size_t width,
                                                        size_t height) {
    std::vector<bool> mask(width * height, false);
    
    const Eigen::Matrix4f viewMatrix = camera.viewMatrix();
    const Eigen::Matrix4f projMatrix = camera.projectionMatrix();
    const Eigen::Matrix4f modelMatrix = object.transform.modelMatrix();
    const Eigen::Matrix4f MVP = projMatrix * viewMatrix * modelMatrix;
    
    const auto& positions = object.mesh.positions;
    const auto& indices = object.mesh.indices;
    
    // project each triangle and mark pixels
    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        const Eigen::Vector3f& p0 = positions[indices[i]];
        const Eigen::Vector3f& p1 = positions[indices[i + 1]];
        const Eigen::Vector3f& p2 = positions[indices[i + 2]];
        
        // Transform to clip space
        Eigen::Vector4f v0h = MVP * Eigen::Vector4f(p0.x(), p0.y(), p0.z(), 1.0f);
        Eigen::Vector4f v1h = MVP * Eigen::Vector4f(p1.x(), p1.y(), p1.z(), 1.0f);
        Eigen::Vector4f v2h = MVP * Eigen::Vector4f(p2.x(), p2.y(), p2.z(), 1.0f);
        
        // Skip triangles behind camera
        if (v0h.w() <= 0.0f || v1h.w() <= 0.0f || v2h.w() <= 0.0f) continue;
        
        // Perspective divide
        v0h /= v0h.w();
        v1h /= v1h.w();
        v2h /= v2h.w();
        
        // NDC to screen space using DrawUtils
        float fx0, fy0, fx1, fy1, fx2, fy2;
        DrawUtils::worldToScreenFloat(v0h, width, height, fx0, fy0);
        DrawUtils::worldToScreenFloat(v1h, width, height, fx1, fy1);
        DrawUtils::worldToScreenFloat(v2h, width, height, fx2, fy2);
        
        const int x0 = static_cast<int>(fx0);
        const int y0 = static_cast<int>(fy0);
        const int x1 = static_cast<int>(fx1);
        const int y1 = static_cast<int>(fy1);
        const int x2 = static_cast<int>(fx2);
        const int y2 = static_cast<int>(fy2);
        
        int minX, maxX, minY, maxY;
        BoundingBoxUtils::computeTriangleBounds(x0, y0, x1, y1, x2, y2,
                                              width, height,
                                              minX, maxX, minY, maxY);
        
        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                if (EdgeFunctionUtils::pointInTriangle(x, y, x0, y0, x1, y1, x2, y2)) {
                    mask[y * width + x] = true;
                }
            }
        }
    }
    
    return mask;
}

} 

