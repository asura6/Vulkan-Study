//#define GLFW_INCLUDE_VULKAN
//#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include "setup.h" 

#include <iostream> 
using std::cout; using std::endl;



int main() { 
    try {
        vk vulkan; 
        vulkan.glfw_init();
        vulkan.init();
        vulkan.run();
    } catch (const std::runtime_error &e) {
        std::cerr << e.what() << endl;
        return EXIT_FAILURE;
    } 
    return EXIT_SUCCESS;
}
