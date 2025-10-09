#pragma once

#include <string>
#include "../scene/Object.h"
#include "../utils/Result.h"

namespace rasterizer {

class ObjLoader {
public:
    Result<Object> load(const std::string& path) const;
};

} 

