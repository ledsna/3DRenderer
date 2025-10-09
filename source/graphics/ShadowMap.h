#pragma once

#include <vector>
#include <limits>
#include <Eigen/Dense>

namespace rasterizer {

class ShadowMap {
public:
    ShadowMap(unsigned int width, unsigned int height)
        : width_(width)
        , height_(height)
        , depthBuffer_(static_cast<size_t>(width) * height, std::numeric_limits<float>::infinity())
    {}
    
    void clear() {
        std::fill(depthBuffer_.begin(), depthBuffer_.end(), std::numeric_limits<float>::infinity());
    }
    
    unsigned int width() const { return width_; }
    unsigned int height() const { return height_; }
    
    std::vector<float>& depthBuffer() { return depthBuffer_; }
    const std::vector<float>& depthBuffer() const { return depthBuffer_; }
    
    bool isInShadow(float u, float v, float compareDepth, float bias = 0.005f) const {
        if (u < 0.0f || u > 1.0f || v < 0.0f || v > 1.0f) {
            return false; // Outside shadow map bounds => not in shadow
        }
        
        const int x = static_cast<int>(u * static_cast<float>(width_));
        const int y = static_cast<int>(v * static_cast<float>(height_));
        
        if (x < 0 || x >= static_cast<int>(width_) || 
            y < 0 || y >= static_cast<int>(height_)) {
            return false;
        }
        
        const size_t index = static_cast<size_t>(y) * width_ + static_cast<size_t>(x);
        const float shadowDepth = depthBuffer_[index];
        
        // If current fragment is farther from light than stored depth, it's in shadow
        return compareDepth > shadowDepth + bias;
    }
    
    // PCF (Percentage Closer Filtering) for soft shadows with bilinear interpolation
    float samplePCF(float u, float v, float compareDepth, float bias = 0.005f) const {
        if (u < 0.0f || u > 1.0f || v < 0.0f || v > 1.0f) {
            return 0.0f; // Outside shadow map => fully lit
        }
        
        const float texelX = u * static_cast<float>(width_);
        const float texelY = v * static_cast<float>(height_);
        
        // Bilinear interpolation: sample 2x2 grid with fractional weights
        const int x0 = static_cast<int>(std::floor(texelX - 0.5f));
        const int y0 = static_cast<int>(std::floor(texelY - 0.5f));
        
        const float fx = (texelX - 0.5f) - std::floor(texelX - 0.5f);
        const float fy = (texelY - 0.5f) - std::floor(texelY - 0.5f);
        
        float shadow = 0.0f;
        float totalWeight = 0.0f;
        
        for (int dy = 0; dy <= 1; ++dy) {
            for (int dx = 0; dx <= 1; ++dx) {
                const int sx = x0 + dx;
                const int sy = y0 + dy;
                
                if (sx >= 0 && sx < static_cast<int>(width_) && 
                    sy >= 0 && sy < static_cast<int>(height_)) {
                    
                    const size_t index = static_cast<size_t>(sy) * width_ + static_cast<size_t>(sx);
                    const float shadowDepth = depthBuffer_[index];
                    
                    // Bilinear weight
                    const float wx = (dx == 0) ? (1.0f - fx) : fx;
                    const float wy = (dy == 0) ? (1.0f - fy) : fy;
                    const float weight = wx * wy;
                    
                    if (compareDepth > shadowDepth + bias) {
                        shadow += weight;
                    }
                    totalWeight += weight;
                }
            }
        }
        
        return (totalWeight > 0.0f) ? (shadow / totalWeight) : 0.0f;
    }
    
private:
    unsigned int width_;
    unsigned int height_;
    std::vector<float> depthBuffer_;
};

} 

