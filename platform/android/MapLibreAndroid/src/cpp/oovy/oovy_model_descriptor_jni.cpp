#include "oovy_model_descriptor_jni.hpp"

namespace {

struct DescriptorMethodIds final {
    jmethodID getAssetId = nullptr;
    jmethodID getLocalPath = nullptr;
    jmethodID getLatitude = nullptr;
    jmethodID getLongitude = nullptr;
    jmethodID getAltitudeMeters = nullptr;
    jmethodID getYawDegrees = nullptr;
    jmethodID getUniformScale = nullptr;
    jmethodID getMinZoom = nullptr;
    jmethodID getMaxZoom = nullptr;
    jmethodID getTargetHeightMeters = nullptr;
    jmethodID getTargetFootprintWidthMeters = nullptr;
    jmethodID getTargetFootprintDepthMeters = nullptr;
};

bool loadMethodIds(
    JNIEnv* env,
    jclass descriptorClass,
    DescriptorMethodIds& methods,
    std::string& error
) {
    methods.getAssetId = env->GetMethodID(
        descriptorClass,
        "getAssetId",
        "()Ljava/lang/String;"
    );

    methods.getLocalPath = env->GetMethodID(
        descriptorClass,
        "getLocalPath",
        "()Ljava/lang/String;"
    );

    methods.getLatitude = env->GetMethodID(
        descriptorClass,
        "getLatitude",
        "()D"
    );

    methods.getLongitude = env->GetMethodID(
        descriptorClass,
        "getLongitude",
        "()D"
    );

    methods.getAltitudeMeters = env->GetMethodID(
        descriptorClass,
        "getAltitudeMeters",
        "()D"
    );

    methods.getYawDegrees = env->GetMethodID(
        descriptorClass,
        "getYawDegrees",
        "()D"
    );

    methods.getUniformScale = env->GetMethodID(
        descriptorClass,
        "getUniformScale",
        "()D"
    );

    methods.getMinZoom = env->GetMethodID(
        descriptorClass,
        "getMinZoom",
        "()D"
    );

    methods.getMaxZoom = env->GetMethodID(
        descriptorClass,
        "getMaxZoom",
        "()D"
    );

    methods.getTargetHeightMeters = env->GetMethodID(
        descriptorClass,
        "getTargetHeightMeters",
        "()D"
    );

    methods.getTargetFootprintWidthMeters = env->GetMethodID(
        descriptorClass,
        "getTargetFootprintWidthMeters",
        "()D"
    );

    methods.getTargetFootprintDepthMeters = env->GetMethodID(
        descriptorClass,
        "getTargetFootprintDepthMeters",
        "()D"
    );

    if (env->ExceptionCheck()) {
        error = "Unable to resolve OovyModelDescriptor getters";
        return false;
    }

    if (
        methods.getAssetId == nullptr ||
        methods.getLocalPath == nullptr ||
        methods.getLatitude == nullptr ||
        methods.getLongitude == nullptr ||
        methods.getAltitudeMeters == nullptr ||
        methods.getYawDegrees == nullptr ||
        methods.getUniformScale == nullptr ||
        methods.getMinZoom == nullptr ||
        methods.getMaxZoom == nullptr ||
        methods.getTargetHeightMeters == nullptr ||
        methods.getTargetFootprintWidthMeters == nullptr ||
        methods.getTargetFootprintDepthMeters == nullptr
    ) {
        error = "OovyModelDescriptor JNI getter is missing";
        return false;
    }

    return true;
}

bool readString(
    JNIEnv* env,
    jobject object,
    jmethodID method,
    const char* fieldName,
    std::string& destination,
    std::string& error
) {
    auto value = static_cast<jstring>(
        env->CallObjectMethod(object, method)
    );

    if (env->ExceptionCheck()) {
        error =
            std::string("Java getter failed for ") +
            fieldName;

        return false;
    }

    if (value == nullptr) {
        error =
            std::string(fieldName) +
            " must not be null";

        return false;
    }

    const char* characters =
        env->GetStringUTFChars(value, nullptr);

    if (characters == nullptr) {
        env->DeleteLocalRef(value);

        error =
            std::string("Unable to read UTF-8 value for ") +
            fieldName;

        return false;
    }

    destination.assign(characters);

    env->ReleaseStringUTFChars(
        value,
        characters
    );

    env->DeleteLocalRef(value);

    return true;
}

bool readDouble(
    JNIEnv* env,
    jobject object,
    jmethodID method,
    const char* fieldName,
    double& destination,
    std::string& error
) {
    destination =
        static_cast<double>(
            env->CallDoubleMethod(
                object,
                method
            )
        );

    if (env->ExceptionCheck()) {
        error =
            std::string("Java getter failed for ") +
            fieldName;

        return false;
    }

    return true;
}

} // namespace

