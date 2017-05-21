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
#include <set>

#include "vulkan_application.h"
#include "debug_print.h"

#include <algorithm>

#include <string>
using std::string;
#include <string.h>


void vk::print_queue_family_properties(void)
{
    cout << "Printing queue capabilities..." << endl;
    VkQueueFlags flags; 

    for (auto &device : devices) {
        for (auto &queueFamilyProperty : device.queueFamilyProperties) { 
            flags = queueFamilyProperty.queueFlags;
            if (flags & VK_QUEUE_GRAPHICS_BIT) { 
                cout << "Graphics capable" << endl;
            } if (flags & VK_QUEUE_COMPUTE_BIT) {
                cout << "Compute capable" << endl;
            } if (flags & VK_QUEUE_TRANSFER_BIT) {
                cout << "Transfer capable" << endl; 
            } if (flags & VK_QUEUE_SPARSE_BINDING_BIT) {
                cout << "Sparse binding" << endl; 
            } 
        }
    } 
}

void vk::print_layer_properties(void) 
{
    cout << "Printing instance layers" << endl;
    for (auto property : layerProperties) { 
        cout <<  property.layerName << endl; 
    }
    cout << endl << "Printing devices layers" << endl; 
    for (auto &dev : devices) {
        for (auto &property : dev.deviceLayerProperties) {
            cout << "Layer name: " << property.layerName << endl; 
        }
    } 
}

void vk::print_available_instance_extensions(void) 
{
    cout << "Printing instance extensions available" << endl;
    for (auto &extension : instanceExtensionProperties) {
        cout << extension.extensionName << endl;
    }
}

void vk::print_device_extensions(void)
{
    cout << "Printing device extensions available" << endl;
    for (auto &dev : devices) {
        for (auto &extension : dev.deviceExtensionProperties) {
            cout << extension.extensionName << endl;
        } 
    }
}

void vk::print_required_instance_extensions(void)
{
    cout << "The following are required instance extensions:" << endl;
    for ( auto &ext : instanceExtensions) {
        cout << ext << endl;
    }
} 

void vk::print_device_info(device_holder_t dev)
{ 
    cout << "============================" << endl;
    cout << "Printing chosen device info:" << endl; 

    VkPhysicalDeviceProperties properties;

    /* Device name */
    vkGetPhysicalDeviceProperties(dev.physicalDevice, &properties); 
    cout << std::setw(20) << std::left <<
        "name: " << properties.deviceName << endl;

    /* Type of processor */
    int type = properties.deviceType;
    cout << std::setw(20) << std::left <<
        "type: "; 
    switch (type) {
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            cout << "Integrated GPU";
            break;
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            cout << "Discrete GPU";
            break;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            cout << "Virtual GPU";
            break;
        case VK_PHYSICAL_DEVICE_TYPE_OTHER:
            cout << "(Other)";
            break;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            cout << "CPU"; 
            break;
        default:;
    }
    cout << endl;

    /* Version */
    cout << std::setw(20) << std::left <<
        "Vulkan version: " << 
        VK_VERSION_MAJOR(properties.apiVersion) << "." <<
        VK_VERSION_MINOR(properties.apiVersion) << "." <<
        VK_VERSION_PATCH(properties.apiVersion) << endl; 

    /* Queue families */
    int32_t i = 0;
    for (auto &property : dev.queueFamilyProperties) {
        cout << "Queue family index " << i << ":"; 
        auto flags = property.queueFlags;
        if (flags & VK_QUEUE_GRAPHICS_BIT) { 
            cout << " | Graphics";
        } if (flags & VK_QUEUE_COMPUTE_BIT) {
            cout << " | Compute";
        } if (flags & VK_QUEUE_TRANSFER_BIT) {
            cout << " | Transfer";
        } if (flags & VK_QUEUE_SPARSE_BINDING_BIT) {
            cout << "| Sparse binding";
        } 
        cout << endl; 
        i++;
    }

    /* Layers */ 
    cout << "The device has " << dev.deviceLayerProperties.size() << " layers";
    cout  <<" available" << endl;
    for (auto &property : dev.deviceLayerProperties) {
        cout << property.layerName << endl; 
    } 
    cout << endl;

    /* Extensions */ 
    cout << "The device has " << dev.deviceExtensionProperties.size();
    cout << " extensions available" << endl;
    for (auto &property : dev.deviceExtensionProperties) {
        cout << property.extensionName << endl; 
    } 
    cout << endl;

    /* Graphics queue */
    if ( dev.queueFamilyIndices.hasGraphicsQueue()) {
        cout << "The device has a graphics queue: index " <<
            dev.queueFamilyIndices.graphicsIndex << endl;
    }
    if ( dev.queueFamilyIndices.hasPresentQueue()) {
        cout << "The device has a present queue: index " <<
            dev.queueFamilyIndices.presentIndex << endl;
    }
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
