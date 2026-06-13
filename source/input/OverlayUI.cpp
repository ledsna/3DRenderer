#include "OverlayUI.h"
#include "../utils/MathUtils.h"
#include "../graphics/Color.h"
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <SFML/OpenGL.hpp>

namespace rasterizer {

OverlayUI::OverlayUI(sf::RenderWindow& window, World& world, int& selectedIndex)
    : window_(window), world_(world), selectedIndex_(selectedIndex) {}

void OverlayUI::init() {
    if (!font_.openFromFile("/System/Library/Fonts/Helvetica.ttc")) {
        if (!font_.openFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf")) {
        }
    }
    text_ = std::make_unique<sf::Text>(font_);
    text_->setCharacterSize(13);
    text_->setFillColor(Colors::Text);
}

bool OverlayUI::handleEvent(const sf::Event& event) {
    auto pos = getMousePos();
    auto ws = window_.getSize();
    
    const bool overInspector = pos.x <= INSPECTOR_WIDTH;
    const bool overHierarchy = pos.x >= static_cast<float>(ws.x) - HIERARCHY_WIDTH;
    const bool overToolbar = pos.y <= TOOLBAR_HEIGHT && 
                             pos.x > INSPECTOR_WIDTH && 
                             pos.x < static_cast<float>(ws.x) - HIERARCHY_WIDTH;
    const bool overUI = overInspector || overHierarchy || overToolbar;
    
    if (const auto* mouseBtn = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (mouseBtn->button == sf::Mouse::Button::Left) {
            onMouseClick(pos);
            return overUI; // Consume if over UI
        }
    }
    else if (const auto* textEv = event.getIf<sf::Event::TextEntered>()) {
        if (activeField_ != FieldType::None) {
            onTextInput(textEv->unicode);
            return true; // Always consume text input when field is active
        }
    }
    else if (const auto* keyEv = event.getIf<sf::Event::KeyPressed>()) {
        onKeyPress(keyEv->scancode);
        return activeField_ != FieldType::None; // Consume if field is active
    }
    else if (const auto* wheelEv = event.getIf<sf::Event::MouseWheelScrolled>()) {
        if (overHierarchy) {
            hierarchyScroll_ -= wheelEv->delta * 20.0f;
            hierarchyScroll_ = std::max(0.0f, hierarchyScroll_);
            return true;
        }
        else if (overInspector) {
            inspectorScroll_ -= wheelEv->delta * 20.0f;
            inspectorScroll_ = std::max(0.0f, inspectorScroll_);
            return true;
        }
    }
    
    return false;
}

void OverlayUI::draw(float deltaSeconds) {
    float fps = 1.0f / std::max(deltaSeconds, 0.0001f);
    fpsEMA_ = fpsEMA_ == 0.0f ? fps : fpsEMA_ * 0.95f + fps * 0.05f;
    
    drawInspector();
    drawHierarchy();
    drawToolbar();
}

void OverlayUI::drawInspector() {
    auto ws = window_.getSize();
    drawPanel(0, 0, INSPECTOR_WIDTH, static_cast<float>(ws.y), Colors::Panel);
    
    if (selectedIndex_ < 0) {
        drawLabel("No Selection", MARGIN, MARGIN, Colors::TextDim);
        return;
    }
    
    size_t objCount = world_.objects.size();
    size_t dirCount = world_.dirLights.size();
    size_t total = objCount + dirCount + world_.pointLights.size();
    
    if (selectedIndex_ >= static_cast<int>(total)) {
        selectedIndex_ = -1;
        return;
    }
    
    float y = MARGIN - inspectorScroll_;
    text_->setCharacterSize(15);
    text_->setStyle(sf::Text::Bold);
    
    std::string title;
    if (selectedIndex_ < static_cast<int>(objCount)) {
        title = world_.objects[selectedIndex_].name;
    } else if (selectedIndex_ < static_cast<int>(objCount + dirCount)) {
        title = world_.dirLights[selectedIndex_ - objCount].name;
    } else {
        title = world_.pointLights[selectedIndex_ - objCount - dirCount].name;
    }
    
    drawLabel(title, MARGIN, y);
    text_->setStyle(sf::Text::Regular);
    text_->setCharacterSize(13);
    y += ROW_HEIGHT + 5.0f;
    
    drawButton("Delete", MARGIN, y, INSPECTOR_WIDTH - MARGIN * 2, 26.0f, btnDelete_, true);
    y += 26.0f + SECTION_SPACING;
    
    if (selectedIndex_ < static_cast<int>(objCount)) {
        drawTransform(y);
        drawMaterial(y);
    } else {
        drawLight(y);
    }
}

void OverlayUI::drawTransform(float& y) {
    drawSectionHeader("Transform", y);
    
    auto& obj = world_.objects[selectedIndex_];
    auto& t = obj.transform;
    
    drawLabel("Position", MARGIN, y);
    float fx = MARGIN + FIELD_LABEL_WIDTH;
    drawField(t.position.x(), FieldType::PosX, fx, y, FIELD_WIDTH, posRects_[0]);
    fx += FIELD_WIDTH + FIELD_GAP;
    drawField(t.position.y(), FieldType::PosY, fx, y, FIELD_WIDTH, posRects_[1]);
    fx += FIELD_WIDTH + FIELD_GAP;
    drawField(t.position.z(), FieldType::PosZ, fx, y, FIELD_WIDTH, posRects_[2]);
    y += ROW_HEIGHT + 5.0f;
    
    drawLabel("Rotation", MARGIN, y);
    fx = MARGIN + FIELD_LABEL_WIDTH;
    drawField(t.rotationEuler.x(), FieldType::RotX, fx, y, FIELD_WIDTH, rotRects_[0]);
    fx += FIELD_WIDTH + FIELD_GAP;
    drawField(t.rotationEuler.y(), FieldType::RotY, fx, y, FIELD_WIDTH, rotRects_[1]);
    fx += FIELD_WIDTH + FIELD_GAP;
    drawField(t.rotationEuler.z(), FieldType::RotZ, fx, y, FIELD_WIDTH, rotRects_[2]);
    y += ROW_HEIGHT + 5.0f;
    
    drawLabel("Scale", MARGIN, y);
    
    float lockSize = 16.0f;
    scaleLockRect_ = sf::FloatRect({MARGIN + 50.0f, y + 2.0f}, {lockSize, lockSize});
    sf::RectangleShape lockBtn(sf::Vector2f(lockSize, lockSize));
    lockBtn.setPosition(sf::Vector2f(scaleLockRect_.position.x, scaleLockRect_.position.y));
    lockBtn.setFillColor(uniformScaleLock_ ? Colors::Accent : Colors::FieldBg);
    window_.draw(lockBtn);
    
    text_->setString(uniformScaleLock_ ? "L" : "U");
    text_->setPosition(sf::Vector2f(scaleLockRect_.position.x + 4.0f, scaleLockRect_.position.y));
    window_.draw(*text_);
    
    fx = MARGIN + FIELD_LABEL_WIDTH;
    drawField(t.scale.x(), FieldType::ScaleX, fx, y, FIELD_WIDTH, scaleRects_[0]);
    fx += FIELD_WIDTH + FIELD_GAP;
    drawField(t.scale.y(), FieldType::ScaleY, fx, y, FIELD_WIDTH, scaleRects_[1]);
    fx += FIELD_WIDTH + FIELD_GAP;
    drawField(t.scale.z(), FieldType::ScaleZ, fx, y, FIELD_WIDTH, scaleRects_[2]);
    y += ROW_HEIGHT + SECTION_SPACING;
}

void OverlayUI::drawMaterial(float& y) {
    drawSectionHeader("Material", y);
    
    auto& mat = world_.objects[selectedIndex_].material;
    
    drawLabel("Albedo", MARGIN, y);
    float fx = MARGIN + FIELD_LABEL_WIDTH;
    uint8_t r = static_cast<uint8_t>(mat.albedo.value & 0xFF);
    uint8_t g = static_cast<uint8_t>((mat.albedo.value >> 8) & 0xFF);
    uint8_t b = static_cast<uint8_t>((mat.albedo.value >> 16) & 0xFF);
    drawColorField(r, FieldType::AlbedoR, fx, y, FIELD_WIDTH, albedoRects_[0]);
    fx += FIELD_WIDTH + FIELD_GAP;
    drawColorField(g, FieldType::AlbedoG, fx, y, FIELD_WIDTH, albedoRects_[1]);
    fx += FIELD_WIDTH + FIELD_GAP;
    drawColorField(b, FieldType::AlbedoB, fx, y, FIELD_WIDTH, albedoRects_[2]);
    y += ROW_HEIGHT + 5.0f;
    
    drawLabel("Alpha", MARGIN, y);
    drawField(mat.alpha, FieldType::Alpha, MARGIN + FIELD_LABEL_WIDTH, y, FIELD_WIDTH, alphaRect_);
    y += ROW_HEIGHT + 5.0f;
    
    drawLabel("Roughness", MARGIN, y);
    drawField(mat.roughness, FieldType::Roughness, MARGIN + FIELD_LABEL_WIDTH, y, FIELD_WIDTH, roughnessRect_);
    y += ROW_HEIGHT + 5.0f;
    
    drawLabel("Specular", MARGIN, y);
    drawField(mat.specularIntensity, FieldType::SpecularIntensity, MARGIN + FIELD_LABEL_WIDTH, y, FIELD_WIDTH, specularIntRect_);
    y += ROW_HEIGHT + 5.0f;
    
    drawLabel("Power", MARGIN, y);
    drawField(mat.specularPower, FieldType::SpecularPower, MARGIN + FIELD_LABEL_WIDTH, y, FIELD_WIDTH, specularPowRect_);
    y += ROW_HEIGHT + SECTION_SPACING;
}

void OverlayUI::drawLight(float& y) {
    drawSectionHeader("Light", y);
    
    size_t objCount = world_.objects.size();
    size_t dirCount = world_.dirLights.size();
    
    bool isDir = selectedIndex_ < static_cast<int>(objCount + dirCount);
    
    if (isDir) {
        auto& light = world_.dirLights[selectedIndex_ - objCount];
        
        drawLabel("Direction", MARGIN, y);
        float fx = MARGIN + FIELD_LABEL_WIDTH;
        drawField(light.direction[0], FieldType::LightPosX, fx, y, FIELD_WIDTH, posRects_[0]);
        fx += FIELD_WIDTH + FIELD_GAP;
        drawField(light.direction[1], FieldType::LightPosY, fx, y, FIELD_WIDTH, posRects_[1]);
        fx += FIELD_WIDTH + FIELD_GAP;
        drawField(light.direction[2], FieldType::LightPosZ, fx, y, FIELD_WIDTH, posRects_[2]);
        y += ROW_HEIGHT + 5.0f;
        
        drawLabel("Intensity", MARGIN, y);
        drawField(light.intensity, FieldType::LightIntensity, MARGIN + FIELD_LABEL_WIDTH, y, FIELD_WIDTH, intensityRect_);
        y += ROW_HEIGHT + 5.0f;
    } else {
        auto& light = world_.pointLights[selectedIndex_ - objCount - dirCount];
        
        drawLabel("Position", MARGIN, y);
        float fx = MARGIN + FIELD_LABEL_WIDTH;
        drawField(light.position[0], FieldType::LightPosX, fx, y, FIELD_WIDTH, posRects_[0]);
        fx += FIELD_WIDTH + FIELD_GAP;
        drawField(light.position[1], FieldType::LightPosY, fx, y, FIELD_WIDTH, posRects_[1]);
        fx += FIELD_WIDTH + FIELD_GAP;
        drawField(light.position[2], FieldType::LightPosZ, fx, y, FIELD_WIDTH, posRects_[2]);
        y += ROW_HEIGHT + 5.0f;
        
        drawLabel("Intensity", MARGIN, y);
        drawField(light.intensity, FieldType::LightIntensity, MARGIN + FIELD_LABEL_WIDTH, y, FIELD_WIDTH, intensityRect_);
        y += ROW_HEIGHT + 5.0f;
        
        drawLabel("Range", MARGIN, y);
        drawField(light.range, FieldType::LightRange, MARGIN + FIELD_LABEL_WIDTH, y, FIELD_WIDTH, rangeRect_);
        y += ROW_HEIGHT + 5.0f;
    }
    
    drawLabel("Color", MARGIN, y);
    float fx = MARGIN + FIELD_LABEL_WIDTH;
    uint32_t col = isDir ? world_.dirLights[selectedIndex_ - objCount].color.value 
                         : world_.pointLights[selectedIndex_ - objCount - dirCount].color.value;
    uint8_t r = static_cast<uint8_t>(col & 0xFF);
    uint8_t g = static_cast<uint8_t>((col >> 8) & 0xFF);
    uint8_t b = static_cast<uint8_t>((col >> 16) & 0xFF);
    drawColorField(r, FieldType::LightColorR, fx, y, FIELD_WIDTH, lightColorRects_[0]);
    fx += FIELD_WIDTH + FIELD_GAP;
    drawColorField(g, FieldType::LightColorG, fx, y, FIELD_WIDTH, lightColorRects_[1]);
    fx += FIELD_WIDTH + FIELD_GAP;
    drawColorField(b, FieldType::LightColorB, fx, y, FIELD_WIDTH, lightColorRects_[2]);
    y += ROW_HEIGHT + SECTION_SPACING;
}

void OverlayUI::drawHierarchy() {
    auto ws = window_.getSize();
    float x = static_cast<float>(ws.x) - HIERARCHY_WIDTH;
    float panelHeight = static_cast<float>(ws.y);  // Full height, no gap for toolbar
    drawPanel(x, 0, HIERARCHY_WIDTH, panelHeight, Colors::Panel);
    
    float y = MARGIN;
    
    text_->setCharacterSize(15);
    text_->setStyle(sf::Text::Bold);
    drawLabel("Hierarchy", x + MARGIN, y);
    text_->setStyle(sf::Text::Regular);
    text_->setCharacterSize(13);
    y += ROW_HEIGHT + 5.0f;
    
    float btnW = (HIERARCHY_WIDTH - 3.0f * MARGIN) * 0.5f;  // 2 columns
    float btnH = 24.0f;
    float btnGap = MARGIN;
    
    float btnX = x + MARGIN;
    drawButton("Cube", btnX, y, btnW, btnH, btnCube_);
    btnX += btnW + btnGap;
    drawButton("Sphere", btnX, y, btnW, btnH, btnSphere_);
    
    y += btnH + btnGap * 0.5f;
    btnX = x + MARGIN;
    drawButton("Dir Light", btnX, y, btnW, btnH, btnDirLight_);
    btnX += btnW + btnGap;
    drawButton("Point Light", btnX, y, btnW, btnH, btnPointLight_);
    
    y += btnH + btnGap * 0.5f;
    btnX = x + MARGIN;
    drawButton("Import OBJ", btnX, y, HIERARCHY_WIDTH - 2.0f * MARGIN, btnH, btnImport_);
    
    y += btnH + MARGIN;
    
    sf::RectangleShape separator(sf::Vector2f(HIERARCHY_WIDTH - 2.0f * MARGIN, 1.0f));
    separator.setPosition(sf::Vector2f(x + MARGIN, y));
    separator.setFillColor(Colors::Border);
    window_.draw(separator);
    y += MARGIN;
    
    float scrollContentStart = y;
    
    auto ws_full = window_.getSize();
    glEnable(GL_SCISSOR_TEST);

    int scissorX = static_cast<int>(x);
    int scissorY = 0;  // From bottom of screen
    int scissorWidth = static_cast<int>(HIERARCHY_WIDTH);
    int scissorHeight = static_cast<int>(ws_full.y - scrollContentStart);  // Up to scrollContentStart
    glScissor(scissorX, scissorY, scissorWidth, scissorHeight);
    
    y = scrollContentStart - hierarchyScroll_;
    
    size_t idx = 0;
    for (const auto& obj : world_.objects) {
        sf::FloatRect rowRect({x, y}, {HIERARCHY_WIDTH, ROW_HEIGHT});
        
        if (static_cast<int>(idx) == selectedIndex_) {
            sf::RectangleShape sel(sf::Vector2f(HIERARCHY_WIDTH, ROW_HEIGHT));
            sel.setPosition(sf::Vector2f(x, y));
            sel.setFillColor(Colors::Selection);
            window_.draw(sel);
        }
        
        drawLabel(obj.name, x + MARGIN, y);
        y += ROW_HEIGHT + 2.0f;
        ++idx;
    }
    
    if (!world_.dirLights.empty() || !world_.pointLights.empty()) {
        y += 5.0f;
        text_->setStyle(sf::Text::Bold);
        drawLabel("Lights", x + MARGIN, y, Colors::TextDim);
        text_->setStyle(sf::Text::Regular);
        y += ROW_HEIGHT + 5.0f;
    }
    
    for (const auto& light : world_.dirLights) {
        sf::FloatRect rowRect({x, y}, {HIERARCHY_WIDTH, ROW_HEIGHT});
        
        if (static_cast<int>(idx) == selectedIndex_) {
            sf::RectangleShape sel(sf::Vector2f(HIERARCHY_WIDTH, ROW_HEIGHT));
            sel.setPosition(sf::Vector2f(x, y));
            sel.setFillColor(Colors::Selection);
            window_.draw(sel);
        }
        
        drawLabel(light.name, x + MARGIN, y);
        y += ROW_HEIGHT + 2.0f;
        ++idx;
    }
    
    for (const auto& light : world_.pointLights) {
        sf::FloatRect rowRect({x, y}, {HIERARCHY_WIDTH, ROW_HEIGHT});
        
        if (static_cast<int>(idx) == selectedIndex_) {
            sf::RectangleShape sel(sf::Vector2f(HIERARCHY_WIDTH, ROW_HEIGHT));
            sel.setPosition(sf::Vector2f(x, y));
            sel.setFillColor(Colors::Selection);
            window_.draw(sel);
        }
        
        drawLabel(light.name, x + MARGIN, y);
        y += ROW_HEIGHT + 2.0f;
        ++idx;
    }
    
    glDisable(GL_SCISSOR_TEST);
}

void OverlayUI::drawToolbar() {
    auto ws = window_.getSize();
    float y = static_cast<float>(ws.y) - TOOLBAR_HEIGHT;
    
    float toolbarX = INSPECTOR_WIDTH;
    float toolbarWidth = static_cast<float>(ws.x) - INSPECTOR_WIDTH - HIERARCHY_WIDTH;
    
    drawPanel(toolbarX, y, toolbarWidth, TOOLBAR_HEIGHT, Colors::PanelDark);
    
    float btnW = 120.0f;  // Increased to fit "View: Normals"
    float btnH = 30.0f;
    float btnX = toolbarX + MARGIN;
    float btnY = y + (TOOLBAR_HEIGHT - btnH) * 0.5f;
    
    drawButton(renderModeText_, btnX, btnY, btnW, btnH, btnRenderMode_);
    
    btnX += btnW + MARGIN;
    btnW = 130.0f;
    std::string shadowText = shadowsEnabled_ ? "Shadows: ON" : "Shadows: OFF";
    drawButton(shadowText, btnX, btnY, btnW, btnH, btnToggleShadows_);
    
    char fpsBuf[32];
    std::snprintf(fpsBuf, sizeof(fpsBuf), "FPS: %.1f", fpsEMA_);
    text_->setCharacterSize(12);
    drawLabel(fpsBuf, btnX + btnW + MARGIN, btnY + (btnH - 12.0f) * 0.5f + 2.0f, Colors::TextDim);
    text_->setCharacterSize(13);
    
    float toolbarRight = toolbarX + toolbarWidth;
    float ambX = toolbarRight - MARGIN - (FIELD_WIDTH * 3 + FIELD_GAP * 2) - 70.0f;
    drawLabel("Ambient", ambX, y + TOOLBAR_HEIGHT * 0.5f - ROW_HEIGHT * 0.5f);
    
    float fx = ambX + 70.0f;
    uint8_t r = static_cast<uint8_t>(world_.ambient.color.value & 0xFF);
    uint8_t g = static_cast<uint8_t>((world_.ambient.color.value >> 8) & 0xFF);
    uint8_t b = static_cast<uint8_t>((world_.ambient.color.value >> 16) & 0xFF);
    drawColorField(r, FieldType::AmbR, fx, y + TOOLBAR_HEIGHT * 0.5f - ROW_HEIGHT * 0.5f, FIELD_WIDTH, ambientRects_[0]);
    fx += FIELD_WIDTH + FIELD_GAP;
    drawColorField(g, FieldType::AmbG, fx, y + TOOLBAR_HEIGHT * 0.5f - ROW_HEIGHT * 0.5f, FIELD_WIDTH, ambientRects_[1]);
    fx += FIELD_WIDTH + FIELD_GAP;
    drawColorField(b, FieldType::AmbB, fx, y + TOOLBAR_HEIGHT * 0.5f - ROW_HEIGHT * 0.5f, FIELD_WIDTH, ambientRects_[2]);
}

void OverlayUI::drawPanel(float x, float y, float w, float h, sf::Color color) {
    sf::RectangleShape panel(sf::Vector2f(w, h));
    panel.setPosition(sf::Vector2f(x, y));
    panel.setFillColor(color);
    window_.draw(panel);
    
    sf::RectangleShape border(sf::Vector2f(1.0f, h));
    border.setPosition(sf::Vector2f(x + w, y));
    border.setFillColor(Colors::Border);
    window_.draw(border);
}

void OverlayUI::drawLabel(const std::string& str, float x, float y, sf::Color color) {
    text_->setCharacterSize(13);
    text_->setStyle(sf::Text::Regular);
    text_->setString(str);
    text_->setPosition(sf::Vector2f(std::round(x), std::round(y)));
    text_->setFillColor(color);
    window_.draw(*text_);
}

void OverlayUI::drawButton(const std::string& label, float x, float y, float w, float h, sf::FloatRect& outRect, bool enabled) {
    outRect = sf::FloatRect({x, y}, {w, h});
    
    sf::Color bg = Colors::FieldBg;
    if (enabled && isMouseInRect(outRect)) {
        bg = Colors::ButtonHover;
    }
    
    sf::RectangleShape btn(sf::Vector2f(w, h));
    btn.setPosition(sf::Vector2f(x, y));
    btn.setFillColor(bg);
    btn.setOutlineColor(Colors::Border);
    btn.setOutlineThickness(1.0f);
    window_.draw(btn);
    
    text_->setCharacterSize(13);
    text_->setStyle(sf::Text::Regular);
    text_->setString(label);
    auto bounds = text_->getLocalBounds();
    float textX = std::round(x + (w - bounds.size.x) * 0.5f - bounds.position.x);
    float textY = std::round(y + (h - bounds.size.y) * 0.5f - bounds.position.y);
    text_->setPosition(sf::Vector2f(textX, textY));
    text_->setFillColor(enabled ? Colors::Text : Colors::TextDim);
    window_.draw(*text_);
}

void OverlayUI::drawField(float value, FieldType field, float x, float y, float w, sf::FloatRect& outRect) {
    outRect = sf::FloatRect({x, y}, {w, ROW_HEIGHT});
    bool active = activeField_ == field;
    
    sf::RectangleShape box(sf::Vector2f(w, ROW_HEIGHT));
    box.setPosition(sf::Vector2f(x, y));
    box.setFillColor(active ? Colors::FieldActive : Colors::FieldBg);
    box.setOutlineColor(active ? Colors::Accent : Colors::Border);
    box.setOutlineThickness(1.0f);
    window_.draw(box);
    
    text_->setCharacterSize(13);
    text_->setStyle(sf::Text::Regular);
    char buf[32];
    if (active) {
        text_->setString(editBuffer_);
    } else {
        std::snprintf(buf, sizeof(buf), "%.2f", value);
        text_->setString(buf);
    }
    
    text_->setPosition(sf::Vector2f(std::round(x + 4.0f), std::round(y + 2.0f)));
    text_->setFillColor(Colors::Text);
    window_.draw(*text_);
}

void OverlayUI::drawColorField(uint8_t value, FieldType field, float x, float y, float w, sf::FloatRect& outRect) {
    outRect = sf::FloatRect({x, y}, {w, ROW_HEIGHT});
    bool active = activeField_ == field;
    
    sf::RectangleShape box(sf::Vector2f(w, ROW_HEIGHT));
    box.setPosition(sf::Vector2f(x, y));
    box.setFillColor(active ? Colors::FieldActive : Colors::FieldBg);
    box.setOutlineColor(active ? Colors::Accent : Colors::Border);
    box.setOutlineThickness(1.0f);
    window_.draw(box);
    
    text_->setCharacterSize(13);
    text_->setStyle(sf::Text::Regular);
    if (active) {
        text_->setString(editBuffer_);
    } else {
        char buf[8];
        std::snprintf(buf, sizeof(buf), "%d", static_cast<int>(value));
        text_->setString(buf);
    }
    
    text_->setPosition(sf::Vector2f(std::round(x + 4.0f), std::round(y + 2.0f)));
    text_->setFillColor(Colors::Text);
    window_.draw(*text_);
}

void OverlayUI::drawSectionHeader(const std::string& title, float& y) {
    text_->setCharacterSize(14);
    text_->setStyle(sf::Text::Bold);
    drawLabel(title, MARGIN, y, Colors::TextDim);
    text_->setStyle(sf::Text::Regular);
    text_->setCharacterSize(13);
    y += ROW_HEIGHT + 5.0f;
}

void OverlayUI::onMouseClick(const sf::Vector2f& pos) {
    auto ws = window_.getSize();
    
    if (btnCube_.contains(pos)) { spawnCubeReq_ = true; return; }
    if (btnSphere_.contains(pos)) { spawnSphereReq_ = true; return; }
    if (btnDirLight_.contains(pos)) { addDirLightReq_ = true; return; }
    if (btnPointLight_.contains(pos)) { addPointLightReq_ = true; return; }
    if (btnImport_.contains(pos)) { importRequested_ = true; return; }
    
    if (btnRenderMode_.contains(pos)) { cycleRenderModeReq_ = true; return; }
    if (btnToggleShadows_.contains(pos)) { 
        toggleShadowsReq_ = true;
        return; 
    }
    
    float toolbarY = static_cast<float>(ws.y) - TOOLBAR_HEIGHT;
    if (pos.y >= toolbarY) {
        if (ambientRects_[0].contains(pos)) {
            activeField_ = FieldType::AmbR;
            editBuffer_.clear();
            return;
        }
        if (ambientRects_[1].contains(pos)) {
            activeField_ = FieldType::AmbG;
            editBuffer_.clear();
            return;
        }
        if (ambientRects_[2].contains(pos)) {
            activeField_ = FieldType::AmbB;
            editBuffer_.clear();
            return;
        }
    }
    
    float hierarchyX = static_cast<float>(ws.x) - HIERARCHY_WIDTH;
    
    if (pos.x >= hierarchyX) {
        float scrollContentStart = MARGIN + ROW_HEIGHT + 5.0f +   // Header
                                   24.0f + MARGIN * 0.5f +        // Row 1 buttons
                                   24.0f + MARGIN * 0.5f +        // Row 2 buttons
                                   24.0f + MARGIN +               // Row 3 button (Import)
                                   MARGIN;                        // Separator
        
        float y = scrollContentStart - hierarchyScroll_;
        size_t idx = 0;
        
        for (size_t i = 0; i < world_.objects.size(); ++i) {
            sf::FloatRect rowRect({hierarchyX, y}, {HIERARCHY_WIDTH, ROW_HEIGHT});
            if (rowRect.contains(pos)) {
                selectedIndex_ = static_cast<int>(idx);
                clearActiveField();
                return;
            }
            y += ROW_HEIGHT + 2.0f;
            ++idx;
        }
        
        if (!world_.dirLights.empty() || !world_.pointLights.empty()) {
            y += 5.0f + ROW_HEIGHT + 5.0f;  // spacing + header + spacing
        }
        
        for (size_t i = 0; i < world_.dirLights.size(); ++i) {
            sf::FloatRect rowRect({hierarchyX, y}, {HIERARCHY_WIDTH, ROW_HEIGHT});
            if (rowRect.contains(pos)) {
                selectedIndex_ = static_cast<int>(idx);
                clearActiveField();
                return;
            }
            y += ROW_HEIGHT + 2.0f;
            ++idx;
        }
        
        for (size_t i = 0; i < world_.pointLights.size(); ++i) {
            sf::FloatRect rowRect({hierarchyX, y}, {HIERARCHY_WIDTH, ROW_HEIGHT});
            if (rowRect.contains(pos)) {
                selectedIndex_ = static_cast<int>(idx);
                clearActiveField();
                return;
            }
            y += ROW_HEIGHT + 2.0f;
            ++idx;
        }
        return;
    }
    
    sf::Vector2f inspectorPos(pos.x, pos.y + inspectorScroll_);
    
    // Scale lock (uses inspector scroll)
    if (scaleLockRect_.contains(inspectorPos)) {
        uniformScaleLock_ = !uniformScaleLock_;
        return;
    }
    
    // Delete button (uses inspector scroll)
    if (btnDelete_.contains(inspectorPos)) {
        deleteSelectedReq_ = true;
        return;
    }
    
    auto checkField = [&](const sf::FloatRect& rect, FieldType field) {
        if (rect.contains(inspectorPos)) {
            activeField_ = field;
            editBuffer_.clear();
            return true;
        }
        return false;
    };
    
    if (selectedIndex_ < 0) return;
    
    const size_t objCount = world_.objects.size();
    const bool isObject = selectedIndex_ < static_cast<int>(objCount);
    
    if (isObject) {
        // Object fields
        if (checkField(posRects_[0], FieldType::PosX)) return;
        if (checkField(posRects_[1], FieldType::PosY)) return;
        if (checkField(posRects_[2], FieldType::PosZ)) return;
        if (checkField(rotRects_[0], FieldType::RotX)) return;
        if (checkField(rotRects_[1], FieldType::RotY)) return;
        if (checkField(rotRects_[2], FieldType::RotZ)) return;
        if (checkField(scaleRects_[0], FieldType::ScaleX)) return;
        if (checkField(scaleRects_[1], FieldType::ScaleY)) return;
        if (checkField(scaleRects_[2], FieldType::ScaleZ)) return;
        
        // Material fields
        if (checkField(albedoRects_[0], FieldType::AlbedoR)) return;
        if (checkField(albedoRects_[1], FieldType::AlbedoG)) return;
        if (checkField(albedoRects_[2], FieldType::AlbedoB)) return;
        if (checkField(alphaRect_, FieldType::Alpha)) return;
        if (checkField(roughnessRect_, FieldType::Roughness)) return;
        if (checkField(specularIntRect_, FieldType::SpecularIntensity)) return;
        if (checkField(specularPowRect_, FieldType::SpecularPower)) return;
    } else {
        // Light fields
        size_t dirCount = world_.dirLights.size();
        bool isDir = selectedIndex_ < static_cast<int>(objCount + dirCount);
        
        if (checkField(posRects_[0], FieldType::LightPosX)) return;
        if (checkField(posRects_[1], FieldType::LightPosY)) return;
        if (checkField(posRects_[2], FieldType::LightPosZ)) return;
        if (checkField(intensityRect_, FieldType::LightIntensity)) return;
        
        // Range only exists for Point Lights
        if (!isDir && checkField(rangeRect_, FieldType::LightRange)) return;
        
        if (checkField(lightColorRects_[0], FieldType::LightColorR)) return;
        if (checkField(lightColorRects_[1], FieldType::LightColorG)) return;
        if (checkField(lightColorRects_[2], FieldType::LightColorB)) return;
    }
    
    // Click outside - clear active field
    clearActiveField();
}

void OverlayUI::onTextInput(uint32_t unicode) {
    if (unicode == '\r' || unicode == '\n') {
        commitActiveField();
    } else if (unicode == 27) { // ESC
        clearActiveField();
    } else if (unicode == '\b' || unicode == 127) {
        if (!editBuffer_.empty()) {
            editBuffer_.pop_back();
        }
    } else if (unicode >= 32 && unicode < 127) {
        if (editBuffer_.size() < 16) {
            editBuffer_ += static_cast<char>(unicode);
        }
    }
}

void OverlayUI::onKeyPress(sf::Keyboard::Scancode key) {
    if (key == sf::Keyboard::Scancode::Enter && activeField_ != FieldType::None) {
        commitActiveField();
    } else if (key == sf::Keyboard::Scancode::Escape) {
        clearActiveField();
    }
}

void OverlayUI::commitActiveField() {
    if (activeField_ == FieldType::None || editBuffer_.empty()) {
        clearActiveField();
        return;
    }
    
    float val = 0.0f;
    try {
        val = std::stof(editBuffer_);
    } catch (...) {
        clearActiveField();
        return;
    }
    
    size_t objCount = world_.objects.size();
    size_t dirCount = world_.dirLights.size();
    
    bool isObj = selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(objCount);
    bool isDir = selectedIndex_ >= static_cast<int>(objCount) && selectedIndex_ < static_cast<int>(objCount + dirCount);
    bool isPoint = selectedIndex_ >= static_cast<int>(objCount + dirCount);
    
    // Transform
    if (isObj) {
        auto& t = world_.objects[selectedIndex_].transform;
        
        switch (activeField_) {
            case FieldType::PosX: t.position.x() = val; break;
            case FieldType::PosY: t.position.y() = val; break;
            case FieldType::PosZ: t.position.z() = val; break;
            case FieldType::RotX: 
                t.rotationEuler.x() = std::fmod(val, 360.0f);
                if (t.rotationEuler.x() < 0.0f) t.rotationEuler.x() += 360.0f;
                break;
            case FieldType::RotY: 
                t.rotationEuler.y() = std::fmod(val, 360.0f);
                if (t.rotationEuler.y() < 0.0f) t.rotationEuler.y() += 360.0f;
                break;
            case FieldType::RotZ: 
                t.rotationEuler.z() = std::fmod(val, 360.0f);
                if (t.rotationEuler.z() < 0.0f) t.rotationEuler.z() += 360.0f;
                break;
            case FieldType::ScaleX:
                if (uniformScaleLock_) {
                    float ratio = val / t.scale.x();
                    t.scale = t.scale * ratio;
                } else {
                    t.scale.x() = val;
                }
                break;
            case FieldType::ScaleY:
                if (uniformScaleLock_) {
                    float ratio = val / t.scale.y();
                    t.scale = t.scale * ratio;
                } else {
                    t.scale.y() = val;
                }
                break;
            case FieldType::ScaleZ:
                if (uniformScaleLock_) {
                    float ratio = val / t.scale.z();
                    t.scale = t.scale * ratio;
                } else {
                    t.scale.z() = val;
                }
                break;
            default: break;
        }
        
        // Material
        auto& mat = world_.objects[selectedIndex_].material;
        // Extract current values (0xAABBGGRR format)
        uint8_t r = static_cast<uint8_t>(mat.albedo.value & 0xFF);
        uint8_t g = static_cast<uint8_t>((mat.albedo.value >> 8) & 0xFF);
        uint8_t b = static_cast<uint8_t>((mat.albedo.value >> 16) & 0xFF);
        
        switch (activeField_) {
            case FieldType::AlbedoR: 
                r = static_cast<uint8_t>(std::clamp(val, 0.0f, 255.0f));
                mat.albedo.value = (0xFFu << 24) | (b << 16) | (g << 8) | r;
                break;
            case FieldType::AlbedoG: 
                g = static_cast<uint8_t>(std::clamp(val, 0.0f, 255.0f));
                mat.albedo.value = (0xFFu << 24) | (b << 16) | (g << 8) | r;
                break;
            case FieldType::AlbedoB: 
                b = static_cast<uint8_t>(std::clamp(val, 0.0f, 255.0f));
                mat.albedo.value = (0xFFu << 24) | (b << 16) | (g << 8) | r;
                break;
            case FieldType::Alpha: mat.alpha = std::clamp(val, 0.0f, 1.0f); break;
            case FieldType::Roughness: mat.roughness = std::clamp(val, 0.0f, 1.0f); break;
            case FieldType::SpecularIntensity: mat.specularIntensity = std::clamp(val, 0.0f, 10.0f); break;
            case FieldType::SpecularPower: mat.specularPower = std::clamp(val, 1.0f, 512.0f); break;
            default: break;
        }
    }
    
    // Lights
    if (isDir) {
        auto& light = world_.dirLights[selectedIndex_ - objCount];
        // Extract current values (0xAABBGGRR format)
        uint8_t r = static_cast<uint8_t>(light.color.value & 0xFF);
        uint8_t g = static_cast<uint8_t>((light.color.value >> 8) & 0xFF);
        uint8_t b = static_cast<uint8_t>((light.color.value >> 16) & 0xFF);
        
        switch (activeField_) {
            case FieldType::LightPosX: light.direction[0] = val; break;
            case FieldType::LightPosY: light.direction[1] = val; break;
            case FieldType::LightPosZ: light.direction[2] = val; break;
            case FieldType::LightIntensity: light.intensity = std::clamp(val, 0.0f, 10.0f); break;
            case FieldType::LightColorR: 
                r = static_cast<uint8_t>(std::clamp(val, 0.0f, 255.0f));
                light.color.value = (0xFFu << 24) | (b << 16) | (g << 8) | r;
                break;
            case FieldType::LightColorG: 
                g = static_cast<uint8_t>(std::clamp(val, 0.0f, 255.0f));
                light.color.value = (0xFFu << 24) | (b << 16) | (g << 8) | r;
                break;
            case FieldType::LightColorB: 
                b = static_cast<uint8_t>(std::clamp(val, 0.0f, 255.0f));
                light.color.value = (0xFFu << 24) | (b << 16) | (g << 8) | r;
                break;
            default: break;
        }
    }
    
    if (isPoint) {
        auto& light = world_.pointLights[selectedIndex_ - objCount - dirCount];
        // Extract current values (0xAABBGGRR format)
        uint8_t r = static_cast<uint8_t>(light.color.value & 0xFF);
        uint8_t g = static_cast<uint8_t>((light.color.value >> 8) & 0xFF);
        uint8_t b = static_cast<uint8_t>((light.color.value >> 16) & 0xFF);
        
        switch (activeField_) {
            case FieldType::LightPosX: light.position[0] = val; break;
            case FieldType::LightPosY: light.position[1] = val; break;
            case FieldType::LightPosZ: light.position[2] = val; break;
            case FieldType::LightIntensity: light.intensity = std::clamp(val, 0.0f, 10.0f); break;
            case FieldType::LightRange: light.range = std::clamp(val, 0.1f, 100.0f); break;
            case FieldType::LightColorR: 
                r = static_cast<uint8_t>(std::clamp(val, 0.0f, 255.0f));
                light.color.value = (0xFFu << 24) | (b << 16) | (g << 8) | r;
                break;
            case FieldType::LightColorG: 
                g = static_cast<uint8_t>(std::clamp(val, 0.0f, 255.0f));
                light.color.value = (0xFFu << 24) | (b << 16) | (g << 8) | r;
                break;
            case FieldType::LightColorB: 
                b = static_cast<uint8_t>(std::clamp(val, 0.0f, 255.0f));
                light.color.value = (0xFFu << 24) | (b << 16) | (g << 8) | r;
                break;
            default: break;
        }
    }
    
    uint8_t r = static_cast<uint8_t>(world_.ambient.color.value & 0xFF);
    uint8_t g = static_cast<uint8_t>((world_.ambient.color.value >> 8) & 0xFF);
    uint8_t b = static_cast<uint8_t>((world_.ambient.color.value >> 16) & 0xFF);
    switch (activeField_) {
        case FieldType::AmbR: 
            r = static_cast<uint8_t>(std::clamp(val, 0.0f, 255.0f));
            world_.ambient.color.value = (0xFFu << 24) | (b << 16) | (g << 8) | r;
            break;
        case FieldType::AmbG: 
            g = static_cast<uint8_t>(std::clamp(val, 0.0f, 255.0f));
            world_.ambient.color.value = (0xFFu << 24) | (b << 16) | (g << 8) | r;
            break;
        case FieldType::AmbB: 
            b = static_cast<uint8_t>(std::clamp(val, 0.0f, 255.0f));
            world_.ambient.color.value = (0xFFu << 24) | (b << 16) | (g << 8) | r;
            break;
        default: break;
    }
    
    clearActiveField();
}

void OverlayUI::clearActiveField() {
    activeField_ = FieldType::None;
    editBuffer_.clear();
}

sf::Vector2f OverlayUI::getMousePos() const {
    auto pixelPos = sf::Mouse::getPosition(window_);
    return window_.mapPixelToCoords(pixelPos);
}

bool OverlayUI::isMouseInRect(const sf::FloatRect& rect) const {
    return rect.contains(getMousePos());
}

bool OverlayUI::consumeSpawnCube() { bool r = spawnCubeReq_; spawnCubeReq_ = false; return r; }
bool OverlayUI::consumeSpawnSphere() { bool r = spawnSphereReq_; spawnSphereReq_ = false; return r; }
bool OverlayUI::consumeAddDirLight() { bool r = addDirLightReq_; addDirLightReq_ = false; return r; }
bool OverlayUI::consumeAddPointLight() { bool r = addPointLightReq_; addPointLightReq_ = false; return r; }
bool OverlayUI::consumeCycleRenderMode() { bool r = cycleRenderModeReq_; cycleRenderModeReq_ = false; return r; }
bool OverlayUI::consumeToggleShadows() { bool r = toggleShadowsReq_; toggleShadowsReq_ = false; return r; }
bool OverlayUI::consumeDeleteSelected() { bool r = deleteSelectedReq_; deleteSelectedReq_ = false; return r; }

void OverlayUI::setRenderMode(bool wireframe, bool depthView, bool normalsView) {
    if (normalsView) {
        renderModeText_ = "View: Normals";
    } else if (depthView) {
        renderModeText_ = "View: Depth";
    } else if (wireframe) {
        renderModeText_ = "View: Wireframe";
    } else {
        renderModeText_ = "View: Shaded";
    }
}

} 

