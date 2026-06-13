#include "Application.h"

#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <chrono>
#include <cmath>
#include <cstdio>
#include "../assets/ObjLoader.h"
#include "../utils/Ray.h"
#include "../utils/FileDialog.h"
#include "../utils/MathUtils.h"
#include "../graphics/SelectionHighlight.h"
#include "../graphics/Shader.h"

namespace rasterizer {

static constexpr float INSPECTOR_WIDTH = 280.0f;
static constexpr float HIERARCHY_WIDTH = 220.0f;

void Application::update(float deltaSeconds) {
    float cy = std::cos(camCtrl_.yaw), sy = std::sin(camCtrl_.yaw);
    float cp = std::cos(camCtrl_.pitch), sp = std::sin(camCtrl_.pitch);
    Eigen::Vector3f forward = Eigen::Vector3f(cy * cp, sp, sy * cp).normalized();
    Eigen::Vector3f right = MathUtils::createCameraBasis(forward, Eigen::Vector3f(0,1,0));
    Eigen::Vector3f up = MathUtils::createCameraUp(forward, right);

    camera_.setTarget(camera_.position() + forward);

    float move = camCtrl_.moveSpeed * deltaSeconds;
    if (isAnyKeyPressed(keyBindings_.sprint)) move *= 4.0f;
    Eigen::Vector3f camPos = camera_.position();
    if (isAnyKeyPressed(keyBindings_.moveForward)) camPos += forward * move;
    if (isAnyKeyPressed(keyBindings_.moveBack))    camPos -= forward * move;
    if (isAnyKeyPressed(keyBindings_.moveLeft))    camPos -= right * move;
    if (isAnyKeyPressed(keyBindings_.moveRight))   camPos += right * move;
    if (isAnyKeyPressed(keyBindings_.moveUp))      camPos += up * move;
    if (isAnyKeyPressed(keyBindings_.moveDown))    camPos -= up * move;
    if ((camPos - camera_.position()).squaredNorm() > 0.0f) {
        camera_.setPosition(camPos);
        camera_.setTarget(camPos + forward);
    }
}

Application::Application(PixelWidth width, PixelHeight height, const std::string& title)
    : width_(width), height_(height),
      window_(sf::VideoMode(sf::Vector2u(width.value, height.value)), title, sf::Style::Default),
      framebufferTexture_(sf::Vector2u(width.value, height.value)),
      framebufferSprite_(framebufferTexture_) {
    window_.setVerticalSyncEnabled(false);

    screen_.resize(width_, height_);
    camera_.setPerspectiveDegrees(FOVY(60.0f), Aspect(width_, height_), Near(0.1f), Far(1000.0f));
    camera_.setPosition(Eigen::Vector3f(0.0f, 0.0f, 5.0f));
    camera_.setTarget(Eigen::Vector3f(0.0f, 0.0f, 0.0f));

    framebufferTexture_.setSmooth(false);
    framebufferSprite_.setPosition(sf::Vector2f(0.0f, 0.0f));
    framebufferSprite_.setScale(sf::Vector2f(1.0f, 1.0f));
    sf::View view(sf::Vector2f(static_cast<float>(width.value) * 0.5f, static_cast<float>(height.value) * 0.5f),
                  sf::Vector2f(static_cast<float>(width.value), static_cast<float>(height.value)));
    window_.setView(view);

    initDemoScene();
    selectedObjectIndex_ = -1;
    initUI();
}

void Application::initDemoScene() {
    DirectionalLight sun;
    sun.name = "Sun";
    sun.direction = {0.3f, -0.8f, 0.5f};
    sun.color = Rgba(0xFFFFF4E6u);
    sun.intensity = 0.9f;
    world_.dirLights.push_back(sun);
    
    PointLight fillLight;
    fillLight.name = "Fill Light";
    fillLight.position = {-3.0f, 2.0f, 3.0f};
    fillLight.color = Rgba(0xFFE6F0FFu);
    fillLight.intensity = 0.6f;
    fillLight.range = 15.0f;
    world_.pointLights.push_back(fillLight);
    
    world_.ambient.color = Rgba::fromRGB(20, 20, 20);
    world_.ambient.intensity = 1.0f;  // 1.0 = full ambient color is applied
}

void Application::importObj(const std::string& path) {
    ObjLoader loader;
    auto result = loader.load(path);
    
    if (result.isError()) {
        const std::string errorMsg = "[ERROR] " + result.error().message;
        std::fprintf(stderr, "%s\n", errorMsg.c_str());
        return;
    }
    
    Object obj = result.takeValue();
    obj.name = generateUniqueName(obj.name);
    
    Eigen::Vector3f forward = (camera_.target() - camera_.position()).normalized();
    obj.transform.position = camera_.position() + forward * 2.0f;
    obj.computeLocalBounds();
    
    world_.objects.push_back(std::move(obj));
    selectedObjectIndex_ = static_cast<int>(world_.objects.size()) - 1;
}

bool Application::isAnyKeyPressed(const std::vector<sf::Keyboard::Scancode>& keys) {
    for (auto k : keys) if (sf::Keyboard::isKeyPressed(k)) return true;
    return false;
}

void Application::run() {
    using clock = std::chrono::steady_clock;
    auto lastTime = clock::now();
    bool running = true;

    while (running) {
        while (auto event = window_.pollEvent()) {
            handleEvent(*event, running);
        }

        auto currentTime = clock::now();
        float deltaTime = std::chrono::duration_cast<std::chrono::duration<float>>(currentTime - lastTime).count();
        lastTime = currentTime;

        try {
            update(deltaTime);
        renderFrame(deltaTime);
            if (overlayUI_) {
                // Переключение режимов рендера
                if (overlayUI_->consumeCycleRenderMode()) {
                    if (debugMode_ == DebugMode::None && !wireframe_) {
                        wireframe_ = true;
                        overlayUI_->setRenderMode(true, false, false);
                    } else if (wireframe_) {
                        wireframe_ = false;
                        debugMode_ = DebugMode::DepthBuffer;
                        overlayUI_->setRenderMode(false, true, false);
                    } else if (debugMode_ == DebugMode::DepthBuffer) {
                        debugMode_ = DebugMode::Normals;
                        overlayUI_->setRenderMode(false, false, true);
                    } else {
                        debugMode_ = DebugMode::None;
                        overlayUI_->setRenderMode(false, false, false);
                    }
                }
                if (overlayUI_->consumeSpawnCube()) {
                    Object cube; cube.mesh = Mesh::makeCube(0.5f); cube.name = generateUniqueName("Cube"); cube.material = Material::defaultMaterial();
                    Eigen::Vector3f forward = (camera_.target() - camera_.position()).normalized(); cube.transform.position = camera_.position() + forward * 2.0f; cube.computeLocalBounds(); world_.objects.push_back(cube);
                }
                if (overlayUI_->consumeSpawnSphere()) {
                    Object sph; sph.mesh = Mesh::makeUVSphere(0.6f, 16, 12); sph.name = generateUniqueName("Sphere"); sph.material = Material::defaultMaterial();
                    Eigen::Vector3f forward = (camera_.target() - camera_.position()).normalized(); sph.transform.position = camera_.position() + forward * 2.0f; sph.computeLocalBounds(); world_.objects.push_back(sph);
                }
                if (overlayUI_->consumeAddDirLight()) {
                    DirectionalLight dl; dl.name = generateUniqueName("Directional Light"); world_.dirLights.push_back(dl);
                }
                if (overlayUI_->consumeAddPointLight()) {
                    PointLight pl; pl.name = generateUniqueName("Point Light"); world_.pointLights.push_back(pl);
                }
                if (overlayUI_->consumeToggleShadows()) {
                    shadowsEnabled_ = !shadowsEnabled_;
                    for (auto& light : world_.dirLights) {
                        light.castsShadows = shadowsEnabled_;
                    }
                    overlayUI_->setShadowsEnabled(shadowsEnabled_);
                }
                if (overlayUI_->consumeDeleteSelected()) {
                    if (selectedObjectIndex_ >= 0) {
                        size_t objCount = world_.objects.size();
                        size_t dirCount = world_.dirLights.size();
                        
                        if (selectedObjectIndex_ < static_cast<int>(objCount)) {
                            world_.objects.erase(world_.objects.begin() + selectedObjectIndex_);
                        } else if (selectedObjectIndex_ < static_cast<int>(objCount + dirCount)) {
                            world_.dirLights.erase(world_.dirLights.begin() + (selectedObjectIndex_ - objCount));
                        } else {
                            world_.pointLights.erase(world_.pointLights.begin() + (selectedObjectIndex_ - objCount - dirCount));
                        }
                        selectedObjectIndex_ = -1;
                    }
                }
            }
            if (overlayUI_ && overlayUI_->importRequested()) {
                auto pathOpt = FileDialog::openFile("Select .obj file", "obj");
                if (pathOpt.has_value()) {
                    importObj(*pathOpt);
                }
                overlayUI_->resetImportRequest();
            }
        } catch (const std::exception& e) {
            std::fprintf(stderr, "Exception in frame: %s\n", e.what());
        } catch (...) {
            std::fprintf(stderr, "Unknown exception in frame\n");
        }
    }
}

void Application::handleEvent(const sf::Event& event, bool& running) {
    if (event.is<sf::Event::Closed>()) {
        window_.close();
        running = false;
        std::exit(0);
    }
    if (const auto* resized = event.getIf<sf::Event::Resized>()) {
        width_ = PixelWidth(resized->size.x);
        height_ = PixelHeight(resized->size.y);
        screen_.resize(width_, height_);
        camera_.setPerspectiveDegrees(FOVY(60.0f), Aspect(width_, height_), Near(0.1f), Far(1000.0f));
        framebufferTexture_ = sf::Texture(sf::Vector2u(width_.value, height_.value));
        framebufferTexture_.setSmooth(false);
        framebufferSprite_ = sf::Sprite(framebufferTexture_);
        framebufferSprite_.setPosition(sf::Vector2f(0.0f, 0.0f));
        framebufferSprite_.setScale(sf::Vector2f(1.0f, 1.0f));
        sf::View view(sf::Vector2f(static_cast<float>(width_.value) * 0.5f, static_cast<float>(height_.value) * 0.5f),
                      sf::Vector2f(static_cast<float>(width_.value), static_cast<float>(height_.value)));
        window_.setView(view);
        return;
    }
    if (const auto* focusLost = event.getIf<sf::Event::FocusLost>()) {
        (void)focusLost;
        camCtrl_.mouseLook = false;
        window_.setMouseCursorGrabbed(false);
        window_.setMouseCursorVisible(true);
        camCtrl_.lastMousePos.reset();
        return;
    }

    if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
        if (key->scancode == sf::Keyboard::Scancode::F1) {
            debugMode_ = DebugMode::None;
            return;
        }
        if (key->scancode == sf::Keyboard::Scancode::F2) {
            debugMode_ = (debugMode_ == DebugMode::DepthBuffer) ? DebugMode::None : DebugMode::DepthBuffer;
            return;
        }
        if (key->scancode == sf::Keyboard::Scancode::F3) {
            debugMode_ = (debugMode_ == DebugMode::Normals) ? DebugMode::None : DebugMode::Normals;
            return;
        }
        
        if (overlayUI_ && overlayUI_->wantsInputCapture()) return;
    }

