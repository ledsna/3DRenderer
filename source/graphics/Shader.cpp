#include "Shader.h"
#include "DrawUtils.h"
#include "../utils/MathUtils.h"
#include <algorithm>
#include <cmath>

namespace rasterizer {

float BlinnPhongShader::computeShadow(const Eigen::Vector3f& worldPos, const Eigen::Vector3f& normal, 
                    const DirectionalLight& light, const ShadowMap& shadowMap, float bias) const {
    Eigen::Vector3f lightDir(light.direction[0], light.direction[1], light.direction[2]);
    lightDir.normalize();
    
    const Eigen::Vector3f lightPos = -lightDir * 50.0f;
    const Eigen::Vector3f target = Eigen::Vector3f::Zero();
    const Eigen::Vector3f up = std::abs(lightDir.y()) < 0.99f 
        ? Eigen::Vector3f(0.0f, 1.0f, 0.0f)
        : Eigen::Vector3f(1.0f, 0.0f, 0.0f);
    
    const Eigen::Vector3f zAxis = (lightPos - target).normalized();
    const Eigen::Vector3f xAxis = MathUtils::createCameraBasis(zAxis, up);
    const Eigen::Vector3f yAxis = MathUtils::createCameraUp(zAxis, xAxis);
    
    Eigen::Matrix4f lightView = Eigen::Matrix4f::Identity();
    lightView.block<1, 3>(0, 0) = xAxis.transpose();
    lightView.block<1, 3>(1, 0) = yAxis.transpose();
    lightView.block<1, 3>(2, 0) = zAxis.transpose();
    lightView(0, 3) = -xAxis.dot(lightPos);
    lightView(1, 3) = -yAxis.dot(lightPos);
    lightView(2, 3) = -zAxis.dot(lightPos);
    
    const float orthoSize = 10.0f;
    const float near = 0.1f;
    const float far = 100.0f;
    
    Eigen::Matrix4f lightProj = Eigen::Matrix4f::Zero();
    lightProj(0, 0) = 1.0f / orthoSize;
    lightProj(1, 1) = 1.0f / orthoSize;
    lightProj(2, 2) = -2.0f / (far - near);
    lightProj(2, 3) = -(far + near) / (far - near);
    lightProj(3, 3) = 1.0f;
    
    const Eigen::Matrix4f lightVP = lightProj * lightView;
    
    const Eigen::Vector3f offsetPos = worldPos + normal * bias * 2.0f;
    
    const Eigen::Vector4f lightSpacePos = lightVP * Eigen::Vector4f(offsetPos.x(), offsetPos.y(), offsetPos.z(), 1.0f);
    const Eigen::Vector3f lightNDC = lightSpacePos.head<3>() / lightSpacePos.w();
    
    float u, v;
    DrawUtils::worldToScreenFloat(Eigen::Vector4f(lightNDC.x(), lightNDC.y(), lightNDC.z(), 1.0f), 
                                 1, 1, u, v); // dummy size, we only need u,v
    const float compareDepth = lightNDC.z();
    
    const float shadowIntensity = shadowMap.samplePCF(u, v, compareDepth, bias * 0.1f);
    
    return 1.0f - shadowIntensity;
}

Eigen::Vector3f BlinnPhongShader::shade(const FragmentInput& input, const World& world, const Eigen::Vector3f& cameraPos) const {
    const Eigen::Vector3f N = input.normal;
    const Eigen::Vector3f V = (cameraPos - input.position).normalized();
    
    const Eigen::Vector3f baseColor = input.color;
    const float roughness = input.roughness;
    const float specularPower = input.specularPower;
    const float specularIntensity = input.specularIntensity;
    
    const Eigen::Vector3f ambientColor = unpackRGBf(world.ambient.color.value) * world.ambient.intensity;
    Eigen::Vector3f accumulatedLight = baseColor.cwiseProduct(ambientColor);
    
    for (size_t lightIdx = 0; lightIdx < world.dirLights.size(); ++lightIdx) {
        const auto& light = world.dirLights[lightIdx];
        const Eigen::Vector3f L(-light.direction[0], -light.direction[1], -light.direction[2]);
        const Eigen::Vector3f Lnorm = L.normalized();
        const float NdotL = std::max(0.0f, N.dot(Lnorm));
        
        if (NdotL > 0.0f) {
            float shadowFactor = 1.0f;
            
            if (light.castsShadows && lightIdx < world.shadowMaps.size() && world.shadowMaps[lightIdx]) {
                const float cosTheta = std::max(0.0f, N.dot(Lnorm));
                const float slopeBias = std::tan(std::acos(cosTheta));
                const float adaptiveBias = light.shadowBias * (1.0f + slopeBias * 0.5f);
                shadowFactor = computeShadow(input.position, N, light, *world.shadowMaps[lightIdx], adaptiveBias);
            }
            
            const Eigen::Vector3f H = (Lnorm + V).normalized();
            const float NdotH = std::max(0.0f, N.dot(H));
            const float specular = std::pow(NdotH, specularPower);
            
            const Eigen::Vector3f lightColor = unpackRGBf(light.color.value) * light.intensity;
            
            const float diffuseStrength = 0.3f + roughness * 0.7f;
            accumulatedLight += baseColor.cwiseProduct(lightColor) * (NdotL * diffuseStrength * shadowFactor);
            
            const float specularStrength = (1.0f - roughness);
            accumulatedLight += lightColor * (specularIntensity * specular * specularStrength * shadowFactor);
        }
    }
    
    for (const auto& light : world.pointLights) {
        const Eigen::Vector3f lightPos(light.position[0], light.position[1], light.position[2]);
        Eigen::Vector3f L = lightPos - input.position;
        const float distance = L.norm();
        
        if (distance > 1e-6f && distance < light.range) {
            L /= distance;
            const float NdotL = std::max(0.0f, N.dot(L));
            
            if (NdotL > 0.0f) {
                const Eigen::Vector3f H = (L + V).normalized();
                const float NdotH = std::max(0.0f, N.dot(H));
                const float specular = std::pow(NdotH, specularPower);
                
                const float distanceRatio = distance / light.range;
                const float attenuation = 1.0f / (1.0f + distance * distance * 0.1f);
                const float rangeFalloff = 1.0f - std::min(1.0f, distanceRatio * distanceRatio);
                const float finalAttenuation = attenuation * rangeFalloff;
                
                const Eigen::Vector3f lightColor = unpackRGBf(light.color.value) * (light.intensity * finalAttenuation);
                
                const float diffuseStrength = 0.3f + roughness * 0.7f;
                accumulatedLight += baseColor.cwiseProduct(lightColor) * (NdotL * diffuseStrength);
                
                const float specularStrength = (1.0f - roughness);
                accumulatedLight += lightColor * (specularIntensity * specular * specularStrength);
            }
        }
    }
    
    return accumulatedLight.cwiseMin(Eigen::Vector3f(1, 1, 1)).cwiseMax(Eigen::Vector3f(0, 0, 0));
}

}

