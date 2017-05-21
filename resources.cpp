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
#include <fstream>

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

        /* Create info structure */
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
                &swapchain);                    
        print_result(result); 

        /* Load properties into global object */
        swapchainImageFormat = format.format;
        swapchainExtent = extent;
}

/* Load the handles that are used to access the swapchain's images. After this
 * function they are accessible in the global object "swapchainImages" */
void vk::load_swapchain_image_handles(void)
{
    //Get vector size
    uint32_t imageCount;
    vkGetSwapchainImagesKHR(
            device,
            swapchain,
            &imageCount,
            nullptr);
    swapchainImages.resize(imageCount);
    VkResult result = vkGetSwapchainImagesKHR( 
            device,
            swapchain,
            &imageCount,
            swapchainImages.data());
    print_result(result);
}

/* Wrap the swapchain images */
void vk::create_swapchain_image_views(void)
{
    /* This object is used to map channels, e.g. we can interpret the red
     * channel in one image and use that to fill in the green channel.
     * Initializeng the object to zero puts all components to zero which is
     * interpreted as VK_COMPONENT_SWIZZLE_IDENTITY meaning that the data in the
     * child image should be read from the corresponding channel in the parent
     * image, i.e. no remap */
    VkComponentMapping components = {}; 

    /* Part of create info */
    VkImageSubresourceRange subresourceRange = {};
    /* The color part of an image */
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; 
    subresourceRange.baseMipLevel = 0; //no mipmapping
    subresourceRange.levelCount = 1;
    /* The image is not an array image, then these are 0 and 1 */
    subresourceRange.baseArrayLayer = 0; 
    subresourceRange.layerCount = 1;

    /* Create info */
    VkImageViewCreateInfo ci = {};
    ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ci.pNext = nullptr;
    ci.flags = 0U; 
    //ci.image ! This field is modified in the loop below
    ci.viewType = VK_IMAGE_VIEW_TYPE_2D; //2D image
    ci.format = swapchainImageFormat;
    ci.components = components;
    /* The child image can be a subset of the parent image */
    ci.subresourceRange = subresourceRange; 

    swapchainImageViews.resize(swapchainImages.size());
    for (uint32_t i = 0; i != swapchainImages.size(); i++) {
        ci.image = swapchainImages[i];
        VkResult result =  vkCreateImageView(
                device,
                &ci,
                nullptr,
                &swapchainImageViews[i]); 
        print_result(result);
    } 
}

void vk::create_renderpass(void)
{
    /* ATTACHMENT */
    /* Create an attachment. An attachment is a single image that is used as
     * input, output or both within one or more subpasses in the renderpass */
    /* A color attachment is an attachment to which subpass write data */
    /* An input attachment are attachments from which subpasses can read data */
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.flags = 0;
    colorAttachment.format = swapchainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; //No multisampling
    /* These next four fields specify what to do with the colorAttachment at the
     * beginning and end of the renderpass */ 
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; //Clear it at load
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; //Store; want to keep it 
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; //Dont use stenc
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; //same
    // We don't care what the format is before the render pass begins
    // This field is only what the image is expected to be in at start 
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    // We want this format to be presented to the swapchain after the renderpass
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; 

    /* ATTACHMENT REFERENCE */
    /* This reference is a simple structure containing the index into an array
     * of attachments (if there are multiple) */
    VkAttachmentReference  colorAttachmentReference = {};
    /* The index. It is referenced in the shader as "layout(location=0)" */
    colorAttachmentReference.attachment = 0; //Index 0
    /* Color buffer */
    colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; 

    /* SUBPASS */
    VkSubpassDescription subpass = {}; 
    subpass.flags = 0; 
    // Graphics pipeline (it is the only one supported anyway)
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.inputAttachmentCount = 0; //No attachment to read, we only write
    subpass.pInputAttachments = nullptr;
    subpass.colorAttachmentCount = 1; //We want to output to a color attachment
    subpass.pColorAttachments = &colorAttachmentReference;
    //Last fields we don't care about
    subpass.pResolveAttachments = nullptr; //For multisampled images
    subpass.pDepthStencilAttachment = nullptr;
    subpass.preserveAttachmentCount = 0; //No attachments we want to store
    subpass.pPreserveAttachments = 0; 

    /* Create info */
    VkRenderPassCreateInfo ci = {
        VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        nullptr,                                    //pNext
        0,                                          //flags
        1,                                          //attachmentCount
        &colorAttachment,                         //pAttachments
        1,                                          //subpassCount
        &subpass,                                   //pSubpasses
        0,                                          //dependencyCount
        nullptr};                                   //pDependencies 

    /* Create the renderpass */
    VkResult result = vkCreateRenderPass(
            device,
            &ci,
            nullptr,
            &renderPass);
    print_result(result);
}

