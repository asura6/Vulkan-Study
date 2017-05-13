#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
using std::cout; using std::endl; using std::cerr;
#include <stdexcept>
#include <functional>
#include <vector>
using std::vector;
#include <string>
using std::string;
#include <cstring>
#include <set>
#include <algorithm>

#include "./hello_triangle.h" 

/*************/
/* Constants */ 
/*************/
const char window_title[] = "Vulkan";
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
/* Validation layer from LunarSDK */
const vector<const char*> validationLayers = {
    "VK_LAYER_LUNARG_standard_validation"
};

const vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

/* Debugging validation layers */
#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif 

/* Main application class */
class HelloTriangleApplication {
    public:
        void run(void) {
            initWindow();
            initVulkan();
            mainLoop();
        }

    private:
        GLFWwindow* window;
        VDeleter<VkInstance> instance {vkDestroyInstance};
        VDeleter<VkDebugReportCallbackEXT> callback
        { instance, DestroyDebugReportCallbackEXT };
        VDeleter<VkSurfaceKHR> surface{instance, vkDestroySurfaceKHR};
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VDeleter<VkDevice> device{vkDestroyDevice};
        VDeleter<VkSwapchainKHR> swapChain{device, vkDestroySwapchainKHR};
        VkQueue presentQueue;
        VkQueue graphicsQueue;


