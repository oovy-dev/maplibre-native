#pragma once

#include <jni.h>

#include <string>

#include "oovy_model_descriptor.hpp"

namespace oovy::android {

bool readModelDescriptor(
    JNIEnv* env,
    jobject javaDescriptor,
    OovyModelDescriptor& descriptor,
    std::string& error
);

} // namespace oovy::android