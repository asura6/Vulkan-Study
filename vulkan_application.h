#ifndef VULKAN_APPLICATION
#define VULKAN_APPLICATION

#define DEBUG 

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h> 

#include <vector> 
#include <string>
#include <iostream> 

typedef struct {
    //Index to use
    int graphicsIndex = -1;
    int presentIndex = -1;

    bool hasGraphicsQueue(void) {
        return graphicsIndex >= 0;
    } 
    bool hasPresentQueue(void) {
        return presentIndex >= 0;
    }
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
    int32_t get_graphics_queue_index(void) {
        return this->queueFamilyIndices.graphicsIndex;
    }
    int32_t get_present_queue_index(void) {
        return this->queueFamilyIndices.presentIndex;
    }
} device_holder_t;

typedef struct { 
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats; 
    std::vector<VkPresentModeKHR> presentModes; 
} swapchain_support_details_t;


class vk {
    public:
        /* RUN */
        void init(void); 
        void glfw_init(void);
        void run(void);

    private: 
        void main_loop(void); 
        void cleanup(void);
        /* SETUP */
        void initWindow(void);
        void create_instance(void); 
        void load_devices(void); 
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
        void print_device_info(device_holder_t dev);
        void create_surface(void);
        /* RESOURCES */ 
        void load_queues(void);
        void load_swapchain_support_details(void);
        void print_swapchain_support_details(void);
        VkSurfaceFormatKHR get_suitable_swapchain_surface_format(void);
        VkPresentModeKHR get_suitable_swapchain_present_mode(void);
        VkExtent2D get_swapchain_extent(void); 
        void create_swapchains(void); 
        void load_swapchain_image_handles(void);
        void create_swapchain_image_views(void); 
        void create_renderpass(void);
        void create_framebuffers(void);
        void create_graphics_pipeline_layout(void); 
        std::vector<char> read_file(const std::string& filename);
        void create_shader_module(const std::vector<char> &code,
                VkShaderModule &shaderModule);
        void create_graphics_pipeline(void);
        void create_command_pool(void); 
        void allocate_command_buffers(void);
        void record_command_buffers(void);
        void create_semaphores(void);
        void draw_frame(void);


        /* GLFW data */
        GLFWwindow* window;
        const uint32_t WIDTH = 800;
        const uint32_t HEIGHT = 600; 

        /* Layers */ 
        const std::vector<const char*> layers = {
#ifndef NDEBUG
            "VK_LAYER_LUNARG_standard_validation"
#endif
        };

        /* Extensions */
        std::vector<VkExtensionProperties> instanceExtensionProperties;
        std::vector<const char *> instanceExtensions;
        const std::vector<const char *> requiredDeviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME 
        }; 

        /* Instance and devices */ 
        VkInstance instance; 
        std::vector<device_holder_t> devices; 
        VkDevice device;  //logical device
        device_holder_t chosenDevice;

        /* Features */ 

        /* Layers */
        std::vector<VkLayerProperties> layerProperties; 

        /* Resources */ 
        VkSurfaceKHR surface;
        VkQueue graphicsQueue; 
        VkQueue presentQueue; 
        VkSwapchainKHR swapchain;
        VkFormat swapchainImageFormat;
        VkExtent2D swapchainExtent;
        swapchain_support_details_t swapchainSupportDetails;
        std::vector<VkImage> swapchainImages;
        std::vector<VkImageView> swapchainImageViews;
        VkRenderPass renderPass;
        std::vector<VkFramebuffer> swapchainFramebuffers;
        VkPipelineLayout graphicsPipelineLayout;
        VkPipeline graphicsPipeline;
        VkCommandPool commandPool;
        std::vector<VkCommandBuffer> commandBuffers;
        VkSemaphore imageAvailableSemaphore;
        VkSemaphore renderFinishedSemaphore;


}; 
#endif 
