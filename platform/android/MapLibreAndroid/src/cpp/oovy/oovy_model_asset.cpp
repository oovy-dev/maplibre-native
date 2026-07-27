#include "oovy_model_asset.hpp"

#include <cstdint>
#include <fstream>
#include <limits>
#include <memory>
#include <string>
#include <vector>

namespace {

bool readBinaryFile(
    const std::string& path,
    std::vector<std::uint8_t>& bytes,
    std::string& error
) {
    bytes.clear();
    error.clear();

    std::ifstream input(
        path,
        std::ios::binary |
            std::ios::ate
    );

    if (!input.is_open()) {
        error =
            "Unable to open local GLB file: " +
            path;

        return false;
    }

    const std::streamoff byteCount =
        input.tellg();

    if (byteCount <= 0) {
        error =
            "Local GLB file is empty or unreadable: " +
            path;

        return false;
    }

    const auto unsignedByteCount =
        static_cast<std::uintmax_t>(
            byteCount
        );

    if (
        unsignedByteCount >
        static_cast<std::uintmax_t>(
            std::numeric_limits<std::size_t>::max()
        )
    ) {
        error =
            "Local GLB file exceeds native memory range: " +
            path;

        return false;
    }

    if (
        byteCount >
        static_cast<std::streamoff>(
            std::numeric_limits<std::streamsize>::max()
        )
    ) {
        error =
            "Local GLB file exceeds stream size range: " +
            path;

        return false;
    }

    bytes.resize(
        static_cast<std::size_t>(
            byteCount
        )
    );

    input.seekg(
        0,
        std::ios::beg
    );

    input.read(
        reinterpret_cast<char*>(
            bytes.data()
        ),
        static_cast<std::streamsize>(
            byteCount
        )
    );

    if (!input) {
        bytes.clear();

        error =
            "Unable to read complete local GLB file: " +
            path;

        return false;
    }

    return true;
}

} // namespace

namespace oovy {

OovyModelAssetPtr loadModelAssetSync(
    const std::string& assetId,
    const std::string& localPath,
    std::string& error
) {
    error.clear();

    if (assetId.empty()) {
        error = "Model assetId must not be empty";
        return {};
    }

    if (localPath.empty()) {
        error = "Model localPath must not be empty";
        return {};
    }

    std::vector<std::uint8_t> bytes;

    if (
        !readBinaryFile(
            localPath,
            bytes,
            error
        )
    ) {
        return {};
    }

    auto asset =
        std::make_shared<OovyModelAsset>();

    asset->assetId = assetId;
    asset->sourcePath = localPath;
    asset->sourceByteCount = bytes.size();

    if (
        !loadGlbMesh(
            bytes.data(),
            bytes.size(),
            asset->mesh,
            asset->summary,
            error
        )
    ) {
        return {};
    }

    return asset;
}

} // namespace oovy