#include "stub_window.h"

#include <vector>
#include <algorithm>

#include <cstdlib>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

#define NOMINMAX
#include <Windows.h>
#include <tchar.h>

#if defined(NDEBUG)
#  undef NDEBUG
#endif
#include <cassert>

inline void Panic(VkResult vk)
{
    if (vk != VK_SUCCESS)
    {
        assert(false && "Vulkan Panic.");
        std::exit(1);
    }
}

struct AppState
{
    VkInstance vk_instance_;
};

#define X_ENABLE_ADDITIONAL_DEBUG() 0

static VkBool32 vkDebugUtilsMessengerCallbackEXT_(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT /*messageTypes*/,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* /*pUserData*/)
{
#if (!X_ENABLE_ADDITIONAL_DEBUG())
    if ((messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
        || (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT))
    {
        return VK_FALSE;
    }
#endif

    const char* const severity = [&]
    {
        switch (messageSeverity)
        {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: return "Verbose";
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT   : return "Info";
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: return "Warning";
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT  : return "Error";
        }
        return "";
    }();
    std::fprintf(stderr, "[Vulkan] [%s]: %s\n", severity, pCallbackData->pMessage);
    std::fflush(stderr);
    return VK_FALSE;
}

// https://vulkan-tutorial.com.
int WINAPI _tWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPTSTR /*lpCmdLine*/, int /*nCmdShow*/)
{
    Panic(::AllocConsole());
    {
        FILE* unused = nullptr;
        freopen_s(&unused, "CONIN$",  "r", stdin);
        freopen_s(&unused, "CONOUT$", "w", stderr);
        freopen_s(&unused, "CONOUT$", "w", stdout);
    }

    AppState app{};

    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext = nullptr;
    app_info.pApplicationName = "vulkan playground";
    app_info.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
    app_info.pEngineName = "playground";
    app_info.engineVersion = VK_MAKE_VERSION(0, 0, 1);
    app_info.apiVersion = VK_API_VERSION_1_0;

    { // See supported extensions.
        uint32_t extensions_count = 0;
        Panic(vkEnumerateInstanceExtensionProperties(nullptr, &extensions_count, nullptr));
        std::vector<VkExtensionProperties> extensions{std::size_t(extensions_count)};
        Panic(vkEnumerateInstanceExtensionProperties(nullptr, &extensions_count, extensions.data()));
        (void)extensions;
    }

    const char* const kEnabledLayers[] =
    {
        "VK_LAYER_KHRONOS_validation",
    };

    const char* const kEnabledExtensions[] =
    {
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
    };

    // See supported layers.
    {
        uint32_t layers_count = 0;
        Panic(vkEnumerateInstanceLayerProperties(&layers_count, nullptr));
        std::vector<VkLayerProperties> layers{std::size_t(layers_count)};
        Panic(vkEnumerateInstanceLayerProperties(&layers_count, layers.data()));

#if (0)
        // Not needed, VK_ERROR_LAYER_NOT_PRESENT error is there.
        const bool all_layers_vailable = std::all_of(
            std::begin(kEnabledLayers), std::end(kEnabledLayers)
            , [&](const char* required_layer)
        {
            auto it = std::find_if(std::begin(layers), std::end(layers)
                , [&](const VkLayerProperties& available_layer)
            {
                return (std::strcmp(required_layer, available_layer.layerName) == 0);
            });
            return (it != std::end(layers));
        });

        Panic(all_layers_vailable);
#endif
    }

    VkDebugUtilsMessengerCreateInfoEXT debug_info{};
    debug_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug_info.pNext = nullptr;
    debug_info.flags = 0;
    debug_info.messageSeverity =
          VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug_info.messageType =
          VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debug_info.pfnUserCallback = &vkDebugUtilsMessengerCallbackEXT_;
    debug_info.pUserData = nullptr;

    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pNext = &debug_info;
    create_info.flags = 0;
    create_info.pApplicationInfo = &app_info;
    create_info.enabledLayerCount = uint32_t(std::size(kEnabledLayers));
    create_info.ppEnabledLayerNames = kEnabledLayers;
    create_info.enabledExtensionCount = uint32_t(std::size(kEnabledExtensions));
    create_info.ppEnabledExtensionNames = kEnabledExtensions;

    Panic(vkCreateInstance(&create_info, nullptr/*Allocator*/, &app.vk_instance_));

    VkDebugUtilsMessengerEXT vk_debug_msg{};
    auto vkCreateDebugUtilsMessengerEXT_ = PFN_vkCreateDebugUtilsMessengerEXT(vkGetInstanceProcAddr(
        app.vk_instance_, "vkCreateDebugUtilsMessengerEXT"));
    Panic(vkCreateDebugUtilsMessengerEXT_(app.vk_instance_, &debug_info, nullptr/*Allocator*/, &vk_debug_msg));

    StubWindow window("xxx_render_playground");
    Panic(::ShowWindow(window.wnd(), SW_SHOW) == 0/*was previously hidden*/);

    VkSurfaceKHR surface{};
    VkWin32SurfaceCreateInfoKHR surface_info{};
    surface_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surface_info.pNext = nullptr;
    surface_info.flags = 0;
    surface_info.hinstance = ::GetModuleHandle(nullptr);
    surface_info.hwnd = window.wnd();

    Panic(vkCreateWin32SurfaceKHR(app.vk_instance_, &surface_info, nullptr/*Allocator*/, &surface));

    uint32_t device_count = 0;
    Panic(vkEnumeratePhysicalDevices(app.vk_instance_, &device_count, nullptr));
    Panic(device_count > 0);
    std::vector<VkPhysicalDevice> devices{std::size_t(device_count)};
    Panic(vkEnumeratePhysicalDevices(app.vk_instance_, &device_count, devices.data()));

    VkPhysicalDevice physical_device{}; // VK_NULL_HANDLE.
    {
        auto it = std::find_if(std::begin(devices), std::end(devices)
            , [](VkPhysicalDevice device)
        {
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(device, &properties);
#if (0)
            VkPhysicalDeviceFeatures features;
            vkGetPhysicalDeviceFeatures(device, &features);
#endif
            return (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);
        });
        Panic(it != std::end(devices));
        physical_device = *it;
    }

    uint32_t family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &family_count, nullptr);
    std::vector<VkQueueFamilyProperties> families{std::size_t(family_count)};
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &family_count, families.data());
    auto graphics_it = std::find_if(std::begin(families), std::end(families)
        , [](const VkQueueFamilyProperties& family_info)
    {
        return (family_info.queueFlags & VK_QUEUE_GRAPHICS_BIT);
    });
    Panic(graphics_it != std::end(families));
    const uint32_t graphic_family_index = uint32_t(std::distance(std::begin(families), graphics_it));

    VkDeviceQueueCreateInfo queue_info{};
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.pNext = nullptr;
    queue_info.flags = 0;
    queue_info.queueFamilyIndex = graphic_family_index;
    queue_info.queueCount = 1;
    const float priority = 1.f;
    queue_info.pQueuePriorities = &priority;

    VkPhysicalDeviceFeatures device_features{};

    VkDeviceCreateInfo device_info{};
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.pNext = nullptr;
    device_info.flags = 0;
    device_info.queueCreateInfoCount = 1;
    device_info.pQueueCreateInfos = &queue_info;
    device_info.enabledLayerCount = uint32_t(std::size(kEnabledLayers)); // Ignored.
    device_info.ppEnabledLayerNames = kEnabledLayers; // Ignored.
    device_info.enabledExtensionCount = 0;
    device_info.ppEnabledExtensionNames = nullptr;
    device_info.pEnabledFeatures = &device_features;

    VkDevice device{};
    Panic(vkCreateDevice(physical_device, &device_info, nullptr/*Allocator*/, &device));

    VkQueue graphics_queue{};
    vkGetDeviceQueue(device, graphic_family_index, 0, &graphics_queue);

    const uint32_t present_family_index = [&]
    {
        for (uint32_t family_index = 0, count = uint32_t(families.size());
            family_index < count; ++family_index)
        {
            VkBool32 present_support = false;
            Panic(vkGetPhysicalDeviceSurfaceSupportKHR(
                physical_device, family_index, surface, &present_support));
            if (present_support)
            {
                return family_index;
            }
        }
        Unreachable();
    }();
    // To simplify things.
    Panic(present_family_index == graphic_family_index);

    // Destroy.
    vkDestroyDevice(device, nullptr/*Allocator*/);
    auto vkDestroyDebugUtilsMessengerEXT_ = PFN_vkDestroyDebugUtilsMessengerEXT(vkGetInstanceProcAddr(
        app.vk_instance_, "vkDestroyDebugUtilsMessengerEXT"));
    vkDestroySurfaceKHR(app.vk_instance_, surface, nullptr);
    vkDestroyDebugUtilsMessengerEXT_(app.vk_instance_, vk_debug_msg, nullptr/*Allocator*/);
    vkDestroyInstance(app.vk_instance_, nullptr/*Allocator*/);

    return 0;
}