    bool uiConsumed = false;
    if (overlayUI_ && !camCtrl_.mouseLook) {
        uiConsumed = overlayUI_->handleEvent(event);
    }
    
    if (!uiConsumed) {
        if (const auto* mouseDown = event.getIf<sf::Event::MouseButtonPressed>()) {
            if (mouseDown->button == sf::Mouse::Button::Left) {
                sf::Vector2i mousePos = sf::Mouse::getPosition(window_);
                float mx = static_cast<float>(mousePos.x);
                float my = static_cast<float>(mousePos.y);
                
                const float TOOLBAR_HEIGHT = 50.0f;
                bool clickedInViewport = mx > INSPECTOR_WIDTH && 
                                        mx < (static_cast<float>(width_.value) - HIERARCHY_WIDTH) &&
                                        my < (static_cast<float>(height_.value) - TOOLBAR_HEIGHT);
                
                if (clickedInViewport) {
                bool clickedGizmo = false;
                if (selectedObjectIndex_ >= 0 && selectedObjectIndex_ < static_cast<int>(world_.objects.size())) {
                    const Eigen::Vector3f gizmoPos = world_.objects[selectedObjectIndex_].transform.position;
                    const Eigen::Vector3f rayDir = camera_.rayDirectionFromPixel(mx, my, width_.value, height_.value);
                    const Ray ray = { camera_.position(), rayDir };
                    
                    TransformGizmo::Axis hitAxis = gizmo_.hitTest(ray, gizmoPos, camera_);
                    if (hitAxis != TransformGizmo::Axis::None) {
                        draggedGizmoAxis_ = hitAxis;
                        dragStartPoint_ = gizmo_.computeInitialDragPoint(ray, hitAxis, gizmoPos);
                        dragStartObjectPos_ = gizmoPos;
                        clickedGizmo = true;
                    }
                }
                
                if (!clickedGizmo) {
                    auto pickedObj = pickObjectAtPixel(mx, my);
                    if (pickedObj) {
                        selectedObjectIndex_ = *pickedObj;
                    } else {
                        selectedObjectIndex_ = -1;
                    }
                }
            }
        } else if (mouseDown->button == sf::Mouse::Button::Right) {
            camCtrl_.mouseLook = true;
            Eigen::Vector3f fwd = (camera_.target() - camera_.position()).normalized();
            camCtrl_.pitch = std::asin(std::clamp(fwd.y(), -1.0f, 1.0f));
            camCtrl_.yaw = std::atan2(fwd.z(), fwd.x());
            window_.setMouseCursorGrabbed(true);
            window_.setMouseCursorVisible(false);
            camCtrl_.lastMousePos = sf::Mouse::getPosition(window_);
        }
    }
        if (const auto* text = event.getIf<sf::Event::TextEntered>()) {
            // Text handled by UI overlay
            (void)text;
        }
        if (const auto* mouseUp = event.getIf<sf::Event::MouseButtonReleased>()) {
        if (mouseUp->button == sf::Mouse::Button::Right) {
            camCtrl_.mouseLook = false;
            window_.setMouseCursorGrabbed(false);
            window_.setMouseCursorVisible(true);
            camCtrl_.lastMousePos.reset();
        }
        if (mouseUp->button == sf::Mouse::Button::Left) {
            if (draggedGizmoAxis_ != TransformGizmo::Axis::None) {
                draggedGizmoAxis_ = TransformGizmo::Axis::None;
            }
        }
    }
        if (const auto* moved = event.getIf<sf::Event::MouseMoved>()) {
        if (camCtrl_.mouseLook) {
            sf::Vector2i p = moved->position;
            if (camCtrl_.ignoreNextMouseMove) { camCtrl_.lastMousePos = p; camCtrl_.ignoreNextMouseMove = false; return; }
            if (camCtrl_.lastMousePos) {
                sf::Vector2i d = p - *camCtrl_.lastMousePos;
                if (d.x != 0 || d.y != 0) {
                    const float sensitivity = 0.0025f;
                    camCtrl_.yaw   += static_cast<float>(d.x) * sensitivity;
                    camCtrl_.pitch -= static_cast<float>(d.y) * sensitivity;
                    const float limit = 1.55334303f;  // ~89 градусов
                    if (camCtrl_.pitch > limit) camCtrl_.pitch = limit;
                    if (camCtrl_.pitch < -limit) camCtrl_.pitch = -limit;
                    if (camCtrl_.yaw > static_cast<float>(M_PI))  camCtrl_.yaw -= 2.0f * static_cast<float>(M_PI);
                    if (camCtrl_.yaw < -static_cast<float>(M_PI)) camCtrl_.yaw += 2.0f * static_cast<float>(M_PI);
                }
            }
            camCtrl_.lastMousePos = p;
        } else {
            if (draggedGizmoAxis_ != TransformGizmo::Axis::None && 
                selectedObjectIndex_ >= 0 && 
                selectedObjectIndex_ < static_cast<int>(world_.objects.size())) {
                
                sf::Vector2i mousePos = moved->position;
                float mx = static_cast<float>(mousePos.x);
                float my = static_cast<float>(mousePos.y);
                
                const Eigen::Vector3f rayDir = camera_.rayDirectionFromPixel(mx, my, width_.value, height_.value);
                const Ray ray = { camera_.position(), rayDir };
                
                Eigen::Vector3f delta = gizmo_.computeDragDelta(ray, draggedGizmoAxis_, dragStartObjectPos_, dragStartPoint_);
                world_.objects[selectedObjectIndex_].transform.position = dragStartObjectPos_ + delta;
                world_.objects[selectedObjectIndex_].computeLocalBounds();
            }
            else if (selectedObjectIndex_ >= 0 && selectedObjectIndex_ < static_cast<int>(world_.objects.size())) {
                sf::Vector2i mousePos = moved->position;
                float mx = static_cast<float>(mousePos.x);
                float my = static_cast<float>(mousePos.y);
                
                bool inViewport = mx > HIERARCHY_WIDTH && mx < (static_cast<float>(width_.value) - INSPECTOR_WIDTH);
                if (inViewport) {
                    const Eigen::Vector3f gizmoPos = world_.objects[selectedObjectIndex_].transform.position;
                    const Eigen::Vector3f rayDir = camera_.rayDirectionFromPixel(mx, my, width_.value, height_.value);
                    const Ray ray = { camera_.position(), rayDir };
                    
                    hoveredGizmoAxis_ = gizmo_.hitTest(ray, gizmoPos, camera_);
                } else {
                    hoveredGizmoAxis_ = TransformGizmo::Axis::None;
                }
            }
        }
    }
    } 
}

