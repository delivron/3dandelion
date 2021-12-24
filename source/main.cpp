#include "cube.h"
#include "utils.h"
#include "camera.h"
#include "application.h"

#include "swap-chain.h"
#include "command-queue.h"

#include <directx/d3dx12.h>

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <array>
#include <vector>
#include <memory>
#include <chrono>

using namespace ddn;
using namespace Microsoft::WRL;

class DandelionApp : public Application
{
public:
    DandelionApp(const std::wstring& title, uint32_t width, uint32_t height)
        : Application(title, width, height)
        , m_camera([width, height]() {
            auto camera = Camera(45.0f, static_cast<float>(width) / height, 0.1f, 100.0f);
            camera.SetPosition(glm::vec3(0.0f, 0.0f, -10.0));
            return camera;
        }())
        , m_model_matrix(1.0f)
    {
        InitDevice();
        InitCommandQueue();
        InitSwapChain();
        InitRtvDescriptorHeap();
        InitDsvDescriptorHeap();
        InitRootSignature();
        InitGraphicsPipelineState();
        InitVertexBuffer();
        InitIndexBuffer();

        m_last_time = std::chrono::steady_clock::now();;
    }

    void OnResize(uint32_t width, uint32_t height) override
    {
        m_camera.SetAspect(static_cast<float>(width) / height);

        m_swap_chain->Resize(width, height);

        UpdateBackBufferViews();
        UpdateDepthBuffer(width, height);
    }

    void OnUpdate() override
    {
        auto time = std::chrono::steady_clock::now();
        auto delta_time_s = std::chrono::duration<float>(time - m_last_time);
        auto delta_angle = glm::radians(s_angular_rate_deg * delta_time_s.count());
        m_model_matrix = glm::rotate(m_model_matrix, delta_angle, glm::vec3(0.0f, 1.0f, 0.0f));

        m_last_time = std::chrono::steady_clock::now();

        const auto buffer_index = m_swap_chain->GetCurrentBackBufferIndex();
        ComPtr<ID3D12Resource> back_buffer = m_swap_chain->GetCurrentBackBuffer();

        ID3D12CommandAllocator* command_allocator = m_command_allocators[buffer_index].Get();
        ValidateResult(command_allocator->Reset());
        ValidateResult(m_command_list->Reset(command_allocator, m_pipeline_state.Get()));

        const Window& window = GetWindow();
        const uint32_t width = window.GetWidth();
        const uint32_t height = window.GetHeight();

        auto viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
        m_command_list->RSSetViewports(1, &viewport);

        auto scissor_rect = CD3DX12_RECT(0, 0, static_cast<LONG>(width), static_cast<LONG>(height));
        m_command_list->RSSetScissorRects(1, &scissor_rect);

        auto barrier1 = CD3DX12_RESOURCE_BARRIER::Transition(back_buffer.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        m_command_list->ResourceBarrier(1, &barrier1);

        const std::array<float, 4> color = { 0.96f, 0.96f, 0.98f, 1.0f };
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(m_rtv_descriptor_heap->GetCPUDescriptorHandleForHeapStart(), buffer_index, m_rtv_descriptor_size);
        m_command_list->ClearRenderTargetView(rtv_handle, color.data(), 0, nullptr);

        auto dsv_handle = m_dsv_descriptor_heap->GetCPUDescriptorHandleForHeapStart();
        m_command_list->ClearDepthStencilView(dsv_handle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

        m_command_list->SetGraphicsRootSignature(m_root_signature.Get());

        const auto camera_matrix = m_camera.GetProjectionViewMatrix();
        const auto mvp_matrix = camera_matrix * m_model_matrix;
        m_command_list->SetGraphicsRoot32BitConstants(0, sizeof(glm::mat4) / sizeof(float), &mvp_matrix, 0);

        m_command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_command_list->IASetVertexBuffers(0, 1, &m_vertex_buffer_view);
        m_command_list->IASetIndexBuffer(&m_index_buffer_view);
        m_command_list->OMSetRenderTargets(1, &rtv_handle, false, &dsv_handle);

        auto index_count = static_cast<UINT>(m_cube.GetIndexCount());
        m_command_list->DrawIndexedInstanced(index_count, 1, 0, 0, 0);

        auto barrier2 = CD3DX12_RESOURCE_BARRIER::Transition(back_buffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        m_command_list->ResourceBarrier(1, &barrier2);

        m_command_list->Close();

        m_command_queue->Clear();
        m_command_queue->Add(m_command_list);
        m_command_queue->Execute();

        m_swap_chain->Present();
    }

    void OnDestroy() override
    {
        m_command_queue->Flush();
    }

private:
    void InitDevice()
    {
        m_factory = CreateFactory();
        auto adapter = GetAdapter(*m_factory.Get(), DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE);
        ValidateResult(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)));
    }

    void InitCommandQueue()
    {
        m_command_queue = std::make_unique<CommandQueue>(*m_device.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT);

        m_command_allocators.resize(s_back_buffer_count);
        for (auto& command_allocator : m_command_allocators) {
            ValidateResult(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&command_allocator)));
        }

