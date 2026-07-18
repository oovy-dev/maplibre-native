package com.oovy.maplibre.style.layers;

import androidx.annotation.Keep;

import org.maplibre.android.style.layers.CustomLayer;

/**
 * Georeferenced cube rendered directly in the MapLibre OpenGL scene.
 */
@Keep
public final class OovyCubeLayer extends CustomLayer {

  public OovyCubeLayer(String id) {
    super(id, nativeCreateHost());
  }

  @Keep
  private static native long nativeCreateHost();
}