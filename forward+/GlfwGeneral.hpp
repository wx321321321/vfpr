
#pragma once
#include "VKBase.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#pragma comment(lib, "glfw3.lib")


GLFWwindow* pWindow;
GLFWmonitor* pMonitor;
const char* windowTitle = "EasyVk";

bool InitializeWindow(VkExtent2D size, bool fullScreen = false, bool isResizable = true, bool limitFrameRate = true) {
	
    //glfw��ʼ��
    if (!glfwInit()) {
		outStream << std::format("[initialwindow error1]\n");
		return false;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, isResizable);

	pMonitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* pMode = glfwGetVideoMode(pMonitor);//��ȡ��Ƶģʽ
	pWindow = fullScreen ?
		glfwCreateWindow(pMode->width, pMode->height, windowTitle, pMonitor, nullptr) :
		glfwCreateWindow(size.width, size.height, windowTitle, nullptr, nullptr);
	if (!pWindow) {
		outStream << std::format("[initializeWindow error2]\n");
		glfwTerminate();
		return false;
	}


    //�����չ��ʼ��
#ifdef _WIN32
    vulkan::graphicsBase::Base().AddInstanceExtension(VK_KHR_SURFACE_EXTENSION_NAME);
    vulkan::graphicsBase::Base().AddInstanceExtension(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#else
    uint32_t extensionCount = 0;
    const char** extensionNames;
    extensionNames = glfwGetRequiredInstanceExtensions(&extensionCount);
    if (!extensionNames) {
        outStream << std::format("[ InitializeWindow ]\nVulkan is not available on this machine!\n");
        glfwTerminate();
        return false;
    }
    for (size_t i = 0; i < extensionCount; i++)
        graphicsBase::Base().AddInstanceExtension(extensionNames[i]);
#endif
    vulkan::graphicsBase::Base().AddDeviceExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);


    //�ڴ���Vulkanʵ��
    vulkan::graphicsBase::Base().UseLatestApiVersion();
    if (vulkan::graphicsBase::Base().CreateInstance()) {
        return false;
    }

    //����surface
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (VkResult result = glfwCreateWindowSurface(vulkan::graphicsBase::Base().Instance(), pWindow, nullptr, &surface)) {
        outStream << std::format("[ InitializeWindow ] ERROR\nFailed to create a window surface!\nError code: {}\n", int32_t(result));
        glfwTerminate();
        return false;
    }
    vulkan::graphicsBase::Base().Surface(surface);

    
    if (//��·����һ������ֱ�ӷ���false
        vulkan::graphicsBase::Base().GetPhysicalDevices() ||  
        vulkan::graphicsBase::Base().DeterminePhysicalDevice(0, true, false) ||     
        vulkan::graphicsBase::Base().CreateDevice()){ 
        return false;
    }
    

    if (vulkan::graphicsBase::Base().CreateSwapchain(limitFrameRate))
        return false;
    return true;
}
void TerminateWindow() {
    vulkan::graphicsBase::Base().WaitIdle();
	glfwTerminate();
}
void TitleFps() {
	static double time0 = glfwGetTime();
	static double time1;
	static double dt;
	static int dframe = -1;
	static std::stringstream info;
	time1 = glfwGetTime();
	dframe++;
	if ((dt = time1 - time0) >= 1) {
		info.precision(1);
		info << windowTitle << "    " << std::fixed << dframe / dt << " FPS";
		glfwSetWindowTitle(pWindow, info.str().c_str());
		info.str("");
		time0 = time1;
		dframe = 0;
	}
}