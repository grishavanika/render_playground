#pragma once

#pragma warning(push)
// macro redefinition
#pragma warning(disable : 4005)
#include <d3d11.h>
#pragma warning(pop)

#include <wrl/client.h>

template<typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;
