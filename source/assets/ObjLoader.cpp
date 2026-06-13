#include "ObjLoader.h"
#include "../utils/Logger.h"

#include <fstream>
#include <sstream>
#include <unordered_map>

namespace rasterizer {

Result<Object> ObjLoader::load(const std::string& path) const {
    try {
        LOG_INFO("Loading OBJ file: " << path);
        
        std::ifstream in(path);
        if (!in) {
            LOG_ERROR("Failed to open OBJ file: " << path);
            return Error("Could not open file: " + path);
        }

    std::vector<Eigen::Vector3f> positions;
    std::vector<Eigen::Vector3f> inNormals;
    std::vector<Eigen::Vector2f> inTexcoords;
    // Final assembled vertex streams and indices
    std::vector<Eigen::Vector3f> outPositions;
    std::vector<Eigen::Vector3f> outNormals;
    std::vector<Eigen::Vector2f> outTexcoords;
    std::vector<uint32_t> indices;
    positions.reserve(1024);
    indices.reserve(2048);

    std::string line;
    // Persistent vertex tuple map across the whole file to weld shared vertices
    struct Triple { uint32_t v, t, n; };
    struct TripleHash { size_t operator()(const Triple& x) const noexcept { return (static_cast<size_t>(x.v) * 73856093u) ^ (static_cast<size_t>(x.t) * 19349663u) ^ (static_cast<size_t>(x.n) * 83492791u); } };
    struct TripleEq { bool operator()(const Triple& a, const Triple& b) const noexcept { return a.v==b.v && a.t==b.t && a.n==b.n; } };
    std::unordered_map<Triple, uint32_t, TripleHash, TripleEq> tupleToIndex;

    auto resolveIndex = [](int idx, size_t count) -> uint32_t {
        // OBJ indices are 1-based; negative indices are relative to end
        if (idx > 0) return static_cast<uint32_t>(idx);
        if (idx < 0) return static_cast<uint32_t>(static_cast<int>(count) + idx + 1);
        return 0; // missing
    };

    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream iss(line);
        std::string tag;
        iss >> tag;
        if (tag == "v") {
            float x,y,z; iss >> x >> y >> z; positions.emplace_back(x,y,z);
        } else if (tag == "vn") {
            float x,y,z; iss >> x >> y >> z; inNormals.emplace_back(x,y,z);
        } else if (tag == "vt") {
            float u,v; iss >> u >> v; inTexcoords.emplace_back(u,v);
        } else if (tag == "f") {
            // Build unique vertex tuples (vi,ti,ni) and remap to unified streams (persistent across faces)
            std::vector<uint32_t> faceRemap;
            std::string vert;
            while (iss >> vert) {
                int vi_i = 0, ti_i = 0, ni_i = 0;
                size_t s1 = vert.find('/');
                if (s1 == std::string::npos) {
                    vi_i = static_cast<int>(std::stol(vert));
                } else {
                    vi_i = static_cast<int>(std::stol(vert.substr(0, s1)));
                    size_t s2 = vert.find('/', s1 + 1);
                    if (s2 == std::string::npos) {
                        ti_i = static_cast<int>(std::stol(vert.substr(s1 + 1)));
                    } else if (s2 == s1 + 1) {
                        ni_i = static_cast<int>(std::stol(vert.substr(s2 + 1)));
                    } else {
                        ti_i = static_cast<int>(std::stol(vert.substr(s1 + 1, s2 - s1 - 1)));
                        ni_i = static_cast<int>(std::stol(vert.substr(s2 + 1)));
                    }
                }
                uint32_t vi = resolveIndex(vi_i, positions.size());
                uint32_t ti = resolveIndex(ti_i, inTexcoords.size());
                uint32_t ni = resolveIndex(ni_i, inNormals.size());
                if (vi == 0) continue; // invalid
                Triple key{vi, ti, ni};
                auto it = tupleToIndex.find(key);
                uint32_t dstIndex;
                if (it == tupleToIndex.end()) {
                    const Eigen::Vector3f pos = positions[vi - 1];
                    Eigen::Vector2f uv(0.0f, 0.0f);
                    Eigen::Vector3f nrm(0.0f, 0.0f, 1.0f);
                    if (ti > 0 && ti - 1 < inTexcoords.size()) uv = inTexcoords[ti - 1];
                    if (ni > 0 && ni - 1 < inNormals.size())   nrm = inNormals[ni - 1];
                    dstIndex = static_cast<uint32_t>(outPositions.size());
                    outPositions.push_back(pos);
                    outNormals.push_back(nrm);
                    outTexcoords.push_back(uv);
                    tupleToIndex.emplace(key, dstIndex);
                } else {
                    dstIndex = it->second;
                }
                faceRemap.push_back(dstIndex);
            }
            if (faceRemap.size() >= 3) {
                for (size_t i = 1; i + 1 < faceRemap.size(); ++i) {
                    indices.push_back(faceRemap[0]);
                    indices.push_back(faceRemap[i]);
                    indices.push_back(faceRemap[i+1]);
                }
            }
        }
    }

    // Fall back to positions-only
    if (outPositions.empty()) {
        if (positions.empty() || indices.empty()) {
            return Error("OBJ file contains no geometry data");
        }
    }
    Object obj;
    if (!outPositions.empty()) {
        obj.mesh.positions = std::move(outPositions);
        obj.mesh.normals = std::move(outNormals);
        obj.mesh.texcoords = std::move(outTexcoords);
        obj.mesh.indices = std::move(indices);
    } else {
        obj.mesh.positions = std::move(positions);
        obj.mesh.indices = std::move(indices);
    }
    // Generate smooth vertex normals only if they are missing
    if (obj.mesh.normals.empty() && !obj.mesh.positions.empty()) {
        obj.mesh.normals.assign(obj.mesh.positions.size(), Eigen::Vector3f(0,0,0));
        for (size_t i = 0; i + 2 < obj.mesh.indices.size(); i += 3) {
            uint32_t i0 = obj.mesh.indices[i];
            uint32_t i1 = obj.mesh.indices[i+1];
            uint32_t i2 = obj.mesh.indices[i+2];
            const Eigen::Vector3f& p0 = obj.mesh.positions[i0];
            const Eigen::Vector3f& p1 = obj.mesh.positions[i1];
            const Eigen::Vector3f& p2 = obj.mesh.positions[i2];
            Eigen::Vector3f e1 = p1 - p0;
            Eigen::Vector3f e2 = p2 - p0;
            Eigen::Vector3f fn = e1.cross(e2);
            obj.mesh.normals[i0] += fn;
            obj.mesh.normals[i1] += fn;
            obj.mesh.normals[i2] += fn;
        }
        for (auto& n : obj.mesh.normals) {
            float len2 = n.squaredNorm();
            if (len2 > 0.0f) n.normalize(); else n = Eigen::Vector3f(0,0,1);
        }
    }
        // Name from path
        auto slash = path.find_last_of("/\\");
        obj.name = (slash == std::string::npos) ? path : path.substr(slash + 1);
        obj.computeLocalBounds();
        
        LOG_INFO("Successfully loaded OBJ: " << obj.name << " (" 
                 << obj.mesh.positions.size() << " vertices, " 
                 << obj.mesh.indices.size() / 3 << " triangles)");
        
        return obj;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Exception while loading OBJ " << path << ": " << e.what());
        return Error(std::string("Failed to parse OBJ file: ") + e.what());
    } catch (...) {
        LOG_ERROR("Unknown exception while loading OBJ " << path);
        return Error("Unknown error while loading OBJ file");
    }
}

} 

