#include "DirectX12_pch.h"

#include "CommandContext.h"
#include <stdexcept>

void CommandContext::Initialize(ID3D12Device* device, ID3D12CommandQueue* commandQueue, ID3D12CommandAllocator* initAllocator) {
    m_CommandQueue = commandQueue;

    // コマンドリストの作成（初期状態は閉じた状態にするため、一度ダミーのコマンドアロケータで作成して閉じる）
    ComPtr<ID3D12CommandAllocator> tempAllocator;
    device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&tempAllocator));

    device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, initAllocator, nullptr, IID_PPV_ARGS(&m_CommandList));
    m_CommandList->Close();
    //device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, tempAllocator.Get(), nullptr, IID_PPV_ARGS(&m_CommandList));
    //m_CommandList->Close();
}

void CommandContext::BeginFrame(ID3D12CommandAllocator* allocator) {
    // 前のフレームで使用したアロケータをリセットし、コマンドリストの記録を再開
    if (!allocator) {
        throw std::runtime_error("CommandContext::BeginFrame: allocator is null");
    }
    HRESULT hr = allocator->Reset();
    if (FAILED(hr)) {
        throw std::runtime_error("CommandContext::BeginFrame: allocator->Reset failed");
    }

    if (!m_CommandList) {
        throw std::runtime_error("CommandContext::BeginFrame: m_CommandList is null");
    }
    hr = m_CommandList->Reset(allocator, nullptr);
    if (FAILED(hr)) {
        throw std::runtime_error("CommandContext::BeginFrame: m_CommandList->Reset failed");
    }
}

void CommandContext::EndFrame() {
    FlushResourceBarriers(); // 張り忘れたバリアがあればここで流す
    m_CommandList->Close();

    // コマンドキューに積んでGPUに実行させる
    ID3D12CommandList* ppCommandLists[] = { m_CommandList.Get() };
    m_CommandQueue->ExecuteCommandLists(1, ppCommandLists);
}

void CommandContext::TransitionResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter) {
    if (stateBefore == stateAfter) return;

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = resource;
    barrier.Transition.StateBefore = stateBefore;
    barrier.Transition.StateAfter = stateAfter;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    m_ResourceBarriers.push_back(barrier);

    // バリアは溜め込まずにすぐフラッシュする（まとめてフラッシュする方が高パフォーマンス）
    FlushResourceBarriers();
}

void CommandContext::FlushResourceBarriers() {
    if (m_ResourceBarriers.empty()) return;

    m_CommandList->ResourceBarrier(static_cast<UINT>(m_ResourceBarriers.size()), m_ResourceBarriers.data());
    m_ResourceBarriers.clear();
}

void CommandContext::SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE* rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE* dsvHandle) {
    m_CommandList->OMSetRenderTargets(1, rtvHandle, FALSE, dsvHandle);
}

void CommandContext::ClearColor(D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, const float color[4]) {
    m_CommandList->ClearRenderTargetView(rtvHandle, color, 0, nullptr);
}

void CommandContext::ClearDepth(D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle)
{
    m_CommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

void CommandContext::SetViewportAndScissor(uint32_t width, uint32_t height)
{
    D3D12_VIEWPORT viewport = { 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f };
    D3D12_RECT scissor = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };

    m_CommandList->RSSetViewports(1, &viewport);
    m_CommandList->RSSetScissorRects(1, &scissor);
}

void CommandContext::SetRootSignature(ID3D12RootSignature* rootSig)
{
    m_CommandList->SetGraphicsRootSignature(rootSig);
}

void CommandContext::SetPipelineState(ID3D12PipelineState* pso)
{
    m_CommandList->SetPipelineState(pso);
}

void CommandContext::SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY topology)
{
	m_CommandList->IASetPrimitiveTopology(topology);
}

void CommandContext::DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertex, uint32_t startInstance)
{
    m_CommandList->DrawInstanced(vertexCount, instanceCount, startVertex, startInstance);
}

void CommandContext::DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndex, int32_t baseVertex, uint32_t startInstance)
{
    m_CommandList->DrawIndexedInstanced(indexCount, instanceCount, startIndex, baseVertex, startInstance);
}