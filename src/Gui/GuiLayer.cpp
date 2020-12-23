#include "Gui/GuiLayer.h"
#include "ProcessIO.h"
#include "UtilityMacros.h"
#include "Vulkan/VulkanLayer.h"
#include "WindowInterface.h"
#include "imgui.h"
#include "imgui_impl_vulkan.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C"
{
#endif

    static uint32_t s_keycodes[] = {
#define KEY_TYPES(a, b, c, d) c,
#include "KeyTypes.inl"
    };

#ifdef _DEBUG
#define IMGUI_VULKAN_DEBUG_REPORT
#endif

    static VkAllocationCallbacks* g_Allocator      = NULL;
    static VkInstance             g_Instance       = VK_NULL_HANDLE;
    static VkPhysicalDevice       g_PhysicalDevice = VK_NULL_HANDLE;
    static VkDevice               g_Device         = VK_NULL_HANDLE;
    static uint32_t               g_QueueFamily    = (uint32_t)-1;
    static VkQueue                g_Queue          = VK_NULL_HANDLE;
    static VkPipelineCache        g_PipelineCache  = VK_NULL_HANDLE;
    static VkDescriptorPool       g_DescriptorPool = VK_NULL_HANDLE;

    static ImGui_ImplVulkanH_Window g_MainWindowData;
    static int                      g_MinImageCount    = 2;
    static bool                     g_SwapChainRebuild = false;

    static void check_vk_result(VkResult err)
    {
        if (err == 0)
            return;
        fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
        if (err < 0)
            abort();
    }

    static void SetupVulkan()
    {
        VkResult err;

        // Select GPU
        {
            uint32_t gpu_count;
            err = vkEnumeratePhysicalDevices(g_Instance, &gpu_count, NULL);
            check_vk_result(err);
            IM_ASSERT(gpu_count > 0);

            VkPhysicalDevice* gpus =
              (VkPhysicalDevice*)WmMalloc(sizeof(VkPhysicalDevice) * gpu_count);
            err = vkEnumeratePhysicalDevices(g_Instance, &gpu_count, gpus);
            check_vk_result(err);

            // If a number >1 of GPUs got reported, you should find the best fit
            // GPU for your purpose e.g. VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU if
            // available, or with the greatest memory available, etc. for sake
            // of simplicity we'll just take the first one, assuming it has a
            // graphics queue family.
            g_PhysicalDevice = gpus[0];
            WmFree(gpus);
        }

        // Select graphics queue family
        {
            uint32_t count;
            vkGetPhysicalDeviceQueueFamilyProperties(
              g_PhysicalDevice, &count, NULL);
            VkQueueFamilyProperties* queues =
              (VkQueueFamilyProperties*)WmMalloc(
                sizeof(VkQueueFamilyProperties) * count);
            vkGetPhysicalDeviceQueueFamilyProperties(
              g_PhysicalDevice, &count, queues);
            for (uint32_t i = 0; i < count; i++)
                if (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                    g_QueueFamily = i;
                    break;
                }
            WmFree(queues);
            IM_ASSERT(g_QueueFamily != (uint32_t)-1);
        }

        // Create Logical Device (with 1 queue)
        {
            int         device_extension_count    = 1;
            const char* device_extensions[]       = { "VK_KHR_swapchain" };
            const float queue_priority[]          = { 1.0f };
            VkDeviceQueueCreateInfo queue_info[1] = {};
            queue_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_info[0].queueFamilyIndex = g_QueueFamily;
            queue_info[0].queueCount       = 1;
            queue_info[0].pQueuePriorities = queue_priority;
            VkDeviceCreateInfo create_info = {};
            create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            create_info.queueCreateInfoCount =
              sizeof(queue_info) / sizeof(queue_info[0]);
            create_info.pQueueCreateInfos       = queue_info;
            create_info.enabledExtensionCount   = device_extension_count;
            create_info.ppEnabledExtensionNames = device_extensions;
            err                                 = vkCreateDevice(
              g_PhysicalDevice, &create_info, g_Allocator, &g_Device);
            check_vk_result(err);

            // Load vulkan level functions using logical device
            {
#define DEVICE_LEVEL_VULKAN_FUNCTION(name)                                     \
    name = (PFN_##name)vkGetDeviceProcAddr(g_Device, #name);                   \
    assert(name&& #name);

#define DEVICE_LEVEL_VULKAN_FUNCTION_FROM_EXTENSION(name, extension)           \
    for (int32_t i = 0; i < device_extension_count; i++) {                     \
        if (STR_EQ(device_extensions[i], extension)) {                         \
            name = (PFN_##name)vkGetDeviceProcAddr(g_Device, #name);           \
            assert(name&& #name);                                              \
        }                                                                      \
    }
#include "Vulkan/ListOfVulkanFunctions.inl"
            }

            vkGetDeviceQueue(g_Device, g_QueueFamily, 0, &g_Queue);
        }

        // Create Descriptor Pool
        {
            VkDescriptorPoolSize pool_sizes[] = {
                { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
                { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
                { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
                { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
                { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
                { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
                { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
                { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
                { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
            };
            VkDescriptorPoolCreateInfo pool_info = {};
            pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
            pool_info.maxSets       = 1000 * IM_ARRAYSIZE(pool_sizes);
            pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
            pool_info.pPoolSizes    = pool_sizes;
            err                     = vkCreateDescriptorPool(
              g_Device, &pool_info, g_Allocator, &g_DescriptorPool);
            check_vk_result(err);
        }
    }

    // All the ImGui_ImplVulkanH_XXX structures/functions are optional helpers
    // used by the demo. Your real engine/app may not use them.
    static void SetupVulkanWindow(ImGui_ImplVulkanH_Window* wd,
                                  VkSurfaceKHR              surface,
                                  int                       width,
                                  int                       height)
    {
        wd->Surface = surface;

        // Check for WSI support
        VkBool32 res;
        vkGetPhysicalDeviceSurfaceSupportKHR(
          g_PhysicalDevice, g_QueueFamily, wd->Surface, &res);
        if (res != VK_TRUE) {
            fprintf(stderr, "Error no WSI support on physical device 0\n");
            exit(-1);
        }

        // Select Surface Format
        const VkFormat requestSurfaceImageFormat[] = { VK_FORMAT_B8G8R8A8_UNORM,
                                                       VK_FORMAT_R8G8B8A8_UNORM,
                                                       VK_FORMAT_B8G8R8_UNORM,
                                                       VK_FORMAT_R8G8B8_UNORM };
        const VkColorSpaceKHR requestSurfaceColorSpace =
          VK_COLORSPACE_SRGB_NONLINEAR_KHR;
        wd->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(
          g_PhysicalDevice,
          wd->Surface,
          requestSurfaceImageFormat,
          (size_t)IM_ARRAYSIZE(requestSurfaceImageFormat),
          requestSurfaceColorSpace);

        // Select Present Mode
#ifdef IMGUI_UNLIMITED_FRAME_RATE
        VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_MAILBOX_KHR,
                                             VK_PRESENT_MODE_IMMEDIATE_KHR,
                                             VK_PRESENT_MODE_FIFO_KHR };
#else
    VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_FIFO_KHR };
#endif
        wd->PresentMode =
          ImGui_ImplVulkanH_SelectPresentMode(g_PhysicalDevice,
                                              wd->Surface,
                                              &present_modes[0],
                                              IM_ARRAYSIZE(present_modes));
        // printf("[vulkan] Selected PresentMode = %d\n", wd->PresentMode);

        // Create SwapChain, RenderPass, Framebuffer, etc.
        IM_ASSERT(g_MinImageCount >= 2);
        ImGui_ImplVulkanH_CreateOrResizeWindow(g_Instance,
                                               g_PhysicalDevice,
                                               g_Device,
                                               wd,
                                               g_QueueFamily,
                                               g_Allocator,
                                               width,
                                               height,
                                               g_MinImageCount);
    }

    static void CleanupVulkan()
    {
        vkDestroyDescriptorPool(g_Device, g_DescriptorPool, g_Allocator);

        vkDestroyDevice(g_Device, g_Allocator);
        vkDestroyInstance(g_Instance, g_Allocator);
    }

    static void CleanupVulkanWindow()
    {
        ImGui_ImplVulkanH_DestroyWindow(
          g_Instance, g_Device, &g_MainWindowData, g_Allocator);
    }

    static void FrameRender(ImGui_ImplVulkanH_Window* wd, ImDrawData* draw_data)
    {
        VkResult err;

        VkSemaphore image_acquired_semaphore =
          wd->FrameSemaphores[wd->SemaphoreIndex].ImageAcquiredSemaphore;
        VkSemaphore render_complete_semaphore =
          wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
        err = vkAcquireNextImageKHR(g_Device,
                                    wd->Swapchain,
                                    UINT64_MAX,
                                    image_acquired_semaphore,
                                    VK_NULL_HANDLE,
                                    &wd->FrameIndex);
        if (err == VK_ERROR_OUT_OF_DATE_KHR) {
            g_SwapChainRebuild = true;
            return;
        }
        check_vk_result(err);

        ImGui_ImplVulkanH_Frame* fd = &wd->Frames[wd->FrameIndex];
        {
            err = vkWaitForFences(
              g_Device,
              1,
              &fd->Fence,
              VK_TRUE,
              UINT64_MAX); // wait indefinitely instead of periodically checking
            check_vk_result(err);

            err = vkResetFences(g_Device, 1, &fd->Fence);
            check_vk_result(err);
        }
        {
            err = vkResetCommandPool(g_Device, fd->CommandPool, 0);
            check_vk_result(err);
            VkCommandBufferBeginInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            err = vkBeginCommandBuffer(fd->CommandBuffer, &info);
            check_vk_result(err);
        }
        {
            VkRenderPassBeginInfo info = {};
            info.sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            info.renderPass  = wd->RenderPass;
            info.framebuffer = fd->Framebuffer;
            info.renderArea.extent.width  = wd->Width;
            info.renderArea.extent.height = wd->Height;
            info.clearValueCount          = 1;
            info.pClearValues             = &wd->ClearValue;
            vkCmdBeginRenderPass(
              fd->CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
        }

        // Record dear imgui primitives into command buffer
        ImGui_ImplVulkan_RenderDrawData(draw_data, fd->CommandBuffer);

        // Submit command buffer
        vkCmdEndRenderPass(fd->CommandBuffer);
        {
            VkPipelineStageFlags wait_stage =
              VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            VkSubmitInfo info         = {};
            info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            info.waitSemaphoreCount   = 1;
            info.pWaitSemaphores      = &image_acquired_semaphore;
            info.pWaitDstStageMask    = &wait_stage;
            info.commandBufferCount   = 1;
            info.pCommandBuffers      = &fd->CommandBuffer;
            info.signalSemaphoreCount = 1;
            info.pSignalSemaphores    = &render_complete_semaphore;

            err = vkEndCommandBuffer(fd->CommandBuffer);
            check_vk_result(err);
            err = vkQueueSubmit(g_Queue, 1, &info, fd->Fence);
            check_vk_result(err);
        }
    }

    static void FramePresent(ImGui_ImplVulkanH_Window* wd)
    {
        if (g_SwapChainRebuild)
            return;
        VkSemaphore render_complete_semaphore =
          wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
        VkPresentInfoKHR info   = {};
        info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        info.waitSemaphoreCount = 1;
        info.pWaitSemaphores    = &render_complete_semaphore;
        info.swapchainCount     = 1;
        info.pSwapchains        = &wd->Swapchain;
        info.pImageIndices      = &wd->FrameIndex;
        VkResult err            = vkQueuePresentKHR(g_Queue, &info);
        if (err == VK_ERROR_OUT_OF_DATE_KHR) {
            g_SwapChainRebuild = true;
            return;
        }
        check_vk_result(err);
        wd->SemaphoreIndex =
          (wd->SemaphoreIndex + 1) %
          wd->ImageCount; // Now we can use the next set of semaphores
    }

    void SetupGuiContext(VkStateBin* vkstate, AppWindowData* win)
    {
        g_Instance = vkstate->m_Inst;
        SetupVulkan();

        ImGui_ImplVulkanH_Window* wd = &g_MainWindowData;
        SetupVulkanWindow(
          wd, vkstate->m_Surface, win->m_ClientW, win->m_ClientH);
        ImGui::CreateContext();

        // Setup back-end capabilities flags
        ImGuiIO& io = ImGui::GetIO();
        io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
        io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
        io.BackendPlatformName = "imgui_vulkan_app";
        io.IniFilename         = "bin/imgui.ini";

        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        io.KeyMap[ImGuiKey_Tab]        = g_KeyIds.WK_KEY_TAB;
        io.KeyMap[ImGuiKey_LeftArrow]  = g_KeyIds.WK_KEY_CURSOR_LEFT;
        io.KeyMap[ImGuiKey_RightArrow] = g_KeyIds.WK_KEY_CURSOR_RIGHT;
        io.KeyMap[ImGuiKey_UpArrow]    = g_KeyIds.WK_KEY_CURSOR_UP;
        io.KeyMap[ImGuiKey_DownArrow]  = g_KeyIds.WK_KEY_CURSOR_DOWN;
        io.KeyMap[ImGuiKey_PageUp]     = g_KeyIds.WK_KEY_PAGE_UP;
        io.KeyMap[ImGuiKey_PageDown]   = g_KeyIds.WK_KEY_PAGE_DOWN;
        io.KeyMap[ImGuiKey_Home]       = g_KeyIds.WK_KEY_HOME;
        io.KeyMap[ImGuiKey_End]        = g_KeyIds.WK_KEY_END;
        // io.KeyMap[ImGuiKey_Insert]  = g_KeyIds.WK_KEY_INSERT;
        io.KeyMap[ImGuiKey_Delete]      = g_KeyIds.WK_KEY_DELETE;
        io.KeyMap[ImGuiKey_Backspace]   = g_KeyIds.WK_KEY_BACKSPACE;
        io.KeyMap[ImGuiKey_Space]       = g_KeyIds.WK_KEY_SPACE;
        io.KeyMap[ImGuiKey_Enter]       = g_KeyIds.WK_KEY_RETURN;
        io.KeyMap[ImGuiKey_Escape]      = g_KeyIds.WK_KEY_ESCAPE;
        io.KeyMap[ImGuiKey_KeyPadEnter] = g_KeyIds.WK_KEY_RETURN;
        io.KeyMap[ImGuiKey_A]           = g_KeyIds.WK_KEY_A;
        io.KeyMap[ImGuiKey_C]           = g_KeyIds.WK_KEY_C;
        io.KeyMap[ImGuiKey_V]           = g_KeyIds.WK_KEY_V;
        io.KeyMap[ImGuiKey_X]           = g_KeyIds.WK_KEY_X;
        io.KeyMap[ImGuiKey_Y]           = g_KeyIds.WK_KEY_Y;
        io.KeyMap[ImGuiKey_Z]           = g_KeyIds.WK_KEY_Z;

        // io.GetClipboardTextFn = GetClipBoard;

        // Setup Platform/Renderer backends
        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance                  = g_Instance;
        init_info.PhysicalDevice            = g_PhysicalDevice;
        init_info.Device                    = g_Device;
        init_info.QueueFamily               = g_QueueFamily;
        init_info.Queue                     = g_Queue;
        init_info.PipelineCache             = g_PipelineCache;
        init_info.DescriptorPool            = g_DescriptorPool;
        init_info.Allocator                 = g_Allocator;
        init_info.MinImageCount             = g_MinImageCount;
        init_info.ImageCount                = wd->ImageCount;
        init_info.CheckVkResultFn           = check_vk_result;
        ImGui_ImplVulkan_Init(&init_info, wd->RenderPass);

        VkResult err;
        // Upload Fonts
        {
            // Use any command queue
            VkCommandPool command_pool = wd->Frames[wd->FrameIndex].CommandPool;
            VkCommandBuffer command_buffer =
              wd->Frames[wd->FrameIndex].CommandBuffer;

            err = vkResetCommandPool(g_Device, command_pool, 0);
            check_vk_result(err);
            VkCommandBufferBeginInfo begin_info = {};
            begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            err = vkBeginCommandBuffer(command_buffer, &begin_info);
            check_vk_result(err);

            ImGui_ImplVulkan_CreateFontsTexture(command_buffer);

            VkSubmitInfo end_info       = {};
            end_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            end_info.commandBufferCount = 1;
            end_info.pCommandBuffers    = &command_buffer;
            err                         = vkEndCommandBuffer(command_buffer);
            check_vk_result(err);
            err = vkQueueSubmit(g_Queue, 1, &end_info, VK_NULL_HANDLE);
            check_vk_result(err);

            err = vkDeviceWaitIdle(g_Device);
            check_vk_result(err);
            ImGui_ImplVulkan_DestroyFontUploadObjects();
        }

        // Change style: corporate gray
        ImGuiStyle& style  = ImGui::GetStyle();
        ImVec4*     colors = style.Colors;

        /// 0 = FLAT APPEARENCE
        /// 1 = MORE "3D" LOOK
        int is3D = 0;

        colors[ImGuiCol_Text]             = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_TextDisabled]     = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
        colors[ImGuiCol_ChildBg]          = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
        colors[ImGuiCol_WindowBg]         = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
        colors[ImGuiCol_PopupBg]          = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
        colors[ImGuiCol_Border]           = ImVec4(0.12f, 0.12f, 0.12f, 0.71f);
        colors[ImGuiCol_BorderShadow]     = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
        colors[ImGuiCol_FrameBg]          = ImVec4(0.42f, 0.42f, 0.42f, 0.54f);
        colors[ImGuiCol_FrameBgHovered]   = ImVec4(0.42f, 0.42f, 0.42f, 0.40f);
        colors[ImGuiCol_FrameBgActive]    = ImVec4(0.56f, 0.56f, 0.56f, 0.67f);
        colors[ImGuiCol_TitleBg]          = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
        colors[ImGuiCol_TitleBgActive]    = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.17f, 0.17f, 0.17f, 0.90f);
        colors[ImGuiCol_MenuBarBg]     = ImVec4(0.335f, 0.335f, 0.335f, 1.000f);
        colors[ImGuiCol_ScrollbarBg]   = ImVec4(0.24f, 0.24f, 0.24f, 0.53f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] =
          ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive] =
          ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
        colors[ImGuiCol_CheckMark]        = ImVec4(0.65f, 0.65f, 0.65f, 1.00f);
        colors[ImGuiCol_SliderGrab]       = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.64f, 0.64f, 0.64f, 1.00f);
        colors[ImGuiCol_Button]           = ImVec4(0.54f, 0.54f, 0.54f, 0.35f);
        colors[ImGuiCol_ButtonHovered]    = ImVec4(0.52f, 0.52f, 0.52f, 0.59f);
        colors[ImGuiCol_ButtonActive]     = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
        colors[ImGuiCol_Header]           = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
        colors[ImGuiCol_HeaderHovered]    = ImVec4(0.47f, 0.47f, 0.47f, 1.00f);
        colors[ImGuiCol_HeaderActive]     = ImVec4(0.76f, 0.76f, 0.76f, 0.77f);
        colors[ImGuiCol_Separator] = ImVec4(0.702f, 0.671f, 0.600f, 0.137f);
        colors[ImGuiCol_SeparatorHovered] =
          ImVec4(0.700f, 0.671f, 0.600f, 0.290f);
        colors[ImGuiCol_SeparatorActive] =
          ImVec4(0.702f, 0.671f, 0.600f, 0.674f);
        colors[ImGuiCol_ResizeGrip]        = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
        colors[ImGuiCol_ResizeGripActive]  = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
        colors[ImGuiCol_PlotLines]         = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered]  = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
        colors[ImGuiCol_PlotHistogram]     = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered] =
          ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_TextSelectedBg]   = ImVec4(0.73f, 0.73f, 0.73f, 0.35f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
        colors[ImGuiCol_DragDropTarget]   = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
        colors[ImGuiCol_NavHighlight]     = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight] =
          ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);

        style.PopupRounding = 3;

        style.WindowPadding = ImVec2(4, 4);
        style.FramePadding  = ImVec2(6, 4);
        style.ItemSpacing   = ImVec2(6, 2);

        style.ScrollbarSize = 18;

        style.WindowBorderSize = 1;
        style.ChildBorderSize  = 1;
        style.PopupBorderSize  = 1;
        style.FrameBorderSize  = is3D;

        style.WindowRounding    = 3;
        style.ChildRounding     = 3;
        style.FrameRounding     = 3;
        style.ScrollbarRounding = 2;
        style.GrabRounding      = 3;

        style.TabBorderSize = is3D;
        style.TabRounding   = 3;

        // colors[ImGuiCol_DockingEmptyBg]     = ImVec4(0.38f, 0.38f,
        // 0.38f, 1.00f);
        colors[ImGuiCol_Tab]          = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
        colors[ImGuiCol_TabHovered]   = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
        colors[ImGuiCol_TabActive]    = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive] =
          ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
        // colors[ImGuiCol_DockingPreview]     = ImVec4(0.85f, 0.85f, 0.85f,
        // 0.28f);
    }

    void ProcessGuiFrame(AppWindowData* win, FrontEndCB f_cb)
    {
        ImGui_ImplVulkanH_Window* wd = &g_MainWindowData;
        ImVec4 clear_color           = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

        // Resize swap chain?
        int width = win->m_ClientW, height = win->m_ClientH;
        if (g_SwapChainRebuild) {
            if (width > 0 && height > 0) {
                ImGui_ImplVulkan_SetMinImageCount(g_MinImageCount);
                ImGui_ImplVulkanH_CreateOrResizeWindow(g_Instance,
                                                       g_PhysicalDevice,
                                                       g_Device,
                                                       &g_MainWindowData,
                                                       g_QueueFamily,
                                                       g_Allocator,
                                                       width,
                                                       height,
                                                       g_MinImageCount);
                g_MainWindowData.FrameIndex = 0;
                g_SwapChainRebuild          = false;
            }
        }

        // Start the Dear ImGui frame
        ImGui_ImplVulkan_NewFrame();

        ImGuiIO&          io       = ImGui::GetIO();
        const KeySymData* key_data = AppRetrieveKeyData();

        io.MouseDown[0] = key_data[g_KeyIds.WK_MOUSE_LEFT].triggered;
        io.MouseDown[1] = key_data[g_KeyIds.WK_MOUSE_RIGHT].triggered;
        io.MouseDown[2] = key_data[g_KeyIds.WK_MOUSE_MIDDLE].triggered;
        if (key_data[g_KeyIds.WK_MOUSE_POS].triggered) {
            io.MousePos = ImVec2((float)key_data[g_KeyIds.WK_MOUSE_POS].x,
                                 (float)key_data[g_KeyIds.WK_MOUSE_POS].y);
        }
        io.MouseWheel  = 0;
        io.MouseWheelH = 0;
        if (key_data[g_KeyIds.WK_MOUSE_SCROLL_UP].triggered) {
            io.MouseWheel++;
        }
        if (key_data[g_KeyIds.WK_MOUSE_SCROLL_DOWN].triggered) {
            io.MouseWheel--;
        }
        if (key_data[g_KeyIds.WK_MOUSE_SCROLL_RIGHT].triggered) {
            io.MouseWheelH++;
        }
        if (key_data[g_KeyIds.WK_MOUSE_SCROLL_LEFT].triggered) {
            io.MouseWheelH--;
        }

        uint32_t*      key_code_ptr = s_keycodes;
        const uint32_t max_keys     = STATIC_ARRAY_COUNT(s_keycodes);
        for (uint32_t ikeycode = 0; ikeycode < max_keys;
             ikeycode++, key_code_ptr++) {
            io.KeysDown[*key_code_ptr] = key_data[*key_code_ptr].triggered;
        }

        io.KeyCtrl = io.KeysDown[g_KeyIds.WK_KEY_CTRL_LEFT] ||
                     io.KeysDown[g_KeyIds.WK_KEY_CTRL_RIGHT];
        io.KeyShift = io.KeysDown[g_KeyIds.WK_KEY_SHIFT_LEFT] ||
                      io.KeysDown[g_KeyIds.WK_KEY_SHIFT_RIGHT];
        io.KeyAlt = io.KeysDown[g_KeyIds.WK_KEY_ALT_LEFT] ||
                    io.KeysDown[g_KeyIds.WK_KEY_ALT_RIGHT];

        if (win->m_LastTypedC && !(io.KeyCtrl || io.KeyAlt)) {
            io.AddInputCharacter(win->m_LastTypedC);
        }

        io.DisplaySize             = ImVec2((float)width, (float)height);
        io.DisplayFramebufferScale = ImVec2(1, 1);

        io.DeltaTime = (float)1.f / 60.f;

        ImGui::NewFrame();

        if (f_cb) {
            f_cb();
        }

        if (AppMustExit() == false) {
            ImGui::Render();
            ImDrawData* draw_data    = ImGui::GetDrawData();
            const bool  is_minimized = (draw_data->DisplaySize.x <= 0.0f ||
                                       draw_data->DisplaySize.y <= 0.0f);
            if (!is_minimized) {
                memcpy(&wd->ClearValue.color.float32[0],
                       &clear_color,
                       4 * sizeof(float));
                FrameRender(wd, draw_data);
                FramePresent(wd);
            }
        }

        UNUSED_VAR(CleanupVulkan);
        UNUSED_VAR(CleanupVulkanWindow);
    }

    void ShutdownGui(AppWindowData* win, FrontEndCB f_cb)
    {
        UNUSED_VAR(win);

        if (f_cb) {
            f_cb();
        }
    }

#ifdef __cplusplus
}
#endif
