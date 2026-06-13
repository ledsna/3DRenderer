#include "RenderPipeline.h"
#include "../graphics/Color.h"
#include "../utils/TriangleClipper.h"
#include "../utils/BoundingBoxUtils.h"
#include "../utils/EdgeFunctionUtils.h"
#include <algorithm>
#include <cmath>

namespace rasterizer {

void RenderPipeline::render(const World& world, const RenderContext& context, Screen& screen, bool wireframe) const {
    std::vector<const Object*> opaqueObjects;
    std::vector<const Object*> transparentObjects;
    
    for (const auto& obj : world.objects) {
        if (!context.frustum().intersectsSphere(obj.getBoundingSphereCenter(), obj.getBoundingSphereRadius())) {
            continue;
        }
        
        if (obj.material.isTransparent()) {
            transparentObjects.push_back(&obj);
        } else {
            opaqueObjects.push_back(&obj);
        }
    }
    
    for (const Object* obj : opaqueObjects) {
        renderObject(*obj, world, context, screen, wireframe);
    }
    
    // render transparent objects from farthest to nearest
    const Eigen::Vector3f camPos = context.cameraPosition();
    std::sort(transparentObjects.begin(), transparentObjects.end(),
        [&camPos](const Object* a, const Object* b) {
            const float distA = (a->transform.position - camPos).squaredNorm();
            const float distB = (b->transform.position - camPos).squaredNorm();
            return distA > distB;
        });
    
    for (const Object* obj : transparentObjects) {
        renderObject(*obj, world, context, screen, wireframe);
    }
}

void RenderPipeline::renderWithShader(const World& world, const RenderContext& context, Screen& screen, 
                                      std::unique_ptr<IShader> shader, bool wireframe) const {
    auto originalShader = std::move(const_cast<RenderPipeline*>(this)->shader_);
    const_cast<RenderPipeline*>(this)->shader_ = std::move(shader);
    
    for (const auto& obj : world.objects) {
        if (!context.frustum().intersectsSphere(obj.getBoundingSphereCenter(), obj.getBoundingSphereRadius())) {
            continue;
        }
        renderObject(obj, world, context, screen, wireframe);
    }
    
    const_cast<RenderPipeline*>(this)->shader_ = std::move(originalShader);
}

void RenderPipeline::renderObject(const Object& obj, const World& world, const RenderContext& context, Screen& screen, bool wireframe) const {
    const Eigen::Matrix4f modelMatrix = obj.transform.modelMatrix();
    const Eigen::Matrix4f mvpMatrix = context.computeMVP(modelMatrix);
    const auto& viewport = context.viewport();
    
    const auto& positions = obj.mesh.positions;
    const auto& indices = obj.mesh.indices;
    
    if (wireframe) {
        std::vector<Eigen::Vector4f> clipSpaceVerts;
        clipSpaceVerts.reserve(positions.size());
        
        for (const auto& p : positions) {
            clipSpaceVerts.push_back(mvpMatrix * Eigen::Vector4f(p.x(), p.y(), p.z(), 1.0f));
        }
        
        for (size_t i = 0; i + 2 < indices.size(); i += 3) {
            const uint32_t ia = indices[i];
            const uint32_t ib = indices[i + 1];
            const uint32_t ic = indices[i + 2];
            
            auto drawEdge = [&](uint32_t i0, uint32_t i1) {
                const Eigen::Vector4f& c0 = clipSpaceVerts[i0];
                const Eigen::Vector4f& c1 = clipSpaceVerts[i1];
                
                if (c0.w() <= 0.0f || c1.w() <= 0.0f) return;
                
                const Eigen::Vector3f ndc0 = c0.head<3>() / c0.w();
                const Eigen::Vector3f ndc1 = c1.head<3>() / c1.w();
                
                if (ndc0.x() < -1 && ndc1.x() < -1) return;
                if (ndc0.x() >  1 && ndc1.x() >  1) return;
                if (ndc0.y() < -1 && ndc1.y() < -1) return;
                if (ndc0.y() >  1 && ndc1.y() >  1) return;
                if (ndc0.z() < -1 && ndc1.z() < -1) return;
                if (ndc0.z() >  1 && ndc1.z() >  1) return;
                
                const Eigen::Vector2f s0 = viewport.ndcToScreen(ndc0.head<2>());
                const Eigen::Vector2f s1 = viewport.ndcToScreen(ndc1.head<2>());
                const float z0 = viewport.ndcToDepth(ndc0.z());
                const float z1 = viewport.ndcToDepth(ndc1.z());
                
                rasterizeLine(screen, viewport, s0, s1, z0, z1, 0xFFFFFFFFu);
            };
            
            drawEdge(ia, ib);
            drawEdge(ib, ic);
            drawEdge(ic, ia);
        }
    } else {
        const Eigen::Vector3f baseColor = unpackRGBf(obj.material.albedo.value);
        
        for (size_t i = 0; i + 2 < indices.size(); i += 3) {
            const Eigen::Vector3f& a = positions[indices[i]];
            const Eigen::Vector3f& b = positions[indices[i + 1]];
            const Eigen::Vector3f& c = positions[indices[i + 2]];
            
            Eigen::Vector3f nA, nB, nC;
            const bool haveNormals = obj.mesh.normals.size() == positions.size();
            
            if (haveNormals) {
                const Eigen::Matrix3f normalMatrix = modelMatrix.topLeftCorner<3,3>().inverse().transpose();
                nA = (normalMatrix * obj.mesh.normals[indices[i]]).normalized();
                nB = (normalMatrix * obj.mesh.normals[indices[i + 1]]).normalized();
                nC = (normalMatrix * obj.mesh.normals[indices[i + 2]]).normalized();
            } else {
                const Eigen::Vector4f wa4 = modelMatrix * Eigen::Vector4f(a.x(), a.y(), a.z(), 1.0f);
                const Eigen::Vector4f wb4 = modelMatrix * Eigen::Vector4f(b.x(), b.y(), b.z(), 1.0f);
                const Eigen::Vector4f wc4 = modelMatrix * Eigen::Vector4f(c.x(), c.y(), c.z(), 1.0f);
                const Eigen::Vector3f e1 = (wb4 - wa4).head<3>();
                const Eigen::Vector3f e2 = (wc4 - wa4).head<3>();
                const Eigen::Vector3f n = e1.cross(e2).normalized();
                nA = nB = nC = n;
            }
            
            const Eigen::Vector4f ha = mvpMatrix * Eigen::Vector4f(a.x(), a.y(), a.z(), 1.0f);
            const Eigen::Vector4f hb = mvpMatrix * Eigen::Vector4f(b.x(), b.y(), b.z(), 1.0f);
            const Eigen::Vector4f hc = mvpMatrix * Eigen::Vector4f(c.x(), c.y(), c.z(), 1.0f);
            
            const Eigen::Vector3f wa = (modelMatrix * Eigen::Vector4f(a.x(), a.y(), a.z(), 1.0f)).head<3>();
            const Eigen::Vector3f wb = (modelMatrix * Eigen::Vector4f(b.x(), b.y(), b.z(), 1.0f)).head<3>();
            const Eigen::Vector3f wc = (modelMatrix * Eigen::Vector4f(c.x(), c.y(), c.z(), 1.0f)).head<3>();
            
            ClipVertex v0{ha, wa, nA};
            ClipVertex v1{hb, wb, nB};
            ClipVertex v2{hc, wc, nC};
            
            auto clippedTriangles = TriangleClipper::clipTriangle(v0, v1, v2);
            
            for (const auto& tri : clippedTriangles) {
                const Eigen::Vector3f na = tri[0].clipPos.head<3>() / tri[0].clipPos.w();
                const Eigen::Vector3f nb = tri[1].clipPos.head<3>() / tri[1].clipPos.w();
                const Eigen::Vector3f nc = tri[2].clipPos.head<3>() / tri[2].clipPos.w();
                
                const Eigen::Vector2f a2(na.x(), na.y());
                const Eigen::Vector2f b2(nb.x(), nb.y());
                const Eigen::Vector2f c2(nc.x(), nc.y());
                const float area = (b2 - a2).x() * (c2 - a2).y() - (b2 - a2).y() * (c2 - a2).x();
                
                if (area == 0.0f) continue;
                if (area < 0.0f) continue;
                
                const Eigen::Vector2f sa = viewport.ndcToScreen(na.head<2>());
                const Eigen::Vector2f sb = viewport.ndcToScreen(nb.head<2>());
                const Eigen::Vector2f sc = viewport.ndcToScreen(nc.head<2>());
                const float za = viewport.ndcToDepth(na.z());
                const float zb = viewport.ndcToDepth(nb.z());
                const float zc = viewport.ndcToDepth(nc.z());
                
                const Eigen::Vector3f& waClipped = tri[0].worldPos;
                const Eigen::Vector3f& wbClipped = tri[1].worldPos;
                const Eigen::Vector3f& wcClipped = tri[2].worldPos;
                const Eigen::Vector3f& nAClipped = tri[0].worldNormal;
                const Eigen::Vector3f& nBClipped = tri[1].worldNormal;
                const Eigen::Vector3f& nCClipped = tri[2].worldNormal;
                
                const float invWa = 1.0f / std::max(1e-6f, tri[0].clipPos.w());
                const float invWb = 1.0f / std::max(1e-6f, tri[1].clipPos.w());
                const float invWc = 1.0f / std::max(1e-6f, tri[2].clipPos.w());
                
                rasterizeTriangleShaded(screen, sa, sb, sc, za, zb, zc, waClipped, wbClipped, wcClipped, 
                                       nAClipped, nBClipped, nCClipped, invWa, invWb, invWc, 
                                       baseColor, obj.material, context, world);
            }
        }
    }
}

void RenderPipeline::rasterizeTriangleShaded(
    Screen& screen,
    const Eigen::Vector2f& screenPosA, const Eigen::Vector2f& screenPosB, const Eigen::Vector2f& screenPosC,
    const float depthA, const float depthB, const float depthC,
    const Eigen::Vector3f& worldPosA, const Eigen::Vector3f& worldPosB, const Eigen::Vector3f& worldPosC,
    const Eigen::Vector3f& worldNormalA, const Eigen::Vector3f& worldNormalB, const Eigen::Vector3f& worldNormalC,
    const float invClipW_A, const float invClipW_B, const float invClipW_C,
    const Eigen::Vector3f& materialColor,
    const Material& material,
    const RenderContext& context,
    const World& world
) const {
    int boundMinX, boundMaxX, boundMinY, boundMaxY;
    BoundingBoxUtils::computeTriangleBounds(screenPosA, screenPosB, screenPosC,
                                          screen.width(), screen.height(),
                                          boundMinX, boundMaxX, boundMinY, boundMaxY);
    
    if (boundMinX > boundMaxX || boundMinY > boundMaxY) return;
    
    const float triangleAreaTimes2 = EdgeFunctionUtils::edgeFunction(screenPosA, screenPosB, screenPosC);
    
    if (triangleAreaTimes2 == 0.0f) return;
    const float invTriangleArea = 1.0f / triangleAreaTimes2;
    
    auto& depthBuffer = screen.depthBuffer();
    auto& colorBuffer = screen.colorBuffer();
    const size_t screenWidth = screen.width();
    
    for (int pixelY = boundMinY; pixelY <= boundMaxY; ++pixelY) {
        for (int pixelX = boundMinX; pixelX <= boundMaxX; ++pixelX) {
            const Eigen::Vector2f pixelCenter(static_cast<float>(pixelX) + 0.5f, 
                                             static_cast<float>(pixelY) + 0.5f);
            
            float barycentricA = EdgeFunctionUtils::edgeFunction(pixelCenter, screenPosB, screenPosC) * invTriangleArea;
            float barycentricB = EdgeFunctionUtils::edgeFunction(pixelCenter, screenPosC, screenPosA) * invTriangleArea;
            float barycentricC = EdgeFunctionUtils::edgeFunction(pixelCenter, screenPosA, screenPosB) * invTriangleArea;
            
            // black magic hack value picked empyrically after hours of testing
            constexpr float edgeEpsilon = -1e-8f;
            if (barycentricA >= edgeEpsilon && barycentricB >= edgeEpsilon && barycentricC >= edgeEpsilon) {
                barycentricA = std::max(0.0f, barycentricA);
                barycentricB = std::max(0.0f, barycentricB);
                barycentricC = std::max(0.0f, barycentricC);
                
                const float perspectiveWeightA = barycentricA * invClipW_A;
                const float perspectiveWeightB = barycentricB * invClipW_B;
                const float perspectiveWeightC = barycentricC * invClipW_C;
                const float perspectiveSum = perspectiveWeightA + perspectiveWeightB + perspectiveWeightC;
                
                if (perspectiveSum > 1e-12f) {
                    const float invPerspectiveSum = 1.0f / perspectiveSum;
                    const float correctedWeightA = perspectiveWeightA * invPerspectiveSum;
                    const float correctedWeightB = perspectiveWeightB * invPerspectiveSum;
                    const float correctedWeightC = perspectiveWeightC * invPerspectiveSum;
                    
                    float fragmentDepth = barycentricA * depthA + barycentricB * depthB + barycentricC * depthC;
                    
                    // Z-fighting fix: slope-scaled bias
                    const float dzdx = std::abs(depthA - depthB) + std::abs(depthB - depthC);
                    const float dzdy = std::abs(depthA - depthC);
                    const float maxSlope = std::max(dzdx, dzdy);
                    
                    const float slopeBias = maxSlope * 0.0001f;
                    const float constantBias = 0.00001f;
                    fragmentDepth += slopeBias + constantBias;
                    
                    const size_t bufferIndex = static_cast<size_t>(pixelY) * screenWidth + static_cast<size_t>(pixelX);
                    
                    const bool isTransparent = material.isTransparent();
                    const bool passDepthTest = fragmentDepth <= depthBuffer[bufferIndex];
                    
                    if (passDepthTest || isTransparent) {
                        if (!isTransparent && passDepthTest) {
                            depthBuffer[bufferIndex] = fragmentDepth - constantBias;
                        }
                        
                        FragmentInput fragment;
                        fragment.position = correctedWeightA * worldPosA + 
                                          correctedWeightB * worldPosB + 
                                          correctedWeightC * worldPosC;
                        fragment.normal = (correctedWeightA * worldNormalA + 
                                         correctedWeightB * worldNormalB + 
                                         correctedWeightC * worldNormalC).normalized();
                        fragment.color = materialColor;
                        fragment.roughness = material.roughness;
                        fragment.specularIntensity = material.specularIntensity;
                        fragment.specularPower = material.specularPower;
                        
                        const Eigen::Vector3f litColor = shader_->shade(fragment, world, context.cameraPosition());
                        
                        if (isTransparent) {
                            const Eigen::Vector3f existingColor = unpackRGBf(colorBuffer[bufferIndex]);
                            const float alpha = material.alpha;
                            const Eigen::Vector3f blendedColor = litColor * alpha + existingColor * (1.0f - alpha);
                            colorBuffer[bufferIndex] = packRGBf(blendedColor);
                        } else {
                            colorBuffer[bufferIndex] = packRGBf(litColor);
                        }
                    }
                }
            }
        }
    }
}

void RenderPipeline::rasterizeLine(Screen& screen, const Viewport& viewport,
                                   const Eigen::Vector2f& startPos, const Eigen::Vector2f& endPos,
                                   const float startDepth, const float endDepth, const uint32_t lineColor) const {
    const float deltaX = endPos.x() - startPos.x();
    const float deltaY = endPos.y() - startPos.y();
    const int numSteps = static_cast<int>(std::ceil(std::max(std::abs(deltaX), std::abs(deltaY))));
    
    if (numSteps <= 0) return;
    
    const float stepX = deltaX / static_cast<float>(numSteps);
    const float stepY = deltaY / static_cast<float>(numSteps);
    const float stepDepth = (endDepth - startDepth) / static_cast<float>(numSteps);
    
    auto& depthBuffer = screen.depthBuffer();
    auto& colorBuffer = screen.colorBuffer();
    const size_t screenWidth = screen.width();
    
    // Rasterize line with sub-pixel accuracy
    float currentX = startPos.x();
    float currentY = startPos.y();
    float currentDepth = startDepth;
    
    for (int step = 0; step <= numSteps; ++step) {
        // Round to nearest pixel
        const int pixelX = static_cast<int>(currentX + 0.5f);
        const int pixelY = static_cast<int>(currentY + 0.5f);
        
        if (viewport.contains(pixelX, pixelY)) {
            const size_t bufferIndex = static_cast<size_t>(pixelY) * screenWidth + static_cast<size_t>(pixelX);
            
            if (currentDepth < depthBuffer[bufferIndex]) {
                depthBuffer[bufferIndex] = currentDepth;
                colorBuffer[bufferIndex] = lineColor;
            }
        }
        
        currentX += stepX;
        currentY += stepY;
        currentDepth += stepDepth;
    }
}

} 

