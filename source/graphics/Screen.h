#pragma once

#include <cstdint>
#include <vector>
#include "../graphics/Color.h"
#include "../utils/Dimensions.h"

namespace rasterizer {

class Screen {
public:
    Screen() = default;

    void resize(PixelWidth width, PixelHeight height);
    void clearColor(Rgba color);
    void clearDepth();

    unsigned int width() const { return width_; }
    unsigned int height() const { return height_; }
    
    const uint8_t* colorBytes() const;

    std::vector<uint32_t>& colorBuffer() { return colorBuffer_; }
    std::vector<float>& depthBuffer() { return depthBuffer_; }
    const std::vector<uint32_t>& colorBuffer() const { return colorBuffer_; }
    const std::vector<float>& depthBuffer() const { return depthBuffer_; }

private:
    unsigned int width_ = 0;
    unsigned int height_ = 0;
    std::vector<uint32_t> colorBuffer_;
    std::vector<float> depthBuffer_;
};

}
