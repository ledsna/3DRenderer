#include "assets/Mesh.h"
#include <algorithm>
#include <cmath>

namespace rasterizer {

Mesh Mesh::makeCube(float half) {
    Mesh m;
    const float h = half;
    // CCW
    // -Z
    m.positions.insert(m.positions.end(), {{-h,-h,-h},{ h,-h,-h},{ h, h,-h},{-h, h,-h}});
    m.normals.insert(m.normals.end(), 4, Eigen::Vector3f(0, 0, -1));
    m.indices.insert(m.indices.end(), { 0,3,2, 0,2,1 });  // CCW from -Z
    // +Z
    m.positions.insert(m.positions.end(), {{-h,-h, h},{ h,-h, h},{ h, h, h},{-h, h, h}});
    m.normals.insert(m.normals.end(), 4, Eigen::Vector3f(0, 0, 1));
    m.indices.insert(m.indices.end(), { 4,5,6, 4,6,7 });  // CCW from +Z
    // -X
    m.positions.insert(m.positions.end(), {{-h,-h,-h},{-h,-h, h},{-h, h, h},{-h, h,-h}});
    m.normals.insert(m.normals.end(), 4, Eigen::Vector3f(-1, 0, 0));
    m.indices.insert(m.indices.end(), { 8,9,10, 8,10,11 });  // CCW from -X
    // +X
    m.positions.insert(m.positions.end(), {{ h,-h,-h},{ h,-h, h},{ h, h, h},{ h, h,-h}});
    m.normals.insert(m.normals.end(), 4, Eigen::Vector3f(1, 0, 0));
    m.indices.insert(m.indices.end(), { 12,15,14, 12,14,13 });  // CCW from +X
    // -Y
    m.positions.insert(m.positions.end(), {{-h,-h,-h},{ h,-h,-h},{ h,-h, h},{-h,-h, h}});
    m.normals.insert(m.normals.end(), 4, Eigen::Vector3f(0, -1, 0));
    m.indices.insert(m.indices.end(), { 16,17,18, 16,18,19 });  // CCW from -Y
    // +Y
    m.positions.insert(m.positions.end(), {{-h, h,-h},{ h, h,-h},{ h, h, h},{-h, h, h}});
    m.normals.insert(m.normals.end(), 4, Eigen::Vector3f(0, 1, 0));
    m.indices.insert(m.indices.end(), { 20,23,22, 20,22,21 });  // CCW from +Y
    
    return m;
}

Mesh Mesh::makeUVSphere(float radius, int segments, int rings) {
    Mesh m;
    segments = std::max(3, segments);
    rings = std::max(2, rings);
    for (int r = 0; r <= rings; ++r) {
        float v = static_cast<float>(r) / static_cast<float>(rings);
        float phi = v * static_cast<float>(M_PI);
        for (int s = 0; s <= segments; ++s) {
            float u = static_cast<float>(s) / static_cast<float>(segments);
            float theta = u * 2.0f * static_cast<float>(M_PI);
            float x = radius * std::sin(phi) * std::cos(theta);
            float y = radius * std::cos(phi);
            float z = radius * std::sin(phi) * std::sin(theta);
            m.positions.emplace_back(x,y,z);
            // Smooth normal points outwards
            Eigen::Vector3f n = Eigen::Vector3f(std::sin(phi) * std::cos(theta), std::cos(phi), std::sin(phi) * std::sin(theta)).normalized();
            m.normals.emplace_back(n);
        }
    }
    auto idx = [&](int s, int r){ return r * (segments + 1) + s; };
    for (int r = 0; r < rings; ++r) {
        for (int s = 0; s < segments; ++s) {
            uint32_t i0 = idx(s, r);
            uint32_t i1 = idx(s + 1, r);
            uint32_t i2 = idx(s + 1, r + 1);
            uint32_t i3 = idx(s, r + 1);
            m.indices.insert(m.indices.end(), { i0, i1, i2, i0, i2, i3 });
        }
    }
    return m;
}

} 

