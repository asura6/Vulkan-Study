TARGET = main

VULKAN_SDK_PATH = '/home/asura/Documents/Programming/Vulkan\
		  /VulkanSDK/1.0.46.0/x86_64'

CC = g++
CFLAGS = -std=c++11 -I$(VULKAN_SDK_PATH)/include
LDFLAGS = -L$(VULKAN_SDK_PATH)/lib `pkg-config --static --libs glfw3` -lvulkan

LIB_PATH=$(VULKAN_SDK_PATH)/lib
LAYER_PATH=$(VULKAN_SDK_PATH)/etc/explicit_layer.d

SRCC = $(wildcard ./*.cpp)
OBJ = $(SRCC:.cpp=.o)

default: $(TARGET) 

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) $(LDFLAGS)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

VulkanTest: main.cpp
	g++ $(CFLAGS) -o $(TARGET) main.cpp $(LDFLAGS)

.PHONY: test clean

test: $(TARGET)
	LD_LIBRARY_PATH=$(LIBPATH) VK_LAYER_PATH=$(LAYER_PATH)
	./$(TARGET)

clean:
	rm -f ./{$(TARGET),*.o}
