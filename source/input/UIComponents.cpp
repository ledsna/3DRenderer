#include "UIComponents.h"
#include <cstdio>
#include <algorithm>
#include <cmath>

namespace rasterizer {

constexpr float ROW_HEIGHT = 20.0f;
constexpr float LABEL_WIDTH = 80.0f;
constexpr float FIELD_WIDTH = 50.0f;
constexpr float FIELD_GAP = 4.0f;

const sf::Color FIELD_BG(60, 60, 65);
const sf::Color FIELD_ACTIVE(80, 80, 255);
const sf::Color BORDER_COLOR(30, 30, 35);
const sf::Color TEXT_COLOR(220, 220, 220);
const sf::Color ACCENT_COLOR(0, 122, 204);

static void drawField(sf::RenderWindow& window, sf::Text& text, const sf::FloatRect& rect, 
                     const std::string& value, bool active) {
    sf::RectangleShape box(sf::Vector2f(rect.size.x, rect.size.y));
    box.setPosition(sf::Vector2f(rect.position.x, rect.position.y));
    box.setFillColor(active ? FIELD_ACTIVE : FIELD_BG);
    box.setOutlineColor(active ? ACCENT_COLOR : BORDER_COLOR);
    box.setOutlineThickness(1.0f);
    window.draw(box);
    
    text.setString(value);
    text.setPosition(sf::Vector2f(rect.position.x + 4.0f, rect.position.y + 2.0f));
    text.setFillColor(TEXT_COLOR);
    window.draw(text);
}

float Vector3Component::draw(sf::RenderWindow& window, sf::Text& text, float x, float y, float) {
    text.setString(label_);
    text.setPosition(sf::Vector2f(x, y + 2.0f));
    text.setFillColor(TEXT_COLOR);
    window.draw(text);
    
    auto values = getter_();
   
    float fieldX = x + LABEL_WIDTH;
    
    for (int i = 0; i < 3; ++i) {
        fieldRects_[i] = sf::FloatRect({fieldX, y}, {FIELD_WIDTH, ROW_HEIGHT});
        
        char buf[32];
        if (activeAxis_ == i) {
            std::snprintf(buf, sizeof(buf), "%.2f", values[i]);
        } else {
            std::snprintf(buf, sizeof(buf), "%.2f", values[i]);
        }
        
        drawField(window, text, fieldRects_[i], buf, activeAxis_ == i);
        fieldX += FIELD_WIDTH + FIELD_GAP;
    }
    
    return ROW_HEIGHT + 4.0f;
}

bool Vector3Component::handleClick(const sf::Vector2f& mousePos, std::string& outEditBuffer) {
    for (int i = 0; i < 3; ++i) {
        if (fieldRects_[i].contains(mousePos)) {
            activeAxis_ = i;
            outEditBuffer.clear();
            return true;
        }
    }
    return false;
}

void Vector3Component::commitEdit(const std::string& buffer) {
    if (activeAxis_ < 0) return;
    
    try {
        float value = std::stof(buffer);
        if (clamp_) value = clamp_(value);
        setter_(activeAxis_, value);
    } catch (...) {
        // ignore invalid input
    }
    
    activeAxis_ = -1;
}

float ColorComponent::draw(sf::RenderWindow& window, sf::Text& text, float x, float y, float) {
    text.setString(label_);
    text.setPosition(sf::Vector2f(x, y + 2.0f));
    text.setFillColor(TEXT_COLOR);
    window.draw(text);
    
    uint32_t color = getter_();
    uint8_t r = static_cast<uint8_t>(color & 0xFF);
    uint8_t g = static_cast<uint8_t>((color >> 8) & 0xFF);
    uint8_t b = static_cast<uint8_t>((color >> 16) & 0xFF);
    
    float fieldX = x + LABEL_WIDTH;
    uint8_t values[] = {r, g, b};
    
    for (int i = 0; i < 3; ++i) {
        fieldRects_[i] = sf::FloatRect({fieldX, y}, {FIELD_WIDTH, ROW_HEIGHT});
        
        char buf[8];
        std::snprintf(buf, sizeof(buf), "%d", static_cast<int>(values[i]));
        
        drawField(window, text, fieldRects_[i], buf, activeChannel_ == i);
        fieldX += FIELD_WIDTH + FIELD_GAP;
    }
    
    return ROW_HEIGHT + 4.0f;
}

bool ColorComponent::handleClick(const sf::Vector2f& mousePos, std::string& outEditBuffer) {
    for (int i = 0; i < 3; ++i) {
        if (fieldRects_[i].contains(mousePos)) {
            activeChannel_ = i;
            outEditBuffer.clear();
            return true;
        }
    }
    return false;
}

void ColorComponent::commitEdit(const std::string& buffer) {
    if (activeChannel_ < 0) return;
    
    try {
        int value = std::stoi(buffer);
        value = std::clamp(value, 0, 255);
        
        uint32_t color = getter_();
        uint8_t r = static_cast<uint8_t>(color & 0xFF);
        uint8_t g = static_cast<uint8_t>((color >> 8) & 0xFF);
        uint8_t b = static_cast<uint8_t>((color >> 16) & 0xFF);
        
        if (activeChannel_ == 0) r = static_cast<uint8_t>(value);
        else if (activeChannel_ == 1) g = static_cast<uint8_t>(value);
        else if (activeChannel_ == 2) b = static_cast<uint8_t>(value);
        
        uint32_t newColor = (0xFFu << 24) | (b << 16) | (g << 8) | r;
        setter_(newColor);
    } catch (...) {
    }
    
    activeChannel_ = -1;
}

float FloatComponent::draw(sf::RenderWindow& window, sf::Text& text, float x, float y, float) {
    text.setString(label_);
    text.setPosition(sf::Vector2f(x, y + 2.0f));
    text.setFillColor(TEXT_COLOR);
    window.draw(text);
    
    float value = getter_();
    
    fieldRect_ = sf::FloatRect({x + LABEL_WIDTH, y}, {FIELD_WIDTH, ROW_HEIGHT});
    
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.2f", value);
    
    drawField(window, text, fieldRect_, buf, editing_);
    
    return ROW_HEIGHT + 4.0f;
}

bool FloatComponent::handleClick(const sf::Vector2f& mousePos, std::string& outEditBuffer) {
    if (fieldRect_.contains(mousePos)) {
        editing_ = true;
        outEditBuffer.clear();
        return true;
    }
    return false;
}

void FloatComponent::commitEdit(const std::string& buffer) {
    if (!editing_) return;
    
    try {
        float value = std::stof(buffer);
        value = std::clamp(value, minVal_, maxVal_);
        setter_(value);
    } catch (...) {
    }
    
    editing_ = false;
}

float ComponentGroup::draw(sf::RenderWindow& window, sf::Text& text, float x, float y, float width) {
    float currentY = y;
    
    text.setString(title_);
    text.setCharacterSize(14);
    text.setStyle(sf::Text::Bold);
    text.setPosition(sf::Vector2f(x, currentY + 2.0f));
    text.setFillColor(TEXT_COLOR);
    window.draw(text);
    
    text.setCharacterSize(13);
    text.setStyle(sf::Text::Regular);
    
    currentY += ROW_HEIGHT + 8.0f;
    
    for (auto& component : components_) {
        float height = component->draw(window, text, x, currentY, width);
        currentY += height;
    }
    
    return currentY - y + 8.0f;
}

bool ComponentGroup::handleClick(const sf::Vector2f& mousePos, std::string& outEditBuffer) {
    for (auto& component : components_) {
        if (component->handleClick(mousePos, outEditBuffer)) {
            return true;
        }
    }
    return false;
}

void ComponentGroup::commitActiveEdit(const std::string& buffer) {
    for (auto& component : components_) {
        if (component->isEditing()) {
            component->commitEdit(buffer);
            return;
        }
    }
}

void ComponentGroup::cancelActiveEdit() {
    for (auto& component : components_) {
        if (component->isEditing()) {
            component->cancelEdit();
            return;
        }
    }
}

bool ComponentGroup::hasActiveEdit() const {
    for (const auto& component : components_) {
        if (component->isEditing()) {
            return true;
        }
    }
    return false;
}

} 

