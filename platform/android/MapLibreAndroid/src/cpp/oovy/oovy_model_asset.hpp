#pragma once

#include <cstddef>
#include <memory>
#include <string>

#include "oovy_glb_inspector.hpp"

namespace oovy {

struct OovyModelAsset final {
    std::string assetId;
    std::string sourcePath;

    GlbMeshData mesh;
    GlbSummary summary;

    std::size_t sourceByteCount = 0;
};

using OovyModelAssetPtr =
    std::shared_ptr<const OovyModelAsset>;

OovyModelAssetPtr loadModelAssetSync(
    const std::string& assetId,
    const std::string& localPath,
    std::string& error
);

} // namespace oovy