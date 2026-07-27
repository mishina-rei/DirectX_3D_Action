#include "DirectX12_pch.h"

#include "CommandContext.h"
#include <stdexcept>

void CommandContext::Initialize(ID3D12Device* device, ID3D12CommandQueue* commandQueue, ID3D12CommandAllocator* initAllocator) {
    this->commandQueue = commandQueue;

    // コマンドリストの作成（初期状態は閉じた状態にするため、一度ダミーのコマンドアロケータで作成して閉じる）
    ComPtr<ID3D12CommandAllocator> tempAllocator;
    device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&tempAllocator));

    device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, initAllocator, nullptr, IID_PPV_ARGS(&commandList));
    commandList->Close();
    //device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, tempAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList));
    //commandList->Close();
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

    if (!commandList) {
        throw std::runtime_error("CommandContext::BeginFrame: commandList is null");
    }
    hr = commandList->Reset(allocator, nullptr);
    if (FAILED(hr)) {
        throw std::runtime_error("CommandContext::BeginFrame: commandList->Reset failed");
    }
}

void CommandContext::EndFrame() {
    FlushResourceBarriers(); // 張り忘れたバリアがあればここで流す
    commandList->Close();

    // コマンドキューに積んでGPUに実行させる
    ID3D12CommandList* ppCommandLists[] = { commandList.Get() };
    commandQueue->ExecuteCommandLists(1, ppCommandLists);
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

    resourceBarriers.push_back(barrier);

    // バリアは溜め込まずにすぐフラッシュする（まとめてフラッシュする方が高パフォーマンス）
    FlushResourceBarriers();
}

void CommandContext::FlushResourceBarriers() {
    if (resourceBarriers.empty()) return;

    commandList->ResourceBarrier(static_cast<UINT>(resourceBarriers.size()), resourceBarriers.data());
    resourceBarriers.clear();
}

void CommandContext::SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE* rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE* dsvHandle) {
    commandList->OMSetRenderTargets(1, rtvHandle, FALSE, dsvHandle);
}

void CommandContext::ClearColor(D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, const float color[4]) {
    commandList->ClearRenderTargetView(rtvHandle, color, 0, nullptr);
}

void CommandContext::ClearDepth(D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle)
{
    commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

void CommandContext::SetViewportAndScissor(uint32_t width, uint32_t height)
{
    D3D12_VIEWPORT viewport = { 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f };
    D3D12_RECT scissor = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };

    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissor);
}

void CommandContext::SetRootSignature(ID3D12RootSignature* rootSig)
{
    commandList->SetGraphicsRootSignature(rootSig);
}

void CommandContext::SetPipelineState(ID3D12PipelineState* pso)
{
    commandList->SetPipelineState(pso);
}

void CommandContext::SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY topology)
{
    commandList->IASetPrimitiveTopology(topology);
}

void CommandContext::DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertex, uint32_t startInstance)
{
    commandList->DrawInstanced(vertexCount, instanceCount, startVertex, startInstance);
}

void CommandContext::DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndex, int32_t baseVertex, uint32_t startInstance)
{
    commandList->DrawIndexedInstanced(indexCount, instanceCount, startIndex, baseVertex, startInstance);
}
void CommandContext::SetVertexBuffer(UINT slot, const D3D12_VERTEX_BUFFER_VIEW& vbView) {
    commandList->IASetVertexBuffers(slot, 1, &vbView);
}

void CommandContext::SetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW& ibView) {
    commandList->IASetIndexBuffer(&ibView);
}