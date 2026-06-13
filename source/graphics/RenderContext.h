#pragma once

#include <Eigen/Core>
#include "../engine/Camera.h"
#include "Viewport.h"
#include "../utils/Frustum.h"

namespace rasterizer {

class RenderContext {
public:
    RenderContext(const Camera& camera, const Viewport& viewport)
        : cameraPosition_(camera.position()),
          viewMatrix_(camera.viewMatrix()),
          projectionMatrix_(camera.projectionMatrix()),
          viewProjectionMatrix_(projectionMatrix_ * viewMatrix_),
          frustum_(Frustum::fromViewProjection(viewProjectionMatrix_)),
          viewport_(viewport) {}
    
    const Eigen::Vector3f& cameraPosition() const { return cameraPosition_; }
    const Eigen::Matrix4f& viewMatrix() const { return viewMatrix_; }
    const Eigen::Matrix4f& projectionMatrix() const { return projectionMatrix_; }
    const Eigen::Matrix4f& viewProjectionMatrix() const { return viewProjectionMatrix_; }
    const Frustum& frustum() const { return frustum_; }
    const Viewport& viewport() const { return viewport_; }
    
    Eigen::Matrix4f computeMVP(const Eigen::Matrix4f& modelMatrix) const {
        return viewProjectionMatrix_ * modelMatrix;
    }

private:
    Eigen::Vector3f cameraPosition_;
    Eigen::Matrix4f viewMatrix_;
    Eigen::Matrix4f projectionMatrix_;
    Eigen::Matrix4f viewProjectionMatrix_;
    Frustum frustum_;
    Viewport viewport_;
};

} 

