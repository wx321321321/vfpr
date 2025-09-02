
#pragma once
#include "VKBase.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#pragma comment(lib, "glfw3.lib")


GLFWwindow* pWindow;
GLFWmonitor* pMonitor;
const char* windowTitle = "EasyVk";



struct cameraAdjustSign {
	bool lmb_down = false;
	bool rmb_down = false;
	glm::vec2 cursor_pos = { 0.0f, 0.0f };
	glm::vec2 prev_cursor_pos = { 0.0f, 0.0f };
	bool w_down = false; // todo use a hash map or something
	bool s_down = false;
	bool a_down = false;
	bool d_down = false;
	bool q_down = false;
	bool e_down = false;
	bool z_pressed = false;

	void onMouseButton(int button, int action, int mods)
	{
		if (action == GLFW_PRESS) {
			double x, y;
			glfwGetCursorPos(pWindow, &x, &y);
			cursor_pos = { x, y };
			prev_cursor_pos = { x, y };

			if (button == GLFW_MOUSE_BUTTON_LEFT)
			{
				lmb_down = true;
			}
			else if (button == GLFW_MOUSE_BUTTON_RIGHT)
			{
				rmb_down = true;
				glfwSetInputMode(pWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			}
		}
		else if (action == GLFW_RELEASE)
		{
			if (button == GLFW_MOUSE_BUTTON_LEFT)
			{
				lmb_down = false;
			}
			else if (button == GLFW_MOUSE_BUTTON_RIGHT)
			{
				rmb_down = false;
				glfwSetInputMode(pWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			}
		}
	}

	void onKeyPress(int key, int scancode, int action, int mods)
	{
		if (action == GLFW_PRESS)
		{
			switch (key)
			{
				// todo use a hash map or something
			case GLFW_KEY_W:
				w_down = true;
				break;
			case GLFW_KEY_S:
				s_down = true;
				break;
			case GLFW_KEY_A:
				a_down = true;
				break;
			case GLFW_KEY_D:
				d_down = true;
				break;
			case GLFW_KEY_Q:
				q_down = true;
				break;
			case GLFW_KEY_E:
				e_down = true;
				break;
			}
		}
		else if (action == GLFW_RELEASE)
		{
			switch (key)
			{
			case GLFW_KEY_W:
				w_down = false;
				break;
			case GLFW_KEY_S:
				s_down = false;
				break;
			case GLFW_KEY_A:
				a_down = false;
				break;
			case GLFW_KEY_D:
				d_down = false;
				break;
			case GLFW_KEY_Q:
				q_down = false;
				break;
			case GLFW_KEY_E:
				e_down = false;
				break;
			case GLFW_KEY_Z:
				z_pressed = true;
			}
		}
	}

	void onCursorPosChanged(double xPos, double yPos)
	{
		//std::cout << xPos << "," << yPos << std::endl;
		if (!lmb_down && !rmb_down) {
			return;
		}
		else if (lmb_down) {
			//glm::vec2 tmp(xPos, yPos);
			//modelRotAngles += (tmp - cursorPos) * 0.01f;

			cursor_pos = { xPos, yPos };
		}
		else if (rmb_down) {
			glm::vec2 tmp(xPos, yPos);

			cursor_pos = { xPos, yPos };
		}

	}


};

cameraAdjustSign AdjustSign;

bool InitializeWindow(VkExtent2D size, bool fullScreen = false, bool isResizable = true, bool limitFrameRate = true) {
	
    //glfw初始化
    if (!glfwInit()) {
		outStream << std::format("[initialwindow error1]\n");
		return false;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, isResizable);

	pMonitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* pMode = glfwGetVideoMode(pMonitor);//获取视频模式
	pWindow = fullScreen ?
		glfwCreateWindow(pMode->width, pMode->height, windowTitle, pMonitor, nullptr) :
		glfwCreateWindow(size.width, size.height, windowTitle, nullptr, nullptr);
	if (!pWindow) {
		outStream << std::format("[initializeWindow error2]\n");
		glfwTerminate();
		return false;
	}
	auto cursorpos_callback = [](GLFWwindow* window, double xPos, double yPos)
		{
			AdjustSign.onCursorPosChanged(xPos, yPos);
		};

	auto mousebutton_callback = [](GLFWwindow* window, int button, int action, int mods)
		{
			AdjustSign.onMouseButton(button, action, mods);
		};

	auto key_callback = [](GLFWwindow* window, int key, int scancode, int action, int mods)
		{
			AdjustSign.onKeyPress(key, scancode, action, mods);
		};
	glfwSetKeyCallback(pWindow, key_callback);
	glfwSetCursorPosCallback(pWindow, cursorpos_callback);
	glfwSetMouseButtonCallback(pWindow, mousebutton_callback);

    //层和扩展初始化
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


    //在创建Vulkan实例
    vulkan::graphicsBase::Base().UseLatestApiVersion();
    if (vulkan::graphicsBase::Base().CreateInstance()) {
        return false;
    }

    //创建surface
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (VkResult result = glfwCreateWindowSurface(vulkan::graphicsBase::Base().Instance(), pWindow, nullptr, &surface)) {
        outStream << std::format("[ InitializeWindow ] ERROR\nFailed to create a window surface!\nError code: {}\n", int32_t(result));
        glfwTerminate();
        return false;
    }
    vulkan::graphicsBase::Base().Surface(surface);

    
    if (//短路，有一个操作直接返回false
        vulkan::graphicsBase::Base().GetPhysicalDevices() ||  
        vulkan::graphicsBase::Base().DeterminePhysicalDevice(0, true, true) ||     
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