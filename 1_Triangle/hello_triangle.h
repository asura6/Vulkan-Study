#ifndef hello_triangle_header
#define hello_triangle_header

#include <functional>

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct QueueFamilyIndices {
    int graphicsFamily = -1;
    int presentFamily = -1;

    bool isComplete() {
        return graphicsFamily >= 0 && presentFamily >= 0;
    }
};

/* Proxy function which looks up the required function to pass the debugging
 * structure to */
VkResult CreateDebugReportCallbackEXT(
        VkInstance instance,
        const VkDebugReportCallbackCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDebugReportCallbackEXT* pCallback) {

    auto func = (PFN_vkCreateDebugReportCallbackEXT)
        vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");

    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pCallback);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
} 

void DestroyDebugReportCallbackEXT(
        VkInstance instance,
        VkDebugReportCallbackEXT callback,
        const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugReportCallbackEXT)
        vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
    if (func != nullptr) {
        func(instance, callback, pAllocator);
    }
}

/* RAII object-Wrapper
 * This wrapper takes care of memory allocation */ 
template <typename T>
class VDeleter {

    public: 
        /* Default constructor with a dummy deleter function which can be used
         * to initialize it later */
        VDeleter() : VDeleter([](T, VkAllocationCallbacks*) {}) {}

        /* The three non-default constructors allows the user to specify three
         * types of deletion functions:
         * - vkDestroyXXX(object, callbacks): 
         *     Only needs to pass the object
         * - vkDestroyXX(instance, object, callbacks):
         *     A VkInstance also needs to bo passed to the cleanup function, the
         *     constructor thus takes a VkInstance reference
         * - vkDestroyXXX(device, object, callback):
         *     Like the previous but with a VkDevice instead of a VkInstance
         */

        /* vkDestroyXXX(object, callbacks) */
        VDeleter(std::function<void(T, VkAllocationCallbacks*)> deletef) { 
            this->deleter =
                [=](T obj) { deletef(obj, nullptr); };
        }

        /* vkDestroyXXX(instance, object, callbacks) */
        VDeleter(const VDeleter<VkInstance> &instance,
                std::function<void(VkInstance, T, VkAllocationCallbacks*) >
                deletef) {
            this->deleter = 
                [&instance, deletef](T obj) { deletef(instance, obj, nullptr);
                };
        }

        /* vkDestroyXXX(device, object, callbacks) */
        VDeleter(const VDeleter<VkDevice> &device,
                std::function<void(VkDevice, T, VkAllocationCallbacks*)>
                deletef) {
            this->deleter = 
                [&device, deletef](T obj) { deletef(device, obj, nullptr);
                };
        }

        /* The object calls the cleanup() function everytime it leaves scope */
        ~VDeleter() {
            cleanup();
        }

        /* Returns a non-constant pointer if the user wants to replace the
         * handle within this wrapper class */
        T* replace() {
            cleanup();
            return &object;
        } 

        /* Overloading address-of operator & */
        const T* operator &() const {
            return &object;
        }

        /* Overloading cast-operator */
        operator T() const {
            return object;
        }

        /* Overloading assignment operator */
        void operator=(T rhs) {
            if (rhs != object) {
                cleanup();
                object = rhs;
            }
        }

        /* Overloading comparison == operator */
        template<typename V>
            bool operator==(V rhs) {
                return object == T(rhs);
            }

    private:

        T object{VK_NULL_HANDLE};
        std::function<void(T)> deleter;

        void cleanup() {
            if (object != VK_NULL_HANDLE) {
                deleter(object);
            }
            object = VK_NULL_HANDLE;
        }
};

#endif
