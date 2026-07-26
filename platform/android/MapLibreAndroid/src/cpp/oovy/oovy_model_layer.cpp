#include <android/log.h>
#include <jni.h>

#include <memory>
#include <string>
#include <utility>

#include <mbgl/style/layers/custom_layer.hpp>

#include "oovy_model_descriptor.hpp"
#include "oovy_model_descriptor_jni.hpp"

namespace {

constexpr const char* kLogTag = "OovyModelLayer";

class DescriptorOnlyModelHost final
    : public mbgl::style::CustomLayerHost {
public:
    explicit DescriptorOnlyModelHost(
        oovy::OovyModelDescriptor descriptor_
    )
        : descriptor(std::move(descriptor_)) {
    }

    bool is3D() const override {
        return true;
    }

    void initialize() override {
        __android_log_print(
            ANDROID_LOG_INFO,
            kLogTag,
            "Descriptor host initialized: assetId=%s path=%s",
            descriptor.assetId.c_str(),
            descriptor.localPath.c_str()
        );
    }

    void render(
        const mbgl::style::CustomLayerRenderParameters&
    ) override {
        // Rendering will be connected after the JNI contract is validated.
    }

    void contextLost() override {
    }

    void deinitialize() override {
        __android_log_print(
            ANDROID_LOG_INFO,
            kLogTag,
            "Descriptor host deinitialized: assetId=%s",
            descriptor.assetId.c_str()
        );
    }

private:
    oovy::OovyModelDescriptor descriptor;
};

void throwIllegalArgument(
    JNIEnv* env,
    const std::string& message
) {
    if (
        env == nullptr ||
        env->ExceptionCheck()
    ) {
        return;
    }

    jclass exceptionClass =
        env->FindClass(
            "java/lang/IllegalArgumentException"
        );

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

        throwIllegalArgument(
            env,
            error.empty()
                ? "Invalid OovyModelDescriptor"
                : error
        );

        return 0;
    }

    __android_log_print(
        ANDROID_LOG_INFO,
        kLogTag,
        "Descriptor converted: assetId=%s "
        "lat=%.7f lon=%.7f altitude=%.2f "
        "yaw=%.2f scale=%.3f zoom=[%.1f, %.1f]",
        descriptor.assetId.c_str(),
        descriptor.latitude,
        descriptor.longitude,
        descriptor.altitudeMeters,
        descriptor.yawDegrees,
        descriptor.uniformScale,
        descriptor.minZoom,
        descriptor.maxZoom
    );

    return reinterpret_cast<jlong>(
        std::make_unique<DescriptorOnlyModelHost>(
            std::move(descriptor)
        ).release()
    );
}