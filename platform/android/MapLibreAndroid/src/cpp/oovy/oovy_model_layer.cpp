#include <android/log.h>
#include <jni.h>

#include <cstdint>
#include <fstream>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <mbgl/style/layers/custom_layer.hpp>

#include "oovy_glb_inspector.hpp"
#include "oovy_model_descriptor.hpp"
#include "oovy_model_descriptor_jni.hpp"
#include "oovy_model_host.hpp"

namespace {

constexpr const char* kLogTag = "OovyModelLayer";

bool readBinaryFile(
    const std::string& path,
    std::vector<std::uint8_t>& bytes,
    std::string& error
) {
    bytes.clear();
    error.clear();

    std::ifstream input(
        path,
        std::ios::binary |
            std::ios::ate
    );

    if (!input.is_open()) {
        error =
            "Unable to open local GLB file: " +
            path;

        return false;
    }

    const std::streamoff byteCount =
        static_cast<std::streamoff>(
            input.tellg()
        );

    if (byteCount <= 0) {
        error =
            "Local GLB file is empty or unreadable: " +
            path;

        return false;
    }

    const auto unsignedByteCount =
        static_cast<std::uintmax_t>(
            byteCount
        );

    if (
        unsignedByteCount >
        static_cast<std::uintmax_t>(
            std::numeric_limits<std::size_t>::max()
        )
    ) {
        error =
            "Local GLB file exceeds native memory range: " +
            path;

        return false;
    }

    if (
        byteCount >
        static_cast<std::streamoff>(
            std::numeric_limits<std::streamsize>::max()
        )
    ) {
        error =
            "Local GLB file exceeds stream size range: " +
            path;

        return false;
    }

    bytes.resize(
        static_cast<std::size_t>(
            byteCount
        )
    );

    input.seekg(
        0,
        std::ios::beg
    );

    input.read(
        reinterpret_cast<char*>(
            bytes.data()
        ),
        static_cast<std::streamsize>(
            byteCount
        )
    );

    if (!input) {
        bytes.clear();

        error =
            "Unable to read complete local GLB file: " +
            path;

        return false;
    }

    return true;
}

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

    if (
        descriptor.targetHeightMeters >
        static_cast<double>(
            std::numeric_limits<float>::max()
        )
    ) {
        error =
            "targetHeightMeters exceeds the native float range";

        __android_log_print(
            ANDROID_LOG_ERROR,
            kLogTag,
            "%s: assetId=%s",
            error.c_str(),
            descriptor.assetId.c_str()
        );

        throwJavaException(
            env,
            "java/lang/IllegalArgumentException",
            error
        );

        return 0;
    }

    std::vector<std::uint8_t> bytes;

    if (
        !readBinaryFile(
            descriptor.localPath,
            bytes,
            error
        )
    ) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            kLogTag,
            "Local GLB read failed: assetId=%s error=%s",
            descriptor.assetId.c_str(),
            error.c_str()
        );

        throwJavaException(
            env,
            "java/lang/IllegalStateException",
            error
        );

        return 0;
    }

    __android_log_print(
        ANDROID_LOG_INFO,
        kLogTag,
        "Local GLB read: assetId=%s bytes=%zu path=%s",
        descriptor.assetId.c_str(),
        bytes.size(),
        descriptor.localPath.c_str()
    );

    oovy::GlbMeshData mesh;
    oovy::GlbSummary summary;

    if (
        !oovy::loadGlbMesh(
            bytes.data(),
            bytes.size(),
            static_cast<float>(
                descriptor.targetHeightMeters
            ),
            mesh,
            summary,
            error
        )
    ) {
        __android_log_print(
            ANDROID_LOG_ERROR,
            kLogTag,
            "Local GLB parsing failed: assetId=%s error=%s",
            descriptor.assetId.c_str(),
            error.c_str()
        );

        throwJavaException(
            env,
            "java/lang/IllegalStateException",
            error.empty()
                ? "Unable to parse local GLB model"
                : error
        );

        return 0;
    }

    __android_log_print(
        ANDROID_LOG_INFO,
        kLogTag,
        "Local GLB parsed: assetId=%s "
        "scenes=%zu nodes=%zu meshes=%zu primitives=%zu "
        "vertices=%zu indices=%zu materials=%zu "
        "dimensions=(%.2f, %.2f, %.2f)m",
        descriptor.assetId.c_str(),
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

    auto host =
        oovy::createModelHost(
            std::move(descriptor),
            std::move(mesh)
        );

    return reinterpret_cast<jlong>(
        host.release()
    );
}