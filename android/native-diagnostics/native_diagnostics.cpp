#include <jni.h>
#include <android/log.h>
#include <vulkan/vulkan.h>
#include <dlfcn.h>

#define LOG_TAG "NFSMW-Native-Diagnostics"

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void*) {
    if (vm == nullptr) return JNI_ERR;

    void* vulkan = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
    if (vulkan == nullptr) {
        __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "Android Vulkan loader not present");
        return JNI_VERSION_1_6;
    }

    auto get_proc = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
        dlsym(vulkan, "vkGetInstanceProcAddr"));
    uint32_t loader_version = VK_API_VERSION_1_0;
    if (get_proc != nullptr) {
        auto enumerate_version = reinterpret_cast<PFN_vkEnumerateInstanceVersion>(
            get_proc(VK_NULL_HANDLE, "vkEnumerateInstanceVersion"));
        if (enumerate_version != nullptr) {
            const VkResult result = enumerate_version(&loader_version);
            if (result != VK_SUCCESS) loader_version = VK_API_VERSION_1_0;
        }
        __android_log_print(ANDROID_LOG_INFO, LOG_TAG,
                            "Vulkan loader available; API %u.%u.%u",
                            VK_VERSION_MAJOR(loader_version),
                            VK_VERSION_MINOR(loader_version),
                            VK_VERSION_PATCH(loader_version));
    } else {
        __android_log_print(ANDROID_LOG_WARN, LOG_TAG,
                            "Vulkan loader does not export vkGetInstanceProcAddr");
    }
    dlclose(vulkan);
    return JNI_VERSION_1_6;
}
