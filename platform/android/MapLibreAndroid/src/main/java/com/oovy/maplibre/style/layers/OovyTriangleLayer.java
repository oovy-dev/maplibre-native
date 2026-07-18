package com.oovy.maplibre.style.layers;

import androidx.annotation.Keep;

import org.maplibre.android.style.layers.CustomLayer;

/**
 * Temporary native layer used to validate the OOVY rendering pipeline.
 */
@Keep
public final class OovyTriangleLayer extends CustomLayer {

  public OovyTriangleLayer(String id) {
    super(id, nativeCreateHost());
  }

  @Keep
  private static native long nativeCreateHost();
}