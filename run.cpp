#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream> 
using std::cout; using std::endl;
#include <vector>
using std::vector;

#include "vulkan_application.h"
#include "debug_print.h"

#include <algorithm>
#include <unistd.h>

#include <string>
using std::string;
#include <string.h>

/*************/
/* FUNCTIONS */
/*************/

void vk::run(void)
{
    main_loop();
}

void vk::main_loop(void)
{ 
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents(); 
        draw_frame(); 
        usleep(10000);
    } 
    cleanup();
}

void vk::glfw_init(void)
{ 
    /* Initialize the GLFW library */
    glfwInit();
    /* Do not use a OpenGL context, disable resizing */
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); 
    /* Create the window */
    window = glfwCreateWindow(
            WIDTH, HEIGHT, "Vulkan", nullptr, nullptr); 
} 

void vk::init(void)
{ 
    load_available_instance_extensions();
    //print_available_instance_extensions(); 
    load_required_instance_extensions();
    //print_required_instance_extensions(); 
    create_instance(); 
    create_surface();
    load_devices();
    load_device_extensions();
    //print_device_extensions(); 
    load_features();
    load_memory_properties();
    load_queue_family_properties();
    //print_queue_family_properties(); 
    create_logical_device();
    load_layer_properties();
    //print_layer_properties(); 
    //print_device_info(chosenDevice);
    load_queues(); 
    load_swapchain_support_details();
    //print_swapchain_support_details(); 
    create_swapchains();
    load_swapchain_image_handles();
    create_swapchain_image_views();
    create_renderpass();
    create_framebuffers();
    create_graphics_pipeline_layout();
    create_graphics_pipeline();
    create_command_pool();
    allocate_command_buffers();
    record_command_buffers();
    create_semaphores();
}

void vk::cleanup(void)
{
    vkDeviceWaitIdle(device); 

    /* Destroy semaphores */
    vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
    vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);

    /* Free command buffers */
    vkFreeCommandBuffers(
            device,
            commandPool,
            (uint32_t)commandBuffers.size(),
            commandBuffers.data());
    /* Destroy graphics pipeline */
    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    /* Destroy framebuffers */
    for (auto &i : swapchainFramebuffers) {
        vkDestroyFramebuffer(device, i, nullptr);
    }
    /* Destroy renderpass */
    vkDestroyRenderPass(device, renderPass, nullptr);
    /* Destroy swapchain imageviews */
    for (auto &i : swapchainImageViews) {
        vkDestroyImageView(device, i, nullptr);
    }
    /* Destroy swapchains */
    vkDestroySwapchainKHR(device, swapchain, nullptr);
    /* Destroy command pool */
    vkDestroyCommandPool(device, commandPool, nullptr);
    /* Destroy surface */
    vkDestroySurfaceKHR(instance, surface, nullptr); 
    /* Destroy logical device */ 
    vkDestroyDevice(device, nullptr); 
    /* Destroy instance */
    vkDestroyInstance(instance, nullptr); 
    /* Terminate window */
    glfwDestroyWindow(window);
    glfwTerminate(); 
}
