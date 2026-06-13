#pragma once

#include <Eigen/Core>
#include <cstdint>
#include <memory>
#include "../graphics/Screen.h"
#include "../graphics/RenderPipeline.h"
#include "../graphics/Viewport.h"
#include "../scene/World.h"
#include "Camera.h"

namespace rasterizer {

class Renderer {
public:
    Renderer();

    Screen render(World& world, const Camera& camera, Screen&& screen, bool wireframe = false, bool shadows = true) const;
    
    template<typename ShaderT>
    Screen renderWithShader(World& world, const Camera& camera, Screen&& screen, ShaderT&& shader, bool wireframe = false) const {
        if (viewport_->width() != screen.width() || viewport_->height() != screen.height()) {
            viewport_ = std::make_unique<Viewport>(PixelWidth(screen.width()), PixelHeight(screen.height()));
        }
        const RenderContext context(camera, *viewport_);
        pipeline_->renderWithShader(world, context, screen, std::make_unique<ShaderT>(std::forward<ShaderT>(shader)), wireframe);
        return std::move(screen);
    }
    
    void generateShadowMaps(World& world) const;

private:
    mutable std::unique_ptr<RenderPipeline> pipeline_;
    mutable std::unique_ptr<Viewport> viewport_;
};

} 