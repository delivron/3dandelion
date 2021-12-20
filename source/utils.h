#pragma once

#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcommon.h>
#include <Windows.h>

#include <string>
#include <filesystem>

namespace ddn
{

void ValidateResult(HRESULT hr);

Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(const std::filesystem::path& file_path, const std::string& entry_point, const std::string& compiler_target);

Microsoft::WRL::ComPtr<IDXGIFactory6> CreateFactory();

Microsoft::WRL::ComPtr<IDXGIAdapter1> GetAdapter(IDXGIFactory6& factory, DXGI_GPU_PREFERENCE gpu_preference = DXGI_GPU_PREFERENCE_UNSPECIFIED);

Microsoft::WRL::ComPtr<ID3D12Device> GetDevice(ID3D12DeviceChild& device_child);

Microsoft::WRL::ComPtr<ID3D12Resource> CreateBuffer(ID3D12Device& device, uint64_t size, D3D12_HEAP_TYPE heap_type, D3D12_RESOURCE_STATES initial_state);

}  // namespace ddn
