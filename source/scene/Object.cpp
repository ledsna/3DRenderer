#include "Object.h"

namespace rasterizer {

void Object::computeLocalBounds() {
    if (mesh.positions.empty()) {
        localBounds = BoundingBox();
        return;
    }
    
    localBounds = BoundingBox::fromPoints(mesh.positions);
}

} 

