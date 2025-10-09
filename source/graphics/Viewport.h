#pragma once

#include <Eigen/Core>
#include "../utils/Dimensions.h"

namespace rasterizer {

class Viewport {
public:
    Viewport(PixelWidth width, PixelHeight height)
        : width_(width.value), height_(height.value) {}
    
    Eigen::Vector2f ndcToScreen(const Eigen::Vector2f& ndc) const {
        return Eigen::Vector2f(
            (ndc.x() * 0.5f + 0.5f) * static_cast<float>(width_),
            (1.0f - (ndc.y() * 0.5f + 0.5f)) * static_cast<float>(height_)
        );
    }
    
    // NDC z to depth buffer value [0,1]
    float ndcToDepth(float ndcZ) const {
        return (ndcZ + 1.0f) * 0.5f;
    }
    
    Eigen::Vector2f screenToNdc(const Eigen::Vector2f& screen) const {
        return Eigen::Vector2f(
            (screen.x() / static_cast<float>(width_)) * 2.0f - 1.0f,
            1.0f - (screen.y() / static_cast<float>(height_)) * 2.0f
        );
    }
    
    bool contains(const Eigen::Vector2f& screen) const {
        return screen.x() >= 0.0f && screen.x() < static_cast<float>(width_) &&
               screen.y() >= 0.0f && screen.y() < static_cast<float>(height_);
    }
    
    bool contains(int x, int y) const {
        return x >= 0 && x < static_cast<int>(width_) &&
               y >= 0 && y < static_cast<int>(height_);
    }
    
    uint32_t width() const { return width_; }
    uint32_t height() const { return height_; }
    float aspectRatio() const { return static_cast<float>(width_) / static_cast<float>(height_); }
    
    void resize(PixelWidth width, PixelHeight height) {
        width_ = width.value;
        height_ = height.value;
    }

private:
    uint32_t width_;
    uint32_t height_;
};

} 