        void initWindow() {
            /* Initialize the GLFW library */
            glfwInit();
            /* Do not use a OpenGL context, disable resizing */
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); 
            /* Create the window */
            window = glfwCreateWindow(
                    WINDOW_WIDTH,WINDOW_HEIGHT, window_title, nullptr, nullptr);

        } 

        void initVulkan(void) {
            createInstance(); 
            setupDebugCallback();
            createSurface();
            pickPhysicalDevice();
            createLogicalDevice();
            createSwapChain();
        }

        void createSwapChain() {
            auto swapChainSupport = querySwapChainSupport(physicalDevice);
            auto surfaceFormat = chooseSwapSurfaceFormat(
                    swapChainSupport.formats);
            auto presentMode = chooseSwapPresentMode(
                    swapChainSupport.presentModes);
            auto extent = chooseSwapExtent(swapChainSupport.capabilities);

            /* Specify swap chain images */ 
            uint32_t imageCount = swapChainSupport.capabilities.minImageCount+1;
            if (swapChainSupport.capabilities.maxImageCount > 0 && 
                    imageCount > swapChainSupport.capabilities.maxImageCount) {
                imageCount = swapChainSupport.capabilities.maxImageCount;
            }

            /* Creating the swap chain */
            VkSwapchainCreateInfoKHR createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            createInfo.surface = surface;
            /* Specifying swap chain images */
            createInfo.minImageCount = imageCount;
            createInfo.imageFormat = surfaceFormat.format;
            createInfo.imageColorSpace = surfaceFormat.colorSpace;
            createInfo.imageExtent = extent;
            createInfo.imageArrayLayers = 1; //layers per image >1 stereoscopic
            // imageUsage: here a direct render target (without post processing)
            createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

            /* It is possible that the graphics queue and present queue are in
             * different families. If that is the case we'll use what is called
             * concurrent mode to deal with this */
            QueueFamilyIndices indices = findQueueFamilies(physicalDevice); 
            uint32_t queueFamilyIndices[] = 
            {(uint32_t)indices.graphicsFamily, (uint32_t)indices.presentFamily};
        
            if (indices.graphicsFamily != indices.presentFamily) {
                createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
                createInfo.queueFamilyIndexCount = 2;
                createInfo.pQueueFamilyIndices = queueFamilyIndices; 
            } else {
                createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
                //createInfo.queueFamilyIndexCount = 0;     //optional
                //createInfo.pQueueFamilyIndices = nullptr; //optional
            }

            /* We do not want to transform the image, e.g. 90 deg rotation */
            createInfo.preTransform =
                swapChainSupport.capabilities.currentTransform;

            /* We do not want to blend with other windows */
            createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

            /* We do not care about colors of pixel which are obscured */
            createInfo.presentMode = presentMode;
            createInfo.clipped = VK_TRUE;

            createInfo.oldSwapchain = VK_NULL_HANDLE; 

            cout << "Creating a swapchain..." << endl; 
            auto result = vkCreateSwapchainKHR(
                    device, &createInfo, nullptr, swapChain.replace());
            if (result != VK_SUCCESS) {
                throw std::runtime_error("failed to create swap chain!");
            } else {
                cout << "\tsuccess:\tswapchain created" << endl; 
            }

        }

        void createSurface() {
            auto result = glfwCreateWindowSurface(
                    instance, window, nullptr, surface.replace());

            cout << "Creating a window surface..." << endl; 
            if (result != VK_SUCCESS) {
                throw std::runtime_error("failed to create window surface!");
            } else {
                cout << "\tsuccess:\twindow surface created" << endl; 
            }
        }

        void createLogicalDevice() {
            QueueFamilyIndices indices = findQueueFamilies(physicalDevice); 


            /* Create a set of all unique queue families */
            vector<VkDeviceQueueCreateInfo> queueCreateInfos;
            std::set<int> uniqueQueueFamilies = 
            { indices.graphicsFamily, indices.presentFamily };

            float queuePriority = 1.0f;

            for (int queueFamily : uniqueQueueFamilies) {
                VkDeviceQueueCreateInfo queueCreateInfo = {};
                queueCreateInfo.sType =
                    VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queueCreateInfo.queueFamilyIndex = indices.graphicsFamily;
                queueCreateInfo.queueCount = 1; 
                queueCreateInfo.pQueuePriorities = &queuePriority;
                queueCreateInfos.push_back(queueCreateInfo);
            }

            /* Right now no device features are used */
            VkPhysicalDeviceFeatures deviceFeatures = {};

            VkDeviceCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

            createInfo.pQueueCreateInfos = queueCreateInfos.data();
            createInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();

            createInfo.pEnabledFeatures = &deviceFeatures; 

            /* Enable required extensions */
            auto extensions = getRequiredExtensions();
            createInfo.enabledExtensionCount = extensions.size();
            createInfo.ppEnabledExtensionNames = extensions.data();

            /* Enable validation layers */ 
            createInfo.enabledExtensionCount = 0;
            if (enableValidationLayers) {
                createInfo.enabledLayerCount = validationLayers.size();
                createInfo.ppEnabledLayerNames = validationLayers.data();
            } else {
                createInfo.enabledLayerCount = 0;
            }

            /* Instantiate the logical device */
            cout << "Creating a logical device..." << endl;
            auto result = vkCreateDevice(
                    physicalDevice, &createInfo, nullptr, device.replace()); 
            if (result != VK_SUCCESS) {
                throw std::runtime_error("failed to create logical device!");
            } else {
                cout << "\tsuccess:\tlogical device created" << endl; 
            }

            vkGetDeviceQueue(device, indices.graphicsFamily, 0, &graphicsQueue);
            vkGetDeviceQueue(device, indices.presentFamily, 0, &presentQueue);
        }

        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) { 
            SwapChainSupportDetails details;

            /* Determine support cabailities taking into account the
             * VkPhysicalDevice and the VkSurfaceKHR window surface */
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
                    device, surface, &details.capabilities);

            /* Query supported surface formats */
            uint32_t formatCount;
            vkGetPhysicalDeviceSurfaceFormatsKHR(
                    device, surface, &formatCount, nullptr);
            if (formatCount != 0) {
                details.formats.resize(formatCount); //Resize vector
                vkGetPhysicalDeviceSurfaceFormatsKHR(
                        device, surface, &formatCount, details.formats.data());
            }

            /* Query supported presentation modes */
            uint32_t presentModeCount;
            vkGetPhysicalDeviceSurfacePresentModesKHR(
                    device, surface, &presentModeCount, nullptr);

            if (presentModeCount != 0) {
                details.presentModes.resize(presentModeCount);
                vkGetPhysicalDeviceSurfacePresentModesKHR(
                        device, surface, &presentModeCount, 
                        details.presentModes.data());
            }


            return details;
        } 

        /* The swap extent is the resolution of the swap chain images */

        VkExtent2D chooseSwapExtent(
                const VkSurfaceCapabilitiesKHR& capabilities) { 

            /* If the width equals the maximum uint32_t value then we choose the
             * resolution which best matches the window otherwire we use the
             * current values */
            if (capabilities.currentExtent.width
                    != std::numeric_limits<uint32_t>::max()) {
                return capabilities.currentExtent; 
            } else {
                VkExtent2D actualExtent = {WINDOW_WIDTH, WINDOW_HEIGHT};

                actualExtent.width = std::max(capabilities.minImageExtent.width,
                        std::min(capabilities.maxImageExtent.width,
                            actualExtent.width));
                actualExtent.height= std::max(capabilities.minImageExtent.height,
                        std::min(capabilities.maxImageExtent.height,
                            actualExtent.height));
                return actualExtent; 
            }

        }

        /* In this function we choose one of the available present modes
         * There are four possible present modes in Vulkan:
         * Immediate: no buffering, produces tearing
         * FIFO: Like vsync
         * FIFO Relaxed: vsync with additional detail
         * Mailbox: ex: triple buffering */ 
        VkPresentModeKHR chooseSwapPresentMode( 
                const vector<VkPresentModeKHR> availablePresentModes) {

            /* FIFO_KHR is guaranteed to be supporte but is not our first
             * choice, we try to replace it */
            VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

            /* We'll look for tripple buffering */
            for (const auto& availablePresentMode : availablePresentModes) {
                if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                    return availablePresentMode;
                } else if (availablePresentMode==VK_PRESENT_MODE_IMMEDIATE_KHR){
                    bestMode = availablePresentMode;
                }
            } 
            return bestMode;

        }

        VkSurfaceFormatKHR chooseSwapSurfaceFormat(
                const vector<VkSurfaceFormatKHR> &availableFormats) {
            if (availableFormats.size() == 1
                    && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
                /* VkSurfaceFormatKHR has entries: format, colorSpace */
                return
                {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}; 
            }
            /* If format is specified we are not free to choose ourselves,
             * therefore we go through this loop */
            for (const auto &availableFormat : availableFormats) {
                if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM
                        && (availableFormat.colorSpace == 
                            VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)) {
                    return availableFormat;
                }
            }
            /* If this fails then we just get what we can take */
            return availableFormats[0];
        }

        bool isDeviceSuitable(VkPhysicalDevice device) {
            VkPhysicalDeviceProperties deviceProperties;
            VkPhysicalDeviceFeatures deviceFeatures;
            vkGetPhysicalDeviceProperties(device, &deviceProperties);
            vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

            /* For now we make no choices depending on what properties is found
             * instead we just print them */
            switch (deviceProperties.deviceType) { 
                default:
                    cout << "Other platform";
                case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                    cout << "Found dedicated GPU ";
                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                    cout << "Found integrated GPU ";
                case VK_PHYSICAL_DEVICE_TYPE_CPU:
                    cout << "(Running on CPU)"; 
            }
            cout << endl; 

            /* Check if extension is supported (e.g. swap-chain) */
            bool extensionsSupported = checkDeviceExtensionSupport(device);

            /* Try to find a queue-family which supports graphics */
            QueueFamilyIndices indices = findQueueFamilies(device);
            if (indices.isComplete()) {
                cout << "Found a queue-family which supports graphics" <<
                    endl; 
            } else {
                cout << "Did not find a queue-family which " 
                    << "supports graphics" << endl;
            } 

            bool swapChainAdequate = false;
            /* If extensions are supported we'll have a look if the swapchain is
             * adequately supported */
            if (extensionsSupported) {
                auto swapChainSupport = querySwapChainSupport(device);
                swapChainAdequate = !swapChainSupport.formats.empty() 
                    && !swapChainSupport.presentModes.empty();
            } 

            return indices.isComplete() 
                && extensionsSupported 
                && swapChainAdequate;
        } 

        bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
            uint32_t extensionCount;
            vkEnumerateDeviceExtensionProperties(
                    device, nullptr, &extensionCount, nullptr);

            vector<VkExtensionProperties> availableExtensions(extensionCount); 
            vkEnumerateDeviceExtensionProperties(
                    device,nullptr,&extensionCount, availableExtensions.data());

            std::set<string> requiredExtensions(
                    deviceExtensions.begin(), deviceExtensions.end());

            for (const auto& extension : availableExtensions) {
                requiredExtensions.erase(extension.extensionName);
            }

            return requiredExtensions.empty();
        }

        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
            QueueFamilyIndices indices;
            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(
                    device, &queueFamilyCount, nullptr);

            vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(
                    device, &queueFamilyCount, queueFamilies.data()); 

            int i = 0;

            /* Find a queue family that supports graphics */
            for (const auto& queueFamily : queueFamilies) {
                if (queueFamily.queueCount > 0 
                        && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                    indices.graphicsFamily = i;
                }

                VkBool32 presentSupport = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(
                        device, i, surface, &presentSupport);

                if (queueFamily.queueCount > 0 && presentSupport) {
                    indices.presentFamily = i;
                }

                if (indices.isComplete()) {
                    break;
                } 
                i++;
            }

            return indices;
        }


        void pickPhysicalDevice() { 
            uint32_t deviceCount = 0;
            vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
            if (deviceCount == 0) {
                throw std::runtime_error(
                        "failed to fing GPUs with Vulkan support!");
            }
            vector<VkPhysicalDevice> devices(deviceCount);
            vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

            for (const auto& device : devices) {
                if (isDeviceSuitable(device)) { 
                    physicalDevice = device;
                    break;
                }
            } 

            if (physicalDevice == VK_NULL_HANDLE) {
                throw std::runtime_error("failed to find a suitable GPU!");
            } 
        } 

        void setupDebugCallback() {
            if (!enableValidationLayers) {
                return;
            }

            VkDebugReportCallbackCreateInfoEXT createInfo = {};

            createInfo.sType = 
                VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;

            createInfo.flags = 
                VK_DEBUG_REPORT_ERROR_BIT_EXT |
                VK_DEBUG_REPORT_WARNING_BIT_EXT;

            createInfo.pfnCallback = debugCallback;

            if (CreateDebugReportCallbackEXT(
                        instance,
                        &createInfo,
                        nullptr,
                        callback.replace()) != VK_SUCCESS) {
                throw std::runtime_error("failed to set up debug callback!");
            }
        }

        void mainLoop(void) {
            while (!glfwWindowShouldClose(window)) {
                glfwPollEvents();
            }

            glfwDestroyWindow(window);
            glfwTerminate(); 
        } 

        void createInstance() { 
            cout << "Enabling validation layers..." << endl;
            if (enableValidationLayers && !checkValidationLayerSupport()) {
                throw std::runtime_error(
                        "validation layers requested, but not available!"); 
            }

            /* Optional information with optimization consequences */
            VkApplicationInfo appInfo = {};
            appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            appInfo.pApplicationName = "Hello Triange";
            appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.pEngineName = "No Engine";
            appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.apiVersion = VK_API_VERSION_1_0;

            /* Print extensions support */
            checkExtensionsSupport();

            /* Necessary information to create a Vulkan instance */
            VkInstanceCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            createInfo.pApplicationInfo = &appInfo; 

            /* Get the required extensions and add these */
            auto extensions = getRequiredExtensions(); 
            createInfo.enabledExtensionCount = extensions.size();
            createInfo.ppEnabledExtensionNames = extensions.data();

            /* Enable validation layers if debug mode is activated */
            if (enableValidationLayers) {
                createInfo.enabledLayerCount = validationLayers.size();
                createInfo.ppEnabledLayerNames = validationLayers.data();
            } else {
                createInfo.enabledLayerCount = 0;
            }


            /* All required information to create an instance is supplied */
            cout << "Creating an instance..." << endl;
            VkResult result = vkCreateInstance(
                    &createInfo, nullptr, instance.replace());
            /* Instance should now be created */
            if (result != VK_SUCCESS) {
                throw std::runtime_error("failed to create instance!");
            } else {
                cout << "\tsuccess:\tinstance created" << endl;
            }
        }

        /* Get the required extensions including glfw extensions */
        vector<const char *> getRequiredExtensions() {
            vector<const char *> extensions;

            unsigned int glfwExtensionCount = 0;
            const char** glfwExtensions;
            glfwExtensions = glfwGetRequiredInstanceExtensions(
                    &glfwExtensionCount);

            for (unsigned int i = 0; i < glfwExtensionCount; i++) {
                extensions.push_back(glfwExtensions[i]);
            }

            if (enableValidationLayers) {
                extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
            }

            return extensions;
        } 

        /* Lists all available extensions */
        void checkExtensionsSupport() {
            /* Query available extensions */
            uint32_t extensionCount = 0;
            vkEnumerateInstanceExtensionProperties(
                    nullptr, &extensionCount, nullptr);
            vector<VkExtensionProperties> extensions(extensionCount);
            /* Query extension details */
            vkEnumerateInstanceExtensionProperties(
                    nullptr, &extensionCount, extensions.data());
            /* Print the extensions */
            cout << "Listing available extensions:..." << endl;
            for (auto extension : extensions) {
                cout << "-->\t" <<  extension.extensionName << endl;
            } 

        }

        /* Checks if all of the requested layers are available */
        bool checkValidationLayerSupport() {
            uint32_t layerCount;
            vkEnumerateInstanceLayerProperties(
                    &layerCount, nullptr);

            vector<VkLayerProperties> availableLayers(layerCount);
            vkEnumerateInstanceLayerProperties(
                    &layerCount, availableLayers.data());

            for (auto layerName : validationLayers) {
                bool layerFound = false;

                for (const auto& layerProperties : availableLayers) {
                    if (strcmp(layerName, layerProperties.layerName) == 0) {
                        layerFound = true;
                        break;
                    }
                }

                if (!layerFound) {
                    return false;
                }
            } 
            cout << "\tSuccess:\trequested validation layers found" <<
                endl;

            return true;
        } 

        /* Callback function to print debugging information using the validation
         * layers */
        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
                VkDebugReportFlagsEXT flags,
                VkDebugReportObjectTypeEXT objType,
                uint64_t obj,
                size_t location,
                int32_t code,
                const char* layerPrefix,
                const char* msg,
                void* userData) {

            cerr << "validation layer: " << msg << endl;

            return VK_FALSE;
        }

};

int main(void) {
    HelloTriangleApplication app;

    try {
        app.run();
    } catch (const std::runtime_error& e) {
        cerr << e.what() << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
