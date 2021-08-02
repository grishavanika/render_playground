#include "stub_window.h"

#include "shaders/vert.spv.h"
#include "shaders/frag.spv.h"

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

    const char* const kEnabledPhysExtensions[] =
    {
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,   // Debug.
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
    create_info.enabledExtensionCount = uint32_t(std::size(kEnabledPhysExtensions));
    create_info.ppEnabledExtensionNames = kEnabledPhysExtensions;

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
    const uint32_t graphics_family_index = uint32_t(std::distance(std::begin(families), graphics_it));

    // To simplify things.
    VkBool32 present_support = false;
    Panic(vkGetPhysicalDeviceSurfaceSupportKHR(
        physical_device, graphics_family_index, surface, &present_support));
    Panic(present_support);

    VkDeviceQueueCreateInfo queue_info{};
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.pNext = nullptr;
    queue_info.flags = 0;
    queue_info.queueFamilyIndex = graphics_family_index;
    queue_info.queueCount = 1;
    const float priority = 1.f;
    queue_info.pQueuePriorities = &priority;

    VkPhysicalDeviceFeatures device_features{};

    const char* const kEnabledLogicalExtensions[] =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

    VkDeviceCreateInfo device_info{};
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.pNext = nullptr;
    device_info.flags = 0;
    device_info.queueCreateInfoCount = 1;
    device_info.pQueueCreateInfos = &queue_info;
    device_info.enabledLayerCount = uint32_t(std::size(kEnabledLayers)); // Ignored.
    device_info.ppEnabledLayerNames = kEnabledLayers; // Ignored.
    device_info.enabledExtensionCount = uint32_t(std::size(kEnabledLogicalExtensions));
    device_info.ppEnabledExtensionNames = kEnabledLogicalExtensions;
    device_info.pEnabledFeatures = &device_features;

    VkDevice device{};
    Panic(vkCreateDevice(physical_device, &device_info, nullptr/*Allocator*/, &device));

    VkQueue graphics_queue{};
    vkGetDeviceQueue(device, graphics_family_index, 0, &graphics_queue);
    
    // For the color space we'll use SRGB if it is available, because it results in more
    // accurate perceived colors.It is also pretty much the standard color space
    // for images, like the textures we'll use later on.
    uint32_t format_count = 0;
    Panic(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, nullptr));
    std::vector<VkSurfaceFormatKHR> formats{std::size_t(format_count)};
    Panic(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, formats.data()));
    Panic(formats.size() > 0);

    // Only the VK_PRESENT_MODE_FIFO_KHR mode is guaranteed to be available.
    uint32_t present_mode_count = 0;
    Panic(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, nullptr));
    std::vector<VkPresentModeKHR> present_modes{std::size_t(present_mode_count)};
    Panic(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, present_modes.data()));
    Panic(present_modes.size() > 0);

    VkSurfaceCapabilitiesKHR capabilities{};
    Panic(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &capabilities));
    Panic(capabilities.currentExtent.height != UINT32_MAX);

    // Simply sticking to this minimum means that we may sometimes have
    // to wait on the driver to complete internal operations before we can acquire
    // another image to render to.Therefore it is recommended to request at least one
    // more image than the minimum.
    const uint32_t images_count = std::clamp(capabilities.minImageCount + 1,
        capabilities.minImageCount,
        (std::max)(capabilities.maxImageCount, capabilities.minImageCount));

    VkSwapchainCreateInfoKHR swapchain_info{};
    swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_info.pNext = nullptr;
    swapchain_info.flags = 0;
    swapchain_info.surface = surface;
    swapchain_info.minImageCount = images_count;
    swapchain_info.imageFormat = VK_FORMAT_B8G8R8A8_SRGB;
    swapchain_info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    swapchain_info.imageExtent = capabilities.currentExtent;
    // Amount of layers each image consists of.
    // This is always 1 unless you are developing a stereoscopic 3D application.
    swapchain_info.imageArrayLayers = 1;
    // We're going to render directly to them, which
    // means that they're used as color attachment.It is also possible that you'll render
    // images to a separate image first to perform operations like post - processing.In
    // that case you may use a value like VK_IMAGE_USAGE_TRANSFER_DST_BIT.
    swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // 1 queue for present & graphics.
    swapchain_info.queueFamilyIndexCount = 0; // Optional.
    swapchain_info.pQueueFamilyIndices = nullptr; // Optional.
    swapchain_info.preTransform = capabilities.currentTransform;
    swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapchain_info.clipped = VK_TRUE;
    swapchain_info.oldSwapchain = VK_NULL_HANDLE;

    VkSwapchainKHR swapchain{};
    Panic(vkCreateSwapchainKHR(device, &swapchain_info, nullptr/*Allocator*/, &swapchain));

    uint32_t actual_images_count = 0;
    Panic(vkGetSwapchainImagesKHR(device, swapchain, &actual_images_count, nullptr));
    std::vector<VkImage> swapchain_images{std::size_t(actual_images_count)};
    Panic(vkGetSwapchainImagesKHR(device, swapchain, &actual_images_count, swapchain_images.data()));

    std::vector<VkImageView> swapchain_image_views;
    swapchain_image_views.reserve(swapchain_images.size());
    for (VkImage image : swapchain_images)
    {
        VkImageViewCreateInfo image_view_info{};
        image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_info.pNext = nullptr;
        image_view_info.flags = 0;
        image_view_info.image = image;
        image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        image_view_info.format = swapchain_info.imageFormat;
        image_view_info.components = {
              VK_COMPONENT_SWIZZLE_IDENTITY
            , VK_COMPONENT_SWIZZLE_IDENTITY
            , VK_COMPONENT_SWIZZLE_IDENTITY
            , VK_COMPONENT_SWIZZLE_IDENTITY };
        // Our images will be used as color targets
        // without any mipmapping levels or multiple layers.
        image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_view_info.subresourceRange.baseMipLevel = 0;
        image_view_info.subresourceRange.levelCount = 1;
        image_view_info.subresourceRange.baseArrayLayer = 0;
        image_view_info.subresourceRange.layerCount = 1;
        
        swapchain_image_views.push_back(VK_NULL_HANDLE);
        Panic(vkCreateImageView(device, &image_view_info, nullptr/*Allocator*/, &swapchain_image_views.back()));
    }

    VkAttachmentDescription color_attachment{};
    color_attachment.flags = 0;
    color_attachment.format = swapchain_info.imageFormat;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_ref{};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.flags = 0;
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.inputAttachmentCount = 0;
    subpass.pInputAttachments = nullptr;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;
    subpass.pResolveAttachments = nullptr;
    subpass.pDepthStencilAttachment = nullptr;
    subpass.preserveAttachmentCount = 0;
    subpass.pPreserveAttachments = nullptr;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency.dependencyFlags = 0;

    VkRenderPassCreateInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.pNext = nullptr;
    render_pass_info.flags = 0;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;

    VkRenderPass render_pass{};
    Panic(vkCreateRenderPass(device, &render_pass_info, nullptr/*Allocator*/, &render_pass));

    auto make_shader = [&](const void* data, size_t size)
    {
        VkShaderModuleCreateInfo shader_info{};
        shader_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shader_info.pNext = nullptr;
        shader_info.flags = 0;
        shader_info.codeSize = size;
        shader_info.pCode = static_cast<const uint32_t*>(data);

        VkShaderModule shader;
        Panic(vkCreateShaderModule(device, &shader_info, nullptr/*Allocator*/, &shader));
        return shader;
    };

    VkShaderModule vertex_bytecode   = make_shader(vert_spv, std::size(vert_spv));
    VkShaderModule fragment_bytecode = make_shader(frag_spv, std::size(frag_spv));

    VkPipelineShaderStageCreateInfo vertex_info{};
    vertex_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertex_info.pNext = nullptr;
    vertex_info.flags = 0;
    vertex_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertex_info.module = vertex_bytecode;
    vertex_info.pName = "main"; // entrypoint.
    vertex_info.pSpecializationInfo = nullptr;

    VkPipelineShaderStageCreateInfo fragment_info{};
    fragment_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragment_info.pNext = nullptr;
    fragment_info.flags = 0;
    fragment_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragment_info.module = fragment_bytecode;
    fragment_info.pName = "main"; // entrypoint.
    fragment_info.pSpecializationInfo = nullptr;

    VkPipelineVertexInputStateCreateInfo vertex_input_info{};
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.pNext = nullptr;
    vertex_input_info.flags = 0;
    vertex_input_info.vertexBindingDescriptionCount = 0;    // No data yet. Hard-coded in the shader.
    vertex_input_info.pVertexBindingDescriptions = nullptr;
    vertex_input_info.vertexAttributeDescriptionCount = 0;
    vertex_input_info.pVertexAttributeDescriptions = nullptr;

    VkPipelineInputAssemblyStateCreateInfo input_assembly_info{};
    input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_info.pNext = nullptr;
    input_assembly_info.flags = 0;
    input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_info.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = float(swapchain_info.imageExtent.width);
    viewport.height = float(swapchain_info.imageExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapchain_info.imageExtent;

    VkPipelineViewportStateCreateInfo viewport_info{};
    viewport_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_info.pNext = nullptr;
    viewport_info.flags = 0;
    viewport_info.viewportCount = 1;
    viewport_info.pViewports = &viewport;
    viewport_info.scissorCount = 1;
    viewport_info.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer_info{};
    rasterizer_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer_info.pNext = nullptr;
    rasterizer_info.flags = 0;
    rasterizer_info.depthClampEnable = VK_FALSE;
    rasterizer_info.rasterizerDiscardEnable = VK_FALSE;
    rasterizer_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer_info.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer_info.depthBiasEnable = VK_FALSE;
    rasterizer_info.depthBiasConstantFactor = 0.f;
    rasterizer_info.depthBiasClamp = 0.f;
    rasterizer_info.depthBiasSlopeFactor = 0.f;
    rasterizer_info.lineWidth = 1.f;

    VkPipelineMultisampleStateCreateInfo multisampling_info{};
    multisampling_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling_info.pNext = nullptr;
    multisampling_info.flags = 0;
    multisampling_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling_info.sampleShadingEnable = VK_FALSE;
    multisampling_info.minSampleShading = 1.f;
    multisampling_info.pSampleMask = nullptr;
    multisampling_info.alphaToCoverageEnable = VK_FALSE;
    multisampling_info.alphaToOneEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.blendEnable = VK_FALSE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo color_blend_info{};
    color_blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_info.pNext = nullptr;
    color_blend_info.flags = 0;
    color_blend_info.logicOpEnable = VK_FALSE;
    color_blend_info.logicOp = VK_LOGIC_OP_COPY;
    color_blend_info.attachmentCount = 1;
    color_blend_info.pAttachments = &color_blend_attachment;
    color_blend_info.blendConstants[0] = 0.f;
    color_blend_info.blendConstants[1] = 0.f;
    color_blend_info.blendConstants[2] = 0.f;
    color_blend_info.blendConstants[3] = 0.f;

    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.pNext = nullptr;
    pipeline_layout_info.flags = 0;
    pipeline_layout_info.setLayoutCount = 0;
    pipeline_layout_info.pSetLayouts = nullptr;
    pipeline_layout_info.pushConstantRangeCount = 0;
    pipeline_layout_info.pPushConstantRanges = nullptr;

    VkPipelineLayout pipeline_layout{};
    Panic(vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &pipeline_layout));

    const VkPipelineShaderStageCreateInfo stages[] = {vertex_info, fragment_info};
    
    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.pNext = nullptr;
    pipeline_info.flags = 0;
    pipeline_info.stageCount = uint32_t(std::size(stages));
    pipeline_info.pStages = stages;
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly_info;
    pipeline_info.pTessellationState = nullptr;
    pipeline_info.pViewportState = &viewport_info;
    pipeline_info.pRasterizationState = &rasterizer_info;
    pipeline_info.pMultisampleState = &multisampling_info;
    pipeline_info.pDepthStencilState = nullptr;
    pipeline_info.pColorBlendState = &color_blend_info;
    pipeline_info.pDynamicState = nullptr;
    pipeline_info.layout = pipeline_layout;
    pipeline_info.renderPass = render_pass;
    pipeline_info.subpass = 0;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.basePipelineIndex = -1;

    VkPipeline graphics_pipeline{};
    Panic(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &graphics_pipeline));

    std::vector<VkFramebuffer> framebuffers;
    framebuffers.reserve(swapchain_image_views.size());
    for (auto& view : swapchain_image_views)
    {
        VkFramebufferCreateInfo framebuffer_info{};
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.pNext = nullptr;
        framebuffer_info.flags = 0;
        framebuffer_info.renderPass = render_pass;
        framebuffer_info.attachmentCount = 1;
        framebuffer_info.pAttachments = &view;
        framebuffer_info.width = swapchain_info.imageExtent.width;
        framebuffer_info.height = swapchain_info.imageExtent.height;
        framebuffer_info.layers = 1;

        framebuffers.push_back(VK_NULL_HANDLE);
        Panic(vkCreateFramebuffer(device, &framebuffer_info, nullptr, &framebuffers.back()));
    }


    VkCommandPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.pNext = nullptr;
    pool_info.flags = 0;
    pool_info.queueFamilyIndex = graphics_family_index;
    
    VkCommandPool command_pool{};
    Panic(vkCreateCommandPool(device, &pool_info, nullptr, &command_pool));
    
    std::vector<VkCommandBuffer> command_buffers{framebuffers.size()};

    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.pNext = nullptr;
    alloc_info.commandPool = command_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = uint32_t(command_buffers.size());
    Panic(vkAllocateCommandBuffers(device, &alloc_info, command_buffers.data()));

    for (uint32_t i = 0, count = uint32_t(framebuffers.size()); i < count; ++i)
    {
        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.pNext = nullptr;
        begin_info.flags = 0;
        begin_info.pInheritanceInfo = nullptr;

        VkRenderPassBeginInfo render_pass_begin_info{};
        render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_begin_info.pNext = nullptr;
        render_pass_begin_info.renderPass = render_pass;
        render_pass_begin_info.framebuffer = framebuffers[i];
        render_pass_begin_info.renderArea.extent = swapchain_info.imageExtent;
        render_pass_begin_info.renderArea.offset = {0, 0};
        VkClearValue clear_color{0.0f, 0.0f, 0.0f, 1.0f};
        render_pass_begin_info.clearValueCount = 1;
        render_pass_begin_info.pClearValues = &clear_color;

        Panic(vkBeginCommandBuffer(command_buffers[i], &begin_info));

        vkCmdBeginRenderPass(command_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);
        vkCmdDraw(command_buffers[i], 3/*vertexCount*/, 1/*instanceCount*/, 0/*firstVertex*/, 0/*firstInstance*/);
        vkCmdEndRenderPass(command_buffers[i]);

        Panic(vkEndCommandBuffer(command_buffers[i]));
    }

    const std::size_t k_MaxFramesInFlight = 2;

    VkSemaphoreCreateInfo semaphore_info{};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_info.pNext = nullptr;
    semaphore_info.flags = 0;

    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.pNext = nullptr;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    std::vector<VkSemaphore> semaphores_image_available{};
    std::vector<VkSemaphore> semaphores_render_finished{};
    std::vector<VkFence> in_flight_fences{};
    std::vector<VkFence> in_flight_images{};
    semaphores_image_available.resize(k_MaxFramesInFlight);
    semaphores_render_finished.resize(k_MaxFramesInFlight);
    in_flight_fences.resize(k_MaxFramesInFlight);
    in_flight_images.resize(swapchain_images.size(), VK_NULL_HANDLE);
    for (std::size_t i = 0; i < k_MaxFramesInFlight; ++i)
    {
        Panic(vkCreateSemaphore(device, &semaphore_info, nullptr, &semaphores_image_available[i]));
        Panic(vkCreateSemaphore(device, &semaphore_info, nullptr, &semaphores_render_finished[i]));
        Panic(vkCreateFence(device, &fence_info, nullptr, &in_flight_fences[i]));
    }

    std::size_t current_frame = 0;
    // Main loop.
    MSG msg{};
    while (WM_QUIT != msg.message)
    {
        if (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            continue;
        }

        Panic(vkWaitForFences(device, 1, &in_flight_fences[current_frame], VK_TRUE, UINT64_MAX));
        Panic(vkResetFences(device, 1, &in_flight_fences[current_frame]));

        uint32_t image_index = 0;
        Panic(vkAcquireNextImageKHR(device
            , swapchain
            , UINT64_MAX
            , semaphores_image_available[current_frame]
            , VK_NULL_HANDLE
            , &image_index));
        // Check if a previous frame is using this image (i.e. there is its fence to wait on).
        if (in_flight_images[image_index] != VK_NULL_HANDLE)
        {
            Panic(vkWaitForFences(device, 1, &in_flight_images[image_index], VK_TRUE, UINT64_MAX));
        }
        // Mark the image as now being in use by this frame.
        in_flight_images[image_index] = in_flight_fences[current_frame];

        VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pNext = nullptr;
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = &semaphores_image_available[current_frame];
        submit_info.pWaitDstStageMask = wait_stages;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffers[image_index];
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = &semaphores_render_finished[current_frame];
        Panic(vkResetFences(device, 1, &in_flight_fences[current_frame]));
        Panic(vkQueueSubmit(graphics_queue, 1, &submit_info, in_flight_fences[current_frame]));

        VkPresentInfoKHR present_info{};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.pNext = nullptr;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &semaphores_render_finished[current_frame];
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &swapchain;
        present_info.pImageIndices = &image_index;
        present_info.pResults = nullptr;

        Panic(vkQueuePresentKHR(graphics_queue, &present_info));
        
        current_frame = ((current_frame + 1) % k_MaxFramesInFlight);
    }

    Panic(vkDeviceWaitIdle(device));

    // Destroy.
    for (std::size_t i = 0; i < k_MaxFramesInFlight; ++i)
    {
        vkDestroySemaphore(device, semaphores_render_finished[i], nullptr);
        vkDestroySemaphore(device, semaphores_image_available[i], nullptr);
        vkDestroyFence(device, in_flight_fences[i], nullptr);
    }
    vkDestroyCommandPool(device, command_pool, nullptr);
    for (VkFramebuffer framebuffer : framebuffers)
    {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
    vkDestroyPipeline(device, graphics_pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
    vkDestroyRenderPass(device, render_pass, nullptr);
    vkDestroyShaderModule(device, vertex_bytecode, nullptr);
    vkDestroyShaderModule(device, fragment_bytecode, nullptr);
    for (VkImageView view : swapchain_image_views)
    {
        vkDestroyImageView(device, view, nullptr);
    }
    vkDestroySwapchainKHR(device, swapchain, nullptr);
    vkDestroySurfaceKHR(app.vk_instance_, surface, nullptr);
    vkDestroyDevice(device, nullptr);
    auto vkDestroyDebugUtilsMessengerEXT_ = PFN_vkDestroyDebugUtilsMessengerEXT(vkGetInstanceProcAddr(
        app.vk_instance_, "vkDestroyDebugUtilsMessengerEXT"));
    vkDestroyDebugUtilsMessengerEXT_(app.vk_instance_, vk_debug_msg, nullptr);
    vkDestroyInstance(app.vk_instance_, nullptr);

    return 0;
}
