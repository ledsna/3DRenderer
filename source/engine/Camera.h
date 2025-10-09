#pragma once

#include <Eigen/Core>
#include <Eigen/Geometry>
#include "../utils/Dimensions.h"

namespace rasterizer {

class Camera {
public:
    Camera();
    
    void setAspect(PixelWidth width, PixelHeight height);
    void setPerspective(FOVY fieldOfViewY, Aspect aspect, Near nearPlane, Far farPlane);
    void setPerspectiveDegrees(FOVY fovy, Aspect aspect, Near nearPlane, Far farPlane);
    
    void setPosition(const Eigen::Vector3f& position);
    void setTarget(const Eigen::Vector3f& target);
    void setUp(const Eigen::Vector3f& up);
    
    const Eigen::Vector3f& position() const { return position_; }
    const Eigen::Vector3f& target() const { return target_; }
    const Eigen::Vector3f& up() const { return up_; }
    
    Eigen::Matrix4f viewMatrix() const;
    Eigen::Matrix4f projectionMatrix() const;
    
    float getFOV() const { return fov_.value; }
    float getAspect() const { return aspect_.value; }
    float getNear() const { return near_.value; }
    float getFar() const { return far_.value; }
    
    /**
     * Create a ray from camera through a screen pixel
     * @param pixelX Pixel X coordinate (0 = left)
     * @param pixelY Pixel Y coordinate (0 = top)
     * @param screenWidth Viewport width
     * @param screenHeight Viewport height
     * @return Normalized ray direction in world space
     */
    Eigen::Vector3f rayDirectionFromPixel(float pixelX, float pixelY, 
                                          float screenWidth, float screenHeight) const;

private:
    static Eigen::Matrix4f buildProjectionMatrix(FOVY fovY, Aspect aspect, Near zNear, Far zFar);
    static Eigen::Matrix4f buildLookAtMatrix(const Eigen::Vector3f& eye, 
                                             const Eigen::Vector3f& center, 
                                             const Eigen::Vector3f& up);
    
    Eigen::Vector3f position_;
    Eigen::Vector3f target_;
    Eigen::Vector3f up_;
    
    FOVY fov_;
    Aspect aspect_;
    Near near_;
    Far far_;
};

} 

