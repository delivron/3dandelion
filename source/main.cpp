#include "application.h"

#include <wrl.h>
#include <d3dx12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>

#include <Windows.h>

#include <array>
#include <sstream>
#include <stdexcept>
#include <algorithm>

using namespace Microsoft::WRL;

void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr)) {
        throw std::runtime_error("Invalid return value");
    }
}

std::string s_shader_code = R"(
struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

PSInput VSMain(float4 position : POSITION, float4 color : COLOR)
{
    PSInput result;
    result.position = position;
    result.color = color;
    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return input.color;
}
)";

class DandelionApp : public ddn::Application
{
public:
    DandelionApp(const std::wstring& title, uint32_t width, uint32_t height)
        : ddn::Application(title, width, height)
    {
        InitDevice();
        InitCommandList();
        InitSwapChain();
        InitRtvDescriptorHeap();
        InitRootSignature();
        InitGraphicsPipelineState();
        InitFence();
        InitVertexBuffer();
    }

    ~DandelionApp()
    {
        if (m_event && m_event != INVALID_HANDLE_VALUE) {
            CloseHandle(m_event);
        }
    }

    void OnResize(uint32_t width, uint32_t height) override
    {
        Flush();

        width = std::max<uint32_t>(1, width);
        height = std::max<uint32_t>(1, height);

        for (auto& back_buffer : m_back_buffers) {
            back_buffer.Reset();
        }

        DXGI_SWAP_CHAIN_DESC1 swap_chian_desc = {};
        ThrowIfFailed(m_swap_chain->GetDesc1(&swap_chian_desc));
        ThrowIfFailed(m_swap_chain->ResizeBuffers(swap_chian_desc.BufferCount, width, height, swap_chian_desc.Format, swap_chian_desc.Flags));

        UpdateBackBufferViews();

        m_buffer_index = m_swap_chain->GetCurrentBackBufferIndex();
    }

    void OnUpdate() override
    {
        ComPtr<ID3D12Resource> back_buffer = m_back_buffers[m_buffer_index];

        ID3D12CommandAllocator* command_allocator = m_command_allocators[m_buffer_index].Get();
        ThrowIfFailed(command_allocator->Reset());
        ThrowIfFailed(m_command_list->Reset(command_allocator, m_pipeline_state.Get()));

        const ddn::Window& window = GetWindow();
        const uint32_t width = window.GetWidth();
        const uint32_t height = window.GetHeight();

        D3D12_VIEWPORT viewport = { 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height) };
        m_command_list->RSSetViewports(1, &viewport);

        D3D12_RECT scissor_rect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
        m_command_list->RSSetScissorRects(1, &scissor_rect);

