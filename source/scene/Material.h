#pragma once

#include <cstdint>
#include <array>
#include "../graphics/Color.h"

namespace rasterizer {

struct Material {
    Rgba albedo{0xFFFFFFFFu};               // White
    float alpha = 1.0f;
    float roughness = 0.5f;
    float specularIntensity = 0.5f;         // Specular strength
    float specularPower = 32.0f;            // Specular shininess
    
    // Check if material requires alpha blending
    bool isTransparent() const { return alpha < 0.99f; }
    
    static Material defaultMaterial() {
        Material m;
        m.albedo = Rgba(0xFFCCCCCCu); // Light gray
        m.roughness = 0.5f;
        m.specularIntensity = 0.5f;
        return m;
    }
    
    static Material plastic() {
        Material m;
        m.albedo = Rgba(0xFFFFFFFFu);
        m.roughness = 0.3f;
        m.specularIntensity = 0.6f;
        m.specularPower = 64.0f;
        return m;
    }
    
    static Material metal() {
        Material m;
        m.albedo = Rgba(0xFFCCCCCCu);
        m.roughness = 0.2f;
        m.specularIntensity = 1.0f;
        m.specularPower = 128.0f;
        return m;
    }
    
    static Material rubber() {
        Material m;
        m.albedo = Rgba(0xFF333333u);
        m.roughness = 0.8f;
        m.specularIntensity = 0.2f;
        m.specularPower = 16.0f;
        return m;
    }
    
    static Material glass() {
        Material m;
        m.albedo = Rgba(0xFFDDEEFFu);
        m.alpha = 0.3f;
        m.roughness = 0.0f;
        m.specularIntensity = 1.0f;
        m.specularPower = 256.0f;
        return m;
    }
};

// basicaly GI
struct AmbientLight {
    Rgba color{0xFF404040u};                // Dim gray by default
    float intensity = 0.3f;                 // Ambient strength
};

struct DirectionalLight {
    std::string name = "Directional Light";
    std::array<float, 3> direction{0.0f, -1.0f, 0.0f};   // Points down
    Rgba color{0xFFFFFFFFu};                             // White
    float intensity = 0.8f;                              // Bright
    bool castsShadows = true;                            // Enable shadow casting
    float shadowBias = 0.003f;                           // Shadow acne prevention (slope-scaled + normal offset)
};

struct PointLight {
    std::string name = "Point Light";
    std::array<float, 3> position{0.0f, 2.0f, 0.0f};     // Above origin
    Rgba color{0xFFFFFFFFu};                             // White
    float intensity = 1.0f;                              // Full brightness
    float range = 10.0f;                                 // Attenuation distance
};

} 
