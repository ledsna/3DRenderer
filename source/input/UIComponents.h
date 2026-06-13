#pragma once

#include <SFML/Graphics.hpp>
#include <functional>
#include <string>
#include <vector>
#include <array>

namespace rasterizer {

struct Vector3Component;
struct ColorComponent;
struct FloatComponent;

class UIComponent {
public:
    virtual ~UIComponent() = default;
    
    virtual float draw(sf::RenderWindow& window, sf::Text& text, float x, float y, float width) = 0;
    
    virtual bool handleClick(const sf::Vector2f& mousePos, std::string& outEditBuffer) = 0;
    
    virtual bool isEditing() const = 0;
    
    virtual void commitEdit(const std::string& buffer) = 0;
    
    virtual void cancelEdit() = 0;
    
    virtual std::string getName() const = 0;
};

// Position, Rotation, Scale, Direction etc.
class Vector3Component : public UIComponent {
public:
    Vector3Component(const std::string& label, std::function<std::array<float, 3>()> getter, 
                     std::function<void(int, float)> setter, 
                     std::function<float(float)> clamp = nullptr)
        : label_(label), getter_(getter), setter_(setter), clamp_(clamp), activeAxis_(-1) {}
    
    float draw(sf::RenderWindow& window, sf::Text& text, float x, float y, float width) override;
    bool handleClick(const sf::Vector2f& mousePos, std::string& outEditBuffer) override;
    bool isEditing() const override { return activeAxis_ >= 0; }
    void commitEdit(const std::string& buffer) override;
    void cancelEdit() override { activeAxis_ = -1; }
    std::string getName() const override { return label_; }
    
private:
    std::string label_;
    std::function<std::array<float, 3>()> getter_;
    std::function<void(int, float)> setter_;  // (axis, value)
    std::function<float(float)> clamp_;
    int activeAxis_;  // -1 = none, 0 = X, 1 = Y, 2 = Z
    std::array<sf::FloatRect, 3> fieldRects_;
};

// RGB Color component
class ColorComponent : public UIComponent {
public:
    ColorComponent(const std::string& label, std::function<uint32_t()> getter, 
                   std::function<void(uint32_t)> setter)
        : label_(label), getter_(getter), setter_(setter), activeChannel_(-1) {}
    
    float draw(sf::RenderWindow& window, sf::Text& text, float x, float y, float width) override;
    bool handleClick(const sf::Vector2f& mousePos, std::string& outEditBuffer) override;
    bool isEditing() const override { return activeChannel_ >= 0; }
    void commitEdit(const std::string& buffer) override;
    void cancelEdit() override { activeChannel_ = -1; }
    std::string getName() const override { return label_; }
    
private:
    std::string label_;
    std::function<uint32_t()> getter_;
    std::function<void(uint32_t)> setter_;
    int activeChannel_;  // -1 = none, 0 = R, 1 = G, 2 = B
    std::array<sf::FloatRect, 3> fieldRects_;
};

// Intensity, Range, Roughness, etc.
class FloatComponent : public UIComponent {
public:
    FloatComponent(const std::string& label, std::function<float()> getter, 
                   std::function<void(float)> setter,
                   float minVal = 0.0f, float maxVal = 1.0f)
        : label_(label), getter_(getter), setter_(setter), 
          minVal_(minVal), maxVal_(maxVal), editing_(false) {}
    
    float draw(sf::RenderWindow& window, sf::Text& text, float x, float y, float width) override;
    bool handleClick(const sf::Vector2f& mousePos, std::string& outEditBuffer) override;
    bool isEditing() const override { return editing_; }
    void commitEdit(const std::string& buffer) override;
    void cancelEdit() override { editing_ = false; }
    std::string getName() const override { return label_; }
    
private:
    std::string label_;
    std::function<float()> getter_;
    std::function<void(float)> setter_;
    float minVal_, maxVal_;
    bool editing_;
    sf::FloatRect fieldRect_;
};

class ComponentGroup {
public:
    ComponentGroup(const std::string& title) : title_(title) {}
    
    void addComponent(std::unique_ptr<UIComponent> component) {
        components_.push_back(std::move(component));
    }
    
    float draw(sf::RenderWindow& window, sf::Text& text, float x, float y, float width);
    bool handleClick(const sf::Vector2f& mousePos, std::string& outEditBuffer);
    void commitActiveEdit(const std::string& buffer);
    void cancelActiveEdit();
    bool hasActiveEdit() const;
    
    const std::string& getTitle() const { return title_; }
    
private:
    std::string title_;
    std::vector<std::unique_ptr<UIComponent>> components_;
};

} 

