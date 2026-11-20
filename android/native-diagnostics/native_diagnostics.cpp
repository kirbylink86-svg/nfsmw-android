#include <jni.h>
#include <android/log.h>
#include <vulkan/vulkan.h>
#include <dlfcn.h>
#include <cstdio>

#define LOG_TAG "NFSMW-Native-Diagnostics"

static const char* vulkan_status() {
    void* library = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
    if (library == nullptr) return "Vulkan: el cargador libvulkan.so no está disponible.";

    auto get_proc = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
        dlsym(library, "vkGetInstanceProcAddr"));
    if (get_proc == nullptr) {
        dlclose(library);
        return "Vulkan: no se encontró vkGetInstanceProcAddr.";
    }

    uint32_t version = VK_API_VERSION_1_0;
    auto enumerate_version = reinterpret_cast<PFN_vkEnumerateInstanceVersion>(
        get_proc(VK_NULL_HANDLE, "vkEnumerateInstanceVersion"));
    if (enumerate_version != nullptr &&
        enumerate_version(&version) != VK_SUCCESS) {
        version = VK_API_VERSION_1_0;
    }

    VkApplicationInfo app{};
    app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app.pApplicationName = "NFSMW Native Diagnostics";
    app.apiVersion = version;

    VkInstanceCreateInfo create{};
    create.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create.pApplicationInfo = &app;

    auto create_instance = reinterpret_cast<PFN_vkCreateInstance>(
        get_proc(VK_NULL_HANDLE, "vkCreateInstance"));
    auto destroy_instance = reinterpret_cast<PFN_vkDestroyInstance>(
        get_proc(VK_NULL_HANDLE, "vkDestroyInstance"));
    auto enumerate_devices = reinterpret_cast<PFN_vkEnumeratePhysicalDevices>(
        get_proc(VK_NULL_HANDLE, "vkEnumeratePhysicalDevices"));

    if (!create_instance || !destroy_instance || !enumerate_devices) {
        dlclose(library);
        return "Vulkan: faltan funciones esenciales del cargador.";
    }

    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = create_instance(&create, nullptr, &instance);
    if (result != VK_SUCCESS) {
        char message[128];
        std::snprintf(message, sizeof(message),
                      "Vulkan: no se pudo crear instancia (código %d).", result);
        dlclose(library);
        static char output[128];
        std::snprintf(output, sizeof(output), "%s", message);
        return output;
    }

    uint32_t count = 0;
    result = enumerate_devices(instance, &count, nullptr);
    destroy_instance(instance, nullptr);
    dlclose(library);

    if (result != VK_SUCCESS || count == 0)
        return "Vulkan: cargador disponible, pero no se detectó una GPU física.";

    static char output[160];
    std::snprintf(output, sizeof(output),
                  "Vulkan disponible. API %u.%u.%u. GPU detectada: sí.",
                  VK_VERSION_MAJOR(version), VK_VERSION_MINOR(version),
                  VK_VERSION_PATCH(version));
    return output;
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_nfsmw_diagnostics_MainActivity_nativeGetVulkanStatus(
    JNIEnv* env, jobject) {
    const char* status = vulkan_status();
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "%s", status);
    return env->NewStringUTF(status);
}

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void*) {
    if (vm == nullptr) return JNI_ERR;
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "Diagnóstico ARM64 inicializado");
    return JNI_VERSION_1_6;
}
