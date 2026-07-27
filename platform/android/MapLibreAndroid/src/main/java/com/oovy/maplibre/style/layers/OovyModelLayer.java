package com.oovy.maplibre.style.layers;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;

import org.maplibre.android.style.layers.CustomLayer;

/**
 * Generic MapLibre custom layer for an OOVY georeferenced 3D model.
 *
 * The native implementation loads and renders the local GLB model described
 * by the supplied immutable descriptor.
 */
@Keep
public final class OovyModelLayer extends CustomLayer {

  public OovyModelLayer(
      @NonNull String id,
      @NonNull OovyModelDescriptor descriptor
  ) {
    super(id, nativeCreateHost(descriptor));
  }

  @Keep
  private static native long nativeCreateHost(
      @NonNull OovyModelDescriptor descriptor
  );
}