#include "engine/renderer/renderer.hpp"
#include "iostream/logger.hpp"

#include "engine/vk/context.h"
#include "engine/vk/swapchain/swapchain.h"
#include "engine/vk/swapchain/swapchain_recreate.h"

const std::vector<std::string> &Renderer::device_info_lines() const
{
    return _device_info_lines;
}

bool Renderer::is_ready() const
{
    return g_device != VK_NULL_HANDLE && g_swap != VK_NULL_HANDLE && g_cmdbuf != nullptr && g_fences != nullptr;
}

void Renderer::try_recover()
{
    if (g_device == VK_NULL_HANDLE || g_swap != VK_NULL_HANDLE) return;

    ftpp::log::warning("Renderer::try_recover: swapchain missing, retrying recreate");
    if (!vk_recreate_swapchain()) {
        ftpp::log::error("Renderer::try_recover: recreate failed, will retry next tick");
    }
}

void Renderer::toggle_vsync()
{
    if (!is_ready()) return;

    g_vsync_enabled = !g_vsync_enabled;
    if (!vk_recreate_swapchain()) {
        g_vsync_enabled = !g_vsync_enabled;
        ftpp::log::error("Renderer::toggle_vsync: swapchain recreate failed, V-Sync unchanged (", g_vsync_enabled ? "on" : "off", ")");
        return;
    }
    ftpp::log::info("Renderer::toggle_vsync: V-Sync now ", g_vsync_enabled ? "on" : "off");
}
