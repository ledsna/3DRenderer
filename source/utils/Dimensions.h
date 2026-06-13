#pragma once

#include <cassert>

namespace rasterizer {

struct PixelWidth {
    unsigned int value;
    constexpr explicit PixelWidth(unsigned int v) : value(v) {}
};

struct PixelHeight {
    unsigned int value;
    constexpr explicit PixelHeight(unsigned int v) : value(v) {}
};

struct Near {
    float value;
    constexpr explicit Near(float v) : value(v) {}
};

struct Far {
    float value;
    constexpr explicit Far(float v) : value(v) {}
};

struct FOVY {
    float value;  // In radians
    constexpr explicit FOVY(float v) : value(v) {}
};

struct Aspect {
    float value;
    
    constexpr explicit Aspect(float v) : value(v) {}
    
    constexpr explicit Aspect(PixelWidth width, PixelHeight height) : value(0.0f) {
        assert(height.value != 0);
        value = static_cast<float>(width.value) / static_cast<float>(height.value);
    }
};

} 