void Application::renderFrame(float deltaSeconds) {
    if (debugMode_ == DebugMode::None && !wireframe_) {
        screen_.clearColor(world_.ambient.color);
    } else {
        screen_.clearColor(Rgba(0xFF000000u));
    }
    screen_.clearDepth();

    if (debugMode_ == DebugMode::DepthBuffer) {
        screen_ = renderer_.renderWithShader(world_, camera_, std::move(screen_), 
                                            DepthShader(camera_.getNear(), camera_.getFar()), false);
    } else if (debugMode_ == DebugMode::Normals) {
        screen_ = renderer_.renderWithShader(world_, camera_, std::move(screen_), NormalShader(), false);
    } else {
        screen_ = renderer_.render(world_, camera_, std::move(screen_), wireframe_, shadowsEnabled_);
    }
    
    if (selectedObjectIndex_ >= 0 && selectedObjectIndex_ < static_cast<int>(world_.objects.size())) {
        if (!wireframe_) {
            SelectionHighlight::drawOutline(screen_, world_.objects[selectedObjectIndex_], camera_);
        }
        
        const Eigen::Vector3f gizmoPos = world_.objects[selectedObjectIndex_].transform.position;
        gizmo_.render(screen_, camera_, gizmoPos, hoveredGizmoAxis_);
    }

    framebufferTexture_.update(screen_.colorBytes());
    window_.clear(sf::Color(0,0,0,255));
    window_.draw(framebufferSprite_);
    drawUI(deltaSeconds);
    window_.display();
}

