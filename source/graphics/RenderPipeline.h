

#pragma once

#include <Eigen/Core>
#include <memory>
#include "Screen.h"
#include "RenderContext.h"
#include "Shader.h"
#include "../scene/World.h"

namespace rasterizer {

class RenderPipeline {
public:
    RenderPipeline() : shader_(std::make_unique<BlinnPhongShader>()) {}
    
    void setShader(std::unique_ptr<IShader> shader) {
        shader_ = std::move(shader);
    }
    
    void render(const World& world, const RenderContext& context, Screen& screen, bool wireframe = false) const;
    
    void renderWithShader(const World& world, const RenderContext& context, Screen& screen, 
                          std::unique_ptr<IShader> shader, bool wireframe = false) const;

private:
    void renderObject(const Object& obj, const World& world, const RenderContext& context, Screen& screen, bool wireframe) const;
    
    void rasterizeTriangleShaded(
        Screen& screen,
        const Eigen::Vector2f& screenPosA, const Eigen::Vector2f& screenPosB, const Eigen::Vector2f& screenPosC,
        float depthA, float depthB, float depthC,
        const Eigen::Vector3f& worldPosA, const Eigen::Vector3f& worldPosB, const Eigen::Vector3f& worldPosC,
        const Eigen::Vector3f& worldNormalA, const Eigen::Vector3f& worldNormalB, const Eigen::Vector3f& worldNormalC,
        float invClipW_A, float invClipW_B, float invClipW_C,
        const Eigen::Vector3f& materialColor,
        const Material& material,
        const RenderContext& context,
        const World& world
    ) const;
    
    void rasterizeLine(Screen& screen, const Viewport& viewport,
                      const Eigen::Vector2f& startPos, const Eigen::Vector2f& endPos,
                      float startDepth, float endDepth, uint32_t lineColor) const;
    
    std::unique_ptr<IShader> shader_;
};

} 

