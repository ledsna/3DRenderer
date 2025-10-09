#include "Transform.h"

namespace rasterizer {

Eigen::Matrix4f Transform::modelMatrix() const {
    constexpr float DEG_TO_RAD = 3.14159265358979323846f / 180.0f;
    const float pitch = rotationEuler.x() * DEG_TO_RAD;
    const float yaw   = rotationEuler.y() * DEG_TO_RAD;
    const float roll  = rotationEuler.z() * DEG_TO_RAD;
    Eigen::AngleAxisf rx(pitch, Eigen::Vector3f::UnitX());
    Eigen::AngleAxisf ry(yaw,   Eigen::Vector3f::UnitY());
    Eigen::AngleAxisf rz(roll,  Eigen::Vector3f::UnitZ());
    Eigen::Matrix3f rot = (ry * rx * rz).toRotationMatrix();
    Eigen::Matrix4f model = Eigen::Matrix4f::Identity();
    model.block<3,3>(0,0) = rot * Eigen::DiagonalMatrix<float,3>(scale.x(), scale.y(), scale.z());
    model(0,3) = position.x();
    model(1,3) = position.y();
    model(2,3) = position.z();
    return model;
}

} 

