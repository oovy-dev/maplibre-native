package com.oovy.maplibre.style.layers;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;

import org.maplibre.android.style.layers.CustomLayer;

/**
 * Generic MapLibre custom layer for an OOVY georeferenced 3D model.
 *
 * The initial native implementation only transfers and validates the model
 * descriptor. Rendering will be connected in a later migration step.
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