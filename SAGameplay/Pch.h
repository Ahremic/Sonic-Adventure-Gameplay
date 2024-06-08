#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#ifndef NO_BLUEBLUR
//#define NO_BLUEBLUR
#include <BlueBlur.h>
#endif

#define EXPORT extern "C" __declspec(dllexport)
#define ASMHOOK void __declspec(naked)
#define NOINLINE __declspec(noinline)

constexpr float RAD2DEGf = 57.2958f;
constexpr float DEG2RADf = 0.0174533f;

constexpr double RAD2DEG = 57.29578018188477;
constexpr double DEG2RAD = 0.01745329238474369;

constexpr float  RAD2REVOLUTIONf = RAD2DEGf / 360.0f;
constexpr double RAD2REVOLUTION  = RAD2DEG  / 360.0;

constexpr int idClassicContext = 0x016D86FC;
constexpr int idModernContext = 0x016D4D64;
constexpr int idModernObject = 0x016D4B2C;

#include <Windows.h>
#include <detours.h>

#include <cstdint>
#include <cstdio>

#define _USE_MATH_DEFINES
#include <cmath>
#include <stdExtensions.h>

#include <INIReader.h>

#include <Helpers.h>


// All my includes anywhere and everywhere

#include "Common.h"
#include "../DebugDrawText.h"

#include <numbers>
#include <iomanip>
#include <iostream>
#include <fstream>

// Add these to blueblur whenever possible.
#include "Messages.h"
#include "Paths.h"


// Helper templates

template <typename T = uint32_t*, typename A>
T GetValue(A* ptr, uint32_t offset)
{
	return *(T*)((uint32_t)ptr + offset);
}

template <typename T = uint32_t*>
T GetValue(int ptr, uint32_t offset)
{
	return *(T*)((uint32_t)ptr + offset);
}

template <typename T = uint32_t*, typename A>
T* GetPointer(A* ptr, uint32_t offset)
{
	return (T*)((uint32_t)ptr + offset);
}

// Java-style hash function used by Hedgehog Engine games.
constexpr int JStrHash(const char* value)
{
	int hash = 0;
	while (*value)
	{
		hash = hash * 31 + *value;
		++value;
	}

	return hash & 0x7FFFFFFF;
}

// Boost workaround for disabling exceptions in release mode
#if !_DEBUG

namespace boost
{
	void throw_exception(std::exception const& e)
	{
		__debugbreak();
	}
}

#endif