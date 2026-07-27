#include <android/log.h>
#include <jni.h>

#include <memory>
#include <string>
#include <utility>

#include <mbgl/style/layers/custom_layer.hpp>

#include "oovy_model_asset.hpp"
#include "oovy_model_descriptor.hpp"
#include "oovy_model_descriptor_jni.hpp"
#include "oovy_model_host.hpp"

namespace {

constexpr const char* kLogTag = "OovyModelLayer";

void throwJavaException(
    JNIEnv* env,
    const char* className,
    const std::string& message
) {
    if (
        env == nullptr ||
        env->ExceptionCheck()
    ) {
        return;
    }

    jclass exceptionClass =
        env->FindClass(className);

    if (exceptionClass == nullptr) {
        return;
    }

    env->ThrowNew(
        exceptionClass,
        message.c_str()
    );

    env->DeleteLocalRef(exceptionClass);
}

} // namespace

extern "C"
JNIEXPORT jlong JNICALL
Java_com_oovy_maplibre_style_layers_OovyModelLayer_nativeCreateHost(
    JNIEnv* env,
    jclass,
    jobject javaDescriptor
) {
    oovy::OovyModelDescriptor descriptor;
    std::string error;

    if (
        !oovy::android::readModelDescriptor(
            env,
            javaDescriptor,
            descriptor,
            error
        )
    ) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            kLogTag,
            "Descriptor conversion failed: %s",
            error.c_str()
        );

        throwJavaException(
            env,
            "java/lang/IllegalArgumentException",
            error.empty()
                ? "Invalid OovyModelDescriptor"
                : error
        );

        return 0;
    }

    auto asset =
        oovy::loadModelAssetSync(
            descriptor.assetId,
            descriptor.localPath,
            error
        );

    if (!asset) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            kLogTag,
            "CPU model loading failed: assetId=%s "
            "path=%s error=%s",
            descriptor.assetId.c_str(),
            descriptor.localPath.c_str(),
            error.c_str()
        );

        throwJavaException(
            env,
            "java/lang/IllegalStateException",
            error.empty()
                ? "Unable to load local GLB model"
                : error
        );

        return 0;
    }

    const oovy::GlbSummary& summary =
        asset->summary;

    const oovy::GlbMeshData& mesh =
        asset->mesh;

    __android_log_print(
        ANDROID_LOG_INFO,
        kLogTag,
        "CPU model loaded: assetId=%s path=%s bytes=%zu "
        "scenes=%zu nodes=%zu meshes=%zu primitives=%zu "
        "vertices=%zu indices=%zu materials=%zu "
        "dimensions=(%.2f, %.2f, %.2f)",
        asset->assetId.c_str(),
        asset->sourcePath.c_str(),
        asset->sourceByteCount,
        summary.sceneCount,
        summary.nodeCount,
        summary.meshCount,
        summary.primitiveCount,
        summary.vertexCount,
        summary.indexCount,
        summary.materialCount,
        mesh.widthMeters,
        mesh.depthMeters,
        mesh.heightMeters
    );

    const std::string assetId =
        descriptor.assetId;

    auto host =
        oovy::createModelHost(
            std::move(descriptor),
            std::move(asset)
        );

    if (!host) {
        error =
            "Unable to create GPU model host";

        __android_log_print(
            ANDROID_LOG_ERROR,
            kLogTag,
            "%s: assetId=%s",
            error.c_str(),
            assetId.c_str()
        );

        throwJavaException(
            env,
            "java/lang/IllegalStateException",
            error
        );

        return 0;
    }

    return reinterpret_cast<jlong>(
        host.release()
    );
}