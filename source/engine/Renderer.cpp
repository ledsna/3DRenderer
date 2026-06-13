#include "Renderer.h"
#include "../graphics/RenderContext.h"
#include "../graphics/Color.h"
#include "../graphics/ShadowMap.h"
#include "../graphics/DrawUtils.h"
#include "../utils/BoundingBoxUtils.h"
#include "../utils/EdgeFunctionUtils.h"
#include "../utils/MathUtils.h"
#include <map>

namespace rasterizer {

Renderer::Renderer()
    : pipeline_(std::make_unique<RenderPipeline>()),
      viewport_(std::make_unique<Viewport>(PixelWidth(800), PixelHeight(600))) {}

Screen Renderer::render(World& world, const Camera& camera, Screen&& screen, bool wireframe, bool shadows) const {
    if (viewport_->width() != screen.width() || viewport_->height() != screen.height()) {
        viewport_ = std::make_unique<Viewport>(PixelWidth(screen.width()), PixelHeight(screen.height()));
    }
    
    if (!wireframe && shadows) {
        generateShadowMaps(world);
    }
    
    const RenderContext context(camera, *viewport_);
    
    pipeline_->render(world, context, screen, wireframe);
    
    return std::move(screen);
}

void Renderer::generateShadowMaps(World& world) const {
    constexpr unsigned int SHADOW_MAP_SIZE = 1024;
    
    while (world.shadowMaps.size() < world.dirLights.size()) {
        world.shadowMaps.push_back(std::make_unique<ShadowMap>(SHADOW_MAP_SIZE, SHADOW_MAP_SIZE));
    }
    
    for (size_t lightIdx = 0; lightIdx < world.dirLights.size(); ++lightIdx) {
        const auto& light = world.dirLights[lightIdx];
        
        if (!light.castsShadows) {
            continue;
        }
        
        auto& shadowMap = world.shadowMaps[lightIdx];
        shadowMap->clear();
        
        Eigen::Vector3f lightDir(light.direction[0], light.direction[1], light.direction[2]);
        lightDir.normalize();
        
        const Eigen::Vector3f lightPos = -lightDir * 50.0f;  // Far enough to see the whole scene
        
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
        
        const float orthoSize = 10.0f;  // Size of shadow frustum
        const float near = 0.1f;
        const float far = 100.0f;
        
        Eigen::Matrix4f lightProj = Eigen::Matrix4f::Zero();
        lightProj(0, 0) = 1.0f / orthoSize;
        lightProj(1, 1) = 1.0f / orthoSize;
        lightProj(2, 2) = -2.0f / (far - near);
        lightProj(2, 3) = -(far + near) / (far - near);
        lightProj(3, 3) = 1.0f;
        
        const Eigen::Matrix4f lightVP = lightProj * lightView;
        
        // Render all objects to shadow map (depth only)
        auto& depthBuffer = shadowMap->depthBuffer();
        const unsigned int width = shadowMap->width();
        const unsigned int height = shadowMap->height();
        
        for (const auto& obj : world.objects) {
            const Eigen::Matrix4f modelMatrix = obj.transform.modelMatrix();
            const Eigen::Matrix4f lightMVP = lightVP * modelMatrix;
            
            const auto& positions = obj.mesh.positions;
            const auto& indices = obj.mesh.indices;
            
            // Rasterize triangles into shadow map
            for (size_t i = 0; i + 2 < indices.size(); i += 3) {
                const Eigen::Vector3f& p0 = positions[indices[i]];
                const Eigen::Vector3f& p1 = positions[indices[i + 1]];
                const Eigen::Vector3f& p2 = positions[indices[i + 2]];
                
                // Transform to light clip space
                Eigen::Vector4f v0 = lightMVP * Eigen::Vector4f(p0.x(), p0.y(), p0.z(), 1.0f);
                Eigen::Vector4f v1 = lightMVP * Eigen::Vector4f(p1.x(), p1.y(), p1.z(), 1.0f);
                Eigen::Vector4f v2 = lightMVP * Eigen::Vector4f(p2.x(), p2.y(), p2.z(), 1.0f);
                
                if (v0.w() > 0.0f) v0 /= v0.w();
                if (v1.w() > 0.0f) v1 /= v1.w();
                if (v2.w() > 0.0f) v2 /= v2.w();
                
                // Convert to shadow map texture space [0, width] x [0, height]
                float sx0, sy0, sx1, sy1, sx2, sy2;
                DrawUtils::worldToScreenFloat(v0, width, height, sx0, sy0);
                DrawUtils::worldToScreenFloat(v1, width, height, sx1, sy1);
                DrawUtils::worldToScreenFloat(v2, width, height, sx2, sy2);
                
                const float sz0 = v0.z();
                const float sz1 = v1.z();
                const float sz2 = v2.z();
                
                int minX, maxX, minY, maxY;
                BoundingBoxUtils::computeTriangleBounds(sx0, sy0, sx1, sy1, sx2, sy2,
                                                      width, height,
                                                      minX, maxX, minY, maxY);
                
                const float area = EdgeFunctionUtils::edgeFunction(sx0, sy0, sx1, sy1, sx2, sy2);
                if (std::abs(area) < 1e-6f) continue;
                const float invArea = 1.0f / area;
                
                // Rasterize
                for (int y = minY; y <= maxY; ++y) {
                    for (int x = minX; x <= maxX; ++x) {
                        const float px = static_cast<float>(x) + 0.5f;
                        const float py = static_cast<float>(y) + 0.5f;
                        
                        const float w0 = EdgeFunctionUtils::edgeFunction(px, py, sx1, sy1, sx2, sy2) * invArea;
                        const float w1 = EdgeFunctionUtils::edgeFunction(px, py, sx2, sy2, sx0, sy0) * invArea;
                        const float w2 = EdgeFunctionUtils::edgeFunction(px, py, sx0, sy0, sx1, sy1) * invArea;
                        
                        if (w0 >= 0.0f && w1 >= 0.0f && w2 >= 0.0f) {
                            const float depth = w0 * sz0 + w1 * sz1 + w2 * sz2;
                            const size_t idx = static_cast<size_t>(y) * width + static_cast<size_t>(x);
                            
                            // Update shadow map depth (keep closest)
                            if (depth < depthBuffer[idx]) {
                                depthBuffer[idx] = depth;
                            }
                        }
                    }
                }
            }
        }
    }
}

} 
