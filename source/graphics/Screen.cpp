#include "Screen.h"
#include <algorithm>
#include <limits>

namespace rasterizer {

void Screen::resize(PixelWidth width, PixelHeight height) {
    width_ = width.value;
    height_ = height.value;
    colorBuffer_.assign(static_cast<size_t>(width_) * static_cast<size_t>(height_), 0xFF000000u);
    depthBuffer_.assign(static_cast<size_t>(width_) * static_cast<size_t>(height_), std::numeric_limits<float>::infinity());
}

void Screen::clearColor(Rgba color) {
    std::fill(colorBuffer_.begin(), colorBuffer_.end(), color.value);
}

void Screen::clearDepth() {
    std::fill(depthBuffer_.begin(), depthBuffer_.end(), std::numeric_limits<float>::infinity());
}

const uint8_t* Screen::colorBytes() const {
    return reinterpret_cast<const uint8_t*>(colorBuffer_.data());
}

}