void Application::initUI() {
    overlayUI_ = std::make_unique<OverlayUI>(window_, world_, selectedObjectIndex_);
    overlayUI_->init();
}

void Application::drawUI(float deltaSeconds) {
    if (overlayUI_) overlayUI_->draw(deltaSeconds);
}

std::string Application::generateUniqueName(const std::string& base) const {
    std::string candidate = base;
    int idx = 1;
    auto exists = [&](const std::string& n){
        for (const auto& o : world_.objects) if (o.name == n) return true;
        for (const auto& dl : world_.dirLights) if (dl.name == n) return true;
        for (const auto& pl : world_.pointLights) if (pl.name == n) return true;
        return false;
    };
    while (exists(candidate)) candidate = base + " (" + std::to_string(idx++) + ")";
    return candidate;
}

std::optional<int> Application::pickObjectAtPixel(float pixelX, float pixelY) {
    Eigen::Vector3f rayDir = camera_.rayDirectionFromPixel(pixelX, pixelY, 
                                                           static_cast<float>(width_.value), 
                                                           static_cast<float>(height_.value));
    Ray ray(camera_.position(), rayDir);
    
    float closestDist = std::numeric_limits<float>::max();
    int closestObjectIdx = -1;
    
    for (size_t objIdx = 0; objIdx < world_.objects.size(); ++objIdx) {
        const auto& obj = world_.objects[objIdx];
        
        Eigen::Matrix4f modelMatrix = obj.transform.modelMatrix();
        BoundingBox worldBounds = obj.localBounds.transformed(modelMatrix);
        
        auto boxHit = intersectAABB(ray, worldBounds.min(), worldBounds.max());
        if (!boxHit) {
            continue;
        }
        
        const auto& positions = obj.mesh.positions;
        const auto& indices = obj.mesh.indices;
        
        for (size_t i = 0; i + 2 < indices.size(); i += 3) {
            const Eigen::Vector3f& v0 = positions[indices[i]];
            const Eigen::Vector3f& v1 = positions[indices[i + 1]];
            const Eigen::Vector3f& v2 = positions[indices[i + 2]];
            
            Eigen::Vector3f w0 = (modelMatrix * Eigen::Vector4f(v0.x(), v0.y(), v0.z(), 1.0f)).head<3>();
            Eigen::Vector3f w1 = (modelMatrix * Eigen::Vector4f(v1.x(), v1.y(), v1.z(), 1.0f)).head<3>();
            Eigen::Vector3f w2 = (modelMatrix * Eigen::Vector4f(v2.x(), v2.y(), v2.z(), 1.0f)).head<3>();
            
            auto hitDist = intersectTriangle(ray, w0, w1, w2);
            if (hitDist && *hitDist < closestDist) {
                closestDist = *hitDist;
                closestObjectIdx = static_cast<int>(objIdx);
            }
        }
    }
    
    if (closestObjectIdx >= 0) {
        return closestObjectIdx;
    }
    
    return std::nullopt;
}

} 
