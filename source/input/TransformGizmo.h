#pragma once

#include "../engine/Camera.h"
#include "../scene/Transform.h"
#include "../graphics/Screen.h"
#include "../utils/Ray.h"
#include <Eigen/Dense>
#include <optional>

namespace rasterizer {

class TransformGizmo {
public:
    enum class Axis {None, X, Y, Z};
    
    TransformGizmo() = default;
    

    void render(Screen& screen, 
                const Camera& camera, 
                const Eigen::Vector3f& worldPosition,
                Axis hoveredAxis = Axis::None);
    

    Axis hitTest(const Ray& ray, 
                 const Eigen::Vector3f& worldPosition,
                 const Camera& camera) const;
    

    Eigen::Vector3f computeInitialDragPoint(const Ray& ray,
                                            Axis dragAxis,
                                            const Eigen::Vector3f& worldPosition) const;
    

    Eigen::Vector3f computeDragDelta(const Ray& ray,
                                     Axis dragAxis,
                                     const Eigen::Vector3f& worldPosition,
                                     const Eigen::Vector3f& dragPlanePoint) const;

private:
    static constexpr float ARROW_LENGTH = 0.77f;
    static constexpr float ARROW_RADIUS = 0.044f;
    static constexpr float ARROW_HEAD_LENGTH = 0.165f;
    static constexpr float ARROW_HEAD_RADIUS = 0.066f;
    
    static constexpr uint32_t COLOR_X = 0xFF0000FFu;
    static constexpr uint32_t COLOR_X_HOVER = 0xFF00FFFFu;
    static constexpr uint32_t COLOR_Y = 0xFF00FF00u;
    static constexpr uint32_t COLOR_Y_HOVER = 0xFF00FFFFu;
    static constexpr uint32_t COLOR_Z = 0xFFFF0000u;
    static constexpr uint32_t COLOR_Z_HOVER = 0xFF00FFFFu;
    
    float computeGizmoScale(const Eigen::Vector3f& gizmoPos, const Camera& camera) const;
    

    void drawArrow(Screen& screen,
                   const Camera& camera,
                   const Eigen::Vector3f& origin,
                   const Eigen::Vector3f& direction,
                   float scale,
                   uint32_t color);
    
    void drawLine3D(Screen& screen,
                    const Camera& camera,
                    const Eigen::Vector3f& start,
                    const Eigen::Vector3f& end,
                    uint32_t color,
                    int thickness = 2);

    void drawFilledTriangle(Screen& screen,
                           const Camera& camera,
                           const Eigen::Vector3f& v0,
                           const Eigen::Vector3f& v1,
                           const Eigen::Vector3f& v2,
                           uint32_t color);
    
    std::optional<float> intersectCylinder(const Ray& ray,
                                           const Eigen::Vector3f& cylinderStart,
                                           const Eigen::Vector3f& cylinderEnd,
                                           float radius) const;
};

} 