/* Create framebuffers for the render pass */
void vk::create_framebuffers(void)
{
    VkFramebufferCreateInfo ci = {};
    ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    ci.pNext = nullptr;
    ci.flags = 0;
    ci.renderPass = renderPass;
    ci.attachmentCount = 1;
    //ci.pAttachments ! this field occurs in the loop
    ci.width = swapchainExtent.width;
    ci.height = swapchainExtent.height;
    ci.layers = 1; 

    swapchainFramebuffers.resize(swapchainImages.size());

    for (uint32_t i = 0; i != swapchainImages.size(); i++) { 
        ci.pAttachments = &swapchainImageViews[i]; 
        VkResult result = vkCreateFramebuffer(
                device,
                &ci,
                nullptr,
                &swapchainFramebuffers[i]);
        print_result(result);
    }
}

void vk::create_graphics_pipeline_layout(void)
{ 
    /* PIPELINE LAYOUT */
    VkPipelineLayoutCreateInfo pl_ci = {};
    pl_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pl_ci.setLayoutCount = 0; //Optional
    pl_ci.pSetLayouts = nullptr; //Optional
    pl_ci.pushConstantRangeCount = 0; //Optional
    pl_ci.pPushConstantRanges = 0; //Optional 

    VkResult result = vkCreatePipelineLayout(
            device,
            &pl_ci,
            nullptr,
            &graphicsPipelineLayout); 
    print_result(result);
}

vector<char> vk::read_file(const string& filename) {
    /* Read file stating at end (ate) and as binary data */
    std::ifstream file(filename, std::ios::ate | std::ios::binary); 
    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }
    /* Since we started reading at the end we can tell the filesize from
     * the read position */
    size_t fileSize = (size_t) file.tellg();
    vector<char> buffer(fileSize);
    /* Now with a buffer allocated we'll go back to the start */
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    return buffer;
}

/* Creates the VkShaderModules. These objects are wrappers around
 * bytecode buffers. They need to be utilized by assignment later on to
 * be useful */
void vk::create_shader_module(
        const vector<char> &code,
        VkShaderModule &shaderModule) { 
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    /* The bytecode is specified with 8-bit pointers but needs to be
     * 32-bit */
    vector<uint32_t> codeAligned(code.size() / sizeof(uint32_t) + 1);
    memcpy(codeAligned.data(), code.data(), code.size());
    createInfo.pCode = codeAligned.data(); 

    VkResult result = vkCreateShaderModule(
            device, &createInfo, nullptr, &shaderModule); 
    print_result(result);
} 

