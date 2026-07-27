#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace oovy {

struct GlbSummary {
    std::size_t sceneCount = 0;
    std::size_t nodeCount = 0;
    std::size_t meshCount = 0;
    std::size_t primitiveCount = 0;
    std::size_t vertexCount = 0;
    std::size_t indexCount = 0;
    std::size_t materialCount = 0;
};

struct GlbVertex {
    float xMeters = 0.0f;
    float yMeters = 0.0f;
    float zMeters = 0.0f;
};

struct GlbMeshData {
    std::vector<GlbVertex> vertices;
    std::vector<std::uint32_t> indices;

    float widthMeters = 0.0f;
    float depthMeters = 0.0f;
    float heightMeters = 0.0f;
};

bool loadGlbMesh(
    const std::uint8_t* bytes,
    std::size_t byteCount,
    GlbMeshData& mesh,
    GlbSummary& summary,
    std::string& error
);

} // namespace oovy