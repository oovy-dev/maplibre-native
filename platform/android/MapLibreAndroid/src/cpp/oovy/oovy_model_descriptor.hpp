#pragma once

#include <cmath>
#include <string>

namespace oovy {

struct OovyModelDescriptor final {
    std::string assetId;
    std::string localPath;

    double latitude = 0.0;
    double longitude = 0.0;
    double altitudeMeters = 0.0;
    double yawDegrees = 0.0;
    double uniformScale = 1.0;
    double minZoom = 0.0;
    double maxZoom = 24.0;

    // Temporary physical calibration required by the current GLB loader.
    double targetHeightMeters = 1.0;
    double targetFootprintWidthMeters = 1.0;
    double targetFootprintDepthMeters = 1.0;

    bool validate(std::string* error = nullptr) const {
        if (error != nullptr) {
            error->clear();
        }

        const auto fail = [error](const char* message) {
            if (error != nullptr) {
                *error = message;
            }

            return false;
        };

        if (assetId.empty()) {
            return fail("assetId must not be empty");
        }

        if (localPath.empty()) {
            return fail("localPath must not be empty");
        }

        if (!std::isfinite(latitude) ||
            latitude < -90.0 ||
            latitude > 90.0) {
            return fail("latitude must be finite and between -90 and 90");
        }

        if (!std::isfinite(longitude) ||
            longitude < -180.0 ||
            longitude > 180.0) {
            return fail(
                "longitude must be finite and between -180 and 180"
            );
        }

        if (!std::isfinite(altitudeMeters)) {
            return fail("altitudeMeters must be finite");
        }

        if (!std::isfinite(yawDegrees)) {
            return fail("yawDegrees must be finite");
        }

        if (!std::isfinite(uniformScale) ||
            uniformScale <= 0.0) {
            return fail(
                "uniformScale must be finite and greater than zero"
            );
        }

        if (!std::isfinite(minZoom) || minZoom < 0.0) {
            return fail(
                "minZoom must be finite and greater than or equal to zero"
            );
        }

        if (!std::isfinite(maxZoom) || maxZoom < minZoom) {
            return fail(
                "maxZoom must be finite and greater than or equal to minZoom"
            );
        }

        if (!std::isfinite(targetHeightMeters) ||
            targetHeightMeters <= 0.0) {
            return fail(
                "targetHeightMeters must be finite and greater than zero"
            );
        }

        if (!std::isfinite(targetFootprintWidthMeters) ||
            targetFootprintWidthMeters <= 0.0) {
            return fail(
                "targetFootprintWidthMeters must be finite and greater "
                "than zero"
            );
        }

        if (!std::isfinite(targetFootprintDepthMeters) ||
            targetFootprintDepthMeters <= 0.0) {
            return fail(
                "targetFootprintDepthMeters must be finite and greater "
                "than zero"
            );
        }

        return true;
    }
};

} // namespace oovy