        ValidateResult(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_command_allocators.front().Get(), nullptr, IID_PPV_ARGS(&m_command_list)));
        ValidateResult(m_command_list->Close());
    }

    void InitSwapChain()
    {
        m_swap_chain = std::make_unique<SwapChain>(*m_factory.Get(), *m_command_queue, GetWindow(), s_back_buffer_count);
    }

    void InitRtvDescriptorHeap()
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.NumDescriptors = m_swap_chain->GetBackBufferCount();
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        ValidateResult(m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_rtv_descriptor_heap)));

        m_rtv_descriptor_size = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        UpdateBackBufferViews();
    }

    void InitDsvDescriptorHeap()
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.NumDescriptors = 1;
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        ValidateResult(m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_dsv_descriptor_heap)));

        m_dsv_descriptor_size = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

        const Window& window = GetWindow();
        UpdateDepthBuffer(window.GetWidth(), window.GetHeight());
    }

    void InitRootSignature()
    {
        CD3DX12_ROOT_PARAMETER1 parameter = {};
        parameter.InitAsConstants(sizeof(glm::mat4) / sizeof(float), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc = {};
        desc.Init_1_1(1, &parameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        ComPtr<ID3DBlob> blob;
        ComPtr<ID3DBlob> error;
        ValidateResult(D3DX12SerializeVersionedRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1_1, &blob, &error));
        ValidateResult(m_device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&m_root_signature)));
    }

    void InitGraphicsPipelineState()
    {
        const auto shader_path = std::filesystem::current_path() / "shaders" / "main.hlsl";
        ComPtr<ID3DBlob> vertex_shader = CompileShader(shader_path, "VSMain", "vs_5_1");
        ComPtr<ID3DBlob> pixel_shader = CompileShader(shader_path, "PSMain", "ps_5_1");

        std::array<D3D12_INPUT_ELEMENT_DESC, 2> input_descs = {};
        input_descs[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(VertexData, position), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA };
        input_descs[1] = { "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(VertexData, color),    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA };

        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
        desc.pRootSignature = m_root_signature.Get();
        desc.VS = CD3DX12_SHADER_BYTECODE(vertex_shader.Get());
        desc.PS = CD3DX12_SHADER_BYTECODE(pixel_shader.Get());
        desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
        desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        desc.InputLayout = { input_descs.data(), static_cast<UINT>(std::size(input_descs)) };
        desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        desc.NumRenderTargets = 1;
        desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        desc.SampleDesc = { 1, 0 };
        ValidateResult(m_device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&m_pipeline_state)));
    }

    void InitVertexBuffer()
    {
        auto vertices = m_cube.GetVertices();
        ComPtr<ID3D12Resource> upload_resource = CreateBuffer(*m_device.Get(), vertices.size(), D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
        m_vertex_buffer = CreateBuffer(*m_device.Get(), vertices.size(), D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COPY_DEST);

        m_vertex_buffer_view.BufferLocation = m_vertex_buffer->GetGPUVirtualAddress();
        m_vertex_buffer_view.SizeInBytes = static_cast<UINT>(vertices.size());
        m_vertex_buffer_view.StrideInBytes = static_cast<UINT>(m_cube.GetVertexSize());

        D3D12_SUBRESOURCE_DATA subresource_data = {};
        subresource_data.pData = vertices.data();
        subresource_data.RowPitch = vertices.size();
        subresource_data.SlicePitch = subresource_data.RowPitch;

        auto buffer_index = m_swap_chain->GetCurrentBackBufferIndex();
        m_command_list->Reset(m_command_allocators[buffer_index].Get(), nullptr);
        UpdateSubresources(m_command_list.Get(), m_vertex_buffer.Get(), upload_resource.Get(), 0, 0, 1, &subresource_data);
        m_command_list->Close();

        m_command_queue->Clear();
        m_command_queue->Add(m_command_list);
        m_command_queue->Execute();
        m_command_queue->Flush();
    }

    void InitIndexBuffer()
    {
        auto indexes = m_cube.GetIndexes();
        ComPtr<ID3D12Resource> upload_resource = CreateBuffer(*m_device.Get(), indexes.size(), D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
        m_index_buffer = CreateBuffer(*m_device.Get(), indexes.size(), D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COPY_DEST);

        m_index_buffer_view.BufferLocation = m_index_buffer->GetGPUVirtualAddress();
        m_index_buffer_view.SizeInBytes = static_cast<UINT>(indexes.size());
        m_index_buffer_view.Format = DXGI_FORMAT_R16_UINT;

        D3D12_SUBRESOURCE_DATA subresource_data = {};
        subresource_data.pData = indexes.data();
        subresource_data.RowPitch = indexes.size();
        subresource_data.SlicePitch = subresource_data.RowPitch;

        auto buffer_index = m_swap_chain->GetCurrentBackBufferIndex();
        m_command_list->Reset(m_command_allocators[buffer_index].Get(), nullptr);
        UpdateSubresources(m_command_list.Get(), m_index_buffer.Get(), upload_resource.Get(), 0, 0, 1, &subresource_data);
        m_command_list->Close();

        m_command_queue->Clear();
        m_command_queue->Add(m_command_list);
        m_command_queue->Execute();
        m_command_queue->Flush();
    }

    void UpdateBackBufferViews()
    {
        auto rtv_handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtv_descriptor_heap->GetCPUDescriptorHandleForHeapStart());
        for (uint32_t i = 0; i < m_swap_chain->GetBackBufferCount(); ++i) {
            auto back_buffer = m_swap_chain->GetBackBuffer(i);
            m_device->CreateRenderTargetView(back_buffer.Get(), nullptr, rtv_handle);
            rtv_handle.Offset(1, m_rtv_descriptor_size);
        }
    }

    void UpdateDepthBuffer(uint32_t width, uint32_t height)
    {
        if (width == 0 || height == 0) {
            return;
        }

        auto heap_properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        auto desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, width, height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
        auto clear_value = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D32_FLOAT, 1.0f, 0);
        ValidateResult(m_device->CreateCommittedResource(&heap_properties, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clear_value, IID_PPV_ARGS(&m_depth_resource)));

        D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
        dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        m_device->CreateDepthStencilView(m_depth_resource.Get(), &dsv_desc, m_dsv_descriptor_heap->GetCPUDescriptorHandleForHeapStart());
    }

