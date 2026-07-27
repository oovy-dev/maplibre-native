#pragma once

#include <memory>

#include <mbgl/style/layers/custom_layer.hpp>

#include "oovy_model_asset.hpp"
#include "oovy_model_descriptor.hpp"

namespace oovy {

std::unique_ptr<mbgl::style::CustomLayerHost> createModelHost(
    OovyModelDescriptor descriptor,
    OovyModelAssetPtr asset
);

} // namespace oovy