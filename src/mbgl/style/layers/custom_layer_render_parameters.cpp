#include <mbgl/style/layers/custom_layer_render_parameters.hpp>
#include <mbgl/renderer/paint_parameters.hpp>
#include <mbgl/util/mat4.hpp>

namespace mbgl {
namespace style {

CustomLayerRenderParameters::CustomLayerRenderParameters(
    const mbgl::PaintParameters& paintParameters,
    bool use3DProjection) {
    const TransformState& state = paintParameters.state;
    width = state.getSize().width;
    height = state.getSize().height;
    latitude = state.getLatLng().latitude();
    longitude = state.getLatLng().longitude();
    zoom = state.getZoom();
    bearing = util::rad2deg(-state.getBearing());
    pitch = state.getPitch();
    fieldOfView = state.getFieldOfView();
    projectionMatrix = use3DProjection
        ? paintParameters.transformParams.nearClippedProjMatrix
        : paintParameters.transformParams.projMatrix;
}

} // namespace style
} // namespace mbgl
