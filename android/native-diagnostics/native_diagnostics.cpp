#include <jni.h>
#include <android/log.h>
#include <vulkan/vulkan.h>

#define LOG_TAG "NFSMW-Native-Diagnostics"

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void*) {
    if (vm == nullptr) return JNI_ERR;

    uint32_t loader_version = VK_API_VERSION_1_0;
    const VkResult result = vkEnumerateInstanceVersion(&loader_version);

    if (result == VK_SUCCESS) {
        __android_log_print(ANDROID_LOG_INFO, LOG_TAG,
                            "Vulkan loader available; API %u.%u.%u",
                            VK_VERSION_MAJOR(loader_version),
                            VK_VERSION_MINOR(loader_version),
                            VK_VERSION_PATCH(loader_version));
    } else {
        __android_log_print(ANDROID_LOG_WARN, LOG_TAG,
                            "Vulkan version query returned %d", result);
    }

    return JNI_VERSION_1_6;
}