namespace oovy::android {

bool readModelDescriptor(
    JNIEnv* env,
    jobject javaDescriptor,
    OovyModelDescriptor& descriptor,
    std::string& error
) {
    error.clear();

    if (env == nullptr) {
        error = "JNI environment is null";
        return false;
    }

    if (javaDescriptor == nullptr) {
        error = "OovyModelDescriptor is null";
        return false;
    }

    jclass descriptorClass =
        env->GetObjectClass(javaDescriptor);

    if (
        descriptorClass == nullptr ||
        env->ExceptionCheck()
    ) {
        error =
            "Unable to resolve OovyModelDescriptor Java class";

        return false;
    }

    DescriptorMethodIds methods;

    const bool methodsLoaded =
        loadMethodIds(
            env,
            descriptorClass,
            methods,
            error
        );

    env->DeleteLocalRef(descriptorClass);

    if (!methodsLoaded) {
        return false;
    }

    if (
        !readString(
            env,
            javaDescriptor,
            methods.getAssetId,
            "assetId",
            descriptor.assetId,
            error
        ) ||
        !readString(
            env,
            javaDescriptor,
            methods.getLocalPath,
            "localPath",
            descriptor.localPath,
            error
        ) ||
        !readDouble(
            env,
            javaDescriptor,
            methods.getLatitude,
            "latitude",
            descriptor.latitude,
            error
        ) ||
        !readDouble(
            env,
            javaDescriptor,
            methods.getLongitude,
            "longitude",
            descriptor.longitude,
            error
        ) ||
        !readDouble(
            env,
            javaDescriptor,
            methods.getAltitudeMeters,
            "altitudeMeters",
            descriptor.altitudeMeters,
            error
        ) ||
        !readDouble(
            env,
            javaDescriptor,
            methods.getYawDegrees,
            "yawDegrees",
            descriptor.yawDegrees,
            error
        ) ||
        !readDouble(
            env,
            javaDescriptor,
            methods.getUniformScale,
            "uniformScale",
            descriptor.uniformScale,
            error
        ) ||
        !readDouble(
            env,
            javaDescriptor,
            methods.getMinZoom,
            "minZoom",
            descriptor.minZoom,
            error
        ) ||
        !readDouble(
            env,
            javaDescriptor,
            methods.getMaxZoom,
            "maxZoom",
            descriptor.maxZoom,
            error
        ) ||
        !readDouble(
            env,
            javaDescriptor,
            methods.getTargetHeightMeters,
            "targetHeightMeters",
            descriptor.targetHeightMeters,
            error
        ) ||
        !readDouble(
            env,
            javaDescriptor,
            methods.getTargetFootprintWidthMeters,
            "targetFootprintWidthMeters",
            descriptor.targetFootprintWidthMeters,
            error
        ) ||
        !readDouble(
            env,
            javaDescriptor,
            methods.getTargetFootprintDepthMeters,
            "targetFootprintDepthMeters",
            descriptor.targetFootprintDepthMeters,
            error
        )
    ) {
        return false;
    }

    return descriptor.validate(&error);
}

} // namespace oovy::android