private:
    static constexpr uint32_t s_back_buffer_count = 2;
    static constexpr float s_angular_rate_deg = 45.0;

    ComPtr<IDXGIFactory6> m_factory;
    ComPtr<ID3D12Device> m_device;

    ComPtr<ID3D12GraphicsCommandList> m_command_list;
    std::vector<ComPtr<ID3D12CommandAllocator>> m_command_allocators;

    ComPtr<ID3D12DescriptorHeap> m_rtv_descriptor_heap;
    UINT m_rtv_descriptor_size = 0;

    ComPtr<ID3D12DescriptorHeap> m_dsv_descriptor_heap;
    UINT m_dsv_descriptor_size = 0;

    ComPtr<ID3D12RootSignature> m_root_signature;
    ComPtr<ID3D12PipelineState> m_pipeline_state;

    ComPtr<ID3D12Resource> m_vertex_buffer;
    D3D12_VERTEX_BUFFER_VIEW m_vertex_buffer_view = {};

    ComPtr<ID3D12Resource> m_index_buffer;
    D3D12_INDEX_BUFFER_VIEW m_index_buffer_view = {};

    ComPtr<ID3D12Resource> m_depth_resource;

    std::unique_ptr<CommandQueue> m_command_queue;
    std::unique_ptr<SwapChain> m_swap_chain;

    Camera m_camera;
    Cube m_cube;

    glm::mat4 m_model_matrix;

    std::chrono::steady_clock::time_point m_last_time = {};
};

int main()
{
    DandelionApp app(L"3Dandelion", 800, 600);
    return app.Run();
}
