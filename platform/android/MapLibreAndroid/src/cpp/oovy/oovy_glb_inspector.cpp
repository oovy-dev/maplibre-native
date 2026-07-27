#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#include "oovy_glb_inspector.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <memory>

namespace oovy {
namespace {

struct Bounds {
    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float minZ = std::numeric_limits<float>::max();

    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();
    float maxZ = std::numeric_limits<float>::lowest();

    void include(float x, float y, float z) {
        minX = std::min(minX, x);
        minY = std::min(minY, y);
        minZ = std::min(minZ, z);

        maxX = std::max(maxX, x);
        maxY = std::max(maxY, y);
        maxZ = std::max(maxZ, z);
    }

    bool valid() const {
        return
            minX <= maxX &&
            minY <= maxY &&
            minZ <= maxZ;
    }
};

const char* resultName(cgltf_result result) {
    switch (result) {
        case cgltf_result_success:
            return "success";
        case cgltf_result_data_too_short:
            return "data_too_short";
        case cgltf_result_unknown_format:
            return "unknown_format";
        case cgltf_result_invalid_json:
            return "invalid_json";
        case cgltf_result_invalid_gltf:
            return "invalid_gltf";
        case cgltf_result_invalid_options:
            return "invalid_options";
        case cgltf_result_file_not_found:
            return "file_not_found";
        case cgltf_result_io_error:
            return "io_error";
        case cgltf_result_out_of_memory:
            return "out_of_memory";
        case cgltf_result_legacy_gltf:
            return "legacy_gltf";
        default:
            return "unknown_error";
    }
}

void transformPosition(
    const cgltf_float matrix[16],
    const cgltf_float position[3],
    float transformed[3]
) {
    const float x = position[0];
    const float y = position[1];
    const float z = position[2];

    transformed[0] =
        matrix[0] * x +
        matrix[4] * y +
        matrix[8] * z +
        matrix[12];

    transformed[1] =
        matrix[1] * x +
        matrix[5] * y +
        matrix[9] * z +
        matrix[13];

    transformed[2] =
        matrix[2] * x +
        matrix[6] * y +
        matrix[10] * z +
        matrix[14];
}

bool appendPrimitive(
    const cgltf_node& node,
    const cgltf_primitive& primitive,
    GlbMeshData& mesh,
    Bounds& bounds,
    std::string& error
) {
    if (primitive.type != cgltf_primitive_type_triangles) {
        error = "Only triangle primitives are currently supported";
        return false;
    }

    const cgltf_accessor* positionAccessor =
        cgltf_find_accessor(
            &primitive,
            cgltf_attribute_type_position,
            0);

    if (positionAccessor == nullptr) {
        error = "Primitive has no POSITION accessor";
        return false;
    }

    if (
        positionAccessor->type != cgltf_type_vec3 ||
        positionAccessor->component_type !=
            cgltf_component_type_r_32f
    ) {
        error = "POSITION must be a float VEC3 accessor";
        return false;
    }

    if (
        positionAccessor->count >
        std::numeric_limits<std::uint32_t>::max() -
            mesh.vertices.size()
    ) {
        error = "Combined vertex buffer exceeds uint32 range";
        return false;
    }

    const std::uint32_t baseVertex =
        static_cast<std::uint32_t>(
            mesh.vertices.size());

    mesh.vertices.reserve(
        mesh.vertices.size() +
        positionAccessor->count);

    std::array<cgltf_float, 16> worldMatrix{};
    cgltf_node_transform_world(
        &node,
        worldMatrix.data());

    for (
        cgltf_size vertexIndex = 0;
        vertexIndex < positionAccessor->count;
        ++vertexIndex
    ) {
        cgltf_float localPosition[3]{};

        if (
            !cgltf_accessor_read_float(
                positionAccessor,
                vertexIndex,
                localPosition,
                3)
        ) {
            error = "Unable to read POSITION accessor";
            return false;
        }

        float worldPosition[3]{};

        transformPosition(
            worldMatrix.data(),
            localPosition,
            worldPosition);

        mesh.vertices.push_back(
            GlbVertex{
                worldPosition[0],
                worldPosition[1],
                worldPosition[2],
            });

        bounds.include(
            worldPosition[0],
            worldPosition[1],
            worldPosition[2]);
    }

    if (primitive.indices != nullptr) {
        if (primitive.indices->type != cgltf_type_scalar) {
            error = "Index accessor must be SCALAR";
            return false;
        }

        mesh.indices.reserve(
            mesh.indices.size() +
            primitive.indices->count);

        for (
            cgltf_size index = 0;
            index < primitive.indices->count;
            ++index
        ) {
            const cgltf_size localIndex =
                cgltf_accessor_read_index(
                    primitive.indices,
                    index);

            if (localIndex >= positionAccessor->count) {
                error = "Primitive index exceeds vertex count";
                return false;
            }

            mesh.indices.push_back(
                baseVertex +
                static_cast<std::uint32_t>(
                    localIndex));
        }
    } else {
        if (positionAccessor->count % 3 != 0) {
            error =
                "Non-indexed triangle primitive has invalid vertex count";
            return false;
        }

        mesh.indices.reserve(
            mesh.indices.size() +
            positionAccessor->count);

        for (
            cgltf_size index = 0;
            index < positionAccessor->count;
            ++index
        ) {
            mesh.indices.push_back(
                baseVertex +
                static_cast<std::uint32_t>(index));
        }
    }

    return true;
}

bool appendNode(
    const cgltf_node& node,
    GlbMeshData& mesh,
    Bounds& bounds,
    std::string& error
) {
    if (node.mesh != nullptr) {
        for (
            cgltf_size primitiveIndex = 0;
            primitiveIndex < node.mesh->primitives_count;
            ++primitiveIndex
        ) {
            if (
                !appendPrimitive(
                    node,
                    node.mesh->primitives[primitiveIndex],
                    mesh,
                    bounds,
                    error)
            ) {
                return false;
            }
        }
    }

    for (
        cgltf_size childIndex = 0;
        childIndex < node.children_count;
        ++childIndex
    ) {
        if (
            node.children[childIndex] != nullptr &&
            !appendNode(
                *node.children[childIndex],
                mesh,
                bounds,
                error)
        ) {
            return false;
        }
    }

    return true;
}

void fillSummary(
    const cgltf_data& data,
    GlbSummary& summary
) {
    summary.sceneCount = data.scenes_count;
    summary.nodeCount = data.nodes_count;
    summary.meshCount = data.meshes_count;
    summary.materialCount = data.materials_count;

    for (
        cgltf_size meshIndex = 0;
        meshIndex < data.meshes_count;
        ++meshIndex
    ) {
        const cgltf_mesh& mesh =
            data.meshes[meshIndex];

        summary.primitiveCount +=
            mesh.primitives_count;

        for (
            cgltf_size primitiveIndex = 0;
            primitiveIndex < mesh.primitives_count;
            ++primitiveIndex
        ) {
            const cgltf_primitive& primitive =
                mesh.primitives[primitiveIndex];

            const cgltf_accessor* positions =
                cgltf_find_accessor(
                    &primitive,
                    cgltf_attribute_type_position,
                    0);

            if (positions != nullptr) {
                summary.vertexCount +=
                    positions->count;
            }

            if (primitive.indices != nullptr) {
                summary.indexCount +=
                    primitive.indices->count;
            } else if (positions != nullptr) {
                summary.indexCount +=
                    positions->count;
            }
        }
    }
}

bool canonicalizeModel(
    GlbMeshData& mesh,
    const Bounds& bounds,
    std::string& error
) {
    if (!bounds.valid()) {
        error = "Model bounds are invalid";
        return false;
    }

    const float rawWidth = bounds.maxX - bounds.minX;
    const float rawHeight = bounds.maxY - bounds.minY;
    const float rawDepth = bounds.maxZ - bounds.minZ;

    if (
        rawWidth <= 0.0f ||
        rawHeight <= 0.0f ||
        rawDepth <= 0.0f
    ) {
        error = "Model dimensions must be greater than zero";
        return false;
    }

    const float centerX =
        (bounds.minX + bounds.maxX) * 0.5f;

    const float centerZ =
        (bounds.minZ + bounds.maxZ) * 0.5f;

    for (GlbVertex& vertex : mesh.vertices) {
        const float gltfX = vertex.xMeters;
        const float gltfY = vertex.yMeters;
        const float gltfZ = vertex.zMeters;

        // glTF: X right, Y up, Z forward.
        // Map local space: X east, Y south, Z up.
        vertex.xMeters = gltfX - centerX;
        vertex.yMeters = -(gltfZ - centerZ);
        vertex.zMeters = gltfY - bounds.minY;
    }

    mesh.widthMeters = rawWidth;
    mesh.depthMeters = rawDepth;
    mesh.heightMeters = rawHeight;

    return true;
}

} // namespace

bool loadGlbMesh(
    const std::uint8_t* bytes,
    std::size_t byteCount,
    GlbMeshData& mesh,
    GlbSummary& summary,
    std::string& error
) {
    mesh = {};
    summary = {};
    error.clear();

    if (bytes == nullptr || byteCount == 0) {
        error = "GLB input is empty";
        return false;
    }

    cgltf_options options{};
    cgltf_data* rawData = nullptr;

    const cgltf_result parseResult =
        cgltf_parse(
            &options,
            bytes,
            byteCount,
            &rawData);

    if (parseResult != cgltf_result_success) {
        error =
            std::string("cgltf_parse failed: ") +
            resultName(parseResult);

        return false;
    }

    std::unique_ptr<cgltf_data, decltype(&cgltf_free)> data(
        rawData,
        &cgltf_free);

    if (data->file_type != cgltf_file_type_glb) {
        error = "Input is not a binary GLB file";
        return false;
    }

    const cgltf_result bufferResult =
        cgltf_load_buffers(
            &options,
            data.get(),
            nullptr);

    if (bufferResult != cgltf_result_success) {
        error =
            std::string("cgltf_load_buffers failed: ") +
            resultName(bufferResult);

        return false;
    }

    const cgltf_result validationResult =
        cgltf_validate(data.get());

    if (validationResult != cgltf_result_success) {
        error =
            std::string("cgltf_validate failed: ") +
            resultName(validationResult);

        return false;
    }

    fillSummary(
        *data,
        summary);

    Bounds bounds;

    const cgltf_scene* activeScene =
        data->scene;

    if (
        activeScene == nullptr &&
        data->scenes_count > 0
    ) {
        activeScene =
            &data->scenes[0];
    }

    if (activeScene != nullptr) {
        for (
            cgltf_size rootIndex = 0;
            rootIndex < activeScene->nodes_count;
            ++rootIndex
        ) {
            if (
                activeScene->nodes[rootIndex] != nullptr &&
                !appendNode(
                    *activeScene->nodes[rootIndex],
                    mesh,
                    bounds,
                    error)
            ) {
                mesh = {};
                return false;
            }
        }
    } else {
        for (
            cgltf_size nodeIndex = 0;
            nodeIndex < data->nodes_count;
            ++nodeIndex
        ) {
            const cgltf_node& node =
                data->nodes[nodeIndex];

            if (
                node.parent == nullptr &&
                !appendNode(
                    node,
                    mesh,
                    bounds,
                    error)
            ) {
                mesh = {};
                return false;
            }
        }
    }

    if (
        mesh.vertices.empty() ||
        mesh.indices.empty()
    ) {
        error = "GLB scene contains no renderable triangles";
        mesh = {};
        return false;
    }

    if (
        !canonicalizeModel(
            mesh,
            bounds,
            error
        )
    ) {
        mesh = {};
        return false;
    }

    return true;
}

} // namespace oovy