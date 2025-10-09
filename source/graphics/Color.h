#pragma once

#include <cstdint>
#include <Eigen/Core>

namespace rasterizer {

struct Rgba {
    uint32_t value;
    constexpr explicit Rgba(uint32_t v) : value(v) {}
    
    // ABGR format for direct SFML compatibility: 0xAABBGGRR
    // On little-endian, this gives us [RR][GG][BB][AA] bytes in memory (RGBA order)
    static constexpr Rgba fromRGB(uint8_t r, uint8_t g, uint8_t b) {
        return Rgba(0xFF000000u | (static_cast<uint32_t>(b) << 16) | (static_cast<uint32_t>(g) << 8) | static_cast<uint32_t>(r));
    }
};

// Channel accessors for ABGR format (0xAABBGGRR)
inline uint8_t getR(Rgba c) { return static_cast<uint8_t>(c.value & 0xFFu); }
inline uint8_t getG(Rgba c) { return static_cast<uint8_t>((c.value >> 8) & 0xFFu); }
inline uint8_t getB(Rgba c) { return static_cast<uint8_t>((c.value >> 16) & 0xFFu); }
inline uint8_t getA(Rgba c) { return static_cast<uint8_t>((c.value >> 24) & 0xFFu); }

inline void setR(Rgba& c, uint8_t r) { c.value = (c.value & 0xFFFFFF00u) | static_cast<uint32_t>(r); }
inline void setG(Rgba& c, uint8_t g) { c.value = (c.value & 0xFFFF00FFu) | (static_cast<uint32_t>(g) << 8); }
inline void setB(Rgba& c, uint8_t b) { c.value = (c.value & 0xFF00FFFFu) | (static_cast<uint32_t>(b) << 16); }
inline void setA(Rgba& c, uint8_t a) { c.value = (c.value & 0x00FFFFFFu) | (static_cast<uint32_t>(a) << 24); }

// Convert ABGR (0xAABBGGRR) to floating point RGB [0,1]
inline Eigen::Vector3f unpackRGBf(uint32_t abgr) {
    const float r = static_cast<float>(abgr & 0xFF) / 255.0f;
    const float g = static_cast<float>((abgr >> 8) & 0xFF) / 255.0f;
    const float b = static_cast<float>((abgr >> 16) & 0xFF) / 255.0f;
    return Eigen::Vector3f(r, g, b);
}

inline uint32_t packRGBf(const Eigen::Vector3f& rgb) {
    const uint32_t r = static_cast<uint32_t>(std::clamp(rgb.x(), 0.0f, 1.0f) * 255.0f);
    const uint32_t g = static_cast<uint32_t>(std::clamp(rgb.y(), 0.0f, 1.0f) * 255.0f);
    const uint32_t b = static_cast<uint32_t>(std::clamp(rgb.z(), 0.0f, 1.0f) * 255.0f);
    return 0xFF000000u | (b << 16) | (g << 8) | r;
}

} 

