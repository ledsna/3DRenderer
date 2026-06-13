#pragma once

#include <SFML/Graphics.hpp>
#include <memory>
#include <string>
#include <vector>
#include <array>

#include "../scene/World.h"

namespace rasterizer {

// Field types for editing
enum class FieldType {
    None,
    // Transform
    PosX, PosY, PosZ,
    RotX, RotY, RotZ,
    ScaleX, ScaleY, ScaleZ,
    // Material
    AlbedoR, AlbedoG, AlbedoB,
    Alpha,
    Roughness, SpecularIntensity, SpecularPower,
    // Light
    LightPosX, LightPosY, LightPosZ,  // For point lights position / directional lights direction
    LightIntensity, LightRange,
    LightColorR, LightColorG, LightColorB,
    // Ambient
    AmbR, AmbG, AmbB
};

// Unity-style cringe UI
class OverlayUI {
public:
    OverlayUI(sf::RenderWindow& window, World& world, int& selectedIndex);
    
    void init();
    bool handleEvent(const sf::Event& event); // Returns true if event was consumed by UI
    void draw(float deltaSeconds);
    
    bool importRequested() const { return importRequested_; }
    void resetImportRequest() { importRequested_ = false; }
    bool consumeSpawnCube();
    bool consumeSpawnSphere();
    bool consumeAddDirLight();
    bool consumeAddPointLight();
    bool consumeCycleRenderMode();
    bool consumeToggleShadows();
    bool consumeDeleteSelected();
    
    void setRenderMode(bool wireframe, bool depthView, bool normalsView);
    void setShadowsEnabled(bool enabled) { shadowsEnabled_ = enabled; }
    
    float panelWidthPx() const { return INSPECTOR_WIDTH; }
    bool wantsInputCapture() const { return activeField_ != FieldType::None; }

private:
    static constexpr float INSPECTOR_WIDTH = 280.0f;
    static constexpr float HIERARCHY_WIDTH = 220.0f;
    static constexpr float TOOLBAR_HEIGHT = 50.0f;
    static constexpr float MARGIN = 10.0f;
    static constexpr float ROW_HEIGHT = 22.0f;
    static constexpr float SECTION_SPACING = 15.0f;
    static constexpr float FIELD_LABEL_WIDTH = 85.0f;
    static constexpr float FIELD_WIDTH = 55.0f;
    static constexpr float FIELD_GAP = 6.0f;
    
    struct Colors {
        static constexpr sf::Color Background{45, 45, 48, 255};
        static constexpr sf::Color Panel{37, 37, 38, 255};
        static constexpr sf::Color PanelDark{30, 30, 30, 255};
        static constexpr sf::Color Border{25, 25, 25, 255};
        static constexpr sf::Color Text{200, 200, 200, 255};
        static constexpr sf::Color TextDim{150, 150, 150, 255};
        static constexpr sf::Color Accent{0, 122, 204, 255};
        static constexpr sf::Color FieldBg{50, 50, 50, 255};
        static constexpr sf::Color FieldActive{60, 60, 60, 255};
        static constexpr sf::Color ButtonHover{70, 70, 70, 255};
        static constexpr sf::Color Selection{65, 95, 125, 255};
    };
    
    void drawInspector();
    void drawHierarchy();
    void drawToolbar();
    
    void drawTransform(float& y);
    void drawMaterial(float& y);
    void drawLight(float& y);
    
    void drawPanel(float x, float y, float w, float h, sf::Color color);
    void drawLabel(const std::string& text, float x, float y, sf::Color color = Colors::Text);
    void drawButton(const std::string& label, float x, float y, float w, float h, sf::FloatRect& outRect, bool enabled = true);
    void drawField(float value, FieldType field, float x, float y, float w, sf::FloatRect& outRect);
    void drawColorField(uint8_t value, FieldType field, float x, float y, float w, sf::FloatRect& outRect);
    void drawSectionHeader(const std::string& title, float& y);
    
    void onMouseClick(const sf::Vector2f& pos);
    void onTextInput(uint32_t unicode);
    void onKeyPress(sf::Keyboard::Scancode key);
    void commitActiveField();
    void clearActiveField();
    
    sf::Vector2f getMousePos() const;
    bool isMouseInRect(const sf::FloatRect& rect) const;
    
private:
    sf::RenderWindow& window_;
    World& world_;
    int& selectedIndex_;
    
    sf::Font font_;
    std::unique_ptr<sf::Text> text_;
    
    FieldType activeField_ = FieldType::None;
    std::string editBuffer_;
    float hierarchyScroll_ = 0.0f;
    float inspectorScroll_ = 0.0f;
    bool uniformScaleLock_ = true;
    float fpsEMA_ = 0.0f;
    
    std::string renderModeText_ = "View: Shaded";
    
    std::array<sf::FloatRect, 3> posRects_;
    std::array<sf::FloatRect, 3> rotRects_;
    std::array<sf::FloatRect, 3> scaleRects_;
    std::array<sf::FloatRect, 3> albedoRects_;
    std::array<sf::FloatRect, 3> lightColorRects_;
    std::array<sf::FloatRect, 3> ambientRects_;
    sf::FloatRect alphaRect_;
    sf::FloatRect roughnessRect_;
    sf::FloatRect specularIntRect_;
    sf::FloatRect specularPowRect_;
    sf::FloatRect intensityRect_;
    sf::FloatRect rangeRect_;
    sf::FloatRect scaleLockRect_;
    
    sf::FloatRect btnCube_, btnSphere_, btnDirLight_, btnPointLight_, btnImport_;
    
    sf::FloatRect btnRenderMode_;
    sf::FloatRect btnToggleShadows_;
    sf::FloatRect btnDelete_;
    
    bool importRequested_ = false;
    bool spawnCubeReq_ = false;
    bool spawnSphereReq_ = false;
    bool addDirLightReq_ = false;
    bool addPointLightReq_ = false;
    bool cycleRenderModeReq_ = false;
    bool toggleShadowsReq_ = false;
    bool deleteSelectedReq_ = false;
    
    bool shadowsEnabled_ = true;
};

} 

