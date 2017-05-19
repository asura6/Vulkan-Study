TARGET = main

CC = g++
VULKAN_SDK_PATH = '/home/asura/Documents/Programming/Vulkan/VulkanSDK/1.0.46.0/x86_64'
CFLAGS = -std=c++11 -I$(VULKAN_SDK_PATH)/include -Wall -Wextra
LDFLAGS = -L$(VULKAN_SDK_PATH)/lib `pkg-config --static --libs glfw3` -lvulkan

SRCC = $(wildcard ./*.cpp)
OBJ = $(SRCC:.cpp=.o)
HEADER = $(wildcard ./*.h)

default: $(TARGET) 

$(TARGET): $(OBJ) $(HEADER)
	$(CC) -o $(TARGET) $(OBJ) $(CFLAGS) $(LDFLAGS)

%.o: %.cpp
	$(CC) -o $@ -c $< $(CFLAGS) $(LDFLAGS)

.PHONY: test clean

clean:
	rm -f ./{$(TARGET),*.o}

test: $(TARGET)
	LD_LIBRARY_PATH=$(VULKAN_SDK_PATH)/lib 
	VK_LAYER_PATH=$(VULKAN_SDK_PATH)/etc/expliit_layer.d
	./$(TARGET) 
