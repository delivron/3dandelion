#pragma once

#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_5.h>
#include <d3dcommon.h>
#include <Windows.h>

#include <string>

namespace ddn
{

void ValidateResult(HRESULT hr);

Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(const std::string& source, const std::string& entry_point, const std::string& compiler_target);

Microsoft::WRL::ComPtr<IDXGIFactory5> CreateFactory();

Microsoft::WRL::ComPtr<ID3D12Device> GetDevice(ID3D12DeviceChild& device_child);

}  // namespace ddn
