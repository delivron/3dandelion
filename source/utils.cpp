#include "utils.h"

#include <d3dcompiler.h>

#include <sstream>
#include <stdexcept>
#include <type_traits>

using namespace Microsoft::WRL;

namespace
{

template <typename T, typename = std::enable_if_t<std::is_integral_v<T>, T>>
T CreateFlags(T initial_flags, T debug_flag)
{
    auto flags = initial_flags;
#ifdef _DEBUG
    flags |= debug_flag;
#endif
    return flags;
}

}

namespace ddn
{

void ValidateResult(HRESULT hr)
{
    if (FAILED(hr)) {
        throw std::runtime_error("Invalid return value");
    }
}

ComPtr<ID3DBlob> CompileShader(const std::string& source, const std::string& entry_point, const std::string& compiler_target)
{
    ComPtr<ID3DBlob> shader;
    ComPtr<ID3DBlob> error;
    auto compiler_flags = CreateFlags<UINT>(0, D3DCOMPILE_DEBUG);
    auto hr = D3DCompile(source.data(), source.size(), nullptr, nullptr, nullptr, entry_point.c_str(), compiler_target.c_str(), compiler_flags, 0, &shader, &error);

    if (FAILED(hr)) {
        std::wostringstream oss;
        oss << "Compilation error: " << static_cast<char*>(error->GetBufferPointer()) << std::endl;
        OutputDebugString(oss.str().c_str());
    }

    ValidateResult(hr);

    return shader;
}

ComPtr<IDXGIFactory5> CreateFactory()
{
    ComPtr<IDXGIFactory5> factory;
    auto factory_flags = CreateFlags<UINT>(0, DXGI_CREATE_FACTORY_DEBUG);
    ValidateResult(CreateDXGIFactory2(factory_flags, IID_PPV_ARGS(&factory)));
    return factory;
}

ComPtr<ID3D12Device> GetDevice(ID3D12DeviceChild& device_child)
{
    ComPtr<ID3D12Device> device;
    device_child.GetDevice(IID_PPV_ARGS(&device));
    return device;
}

}  // namespace ddn