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

void vk::create_buffers(void)
{
    const VkBufferCreateInfo createInfo = {
        //sType
        //pNext
        //flags
        //size
        //usage
        //sharingMode
        //queueFamilyIndexCount
        //pQueueFamilyIndices
    }

    //VkResult = vkCreateBuffer(
    //device,
    //&createInfo,
    //nullptr,
    //&buffers[0]);
    


}
