#include <jni.h>
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#define VK_USE_PLATFORM_ANDROID_KHR
#include <vulkan/vulkan.h>
#include <dlfcn.h>
#include <cstdio>
#include <cstring>
#include <SDL3/SDL.h>

#define LOG_TAG "NFSMW-Native-Diagnostics"

static bool has_instance_extension(PFN_vkEnumerateInstanceExtensionProperties enumerate,
                                   const char* wanted) {
    uint32_t count = 0;
    if (enumerate(nullptr, &count, nullptr) != VK_SUCCESS || count == 0) return false;
    VkExtensionProperties* properties = new VkExtensionProperties[count];
    const VkResult result = enumerate(nullptr, &count, properties);
    bool found = false;
    if (result == VK_SUCCESS) {
        for (uint32_t i = 0; i < count; ++i) {
            if (std::strcmp(properties[i].extensionName, wanted) == 0) {
                found = true;
                break;
            }
        }
    }
    delete[] properties;
    return found;
}

static void run_vulkan_surface_probe(ANativeWindow* window, char* output, size_t output_size) {
    void* library = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
    if (!library) {
        std::snprintf(output, output_size, "Vulkan: no se pudo abrir libvulkan.so.");
        return;
    }

    auto get_proc = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
        dlsym(library, "vkGetInstanceProcAddr"));
    if (!get_proc) {
        std::snprintf(output, output_size, "Vulkan: vkGetInstanceProcAddr no disponible.");
        dlclose(library);
        return;
    }

    auto enumerate_extensions = reinterpret_cast<PFN_vkEnumerateInstanceExtensionProperties>(
        get_proc(VK_NULL_HANDLE, "vkEnumerateInstanceExtensionProperties"));
    auto create_instance = reinterpret_cast<PFN_vkCreateInstance>(
        get_proc(VK_NULL_HANDLE, "vkCreateInstance"));
    if (!enumerate_extensions || !create_instance ||
        !has_instance_extension(enumerate_extensions, VK_KHR_SURFACE_EXTENSION_NAME) ||
        !has_instance_extension(enumerate_extensions, VK_KHR_ANDROID_SURFACE_EXTENSION_NAME)) {
        std::snprintf(output, output_size,
                      "Vulkan: faltan extensiones para crear una superficie Android.");
        dlclose(library);
        return;
    }

    const char* extensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_ANDROID_SURFACE_EXTENSION_NAME
    };
    VkApplicationInfo app{};
    app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app.pApplicationName = "SDL3 Vulkan Surface Diagnostics";
    app.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo create{};
    create.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create.pApplicationInfo = &app;
    create.enabledExtensionCount = 2;
    create.ppEnabledExtensionNames = extensions;

    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = create_instance(&create, nullptr, &instance);
    if (result != VK_SUCCESS) {
        std::snprintf(output, output_size, "Vulkan: no se pudo crear instancia (%d).", result);
        dlclose(library);
        return;
    }

    auto destroy_instance = reinterpret_cast<PFN_vkDestroyInstance>(
        get_proc(instance, "vkDestroyInstance"));
    auto create_surface = reinterpret_cast<PFN_vkCreateAndroidSurfaceKHR>(
        get_proc(instance, "vkCreateAndroidSurfaceKHR"));
    auto destroy_surface = reinterpret_cast<PFN_vkDestroySurfaceKHR>(
        get_proc(instance, "vkDestroySurfaceKHR"));
    auto enumerate_devices = reinterpret_cast<PFN_vkEnumeratePhysicalDevices>(
        get_proc(instance, "vkEnumeratePhysicalDevices"));
    auto get_queue_properties = reinterpret_cast<PFN_vkGetPhysicalDeviceQueueFamilyProperties>(
        get_proc(instance, "vkGetPhysicalDeviceQueueFamilyProperties"));
    auto get_surface_support = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceSupportKHR>(
        get_proc(instance, "vkGetPhysicalDeviceSurfaceSupportKHR"));

    if (!destroy_instance || !create_surface || !destroy_surface ||
        !enumerate_devices || !get_queue_properties || !get_surface_support) {
        std::snprintf(output, output_size, "Vulkan: faltan funciones para probar la superficie.");
        if (destroy_instance) destroy_instance(instance, nullptr);
        dlclose(library);
        return;
    }

    VkAndroidSurfaceCreateInfoKHR surface_info{};
    surface_info.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    surface_info.window = window;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    result = create_surface(instance, &surface_info, nullptr, &surface);
    if (result != VK_SUCCESS) {
        std::snprintf(output, output_size, "Vulkan: no se pudo crear superficie Android (%d).", result);
        destroy_instance(instance, nullptr);
        dlclose(library);
        return;
    }

    uint32_t device_count = 0;
    result = enumerate_devices(instance, &device_count, nullptr);
    bool presentation_supported = false;
    if (result == VK_SUCCESS && device_count > 0) {
        VkPhysicalDevice* devices = new VkPhysicalDevice[device_count];
        if (enumerate_devices(instance, &device_count, devices) == VK_SUCCESS) {
            for (uint32_t d = 0; d < device_count && !presentation_supported; ++d) {
                uint32_t family_count = 0;
                get_queue_properties(devices[d], &family_count, nullptr);
                VkQueueFamilyProperties* families = new VkQueueFamilyProperties[family_count];
                get_queue_properties(devices[d], &family_count, families);
                for (uint32_t q = 0; q < family_count; ++q) {
                    VkBool32 supported = VK_FALSE;
                    if (families[q].queueCount > 0 &&
                        get_surface_support(devices[d], q, surface, &supported) == VK_SUCCESS &&
                        supported == VK_TRUE) {
                        presentation_supported = true;
                        break;
                    }
                }
                delete[] families;
            }
        }
        delete[] devices;
    }

    destroy_surface(instance, surface, nullptr);
    destroy_instance(instance, nullptr);
    dlclose(library);

    std::snprintf(output, output_size,
                  "Vulkan: superficie Android creada. Presentación: %s.",
                  presentation_supported ? "compatible" : "no confirmada");
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_nfsmw_diagnostics_MainActivity_nativeRunDiagnostics(
    JNIEnv* env, jobject, jobject java_surface) {
    char sdl_status[128] = {};
    if (SDL_Init(SDL_INIT_EVENTS)) {
        std::snprintf(sdl_status, sizeof(sdl_status),
                      "SDL3: inicialización correcta (versión %u).", SDL_GetVersion());
        SDL_Quit();
    } else {
        std::snprintf(sdl_status, sizeof(sdl_status),
                      "SDL3: error de inicialización: %.80s", SDL_GetError());
    }

    if (!java_surface) {
        char message[320];
        std::snprintf(message, sizeof(message), "%s Vulkan: falta la superficie Android.",
                      sdl_status);
        return env->NewStringUTF(message);
    }

    ANativeWindow* window = ANativeWindow_fromSurface(env, java_surface);
    if (!window) {
        char message[320];
        std::snprintf(message, sizeof(message), "%s Vulkan: no se obtuvo ANativeWindow.",
                      sdl_status);
        return env->NewStringUTF(message);
    }

    char vulkan_status[192] = {};
    run_vulkan_surface_probe(window, vulkan_status, sizeof(vulkan_status));
    ANativeWindow_release(window);

    char combined[384];
    std::snprintf(combined, sizeof(combined), "%s\n%s", sdl_status, vulkan_status);
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "%s", combined);
    return env->NewStringUTF(combined);
}

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void*) {
    if (vm == nullptr) return JNI_ERR;
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "Diagnóstico ARM64 inicializado");
    return JNI_VERSION_1_6;
}
