#include "Camera.h"
#include "../graphics/Viewport.h"
#include "../utils/MathUtils.h"
#include <cmath>

namespace rasterizer {

Camera::Camera()
    : position_(0.0f, 0.0f, 10.0f),
      target_(0.0f, 0.0f, 0.0f),
      up_(0.0f, 1.0f, 0.0f),
      fov_(60.0f * static_cast<float>(M_PI) / 180.0f),
      aspect_(Aspect(PixelWidth(800), PixelHeight(600))),
      near_(Near(0.1f)),
      far_(Far(1000.0f)) {}

void Camera::setAspect(PixelWidth width, PixelHeight height) {
    aspect_ = Aspect(width, height);
}

void Camera::setPerspective(FOVY fieldOfViewY, Aspect aspect, Near nearPlane, Far farPlane) {
    fov_ = fieldOfViewY;
    aspect_ = aspect;
    near_ = nearPlane;
    far_ = farPlane;
}

void Camera::setPerspectiveDegrees(FOVY fovy, Aspect aspect, Near nearPlane, Far farPlane) {
    setPerspective(FOVY(fovy.value * static_cast<float>(M_PI) / 180.0f), aspect, nearPlane, farPlane);
}

void Camera::setPosition(const Eigen::Vector3f& position) {
    position_ = position;
}

void Camera::setTarget(const Eigen::Vector3f& target) {
    target_ = target;
}

void Camera::setUp(const Eigen::Vector3f& up) {
    up_ = up;
}

Eigen::Matrix4f Camera::viewMatrix() const {
    return buildLookAtMatrix(position_, target_, up_);
}

Eigen::Matrix4f Camera::projectionMatrix() const {
    return buildProjectionMatrix(fov_, aspect_, near_, far_);
}

Eigen::Vector3f Camera::rayDirectionFromPixel(float pixelX, float pixelY,
                                               float screenWidth, float screenHeight) const {
    Viewport viewport(PixelWidth(static_cast<uint32_t>(screenWidth)), 
                      PixelHeight(static_cast<uint32_t>(screenHeight)));
    Eigen::Vector2f ndc = viewport.screenToNdc(Eigen::Vector2f(pixelX, pixelY));
    
    float tanHalfFovY = std::tan(fov_.value * 0.5f);
    float viewX = ndc.x() * aspect_.value * tanHalfFovY;
    float viewY = ndc.y() * tanHalfFovY;
    
    // Transform ray direction to world space
    Eigen::Vector3f forward = (target_ - position_).normalized();
    Eigen::Vector3f right = MathUtils::createCameraBasis(forward, up_);
    Eigen::Vector3f upVector = MathUtils::createCameraUp(forward, right);
    
    Eigen::Vector3f rayDirWorld = forward + right * viewX + upVector * viewY;
    return rayDirWorld.normalized();
}

Eigen::Matrix4f Camera::buildProjectionMatrix(FOVY fovY, Aspect aspect, Near zNear, Far zFar) {
    float fovYHalf = fovY.value * 0.5f;
    float f = 1.0f / std::tan(fovYHalf);
    
    Eigen::Matrix4f matrix = Eigen::Matrix4f::Zero();
    matrix(0, 0) = f / aspect.value;
    matrix(1, 1) = f;
    matrix(2, 2) = (zFar.value + zNear.value) / (zNear.value - zFar.value);
    matrix(2, 3) = (2.0f * zFar.value * zNear.value) / (zNear.value - zFar.value);
    matrix(3, 2) = -1.0f;
    return matrix;
}

Eigen::Matrix4f Camera::buildLookAtMatrix(const Eigen::Vector3f& eye,
                                          const Eigen::Vector3f& center,
                                          const Eigen::Vector3f& up) {
    Eigen::Vector3f forward = (center - eye).normalized();
    Eigen::Vector3f right = MathUtils::createCameraBasis(forward, up);
    Eigen::Vector3f upVector = MathUtils::createCameraUp(forward, right);
    
    Eigen::Matrix4f rotationMatrix = Eigen::Matrix4f::Identity();
    rotationMatrix(0, 0) = right.x(); 
    rotationMatrix(0, 1) = right.y(); 
    rotationMatrix(0, 2) = right.z();
    rotationMatrix(1, 0) = upVector.x(); 
    rotationMatrix(1, 1) = upVector.y(); 
    rotationMatrix(1, 2) = upVector.z();
    rotationMatrix(2, 0) = -forward.x(); 
    rotationMatrix(2, 1) = -forward.y(); 
    rotationMatrix(2, 2) = -forward.z();
    
    Eigen::Matrix4f translationMatrix = Eigen::Matrix4f::Identity();
    translationMatrix(0, 3) = -eye.x();
    translationMatrix(1, 3) = -eye.y();
    translationMatrix(2, 3) = -eye.z();
    
    return rotationMatrix * translationMatrix;
}

} 

