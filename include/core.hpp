#pragma once

#include <array>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <string>
#include <unordered_set>
#include <algorithm>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#ifndef NDEBUG
// Check bugs.
#define ASSERT(...) if (!(__VA_ARGS__)) throw std::runtime_error("Assertion \"" #__VA_ARGS__ "\" failed at " THISFILE " (line " + std::to_string(__LINE__) + ").")
#else
#define ASSERT(...)
#endif

// Handles unexpected runtime errors that has no point in saving. (e.g. device missing capabilities required)
#define VALIDATE(...) if (!(__VA_ARGS__)) throw std::runtime_error("Runtime Error: Statement \"" #__VA_ARGS__ "\" failed at " THISFILE " (line " + std::to_string(__LINE__) + ").")