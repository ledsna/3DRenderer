#pragma once

#include <Eigen/Core>
#include <cstdint>
#include <vector>

namespace rasterizer {

struct Mesh {
    std::vector<Eigen::Vector3f> positions;
    std::vector<Eigen::Vector3f> normals;
    std::vector<Eigen::Vector2f> texcoords;
    std::vector<uint32_t> indices;

    static Mesh makeCube(float half);
    static Mesh makeUVSphere(float radius, int segments, int rings);
};

} 