        auto barrier1 = CD3DX12_RESOURCE_BARRIER::Transition(back_buffer.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        m_command_list->ResourceBarrier(1, &barrier1);

        const float color[4] = { 0.96f,  0.96f, 0.98f, 1.0f };
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(m_descriptor_heap->GetCPUDescriptorHandleForHeapStart(), m_buffer_index, m_rtv_descriptor_size);
        m_command_list->ClearRenderTargetView(rtv_handle, color, 0, nullptr);

        m_command_list->SetGraphicsRootSignature(m_root_signature.Get());
        m_command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_command_list->IASetVertexBuffers(0, 1, &m_vertex_buffer_view);
        m_command_list->OMSetRenderTargets(1, &rtv_handle, FALSE, nullptr);
        m_command_list->DrawInstanced(3, 1, 0, 0);

        auto barrier2 = CD3DX12_RESOURCE_BARRIER::Transition(back_buffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        m_command_list->ResourceBarrier(1, &barrier2);

        m_command_list->Close();

        ID3D12CommandList* command_lists[] = { m_command_list.Get() };
        m_queue->ExecuteCommandLists(static_cast<UINT>(std::size(command_lists)), command_lists);

        m_buffer_fence_values[m_buffer_index] = Signal();

        ThrowIfFailed(m_swap_chain->Present(0, 0));

        m_buffer_index = m_swap_chain->GetCurrentBackBufferIndex();
        Wait(m_buffer_fence_values[m_buffer_index]);
    }

    void OnDestroy() override
    {
        Flush();
    }

private:
    void InitDevice()
    {
        ThrowIfFailed(CreateDXGIFactory2(0, IID_PPV_ARGS(&m_factory)));
        ThrowIfFailed(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)));
    }

    void InitCommandList()
    {
        D3D12_COMMAND_QUEUE_DESC queue_desc = {};
        queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

        ThrowIfFailed(m_device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&m_queue)));

        for (UINT i = 0; i < s_back_buffer_count; ++i) {
            ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_command_allocators[i])));
        }

        ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_command_allocators.front().Get(), nullptr, IID_PPV_ARGS(&m_command_list)));
        ThrowIfFailed(m_command_list->Close());
    }

    void InitSwapChain()
    {
        const ddn::Window& window = GetWindow();
        const HWND window_handle = window.GetHandle();

        ComPtr<IDXGISwapChain1> swap_chain;
        DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
        swap_chain_desc.BufferCount = s_back_buffer_count;
        swap_chain_desc.Width = window.GetWidth();
        swap_chain_desc.Height = window.GetHeight();
        swap_chain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swap_chain_desc.SampleDesc.Count = 1;
        ThrowIfFailed(m_factory->CreateSwapChainForHwnd(m_queue.Get(), window_handle, &swap_chain_desc, nullptr, nullptr, &swap_chain));
        ThrowIfFailed(swap_chain.As(&m_swap_chain));

        m_buffer_index = m_swap_chain->GetCurrentBackBufferIndex();
    }

    void InitRtvDescriptorHeap()
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.NumDescriptors = s_back_buffer_count;
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        ThrowIfFailed(m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_descriptor_heap)));

        m_rtv_descriptor_size = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        UpdateBackBufferViews();
    }

    void InitRootSignature()
    {
        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc = {};
        desc.Init_1_1(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        ComPtr<ID3DBlob> blob;
        ComPtr<ID3DBlob> error;
        ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1_1, &blob, &error));
        ThrowIfFailed(m_device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&m_root_signature)));
    }

    void InitGraphicsPipelineState()
    {
        ComPtr<ID3DBlob> vertex_shader = CompileShader(s_shader_code, "VSMain", "vs_5_1");
        ComPtr<ID3DBlob> pixel_shader = CompileShader(s_shader_code, "PSMain", "ps_5_1");

        std::array<D3D12_INPUT_ELEMENT_DESC, 2> input_descs = {};
        input_descs[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, position), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA };
        input_descs[1] = { "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, color),    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA };

        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
        desc.pRootSignature = m_root_signature.Get();
        desc.VS = CD3DX12_SHADER_BYTECODE(vertex_shader.Get());
        desc.PS = CD3DX12_SHADER_BYTECODE(pixel_shader.Get());
        desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
        desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        desc.InputLayout = { input_descs.data(), static_cast<UINT>(std::size(input_descs)) };
        desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        desc.NumRenderTargets = 1;
        desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc = { 1, 0 };
        ThrowIfFailed(m_device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&m_pipeline_state)));
    }

    void InitFence()
    {
        ThrowIfFailed(m_device->CreateFence(m_fence_value, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
        m_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    }

    void InitVertexBuffer()
    {
        std::array<Vertex, 3> vertices = {};
        vertices[0] = { { 0.5f, -0.5f, 1.0f }, { 0.00f, 0.66f, 1.00f } };
        vertices[1] = { {-0.5f, -0.5f, 1.0f }, { 0.00f, 0.66f, 1.00f } };
        vertices[2] = { { 0.0f,  0.5f, 1.0f }, { 0.61f, 0.53f, 1.00f } };

        const size_t data_size = sizeof(vertices);

        ComPtr<ID3D12Resource> upload_resource;
        auto upload_heap_properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto upload_resource_desc = CD3DX12_RESOURCE_DESC::Buffer(data_size);
        ThrowIfFailed(m_device->CreateCommittedResource(&upload_heap_properties, D3D12_HEAP_FLAG_NONE, &upload_resource_desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&upload_resource)));

        void* data = nullptr;
        ThrowIfFailed(upload_resource->Map(0, &CD3DX12_RANGE(), &data));
        std::copy(vertices.cbegin(), vertices.cend(), static_cast<Vertex*>(data));
        upload_resource->Unmap(0, nullptr);

        auto gpu_heap_properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        auto gpu_resource_desc = CD3DX12_RESOURCE_DESC::Buffer(data_size);
        ThrowIfFailed(m_device->CreateCommittedResource(&gpu_heap_properties, D3D12_HEAP_FLAG_NONE, &gpu_resource_desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_vertex_buffer)));

        m_vertex_buffer_view.BufferLocation = m_vertex_buffer->GetGPUVirtualAddress();
        m_vertex_buffer_view.SizeInBytes = data_size;
        m_vertex_buffer_view.StrideInBytes = sizeof(Vertex);

        m_command_list->Reset(m_command_allocators[m_buffer_index].Get(), nullptr);
        m_command_list->CopyResource(m_vertex_buffer.Get(), upload_resource.Get());
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_vertex_buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        m_command_list->ResourceBarrier(1, &barrier);
        m_command_list->Close();

        ID3D12CommandList* command_lists[] = { m_command_list.Get() };
        m_queue->ExecuteCommandLists(static_cast<UINT>(std::size(command_lists)), command_lists);

        Flush();
    }

    ComPtr<ID3DBlob> CompileShader(const std::string& source, const std::string& entry_point, const std::string& compiler_target)
    {
        DWORD compilerFlags = 0;
#ifdef _DEBUG
        compilerFlags = D3DCOMPILE_DEBUG;
#endif

        ComPtr<ID3DBlob> shader;
        ComPtr<ID3DBlob> error;
        auto hr = D3DCompile(source.data(), source.size(), nullptr, nullptr, nullptr, entry_point.c_str(), compiler_target.c_str(), 0, 0, &shader, &error);

        if (FAILED(hr)) {
            std::wostringstream oss;
            oss << "Compilation error: " << static_cast<char*>(error->GetBufferPointer()) << std::endl;
            OutputDebugString(oss.str().c_str());
        }

        ThrowIfFailed(hr);

        return shader;
    }

    uint64_t Signal()
    {
        ++m_fence_value;
        ThrowIfFailed(m_queue->Signal(m_fence.Get(), m_fence_value));
        return m_fence_value;
    }

    void Wait(uint64_t value)
    {
        if (m_fence->GetCompletedValue() < value) {
            ThrowIfFailed(m_fence->SetEventOnCompletion(value, m_event));
            WaitForSingleObject(m_event, INFINITE);
        }
    }

    void Flush()
    {
        auto value = Signal();
        Wait(value);
    }

    void UpdateBackBufferViews()
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(m_descriptor_heap->GetCPUDescriptorHandleForHeapStart());
        for (UINT i = 0; i < s_back_buffer_count; ++i) {
            ThrowIfFailed(m_swap_chain->GetBuffer(i, IID_PPV_ARGS(&m_back_buffers[i])));
            m_device->CreateRenderTargetView(m_back_buffers[i].Get(), nullptr, rtv_handle);
            rtv_handle.Offset(1, m_rtv_descriptor_size);
        }
    }

