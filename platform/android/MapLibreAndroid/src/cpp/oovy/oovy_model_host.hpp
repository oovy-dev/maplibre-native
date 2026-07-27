#pragma once

#include <memory>

#include <mbgl/style/layers/custom_layer.hpp>

#include "oovy_glb_inspector.hpp"
#include "oovy_model_descriptor.hpp"

namespace oovy {

std::unique_ptr<mbgl::style::CustomLayerHost> createModelHost(
    OovyModelDescriptor descriptor,
    GlbMeshData mesh
);

} // namespace oovy