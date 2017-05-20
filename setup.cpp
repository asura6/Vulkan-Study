//#define NDEBUG

//#define GLFW_INCLUDE_VULKAN
//#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream> 
using std::cout; using std::endl;
#include <vector>
using std::vector;

#include "setup.h"
#include "debug_print.h"

#include <algorithm>

#include <string>
using std::string;
#include <string.h>

/*************/
/* FUNCTIONS */
/*************/

void vk::init(void)
{
    load_instance_extensions();
    print_instance_extensions();

    //load_required_instance_extensions();

    create_instance();
    load_devices();
    load_device_extensions();
    print_device_extensions(); 
    print_device_infos();
    load_features();
    load_memory_properties();
    load_queue_family_properties();
    print_queue_family_properties(); 
    create_logical_device();
    load_layer_properties();
    print_layer_properties(); 
}

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
        (uint32_t)validationLayers.size(),  //enabledLayerCount
        validationLayers.data(),            //ppEnabledLayerNames
        0,//(uint32_t)deviceExtensions.size(),  //enabledExtensionCount
        0//deviceExtensions.data()              //ppEnabledExtensionNames 
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

void vk::print_device_infos(void) 
{
    cout << "Printing device infos:" << endl; 
    for (uint32_t i = 0; i!= devices.size(); i++) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(devices[i].physicalDevice, &properties); 
        cout << std::setw(20) << std::left <<
            "--> Device name: " << properties.deviceName << endl;

        int type = properties.deviceType;
        cout << std::setw(20) << std::left <<
            "--> Device type: "; 

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

        cout << std::setw(20) << std::left <<
            "--> Vulkan version: " << 
            VK_VERSION_MAJOR(properties.apiVersion) << "." <<
            VK_VERSION_MINOR(properties.apiVersion) << "." <<
            VK_VERSION_PATCH(properties.apiVersion) << endl; 
    }
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
        //device.queueFamilyIndices.resize(queueFamilyCount);
        //Get queue family properties
        vkGetPhysicalDeviceQueueFamilyProperties(
                device.physicalDevice,
                &queueFamilyCount,
                device.queueFamilyProperties.data()
                );

        for (uint32_t j = 0; j != queueFamilyCount; j++) { 
            device.queueFamilyIndices.graphicsIndex = j;
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
uint32_t vk::find_suitable_device(void)
{ 
    uint32_t extensions_found = 0;
    //for (auto &dev : devices) {
    for (uint32_t i = 0; i != devices.size(); i++) {
        // Check if a graphics queue is supported
        if (!devices[i].queueFamilyIndices.hasGraphicsQueue()) { 
            return -1;
        }
        // Check if required extensions are supported 
        for (auto &requirement : requiredDeviceExtensions) {
            for (auto &extension : devices[i].deviceExtensionProperties) {
                if (extension.extensionName == requirement) { 
                    extensions_found++;
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
    uint32_t deviceIndex = find_suitable_device(); 
    const float queuePriority = 1.f;

    /* Convert string to const char * */
    vector<const char *> reqExtensions;
    for (auto &c : requiredDeviceExtensions) {
        reqExtensions.push_back(c.c_str());
    }

    const VkDeviceQueueCreateInfo deviceQueueCreateInfo  = {
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        nullptr,                                //pNext
        0,                                      //flags
        devices[deviceIndex].get_graphics_queue_index(), //queueFamilyIndex
        1,                                      //Queue count 
        &queuePriority                          //pQueuePriorities
    }; 

    const VkDeviceCreateInfo createInfo = {
        VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        nullptr,                              //pNext
        0,                                    //flags
        1,                                    //queueCreateInfoCount
        &deviceQueueCreateInfo,               //pQueueCreateInfos
        (uint32_t)validationLayers.size(),    //enaledLayerCount
        validationLayers.data(),              //ppEnabledLayerNames,
        (uint32_t)reqExtensions.size(),       //enabledExtensionCount,
        reqExtensions.data(),                 //ppEnabledExtensionNames
        &devices[deviceIndex].features        //pEnabledFeatures 
    };

    (void)createInfo;

    VkResult result; 
    VkDevice logicalDevice;
    result = vkCreateDevice(
            devices[deviceIndex].physicalDevice, 
            &createInfo, 
            nullptr, 
            &logicalDevice);

    logicalDevices.resize(logicalDevices.size() + 1);
    logicalDevices[logicalDevices.size() - 1].logicalDevice = logicalDevice;
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
        cout << "Layer name: " << property.layerName << endl; 
    }
    cout << endl << "Printing devices layers" << endl; 
    for (auto &dev : devices) {
        for (auto &property : dev.deviceLayerProperties) {
            cout << "Layer name: " << property.layerName << endl; 
        }
    } 
}

void vk::load_instance_extensions(void)
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

void vk::print_instance_extensions(void) 
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

//void vk::load_required_instance_extensions(void)
//{
//    vector<string> extensions;
//    uint32_t extensionCount = 0;
//    const char** glfwExtensions;
//    glfwExtensions = glfwGetRequiredInstanceExtensions(&extensionCount);
//
//    for (uint32_t i = 0; i != extensionCount; i++) {
//        extensions.push_back(string(glfwExtensions[i], strnlen(glfwExtensions[i], 255U)));
//    }
//
//    cout << "Found " << endl;
//    for (auto s : extensions) {
//        cout << s << endl;
//    } 
//}
//
//void vk::print_required_instance_extensions(void)
//{
//    cout << "Printing required instance extensions..." << endl;
//    for ( auto ext : require
//}

void vk::run(void)
{
    main_loop();
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

void vk::main_loop(void)
{
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        //drawFrame();
    } 
    //vkDeviceWaitIdle(device);
    glfwDestroyWindow(window);
    glfwTerminate(); 
}
