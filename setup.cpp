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

/*************/
/* FUNCTIONS */
/*************/ 

/* Create a Vulkan instance */
void vk::create_instance(void)
{ 
    /* Application Info */ 
    const VkApplicationInfo appInfo = {
        VK_STRUCTURE_TYPE_APPLICATION_INFO,
        nullptr,                            //pNext
        "Asura application",                //pApplicationName
        VK_MAKE_VERSION(1, 0, 0),           //applicationVersion
        "Asura engine",                     //pEngineName
        VK_MAKE_VERSION(1, 0, 0),           //engineVersion
        VK_API_VERSION_1_0                  //apiVersion
    }; 

    /* Instance Create Info */ 
    //!!! Missing extensions !!!
    const VkInstanceCreateInfo createInfo = {
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        nullptr,                            //pNext
        0,                                  //flags
        &appInfo,                           //pApplicationInfo
        (uint32_t)layers.size(),            //enabledLayerCount
        layers.data(),                      //ppEnabledLayerNames
        (uint32_t)instanceExtensions.size(),//enabledExtensionCount
        instanceExtensions.data()           //ppEnabledExtensionNames 
    }; 

    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance); 
    print_result(result);
} 

void vk::load_devices(void) 
{
    vector<VkPhysicalDevice> physicalDevices; 
    uint32_t deviceCount;
    //Get number of devieces
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    // Resize vector to fit
    physicalDevices.resize(deviceCount);
    devices.resize(deviceCount);
    //Get physicalDevices
    VkResult result = vkEnumeratePhysicalDevices(
            instance, &deviceCount, physicalDevices.data()); 

    for (uint32_t i = 0; i != physicalDevices.size(); i++) {
        devices[i].physicalDevice = physicalDevices[i];
    } 
    print_result(result);
} 

void vk::load_features(void)
{ 
    for (uint32_t i = 0; i != devices.size(); i++) { 
        vkGetPhysicalDeviceFeatures(
                devices[i].physicalDevice, &devices[i].features);
    }
}

void vk::load_memory_properties(void) 
{ 
    for (uint32_t i = 0; i != devices.size(); i++) { 
        vkGetPhysicalDeviceMemoryProperties(
                devices[i].physicalDevice, &devices[i].memoryProperties); 
    } 
}

void vk::load_queue_family_properties(void) 
{ 
    uint32_t queueFamilyCount; 
    for (auto &device : devices) { 
        //Read number of queues
        vkGetPhysicalDeviceQueueFamilyProperties(
                device.physicalDevice, &queueFamilyCount, nullptr); 
        //Resize vector to fit 
        device.queueFamilyProperties.resize(queueFamilyCount); 
        //Get queue family properties
        vkGetPhysicalDeviceQueueFamilyProperties(
                device.physicalDevice,
                &queueFamilyCount,
                device.queueFamilyProperties.data()
                );

        VkBool32 presentSupport = false;
        for (uint32_t j = 0; j != queueFamilyCount; j++) { 
            //Get graphics queue index
            if (device.queueFamilyProperties[j].queueFlags 
                    & VK_QUEUE_GRAPHICS_BIT) {
                device.queueFamilyIndices.graphicsIndex = j;
            }
            //Check if present supported
            vkGetPhysicalDeviceSurfaceSupportKHR(
                    device.physicalDevice, 
                    j,
                    surface,
                    &presentSupport); 
            if (presentSupport) {
                device.queueFamilyIndices.presentIndex = j;
            }

        } 
    }
}

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

/* Looks for and returns an index to a device with a graphics queue family */
int32_t vk::find_suitable_device(void)
{ 
    uint32_t extensions_found = 0; 
    for (uint32_t i = 0; i != devices.size(); i++) {
        // Check if a graphics queue is supported
        if (!devices[i].queueFamilyIndices.hasGraphicsQueue()) { 
            return -1; 
        } 
        // Check if a present queue is supported
        if (!devices[i].queueFamilyIndices.hasPresentQueue()) {
            return -1;
        } 
        // Check if required extensions are supported 
        for (uint32_t j = 0; j != requiredDeviceExtensions.size(); j++) { 
            for (auto &extension : devices[i].deviceExtensionProperties) { 
                if (strcmp(
                            extension.extensionName,
                            requiredDeviceExtensions[j])
                        == 0) { 
                    extensions_found++;
                    break;
                }
            } 
        } 
        if (extensions_found == requiredDeviceExtensions.size()) { 
            return i;
        }

    }
    return -1;
}