void vk::create_graphics_pipeline(void)
{
    ////
    /* CODE WRAPPERS */
    //Code to compile
    auto vertShaderCode = read_file("shaders/vert.spv");
    auto fragShaderCode = read_file("shaders/frag.spv");
    //Objets to hold the shader modules
    VkShaderModule vertShaderModule;
    VkShaderModule fragShaderModule;
    //Create the shader modules
    create_shader_module(vertShaderCode, vertShaderModule);
    create_shader_module(fragShaderCode, fragShaderModule);

    ////
    /* SHADER STAGES */
    //Vertex shader
    VkPipelineShaderStageCreateInfo vertShader_ci = {};
    vertShader_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShader_ci.pNext = nullptr;
    vertShader_ci.flags = 0;
    vertShader_ci.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShader_ci.module = vertShaderModule; //Code wrapper
    vertShader_ci.pName = "main"; //function to invoke in the shader program
    vertShader_ci.pSpecializationInfo = nullptr; //For optimization

    //Fragment shader
    VkPipelineShaderStageCreateInfo fragShader_ci = {};
    fragShader_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShader_ci.pNext = nullptr;
    fragShader_ci.flags = 0;
    fragShader_ci.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShader_ci.module = fragShaderModule; //Code wrapper
    fragShader_ci.pName = "main"; //function to invoke in the shader program
    fragShader_ci.pSpecializationInfo = nullptr; //For optimization 

    //Shader stages
    VkPipelineShaderStageCreateInfo shaderStages[] = 
    {vertShader_ci, fragShader_ci};

    ////
    /* VERTEX SETUP */
    //We specify there is no data to load as we will hardcode the data directly
    //into the shader
    VkPipelineVertexInputStateCreateInfo vert_ci = {};
    vert_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vert_ci.pNext = nullptr;
    vert_ci.flags = 0;
    //These fields describe buffers containing vertex data and how the data is
    //orederd, since we hardcode the vertex we don't use any of these
    vert_ci.vertexBindingDescriptionCount = 0;
    vert_ci.pVertexBindingDescriptions = nullptr;
    vert_ci.vertexAttributeDescriptionCount = 0;
    vert_ci.pVertexAttributeDescriptions = nullptr;

    ////
    /* INPUT ASSEMBLY */
    //This stage groups vertex data into primitives
    VkPipelineInputAssemblyStateCreateInfo asmb_ci = {};
    asmb_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    asmb_ci.pNext = nullptr;
    asmb_ci.flags = 0; 

    //Try any of these primitives!
    //asmb_ci.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    //asmb_ci.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    //asmb_ci.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; 
    asmb_ci.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
    //asmb_ci.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    //asmb_ci.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
    //These topologies are often used when a geoetry shader is enabled, this is
    //because some vertices only contain adjacency information and will not
    //produce any interesting result if not a shader is present
    //asmb_ci.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY;
    //asmb_ci.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY;
    //asmb_ci.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY;
    //asmb_ci.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY;

    asmb_ci.primitiveRestartEnable = VK_FALSE;

    ////
    /* VIEWPORT */
    //The final coordinate transform before rasterization, from normalized
    //device coordinates into window coordinates
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapchainExtent.width;
    viewport.height = (float)swapchainExtent.height;
    viewport.minDepth = 1.0f;
    viewport.maxDepth = 1.0f;
    //Scissor
    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = swapchainExtent; //Draw whole image

    VkPipelineViewportStateCreateInfo vp_ci;
    vp_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp_ci.pNext = nullptr;
    vp_ci.flags = 0;
    vp_ci.viewportCount = 1;
    vp_ci.pViewports = &viewport;
    vp_ci.scissorCount = 1;
    vp_ci.pScissors = &scissor;

    ////
    /* RASTERIZER */
    //Primitives represented by vertices are now turned into streams of
    //fragments ready to be shaded by the fragment shader
    VkPipelineRasterizationStateCreateInfo ra_ci;
    ra_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    ra_ci.pNext = nullptr;
    ra_ci.flags = 0;
    ra_ci.depthClampEnable = VK_FALSE;
    ra_ci.rasterizerDiscardEnable = VK_FALSE;

    //Try these three different polygon modes!
    ra_ci.polygonMode = VK_POLYGON_MODE_FILL;
    //ra_ci.polygonMode = VK_POLYGON_MODE_LINE;
    //ra_ci.polygonMode = VK_POLYGON_MODE_POINT;

    //Cull polygons facing away. This only affects polygons, not lines or points
    ra_ci.cullMode = VK_CULL_MODE_BACK_BIT;
    //Determine which vertix winding order is front or back
    ra_ci.frontFace = VK_FRONT_FACE_CLOCKWISE;
    ra_ci.depthBiasEnable = VK_FALSE;
    ra_ci.depthBiasConstantFactor = 0.0f;
    ra_ci.depthBiasClamp = 0.0f;
    ra_ci.depthBiasSlopeFactor = 0.0f;
    ra_ci.lineWidth = 1.0f; //Width of primitives in pixels

    ////
    /* MULTISAMPLE STATE */
    VkPipelineMultisampleStateCreateInfo ms_ci = {};
    ms_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms_ci.pNext = nullptr;
    ms_ci.flags = 0;
    ms_ci.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    ms_ci.sampleShadingEnable = VK_FALSE;
    ms_ci.minSampleShading = 1.f;
    ms_ci.pSampleMask = nullptr;
    ms_ci.alphaToCoverageEnable = VK_FALSE;
    ms_ci.alphaToOneEnable = VK_FALSE;

    ////
    /* DEPTH AND/OR STENCIL BUFFER */
    //not used, pass nullptr instead

    ////
    /* COLOR BLEND STATE */
    VkPipelineColorBlendAttachmentState cbAttachmentState;
    //Ignore all blending parameters
    cbAttachmentState.blendEnable = VK_FALSE;
    cbAttachmentState.colorWriteMask = 
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    // Following are all optional settings
    cbAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    cbAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    cbAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
    cbAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    cbAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    cbAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD; 

    VkPipelineColorBlendStateCreateInfo cb_ci;
    cb_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cb_ci.pNext = nullptr;
    cb_ci.flags = 0;
    cb_ci.logicOpEnable = VK_FALSE;
    cb_ci.logicOp = VK_LOGIC_OP_COPY;
    cb_ci.attachmentCount = 1;
    cb_ci.pAttachments = &cbAttachmentState; 
    cb_ci.blendConstants[0] = 0.0f; //Optional
    cb_ci.blendConstants[1] = 0.0f; //Optional
    cb_ci.blendConstants[2] = 0.0f; //Optional
    cb_ci.blendConstants[3] = 0.0f; //Optional 

    ////
    /* DYNAMIC STATE */
    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_LINE_WIDTH
    };
    VkPipelineDynamicStateCreateInfo ds_ci = {};
    ds_ci.sType =
        VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    ds_ci.dynamicStateCount = 2;
    ds_ci.pDynamicStates = dynamicStates;
    (void)ds_ci; //Unused as of now

    ////
    /* GRAPHICS PIPELINE */
    VkGraphicsPipelineCreateInfo ci = {};
    ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    ci.pNext = nullptr;
    ci.flags = 0;
    ci.stageCount = 2; //Two shader stages
    ci.pStages = shaderStages;
    ci.pVertexInputState = &vert_ci;
    ci.pInputAssemblyState = &asmb_ci;
    ci.pViewportState = &vp_ci;
    ci.pRasterizationState = &ra_ci;
    ci.pMultisampleState = &ms_ci;
    ci.pDepthStencilState = nullptr;
    ci.pColorBlendState = &cb_ci;
    ci.pDynamicState = nullptr;//&ds_ci;
    ci.layout = graphicsPipelineLayout;
    ci.renderPass = renderPass;
    ci.subpass = 0;
    ci.basePipelineHandle = VK_NULL_HANDLE; //Optional
    ci.basePipelineIndex = -1; //Optional 

    VkResult result = vkCreateGraphicsPipelines(
            device,             //device
            VK_NULL_HANDLE,     //pipelineCache
            1,                  //createInfoCount
            &ci,                //pCreateInfos
            nullptr,            //pAllocator
            &graphicsPipeline); //pPipelines
    print_result(result);

    ////
    /* Cleanup */
    //Destroy pipeline layout
    vkDestroyPipelineLayout(device, graphicsPipelineLayout, nullptr);
    //Destroy shaders
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
    vkDestroyShaderModule(device, fragShaderModule, nullptr); 
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

