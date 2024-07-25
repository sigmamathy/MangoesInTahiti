#pragma once

namespace graphics
{

// Create vulkan instance
// Create debug messenger (ifndef NDEBUG)
void LaunchVulkan();

// Destroy vulkan instance
// Destroy debug messenger (ifndef NDEBUG)
void EndVulkan();


struct WindowData
{
	int Width, Height;
	bool Fullscreen;
};

// Create GLFW window
// Create window surface
void CreateWindow(int x, int y, bool fullscreen);

// Destroy GLFW window
// Destroy window surface
void DestroyWindow();

void ResizeWindow(int x, int y);

WindowData QueryWindowData();


void CreateGraphicsLogicalDevice();

void DestroyGraphicsLogicalDevice();

void CreateSwapchain();

void DestroySwapchain();

}