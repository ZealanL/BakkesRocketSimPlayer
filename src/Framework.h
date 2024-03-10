#pragma once

#define BRSP_VERSION "0.0"	

#include <stdint.h>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <queue>
#include <deque>
#include <stack>
#include <cassert>
#include <map>
#include <set>
#include <unordered_set>
#include <functional>
#include <chrono>
#include <filesystem>

#define _USE_MATH_DEFINES // for M_PI and similar
#include <cmath>

// Remove need for std namespace scope for very common datatypes
using std::vector;
using std::map;
using std::unordered_map;
using std::set;
using std::multiset;
using std::unordered_set;
using std::list;
using std::stack;
using std::deque;
using std::string;
using std::wstring;
using std::pair;

// Integer typedefs
typedef int8_t	int8;	typedef uint8_t	 uint8;
typedef int16_t int16;	typedef uint16_t uint16;
typedef int32_t int32;	typedef uint32_t uint32;
typedef int64_t int64;	typedef uint64_t uint64;
typedef uint8_t byte;

// Time
typedef std::chrono::steady_clock::time_point time_point;
#define CURTIME() std::chrono::high_resolution_clock::now()
#define TIME_SINCE(timePoint) (std::chrono::duration<double>(CURTIME() - timePoint).count())
#define CUR_MS() (std::chrono::duration_cast<std::chrono::milliseconds>(CURTIME().time_since_epoch()).count())

#define MAX(a, b) ((a > b) ? a : b)
#define MIN(a, b) ((a < b) ? a : b)

#define CLAMP(val, min, max) MIN(MAX(val, min), max)

#define STR(s) ([&]{ std::stringstream __macroStream; __macroStream << s; return __macroStream.str(); }())

// Returns sign of number (1 if positive, -1 if negative, and 0 if 0)
#define SGN(val) ((val > 0) - (val < 0))

// Bakkesmod
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/wrappers/CanvasWrapper.h"
#include "bakkesmod/wrappers/GameObject/BallWrapper.h"
#include "bakkesmod/wrappers/Engine/WorldInfoWrapper.h"
#include "bakkesmod/wrappers/GameEvent/ServerWrapper.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"

inline BakkesMod::Plugin::BakkesModPlugin* g_PluginInst = NULL;
inline std::mutex g_PluginInstMutex = {};
inline void _LOG(std::string s) {
	static std::mutex printLock;
	g_PluginInstMutex.lock();
	printLock.lock();
	{
		if (g_PluginInst && g_PluginInst->cvarManager) { 
			g_PluginInst->cvarManager->log(STR(s)); 
		}
	}
	printLock.unlock();
	g_PluginInstMutex.unlock();
}
#define LOG(s) _LOG(STR(s))

#define THROW_ERR(s) throw std::runtime_error(STR(s))

// Debug logging
#if defined(_RELDEBUG) or defined(_DEBUG)
#define DLOG(s) LOG("[DBG] " << s)
#else
#define DLOG(s) {}
#endif

#define DEG2RAD(x) (x * (M_PI / 180))
#define RAD2DEG(x) (x * (180 / M_PI))

inline std::ostream& operator <<(std::ostream& stream, Vector v) {
	stream << "[ " << v.X << ", " << v.Y << ", " << v.Z << " ]";
	return stream;
}

#define TICKTIME (1/120.f)