void vk::allocate_command_buffers(void)
{
    commandBuffers.resize(swapchainFramebuffers.size());
    VkCommandBufferAllocateInfo ai = {};
    ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    ai.pNext = nullptr;
    ai.commandPool = commandPool;
    ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandBufferCount = (uint32_t)commandBuffers.size(); 

    VkResult result = vkAllocateCommandBuffers(
            device,
            &ai,
            commandBuffers.data());
    print_result(result);
} 

void vk::record_command_buffers(void)
{
    /* Record all the command buffers */
    for (uint32_t i = 0; i != commandBuffers.size(); i++) {
        ////
        /* BEGIN COMMAND BUFFER */ 
        VkCommandBufferBeginInfo bi = {};
        bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        bi.pNext = nullptr;
        bi.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        bi.pInheritanceInfo = nullptr;
        //Begin the command buffer (resetting it to an initial state) 
        vkBeginCommandBuffer(commandBuffers[i], &bi); 

        ////
        /* BEGIN RENDER PASS */
        VkRenderPassBeginInfo rpi = {};
        rpi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpi.pNext = nullptr;
        rpi.renderPass = renderPass;
        rpi.framebuffer = swapchainFramebuffers[i];
        //Render onto the whole rendering area of the framebuffer
        rpi.renderArea.offset = {0, 0};
        rpi.renderArea.extent = swapchainExtent;
        // Clear color: black with 100% opacity 
        VkClearValue clearColor = {0.f, 0.f, 0.f, 0.f};
        rpi.clearValueCount = 1;
        rpi.pClearValues = &clearColor;
        //Inline: commands embedded directly into the primary command buffer
        vkCmdBeginRenderPass(
                commandBuffers[i],
                &rpi,
                VK_SUBPASS_CONTENTS_INLINE);

        /* DRAW */
        vkCmdBindPipeline(
                commandBuffers[i],
                VK_PIPELINE_BIND_POINT_GRAPHICS, //graphics pipeline
                graphicsPipeline);
        vkCmdDraw(commandBuffers[i],
                4, //vertexCount 3
                1, //instanceCount 1: no instancing
                0, //firstVertex 0
                0);//firstInstance 0
        /* END RENDER PASS */
        vkCmdEndRenderPass(commandBuffers[i]);
        VkResult result = vkEndCommandBuffer(commandBuffers[i]);
        print_result(result); 
    }
}

