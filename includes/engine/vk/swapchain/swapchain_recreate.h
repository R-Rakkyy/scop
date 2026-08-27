#ifndef SCOP_VK_SWAPCHAIN_RECREATE_H
#define SCOP_VK_SWAPCHAIN_RECREATE_H

#ifdef __cplusplus
extern "C" {
#endif

/*

 * Swapchain (recreation) management, side note: (known issue)
 * 
 * On some machine, i have a bug where if i change the window size and place,
 * it then freezes the render when i turn off V-Sync until we input a movement/action.
 * This is due to either some garbage code i did around the swapchain recreation and resize detection
 * or simply because my window manager doesn't like what i am trying to do. (Wayland is very bad for this).
 * 
 * Since i can't find an easy way to fix this, since this issue doesn't affect the overall functionality 
 * and that the V-Sync isn't mandatory at all, i am letting this bug be for now.

*/

int     vk_recreate_swapchain(void);

#ifdef __cplusplus
}
#endif

#endif /* SCOP_VK_SWAPCHAIN_RECREATE_H */
