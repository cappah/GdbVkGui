#include "Vulkan/VulkanLayer.h"
#include "UtilityMacros.h"
#include "WindowInterface.h"
#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>

#define LoadFunction dlsym

//------------------------------------------------------------------------------

static VkStateBin s_vkstate;

//------------------------------------------------------------------------------

VKAPI_ATTR VkBool32 VKAPI_CALL
VGDebugCB(VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
          VkDebugUtilsMessageTypeFlagsEXT             type,
          const VkDebugUtilsMessengerCallbackDataEXT* error_data,
          void*                                       user_data)
{
    UNUSED_VAR(user_data);

    int log_type = 0;
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
        log_type = 0;
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        log_type = 0;
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        log_type = 1;
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        log_type = 2;
    }

    const char* header = " ";
    if (type & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) {
        header = "GENERAL ";
    }
    if (type & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
        header = "VALIDATION ";
    } else if (type & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
        header = "PERF ";
    }

    switch (log_type) {
        case 0: {
            fprintf(stdout,
                    "%s - %d : %s : %s\n",
                    header,
                    error_data->messageIdNumber,
                    error_data->pMessageIdName,
                    error_data->pMessage);
            break;
        }
        case 1: {
            fprintf(stdout,
                    "Warning: %s - %d : %s : %s\n",
                    header,
                    error_data->messageIdNumber,
                    error_data->pMessageIdName,
                    error_data->pMessage);
            break;
        }
        case 2: {
            fprintf(stderr,
                    "Error: %s - %d : %s : %s\n",
                    header,
                    error_data->messageIdNumber,
                    error_data->pMessageIdName,
                    error_data->pMessage);
            break;
        }
        default:
            fprintf(stdout,
                    "%s - %d : %s : %s\n",
                    header,
                    error_data->messageIdNumber,
                    error_data->pMessageIdName,
                    error_data->pMessage);
            break;
    }

    assert(log_type != 2 && "Vulkan library error");

    return VK_FALSE;
}

