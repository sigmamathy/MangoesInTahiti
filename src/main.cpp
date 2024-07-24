#include "vksys.hpp"

int main()
{
	graphics::LaunchVulkan();
	graphics::CreateWindow(1600, 900, false);
	graphics::CreateGraphicsLogicalDevice();



	graphics::DestroyGraphicsLogicalDevice();
	graphics::DestroyWindow();
	graphics::EndVulkan();
	return 0;
}