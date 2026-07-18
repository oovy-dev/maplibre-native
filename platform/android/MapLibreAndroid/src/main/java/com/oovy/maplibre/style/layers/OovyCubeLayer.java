package com.oovy.maplibre.style.layers;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;

import org.maplibre.android.style.layers.CustomLayer;

/**
 * Temporary OOVY custom layer used to validate native GLB loading.
 *
 * The cube remains rendered until the GLB mesh extraction is implemented.
 */
@Keep
public final class OovyCubeLayer extends CustomLayer {

  public OovyCubeLayer(
      @NonNull String id,
      @NonNull byte[] glbData
  ) {
    super(id, nativeCreateHost(glbData));
  }

  @Keep
  private static native long nativeCreateHost(
      @NonNull byte[] glbData
  );
}