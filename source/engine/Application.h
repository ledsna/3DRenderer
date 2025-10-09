#pragma once

#include <SFML/Graphics.hpp>
#include <memory>
#include <string>
#include <vector>
#include <optional>

#include "Renderer.h"
#include "Camera.h"
#include "../assets/Mesh.h"
#include "../graphics/Screen.h"
#include "../scene/World.h"
#include "../input/OverlayUI.h"
#include "../input/TransformGizmo.h"
#include "../utils/Dimensions.h"

namespace rasterizer {

class Application {
public:
    Application(PixelWidth width, PixelHeight height, const std::string& title);
    void run();

private:
    void handleEvent(const sf::Event& event, bool& running);
    void renderFrame(float deltaSeconds);
    void render();
    void initUI();
    void initDemoScene();
    void update(float deltaSeconds);
    void drawUI(float deltaSeconds);
    std::string generateUniqueName(const std::string& base) const;
    static bool isAnyKeyPressed(const std::vector<sf::Keyboard::Scancode>& keys);
    void importObj(const std::string& path);
    std::optional<int> pickObjectAtPixel(float pixelX, float pixelY);

    PixelWidth width_;
    PixelHeight height_;

    sf::RenderWindow window_;
    sf::Texture framebufferTexture_;
    sf::Sprite framebufferSprite_;
    Screen screen_;

    std::unique_ptr<OverlayUI> overlayUI_;

    struct KeyBindings {
        std::vector<sf::Keyboard::Scancode> moveForward { sf::Keyboard::Scancode::W };
        std::vector<sf::Keyboard::Scancode> moveBack    { sf::Keyboard::Scancode::S };
        std::vector<sf::Keyboard::Scancode> moveLeft    { sf::Keyboard::Scancode::A };
        std::vector<sf::Keyboard::Scancode> moveRight   { sf::Keyboard::Scancode::D };
        std::vector<sf::Keyboard::Scancode> moveUp      { sf::Keyboard::Scancode::Space };
        std::vector<sf::Keyboard::Scancode> moveDown    { sf::Keyboard::Scancode::C };
        std::vector<sf::Keyboard::Scancode> sprint      { sf::Keyboard::Scancode::LShift };
    } keyBindings_;

    Renderer renderer_;
    bool wireframe_ = false;
    bool shadowsEnabled_ = true;
    
    enum class DebugMode {
        None,
        DepthBuffer,
        Normals
    };
    DebugMode debugMode_ = DebugMode::None;
    
    Camera camera_;
    World world_;

    int selectedObjectIndex_ = -1;
    
    TransformGizmo gizmo_;
    TransformGizmo::Axis hoveredGizmoAxis_ = TransformGizmo::Axis::None;
    TransformGizmo::Axis draggedGizmoAxis_ = TransformGizmo::Axis::None;
    Eigen::Vector3f dragStartPoint_;
    Eigen::Vector3f dragStartObjectPos_;

    struct CameraControllerState {
        float yaw = 0.0f;
        float pitch = 0.0f;
        float moveSpeed = 3.0f;
        bool mouseLook = false;
        std::optional<sf::Vector2i> lastMousePos;
        bool ignoreNextMouseMove = false;
    } camCtrl_;
};

} 