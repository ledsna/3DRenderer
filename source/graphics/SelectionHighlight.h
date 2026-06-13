#pragma once

#include "Screen.h"
#include "../scene/Object.h"
#include "../engine/Camera.h"
#include "../graphics/Color.h"
#include <Eigen/Dense>
#include <vector>

namespace rasterizer {

class SelectionHighlight {
public:
    SelectionHighlight() = default;
    

    static void drawOutline(Screen& screen, 
                           const Object& object, 
                           const Camera& camera,
                           const Rgba& outlineColor = Rgba::fromRGB(255, 165, 0),
                           int thickness = 2);

private:
    static std::vector<bool> createObjectMask(const Object& object,
                                               const Camera& camera,
                                               size_t width,
                                               size_t height);
    
    static bool pointInTriangle(int px, int py, 
                               int ax, int ay, 
                               int bx, int by, 
                               int cx, int cy);
};

} 

