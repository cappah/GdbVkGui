#pragma once

#define VK_USE_PLATFORM_XCB_KHR
#define VG_WINDOW_SURFACE VK_KHR_XCB_SURFACE_EXTENSION_NAME

#include "UtilityMacros.h"
#include "WindowInterface.h"
#include <vulkan/vulkan.h>

typedef struct VkStateBin
{
    void*                    m_VkLib;
    VkInstance               m_Inst;
    VkSurfaceKHR             m_Surface;
    VkDebugUtilsMessengerEXT m_DebugMsgr;
} VkStateBin;

//------------------------------------------------------------------------------

VKAPI_ATTR VkBool32 VKAPI_CALL
VGDebugCB(VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
          VkDebugUtilsMessageTypeFlagsEXT             type,
          const VkDebugUtilsMessengerCallbackDataEXT* error_data,
          void*                                       user_data);

VkStateBin*
LoadVulkanState(AppWindowData* win);

//------------------------------------------------------------------------------

#define EXPORTED_VULKAN_FUNCTION(name) extern PFN_##name name;
#define GLOBAL_LEVEL_VULKAN_FUNCTION(name) extern PFN_##name name;
#define INSTANCE_LEVEL_VULKAN_FUNCTION(name) extern PFN_##name name;
#define INSTANCE_LEVEL_VULKAN_FUNCTION_FROM_EXTENSION(name, extension)         \
    extern PFN_##name name;
#define DEVICE_LEVEL_VULKAN_FUNCTION(name) extern PFN_##name name;
#define DEVICE_LEVEL_VULKAN_FUNCTION_FROM_EXTENSION(name, extension)           \
    extern PFN_##name name;
#include "Vulkan/ListOfVulkanFunctions.inl"
