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

void vk::run(void)
{
    main_loop();
}

void vk::main_loop(void)
{
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        //drawFrame();
    } 
    /* Destroy logical device */
    vkDeviceWaitIdle(device); 
    vkDestroyDevice(device, nullptr); 
    /* Destroy instance */
    vkDestroyInstance(instance, nullptr);

    /* Terminate window */
    glfwDestroyWindow(window);
    glfwTerminate(); 
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
    load_devices();
    load_device_extensions();
    //print_device_extensions(); 
    //print_device_infos();
    load_features();
    load_memory_properties();
    load_queue_family_properties();
    //print_queue_family_properties(); 
    create_logical_device();
    load_layer_properties();
    //print_layer_properties(); 
}
