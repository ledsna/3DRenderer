#include "TransformGizmo.h"
#include "../graphics/DrawUtils.h"
#include "../utils/MathUtils.h"
#include <cmath>
#include <algorithm>

namespace rasterizer {

namespace {
    Eigen::Vector3f getAxisDirection(TransformGizmo::Axis axis) {
        switch (axis) {
            case TransformGizmo::Axis::X: return Eigen::Vector3f(1.0f, 0.0f, 0.0f);
            case TransformGizmo::Axis::Y: return Eigen::Vector3f(0.0f, 1.0f, 0.0f);
            case TransformGizmo::Axis::Z: return Eigen::Vector3f(0.0f, 0.0f, 1.0f);
            default: return Eigen::Vector3f::Zero();
        }
    }

    Eigen::Vector3f findClosestPointOnLineToRay(
        const Ray& ray,
        const Eigen::Vector3f& lineOrigin,
        const Eigen::Vector3f& lineDirection
    ) {
        const Eigen::Vector3f w = ray.origin - lineOrigin;
        const float a = lineDirection.dot(lineDirection);
        const float b = lineDirection.dot(ray.direction);
        const float c = ray.direction.dot(ray.direction);
        const float d = lineDirection.dot(w);
        const float e = ray.direction.dot(w);
        
        const float denom = a * c - b * b;
        if (std::abs(denom) < 1e-6f) {
            return lineOrigin;
        }
        
        const float t = -(b * e - c * d) / denom;
        return lineOrigin + lineDirection * t;
    }
}

void TransformGizmo::render(Screen& screen, 
                            const Camera& camera, 
                            const Eigen::Vector3f& worldPosition,
                            Axis hoveredAxis) {
    const float scale = computeGizmoScale(worldPosition, camera);
    
    const Axis axes[] = {Axis::X, Axis::Y, Axis::Z};
    const uint32_t colors[] = {COLOR_X, COLOR_Y, COLOR_Z};
    const uint32_t hoverColors[] = {COLOR_X_HOVER, COLOR_Y_HOVER, COLOR_Z_HOVER};
    
    for (int i = 0; i < 3; ++i) {
        const Eigen::Vector3f dir = getAxisDirection(axes[i]);
        const uint32_t color = (hoveredAxis == axes[i]) ? hoverColors[i] : colors[i];
        drawArrow(screen, camera, worldPosition, dir, scale, color);
    }
}

TransformGizmo::Axis TransformGizmo::hitTest(const Ray& ray, 
                                              const Eigen::Vector3f& worldPosition,
                                              const Camera& camera) const {
    const float scale = computeGizmoScale(worldPosition, camera);
    const float testRadius = ARROW_RADIUS * scale * 3.0f;
    
    float closestDist = std::numeric_limits<float>::max();
    Axis hitAxis = Axis::None;
    
    const Axis axes[] = {Axis::X, Axis::Y, Axis::Z};
    
    for (Axis axis : axes) {
        const Eigen::Vector3f dir = getAxisDirection(axis);
        const Eigen::Vector3f shaftEnd = worldPosition + dir * ARROW_LENGTH * scale;
        const Eigen::Vector3f tip = worldPosition + dir * (ARROW_LENGTH + ARROW_HEAD_LENGTH) * scale;
        
        auto shaftHit = intersectCylinder(ray, worldPosition, shaftEnd, testRadius);
        if (shaftHit && *shaftHit < closestDist) {
            closestDist = *shaftHit;
            hitAxis = axis;
        }
        
        auto coneHit = intersectCylinder(ray, shaftEnd, tip, ARROW_HEAD_RADIUS * scale * 2.0f);
        if (coneHit && *coneHit < closestDist) {
            closestDist = *coneHit;
            hitAxis = axis;
        }
    }
    
    return hitAxis;
}

Eigen::Vector3f TransformGizmo::computeInitialDragPoint(const Ray& ray,
                                                        Axis dragAxis,
                                                        const Eigen::Vector3f& worldPosition) const {
    if (dragAxis == Axis::None) {
        return worldPosition;
    }
    
    const Eigen::Vector3f axisDir = getAxisDirection(dragAxis);
    return findClosestPointOnLineToRay(ray, worldPosition, axisDir);
}

Eigen::Vector3f TransformGizmo::computeDragDelta(const Ray& ray,
                                                 Axis dragAxis,
                                                 const Eigen::Vector3f& worldPosition,
                                                 const Eigen::Vector3f& dragPlanePoint) const {
    if (dragAxis == Axis::None) {
        return Eigen::Vector3f::Zero();
    }
    
    const Eigen::Vector3f axisDir = getAxisDirection(dragAxis);
    const Eigen::Vector3f closestOnAxis = findClosestPointOnLineToRay(ray, worldPosition, axisDir);
    
    const Eigen::Vector3f delta = closestOnAxis - dragPlanePoint;
    
    if (delta.norm() < 0.001f) {
        return Eigen::Vector3f::Zero();
    }
    
    return axisDir * axisDir.dot(delta);
}

float TransformGizmo::computeGizmoScale(const Eigen::Vector3f& gizmoPos, const Camera& camera) const {
    const float distance = (gizmoPos - camera.position()).norm();
    return distance * 0.132f;
}

void TransformGizmo::drawArrow(Screen& screen,
                               const Camera& camera,
                               const Eigen::Vector3f& origin,
                               const Eigen::Vector3f& direction,
                               float scale,
                               uint32_t color) {
    const Eigen::Vector3f shaftEnd = origin + direction * ARROW_LENGTH * scale;
    const Eigen::Vector3f arrowTip = origin + direction * (ARROW_LENGTH + ARROW_HEAD_LENGTH) * scale;
    
    drawLine3D(screen, camera, origin, shaftEnd, color, 1);
    
    const Eigen::Vector3f headBase = shaftEnd;
    const float headRadius = ARROW_HEAD_RADIUS * scale;
    
    Eigen::Vector3f perp1, perp2;
    perp1 = MathUtils::createOrthogonalVector(direction);
    perp2 = direction.cross(perp1).normalized();
    
    const int segments = 12;
    for (int i = 0; i < segments; ++i) {
        const float angle1 = static_cast<float>(i) * 6.28318530718f / static_cast<float>(segments);
        const float angle2 = static_cast<float>(i + 1) * 6.28318530718f / static_cast<float>(segments);
        
        const Eigen::Vector3f p1 = headBase + (perp1 * std::cos(angle1) + perp2 * std::sin(angle1)) * headRadius;
        const Eigen::Vector3f p2 = headBase + (perp1 * std::cos(angle2) + perp2 * std::sin(angle2)) * headRadius;
        
        drawFilledTriangle(screen, camera, p1, p2, arrowTip, color);
    }
}

void TransformGizmo::drawFilledTriangle(Screen& screen,
                                        const Camera& camera,
                                        const Eigen::Vector3f& v0,
                                        const Eigen::Vector3f& v1,
                                        const Eigen::Vector3f& v2,
                                        uint32_t color) {
    int x0, y0, x1, y1, x2, y2;
    
    if (!DrawUtils::worldToScreen(v0, camera, screen.width(), screen.height(), x0, y0)) return;
    if (!DrawUtils::worldToScreen(v1, camera, screen.width(), screen.height(), x1, y1)) return;
    if (!DrawUtils::worldToScreen(v2, camera, screen.width(), screen.height(), x2, y2)) return;
    
    DrawUtils::drawTriangle2D(screen, x0, y0, x1, y1, x2, y2, color);
}

void TransformGizmo::drawLine3D(Screen& screen,
                                const Camera& camera,
                                const Eigen::Vector3f& start,
                                const Eigen::Vector3f& end,
                                uint32_t color,
                                int thickness) {
    int x0, y0, x1, y1;
    
    if (!DrawUtils::worldToScreen(start, camera, screen.width(), screen.height(), x0, y0)) return;
    if (!DrawUtils::worldToScreen(end, camera, screen.width(), screen.height(), x1, y1)) return;
    
    DrawUtils::drawLine2D(screen, x0, y0, x1, y1, color, thickness);
}

std::optional<float> TransformGizmo::intersectCylinder(const Ray& ray,
                                                       const Eigen::Vector3f& cylinderStart,
                                                       const Eigen::Vector3f& cylinderEnd,
                                                       float radius) const {
    const Eigen::Vector3f segmentDir = (cylinderEnd - cylinderStart).normalized();
    const float segmentLength = (cylinderEnd - cylinderStart).norm();
    
    const Eigen::Vector3f w = ray.origin - cylinderStart;
    const float a = ray.direction.dot(ray.direction);
    const float b = ray.direction.dot(segmentDir);
    const float c = segmentDir.dot(segmentDir);
    const float d = ray.direction.dot(w);
    const float e = segmentDir.dot(w);
    
    const float denom = a * c - b * b;
    if (std::abs(denom) < 1e-6f) {
        return std::nullopt;
    }
    
    const float rayT = (b * e - c * d) / denom;
    const float segmentT = (a * e - b * d) / denom;
    
    if (segmentT < 0.0f || segmentT > segmentLength) {
        return std::nullopt;
    }
    
    const Eigen::Vector3f rayPoint = ray.origin + ray.direction * rayT;
    const Eigen::Vector3f segmentPoint = cylinderStart + segmentDir * segmentT;
    const float distance = (rayPoint - segmentPoint).norm();
    
    if (distance <= radius && rayT > 0.0f) {
        return rayT;
    }
    
    return std::nullopt;
}

}
