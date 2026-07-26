package com.oovy.maplibre.style.layers;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;

/**
 * Immutable description of a georeferenced OOVY 3D model.
 *
 * localPath must reference a real filesystem file. Android asset paths and
 * HTTP URLs are intentionally unsupported at this layer.
 */
@Keep
public final class OovyModelDescriptor {

  @NonNull
  private final String assetId;

  @NonNull
  private final String localPath;

  private final double latitude;
  private final double longitude;
  private final double altitudeMeters;
  private final double yawDegrees;
  private final double uniformScale;
  private final double minZoom;
  private final double maxZoom;

  // Temporary physical calibration required by the current GLB loader.
  private final double targetHeightMeters;
  private final double targetFootprintWidthMeters;
  private final double targetFootprintDepthMeters;

  public OovyModelDescriptor(
      @NonNull String assetId,
      @NonNull String localPath,
      double latitude,
      double longitude,
      double altitudeMeters,
      double yawDegrees,
      double uniformScale,
      double minZoom,
      double maxZoom,
      double targetHeightMeters,
      double targetFootprintWidthMeters,
      double targetFootprintDepthMeters
  ) {
    this.assetId = requireText(assetId, "assetId");
    this.localPath = requireText(localPath, "localPath");

    requireFinite(latitude, "latitude");
    requireFinite(longitude, "longitude");
    requireFinite(altitudeMeters, "altitudeMeters");
    requireFinite(yawDegrees, "yawDegrees");
    requireFinite(uniformScale, "uniformScale");
    requireFinite(minZoom, "minZoom");
    requireFinite(maxZoom, "maxZoom");
    requireFinite(targetHeightMeters, "targetHeightMeters");
    requireFinite(
        targetFootprintWidthMeters,
        "targetFootprintWidthMeters"
    );
    requireFinite(
        targetFootprintDepthMeters,
        "targetFootprintDepthMeters"
    );

    if (latitude < -90.0 || latitude > 90.0) {
      throw new IllegalArgumentException(
          "latitude must be between -90 and 90"
      );
    }

    if (longitude < -180.0 || longitude > 180.0) {
      throw new IllegalArgumentException(
          "longitude must be between -180 and 180"
      );
    }

    if (uniformScale <= 0.0) {
      throw new IllegalArgumentException(
          "uniformScale must be greater than zero"
      );
    }

    if (minZoom < 0.0) {
      throw new IllegalArgumentException(
          "minZoom must be greater than or equal to zero"
      );
    }

    if (maxZoom < minZoom) {
      throw new IllegalArgumentException(
          "maxZoom must be greater than or equal to minZoom"
      );
    }

    if (targetHeightMeters <= 0.0) {
      throw new IllegalArgumentException(
          "targetHeightMeters must be greater than zero"
      );
    }

    if (targetFootprintWidthMeters <= 0.0) {
      throw new IllegalArgumentException(
          "targetFootprintWidthMeters must be greater than zero"
      );
    }

    if (targetFootprintDepthMeters <= 0.0) {
      throw new IllegalArgumentException(
          "targetFootprintDepthMeters must be greater than zero"
      );
    }

    this.latitude = latitude;
    this.longitude = longitude;
    this.altitudeMeters = altitudeMeters;
    this.yawDegrees = yawDegrees;
    this.uniformScale = uniformScale;
    this.minZoom = minZoom;
    this.maxZoom = maxZoom;
    this.targetHeightMeters = targetHeightMeters;
    this.targetFootprintWidthMeters = targetFootprintWidthMeters;
    this.targetFootprintDepthMeters = targetFootprintDepthMeters;
  }

  @NonNull
  public String getAssetId() {
    return assetId;
  }

  @NonNull
  public String getLocalPath() {
    return localPath;
  }

  public double getLatitude() {
    return latitude;
  }

  public double getLongitude() {
    return longitude;
  }

  public double getAltitudeMeters() {
    return altitudeMeters;
  }

  public double getYawDegrees() {
    return yawDegrees;
  }

  public double getUniformScale() {
    return uniformScale;
  }

  public double getMinZoom() {
    return minZoom;
  }

  public double getMaxZoom() {
    return maxZoom;
  }

  public double getTargetHeightMeters() {
    return targetHeightMeters;
  }

  public double getTargetFootprintWidthMeters() {
    return targetFootprintWidthMeters;
  }

  public double getTargetFootprintDepthMeters() {
    return targetFootprintDepthMeters;
  }

  @NonNull
  private static String requireText(
      @NonNull String value,
      @NonNull String fieldName
  ) {
    final String normalized = value.trim();

    if (normalized.isEmpty()) {
      throw new IllegalArgumentException(
          fieldName + " must not be empty"
      );
    }

    return normalized;
  }

  private static void requireFinite(
      double value,
      @NonNull String fieldName
  ) {
    if (Double.isNaN(value) || Double.isInfinite(value)) {
      throw new IllegalArgumentException(
          fieldName + " must be finite"
      );
    }
  }
}