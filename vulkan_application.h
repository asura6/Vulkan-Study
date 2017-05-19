#define DEBUG

#ifndef VULKAN_APPLICATION
#define VULKAN_APPLICATION

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h> 

#include <vector> 
#include <string>
#include <iostream>


typedef struct {
    //Index to use
    int graphicsIndex = -1;
    int presentIndex = -1;

    bool hasGraphicsQueue(void)
    {
        return graphicsIndex >= 0;
    }; 
    //bool hasComputeQueue(void) 
    //{
    //    return (graphicsFamily & VK_QUEUE_COMPUTE_BIT);
    //};

    //bool isComplete() {
    //    return (graphicsFamily >= 1) & (presentFamily >= 1);
    //}
} QueueFamilyIndices_t;

typedef struct {
    VkPhysicalDevice physicalDevice;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceMemoryProperties memoryProperties;
    std::vector<VkQueueFamilyProperties> queueFamilyProperties;
    VkDevice device; 
    QueueFamilyIndices_t queueFamilyIndices;
    std::vector<VkLayerProperties> deviceLayerProperties;
    std::vector<VkExtensionProperties> deviceExtensionProperties;

    uint32_t get_graphics_queue_index(void) 
    {
        return this->queueFamilyIndices.graphicsIndex;
    }
} device_holder_t;

class vk {
    public:
        /* RUN */
        void init(void); 
        void glfw_init(void);
        void run(void);

    private:
        void main_loop(void); 
        /* SETUP */
        void initWindow(void);
        void create_instance(void); 
        void load_devices(void); 
        void print_device_infos(void);
        void load_features(void);
        void load_memory_properties(void);
        void load_queue_family_properties(void);
        void print_queue_family_properties(void);
        void create_logical_device(void); 
        void load_layer_properties(void);
        void print_layer_properties(void);
        void load_available_instance_extensions(void);
        void print_available_instance_extensions(void);
        void load_device_extensions(void);
        void print_device_extensions(void); 
        void load_required_instance_extensions(void);
        void print_required_instance_extensions(void);
        int32_t find_suitable_device(void); 
        /* RESOURCES */ 

        /* GLFW variables */
        GLFWwindow* window;
        const int WIDTH = 800;
        const int HEIGHT = 600; 

        /* Layers */ 
        const std::vector<const char*> layers = {
#ifndef NDEBUG
            "VK_LAYER_LUNARG_standard_validation"
                //"VK_LAYER_LUNARG_core_validation",
                //"VK_LAYER_LUNARG_device_limits", 
                //"VK_LAYER_LUNARG_parameter_validation", 
                //"VK_LAYER_LUNARG_object_tracker", 
                //"VK_LAYER_LUNARG_swapchain",
                //"VK_LAYER_GOOGLE_threading",
                //"VK_LAYER_GOOGLE_unique_objects"
#endif
        };

        ///* Extensions */
        std::vector<VkExtensionProperties> instanceExtensionProperties;
        std::vector<const char *> instanceExtensions;
        const std::vector<const char *> requiredDeviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME 
        }; 

        /* Instance and devices */ 
        VkInstance instance; 
        std::vector<device_holder_t> devices; 
        VkDevice device;  //logical device

        ///* Features */
        //std::vector<VkPhysicalDeviceFeatures> features;

        /* Layers */
        std::vector<VkLayerProperties> layerProperties; 

        ///* Memory */
        //std::vector<VkPhysicalDeviceMemoryProperties> memoryProperties;
        std::vector<VkBuffer> buffers;

        ///* Queues */
        //std::vector<std::vector<VkQueueFamilyProperties>> queueFamilyProperties;

        /* Private functions */

}; 

#endif 