void vk::create_semaphores(void)
{
    VkSemaphoreCreateInfo ci = {};
    ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    ci.pNext = nullptr;
    ci.flags = 0;

    VkResult result = vkCreateSemaphore(
            device,
            &ci,
            nullptr,
            &imageAvailableSemaphore);
    print_result(result);

    result = vkCreateSemaphore(
            device,
            &ci,
            nullptr,
            &renderFinishedSemaphore);
    print_result(result); 
}

void vk::draw_frame(void)
{
    /* Acquire an image from the swapchain */
    uint32_t imageIndex;
    vkAcquireNextImageKHR(
            device,
            swapchain,
            (uint64_t)-1,
            imageAvailableSemaphore,
            VK_NULL_HANDLE,
            &imageIndex);

    /* Submitting the command buffer */
    VkSubmitInfo si = {};
    si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO; 

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphore}; 
    si.waitSemaphoreCount = 1;
    si.pWaitSemaphores = waitSemaphores;

    VkPipelineStageFlags waitStages[] =
    {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT}; 
    si.pWaitDstStageMask = waitStages;

    si.commandBufferCount = 1;
    si.pCommandBuffers = &commandBuffers[imageIndex]; 

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
    si.signalSemaphoreCount = 1;
    si.pSignalSemaphores = signalSemaphores;

    vkQueueSubmit(graphicsQueue, 1, &si, VK_NULL_HANDLE);

    /* Presentaton */
    VkPresentInfoKHR pi = {};
    pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    pi.waitSemaphoreCount = 1;
    pi.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapchains[] = {swapchain};
    pi.swapchainCount = 1;
    pi.pSwapchains = swapchains;
    pi.pImageIndices = &imageIndex;
    pi.pResults = nullptr;
    vkQueuePresentKHR(presentQueue, &pi);
}
