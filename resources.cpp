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

#include <string>
using std::string;
#include <string.h>

/*************/
/* FUNCTIONS */
/*************/

void vk::load_queues(void)
{
    vkGetDeviceQueue(
            device,
            chosenDevice.get_graphics_queue_index(),
            0,
            &graphicsQueue);
    vkGetDeviceQueue(
            device,
            chosenDevice.get_graphics_queue_index(),
            0,
            &presentQueue);
}

void vk::create_command_pool(void)
{
    /* Short lived command buffers + individual reset */
    uint32_t flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT
        | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; 

    const VkCommandPoolCreateInfo createInfo = {
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        nullptr,
        flags,
        (uint32_t)chosenDevice.get_graphics_queue_index()};

    VkResult result = vkCreateCommandPool(
            device,
            &createInfo,
            nullptr,
            &commandPool);
    print_result(result);
}

//void vk::get_command_buffers(void)
//{
//}

/* Fill out the swapchainSupportDetails structure 
 * The objects contained in the structure are
 * VkSurfaceCapabilitiesKHR
 * vector<VkSurfaceFormatKHR>
 * vector<VkPresentModeKHR>*/ 
void vk::load_swapchain_support_details(void)
{
    /* Determine support cabailities */
    uint32_t result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR( 
            chosenDevice.physicalDevice,
            surface,
            &swapchainSupportDetails.capabilities);

    /* Query supported surface formats */
    uint32_t formatCount;
    result *= vkGetPhysicalDeviceSurfaceFormatsKHR(
            chosenDevice.physicalDevice,
            surface,
            &formatCount,
            nullptr);
    swapchainSupportDetails.formats.resize(formatCount);
    result *= vkGetPhysicalDeviceSurfaceFormatsKHR(
            chosenDevice.physicalDevice,
            surface,
            &formatCount,
            swapchainSupportDetails.formats.data());
    /* Query supported presentation modes */
    uint32_t presentModeCount;
    result *= vkGetPhysicalDeviceSurfacePresentModesKHR(
            chosenDevice.physicalDevice,
            surface,
            &presentModeCount,
            nullptr);
    swapchainSupportDetails.presentModes.resize(presentModeCount);
    result *= vkGetPhysicalDeviceSurfacePresentModesKHR(
            chosenDevice.physicalDevice,
            surface,
            &presentModeCount,
            swapchainSupportDetails.presentModes.data()); 
    print_result(result);
} 

void vk::print_swapchain_support_details(void)
{
    cout << "===================================" << endl;
    cout << "Printing swapchain support details:" << endl;
    /* Surface capabilities */
    cout << "SURFACE CAPABILITIES" << endl; 
    VkSurfaceCapabilitiesKHR &c = swapchainSupportDetails.capabilities;
    cout << "minImageCount:\t" << c.minImageCount << endl;
    cout << "maxImageCount:\t" << c.maxImageCount << endl;
    cout << "currentExtent:\t" <<
        c.currentExtent.width << "x" << c.currentExtent.height << endl;
    cout << "minImageExtent:\t" <<
        c.minImageExtent.width << "x" << c.minImageExtent.height << endl;
    cout << "maxImageExtent:\t" <<
        c.maxImageExtent.width << "x" << c.maxImageExtent.height << endl;
    cout << "maxImageArrayLayers:\t" << c.maxImageArrayLayers << endl;
    cout << "supportedTransforms:\t" << c.supportedTransforms << endl;
    cout << "currentTransform:\t" << c.currentTransform << endl;
    cout << "supportedCompositeAlpha:\t" << c.supportedCompositeAlpha << endl;
    cout << "supportedUsageFlags:\t" << c.supportedUsageFlags << endl; 

    /* Formats */
    uint32_t i = 0;
    cout << "FORMATS" << endl;
    for (auto &d : swapchainSupportDetails.formats) {
        cout << "formats index: " << i << '\t';
        cout << "Format flag: " << d.format << '\t'; 
        cout << "Colorspace flag: " << d.colorSpace << endl; 
        i++;
    }

    /* Present modes */
    cout << "PRESENT MODES" << endl;
    i = 0;
    for (auto &e : swapchainSupportDetails.presentModes) {
        cout << "present mode index: " << i << '\t';
        switch (e) {
            case VK_PRESENT_MODE_IMMEDIATE_KHR:
                cout << "Immediate mode" << endl;
                break;
            case VK_PRESENT_MODE_MAILBOX_KHR:
                cout << "Mailbox mode" << endl;
                break;
            case VK_PRESENT_MODE_FIFO_KHR:
                cout << "FIFO mode" << endl;
                break;
            case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
                cout << "FIFO relaxed mode" << endl;
                break;
            default:; 
        } 
        i++;
    } 
}