VkStateBin*
LoadVulkanState(AppWindowData* win)
{
    VkResult success  = 0;
    s_vkstate.m_VkLib = dlopen("libvulkan.so.1", RTLD_NOW);

    assert(s_vkstate.m_VkLib && "Failed to load vulkan library");

    // Load exported vulkan functions from dynamic library
    {
#define EXPORTED_VULKAN_FUNCTION(name)                                         \
    name = (PFN_##name)LoadFunction(s_vkstate.m_VkLib, #name);                 \
    assert(name && "Error loading exported vulkan function: " #name);
#include "Vulkan/ListOfVulkanFunctions.inl"

        // Load global vulkan functions from dynamic library

#define GLOBAL_LEVEL_VULKAN_FUNCTION(name)                                     \
    name = (PFN_##name)vkGetInstanceProcAddr(NULL, #name);                     \
    assert(name && "Error loading global vulkan function: " #name);
#include "Vulkan/ListOfVulkanFunctions.inl"
    }

    VkApplicationInfo app_info = { 0 };
    app_info.sType             = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName  = APP_NAME;
    app_info.pEngineName       = "GdbDebugEngine";

    // Load validation layer
    const char* validation_layer[] = { "VK_LAYER_KHRONOS_validation",
                                       "VK_LAYER_LUNARG_standard_validation" };
    uint32_t    validation_idx     = 0;
    {
        uint32_t available_layers;

        success = vkEnumerateInstanceLayerProperties(&available_layers, NULL);
        assert(success == VK_SUCCESS && "vkEnumerateInstanceLayerProperties");

        assert(available_layers > 0 && "Failed to find any vulkan m_Layers");
        VkLayerProperties layer_props[50];
        assert(50 >= available_layers);

        success =
          vkEnumerateInstanceLayerProperties(&available_layers, layer_props);
        assert(success == VK_SUCCESS && "vkEnumerateInstanceLayerProperties");

        bool layer_found = false;

        uint32_t validation_count = STATIC_ARRAY_COUNT(validation_layer);
        for (uint32_t i = 0; i < available_layers && !layer_found; i++) {
            for (uint32_t j = 0; j < validation_count; j++) {
                if (STR_EQ(layer_props[i].layerName, validation_layer[j])) {
                    validation_idx = j;
                    layer_found    = true;
                }
            }
        }

        assert(layer_found && "Failed to init validation layer");
    }

    const char* enabled_exts[] = { VK_KHR_SURFACE_EXTENSION_NAME,
                                   VG_WINDOW_SURFACE,
                                   VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
                                   VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME };

    uint32_t enabled_ext_count = STATIC_ARRAY_COUNT(enabled_exts);
    {
        VkExtensionProperties props[50];
        uint32_t              num_props = 0;

        success =
          vkEnumerateInstanceExtensionProperties(NULL, &num_props, NULL);
        assert(success == VK_SUCCESS &&
               "vkEnumerateInstanceExtensionProperties");

        if (num_props > STATIC_ARRAY_COUNT(props)) {
            num_props = STATIC_ARRAY_COUNT(props);
        }

        success =
          vkEnumerateInstanceExtensionProperties(NULL, &num_props, props);
        assert(success == VK_SUCCESS &&
               "vkEnumerateInstanceExtensionProperties");

        // refine "enabled_exts" list
        uint32_t curr_idx = 0;
        while (curr_idx < enabled_ext_count) {
            bool found = false;
            for (uint32_t j = 0; j < num_props && !found; j++) {
                found = STR_EQ(props[j].extensionName, enabled_exts[curr_idx]);
            }

            if (found) {
                curr_idx++;
            } else {
                enabled_exts[curr_idx] = enabled_exts[enabled_ext_count - 1];
                enabled_ext_count--;
            }
        }
    }

    VkInstanceCreateInfo inst_info    = {};
    inst_info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    inst_info.pApplicationInfo        = &app_info;
    inst_info.enabledLayerCount       = 1;
    inst_info.ppEnabledLayerNames     = &validation_layer[validation_idx];
    inst_info.enabledExtensionCount   = enabled_ext_count;
    inst_info.ppEnabledExtensionNames = enabled_exts;

    success = vkCreateInstance(&inst_info, NULL, &s_vkstate.m_Inst);
    assert(success == VK_SUCCESS && "vkCreateInstance");

    // load instance vulkan functions
    {
#define INSTANCE_LEVEL_VULKAN_FUNCTION(name)                                   \
    name = (PFN_##name)vkGetInstanceProcAddr(s_vkstate.m_Inst, #name);         \
    assert(name&& #name);

#define INSTANCE_LEVEL_VULKAN_FUNCTION_FROM_EXTENSION(name, extension)         \
    for (uint32_t i = 0; i < enabled_ext_count; i++) {                         \
        if (STR_EQ(enabled_exts[i], extension)) {                              \
            name = (PFN_##name)vkGetInstanceProcAddr(s_vkstate.m_Inst, #name); \
            assert(name&& #name);                                              \
        }                                                                      \
    }
#include "Vulkan/ListOfVulkanFunctions.inl"
    }

    VkDebugUtilsMessengerCreateInfoEXT callback_info = {
        VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        NULL,
        0,
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
          VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
          VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
          VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        // VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
          VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        VGDebugCB,
        NULL
    };

    success = vkCreateDebugUtilsMessengerEXT(
      s_vkstate.m_Inst, &callback_info, NULL, &s_vkstate.m_DebugMsgr);
    assert(success == VK_SUCCESS && "vkCreateDebugUtilsMessengerEXT");

    VkXcbSurfaceCreateInfoKHR window_create_info = {};
    window_create_info.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    window_create_info.connection = win->m_Connection;
    window_create_info.window     = win->m_Window;

    success = vkCreateXcbSurfaceKHR(
      s_vkstate.m_Inst, &window_create_info, NULL, &s_vkstate.m_Surface);
    assert(success == VK_SUCCESS && "vkCreateXcbSurfaceKHR");

    return &s_vkstate;
}

//------------------------------------------------------------------------------

#define EXPORTED_VULKAN_FUNCTION(name) PFN_##name name;
#define GLOBAL_LEVEL_VULKAN_FUNCTION(name) PFN_##name name;
#define INSTANCE_LEVEL_VULKAN_FUNCTION(name) PFN_##name name;
#define INSTANCE_LEVEL_VULKAN_FUNCTION_FROM_EXTENSION(name, extension)         \
    PFN_##name name;
#define DEVICE_LEVEL_VULKAN_FUNCTION(name) PFN_##name name;
#define DEVICE_LEVEL_VULKAN_FUNCTION_FROM_EXTENSION(name, extension)           \
    PFN_##name name;

#include "Vulkan/ListOfVulkanFunctions.inl"
