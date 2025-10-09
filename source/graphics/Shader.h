#pragma once

#include <Eigen/Core>
#include "../scene/World.h"
#include "../graphics/Color.h"
#include "ShadowMap.h"

namespace rasterizer {

struct FragmentInput {
    Eigen::Vector3f position;  // World-space position
    Eigen::Vector3f normal;    // World-space normalized normal
    Eigen::Vector2f texcoord;  // Texture coordinates (future use)
    Eigen::Vector3f color;     // [0,1]
    float roughness;           // [0,1]
    float specularIntensity;   // Specular intensity
    float specularPower;       // Specular power
};

class IShader {
public:
    virtual ~IShader() = default;
    
    // Shade a fragment and return final color
    virtual Eigen::Vector3f shade(const FragmentInput& input, const World& world, const Eigen::Vector3f& cameraPos) const = 0;
};

class FlatShader : public IShader {
public:
    Eigen::Vector3f shade(const FragmentInput& input, const World& /*world*/, const Eigen::Vector3f& /*cameraPos*/) const override {
        return input.color;
    }
};

class BlinnPhongShader : public IShader {
public:
    BlinnPhongShader() = default;
    
    Eigen::Vector3f shade(const FragmentInput& input, const World& world, const Eigen::Vector3f& cameraPos) const override;
    
private:
    float computeShadow(const Eigen::Vector3f& worldPos, const Eigen::Vector3f& normal, 
                        const DirectionalLight& light, const ShadowMap& shadowMap, float bias) const;
};

class NormalShader : public IShader {
public:
    Eigen::Vector3f shade(const FragmentInput& input, const World& /*world*/, const Eigen::Vector3f& /*cameraPos*/) const override {
        return (input.normal + Eigen::Vector3f(1, 1, 1)) * 0.5f;
    }
};

// Renders fragments based on their distance from camera
class DepthShader : public IShader {
public:
    DepthShader(float nearPlane, float /*тут могла быть ваша реклама*/) 
        : near_(nearPlane), visualizeFar_(30.0f) {}
    
    Eigen::Vector3f shade(const FragmentInput& input, const World& /*world*/, const Eigen::Vector3f& cameraPos) const override {
        float eyeDepth = (input.position - cameraPos).norm();
        float normalized = std::clamp((eyeDepth - near_) / (visualizeFar_ - near_), 0.0f, 1.0f);
        float gray = 1.0f - normalized;
        return Eigen::Vector3f(gray, gray, gray);
    }
    
private:
    float near_;
    float visualizeFar_;
};

} 

