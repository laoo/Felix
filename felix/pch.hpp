#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <chrono>
#include <cmath>
#include <coroutine>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>


#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#pragma warning(push)
#pragma warning( disable: 4005 )
#include <d3d11.h>
#pragma warning(pop)

#include <wrl/client.h>
template<typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

#endif