void vk::create_logical_device() 
{ 
    int32_t deviceIndex = find_suitable_device(); 
    chosenDevice = devices[deviceIndex];
    vector<VkDeviceQueueCreateInfo> deviceQueueCreateInfos;

    /* throw exception if no suitable queue available */
    if (deviceIndex < 0) { 
        throw std::runtime_error("Could not find a suitable device");
    }
    const float queuePriority = 1.f; 

    /* Use a set so we only iterate over the unique values */
    std::set<int> uniqueQueueFamilies = {
        devices[deviceIndex].get_graphics_queue_index(),
        devices[deviceIndex].get_present_queue_index()};

    for (auto &queueFamily : uniqueQueueFamilies) {
        const VkDeviceQueueCreateInfo deviceQueueCreateInfo  = {
            VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            nullptr,                                //pNext
            0,                                      //flags
            (uint32_t)queueFamily,                  //queueFamilyIndex
            1,                                      //Queue count 
            &queuePriority                          //pQueuePriorities
        }; 
        deviceQueueCreateInfos.push_back(deviceQueueCreateInfo);
    } 

    /* Fill out create info structures */
    const VkDeviceCreateInfo createInfo = {
        VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        nullptr,                              //pNext
        0,                                    //flags
        (uint32_t)deviceQueueCreateInfos.size(),//queueCreateInfoCount
        deviceQueueCreateInfos.data(),        //pQueueCreateInfos
        (uint32_t)layers.size(),              //enaledLayerCount
        layers.data(),                        //ppEnabledLayerNames,
        (uint32_t)requiredDeviceExtensions.size(),  //enabledExtensionCount,
        requiredDeviceExtensions.data(),      //ppEnabledExtensionNames
        &devices[deviceIndex].features        //pEnabledFeatures 
    }; 

    /* Create the logical device */
    VkResult result; 
    result = vkCreateDevice(
            devices[deviceIndex].physicalDevice, 
            &createInfo, 
            nullptr, 
            &device);

    print_result(result); 
}

void vk::load_layer_properties(void) 
{
    uint32_t propertyCount;
    //Get vector size
    vkEnumerateInstanceLayerProperties(&propertyCount, nullptr);
    //Resize vector
    layerProperties.resize(propertyCount);
    //Get properties
    vkEnumerateInstanceLayerProperties(&propertyCount, layerProperties.data());

    //Load device properties
    for (auto &dev: devices) { 
        //Get vector size
        vkEnumerateDeviceLayerProperties(dev.physicalDevice, &propertyCount, nullptr); 
        //Resize vector
        dev.deviceLayerProperties.resize(propertyCount);
        //Get properties
        auto result = vkEnumerateDeviceLayerProperties(
                dev.physicalDevice,
                &propertyCount,
                dev.deviceLayerProperties.data());
        print_result(result);
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

void vk::load_available_instance_extensions(void)
{ 
    uint32_t propertyCount;
    //Get vector size
    vkEnumerateInstanceExtensionProperties(
            nullptr,
            &propertyCount,
            nullptr);
    //resize vector
    instanceExtensionProperties.resize(propertyCount);
    //Get extensions
    auto result = vkEnumerateInstanceExtensionProperties(
            nullptr,
            &propertyCount,
            instanceExtensionProperties.data()); 
    print_result(result);
}

void vk::print_available_instance_extensions(void) 
{
    cout << "Printing instance extensions available" << endl;
    for (auto &extension : instanceExtensionProperties) {
        cout << extension.extensionName << endl;
    }
}


void vk::load_device_extensions(void)
{
    uint32_t propertyCount;
    for (auto &dev : devices) {
        //get vector size
        vkEnumerateDeviceExtensionProperties(
                dev.physicalDevice,
                nullptr,
                &propertyCount,
                nullptr);
        //Resize vector
        dev.deviceExtensionProperties.resize(propertyCount);
        //Get extensions
        auto result = vkEnumerateDeviceExtensionProperties(
                dev.physicalDevice,
                nullptr,
                &propertyCount,
                dev.deviceExtensionProperties.data()); 
        print_result(result);
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

void vk::load_required_instance_extensions(void)
{ 
    uint32_t extensionCount = 0; 
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&extensionCount); 

    for (uint32_t i = 0; i != extensionCount; i++) {
        instanceExtensions.push_back(glfwExtensions[i]);
    } 
    /* If in debug mode then add the debug extension as well */
#ifndef NDEBUG
    instanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#endif 
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

void vk::create_surface(void)
{
    auto result = glfwCreateWindowSurface(
            instance, window, nullptr, &surface);
    print_result(result);
}