/* Try to get VK_FORMAT_B8G8R8A8_UNORM */
VkSurfaceFormatKHR vk::get_suitable_swapchain_surface_format(void)
{
    for (const auto& format : swapchainSupportDetails.formats) {
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM) { 
            return format; 
        }
    } 
    //Return the first format whatever it might be
    return swapchainSupportDetails.formats[0];
}

/* Try to get mailbox mode, then immediate mode and lastly FIFO mode */
VkPresentModeKHR vk::get_suitable_swapchain_present_mode(void)
{
    VkPresentModeKHR currentBestMode = VK_PRESENT_MODE_FIFO_KHR; 
    for (const auto& mode : swapchainSupportDetails.presentModes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {     
            return mode;
        } else if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
            currentBestMode = mode;
        }
    }
    return currentBestMode; 
} 

VkExtent2D vk::get_swapchain_extent(void)
{ 
    //Short hand variable for capabilities
    VkSurfaceCapabilitiesKHR &c = swapchainSupportDetails.capabilities;

    /* If the width equals the maximum uint32_t then it means that we must
     * choose the resolution ourselves which best matches the window otherwise
     * we can just use the current values */ 
    if (c.currentExtent.width == (uint32_t)-1) {//Trick to get maximum uint32
        return c.currentExtent;
    } else {
        VkExtent2D actualExtent = {WIDTH, HEIGHT}; 
        actualExtent.width = std::max(c.minImageExtent.width,
                std::min(c.maxImageExtent.width,
                    actualExtent.width));
        actualExtent.height= std::max(c.minImageExtent.height,
                std::min(c.maxImageExtent.height,
                    actualExtent.height));
        return actualExtent; 
    } 
}

void vk::create_swapchains(void)
{
    VkSurfaceFormatKHR format = get_suitable_swapchain_surface_format();
    VkPresentModeKHR presentMode = get_suitable_swapchain_present_mode();
    VkExtent2D extent = get_swapchain_extent();

    //    /* Create info structure */
        VkSwapchainCreateInfoKHR ci {}; //createInfo
        ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        ci.pNext = nullptr;
        ci.flags = 0U;
        ci.surface = surface; 
        ci.minImageCount = 3U; //To allow tripple-buffering
        ci.imageFormat = format.format;
        ci.imageColorSpace = format.colorSpace;
        ci.imageExtent = extent;
        ci.imageArrayLayers = 1; //non-stereoscopic
        
        //Direct render target (without post-processing)
        ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        /* Here we distinguish if the presentqueue is different from the
         * graphics queue */
        if (chosenDevice.get_graphics_queue_index() ==
                chosenDevice.get_present_queue_index()) {
            ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            /* These commented fields are ignored in exclusive mode */
            //ci.queueFamilyIndexCount = 0U;
            //ci.pQueueFamilyIndices = nullptr;
        } else {
            ci.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            ci.queueFamilyIndexCount = 2U;
            uint32_t queueFamilyIndices[] = {
                (uint32_t)chosenDevice.get_graphics_queue_index(),
                (uint32_t)chosenDevice.get_present_queue_index()};
            ci.pQueueFamilyIndices = queueFamilyIndices; 
        }

        /* No transformation, of which can be e.g. 90 degree rotation */
        ci.preTransform = swapchainSupportDetails.capabilities.currentTransform;

        /* No blending with other windows */
        ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

        ci.presentMode = presentMode;
        
        /* Allow omitted rendering of parts of the image which is not visible */
        ci.clipped = VK_TRUE;

        /* No recycled swapchain. This can happen when resizing a window and the
         * swapchains needs to be reallocated with larger images */
        ci.oldSwapchain = VK_NULL_HANDLE; 

        /* CREATE THE SWAPCHAIN */
        VkResult result = vkCreateSwapchainKHR(
                device,                           
                &ci, //createInfo
                nullptr,                         
                &swapChain);                    
        print_result(result);

}