private:
    struct Vertex
    {
        float position[3];
        float color[3];
    };

private:
    static constexpr UINT s_back_buffer_count = 2;

    ComPtr<IDXGIFactory4> m_factory;
    ComPtr<ID3D12Device> m_device;

    ComPtr<ID3D12CommandQueue> m_queue;
    ComPtr<ID3D12GraphicsCommandList> m_command_list;
    std::array<ComPtr<ID3D12CommandAllocator>, s_back_buffer_count> m_command_allocators;

    ComPtr<IDXGISwapChain3> m_swap_chain;
    std::array<ComPtr<ID3D12Resource>, s_back_buffer_count> m_back_buffers;
    UINT m_buffer_index = 0;

    ComPtr<ID3D12DescriptorHeap> m_descriptor_heap;
    UINT m_rtv_descriptor_size = 0;

    ComPtr<ID3D12RootSignature> m_root_signature;
    ComPtr<ID3D12PipelineState> m_pipeline_state;

    ComPtr<ID3D12Fence> m_fence;
    std::array<uint64_t, s_back_buffer_count> m_buffer_fence_values = {};
    uint64_t m_fence_value = 0;
    HANDLE m_event = nullptr;

    ComPtr<ID3D12Resource> m_vertex_buffer;
    D3D12_VERTEX_BUFFER_VIEW m_vertex_buffer_view = {};
};

int main()
{
    DandelionApp app(L"3Dandelion", 800, 600);
    return app.Run();
}
