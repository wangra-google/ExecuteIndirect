//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#pragma once

#include "DXSample.h"

using namespace DirectX;

// Note that while ComPtr is used to manage the lifetime of resources on the CPU,
// it has no understanding of the lifetime of resources on the GPU. Apps must account
// for the GPU lifetime of resources to avoid destroying objects that may still be
// referenced by the GPU.
// An example of this can be found in the class method: OnDestroy().
using Microsoft::WRL::ComPtr;

class D3D12ExecuteIndirect : public DXSample
{
public:
    D3D12ExecuteIndirect(UINT width, UINT height, std::wstring name);

    virtual void OnInit();
    virtual void OnUpdate();
    virtual void OnRender();
    virtual void OnDestroy();
    virtual void OnKeyDown(UINT8 key);

private:
    static const UINT FrameCount = 3;
    static const UINT TriangleCount = 1024;
    static const UINT TriangleResourceCount = TriangleCount * FrameCount;
    static const UINT CommandSizePerFrame;                // The size of the indirect commands to draw all of the triangles in a single frame.
    static const UINT CommandBufferCounterOffset;        // The offset of the UAV counter in the processed command buffer.
    static const UINT ComputeThreadBlockSize = 128;        // Should match the value in compute.hlsl.
    static const float TriangleHalfWidth;                // The x and y offsets used by the triangle vertices.
    static const float TriangleDepth;                    // The z offset used by the triangle vertices.
    static const float CullingCutoff;                    // The +/- x offset of the clipping planes in homogenous space [-1,1].

    // Vertex definition.
    struct Vertex
    {
		XMFLOAT3 position;
		XMFLOAT2 texcoord;
    };

    // Constant buffer definition.
    struct SceneConstantBuffer
    {
        XMFLOAT4 velocity;
        XMFLOAT4 offset;
        XMFLOAT4 color;
        XMFLOAT4X4 projection;
        // Constant buffers are 256-byte aligned. Add padding in the struct to allow multiple buffers
        // to be array-indexed.
        float padding[36];
    };

	struct IndirectArgsConstantBuffer
	{
		D3D12_GPU_VIRTUAL_ADDRESS   srvAddress[2];
        D3D12_INDEX_BUFFER_VIEW     ib[2];
        D3D12_VERTEX_BUFFER_VIEW    vb[2];
		XMFLOAT4                    rootConstants[2];
        uint32_t                    indexCountPerInstance[2];
		float padding[34];
	};

    // Root constants for the compute shader.
    struct CSRootConstants
    {
        float xOffset;
        float zOffset;
        float cullOffset;
        float commandCount;
    };

    // Data structure to match the command signature used for ExecuteIndirect.
    struct IndirectCommand
    {
		D3D12_GPU_VIRTUAL_ADDRESS       cbv;
		D3D12_GPU_VIRTUAL_ADDRESS       srv;
        D3D12_INDEX_BUFFER_VIEW         ib;
        D3D12_VERTEX_BUFFER_VIEW        vb;
		XMFLOAT4                        rootConstants;
		D3D12_DRAW_INDEXED_ARGUMENTS    drawArguments;
        // RW: compiler will auto pad this if this is not here (to 88 bytes)
        // the command would work since commandSignatureDesc.ByteStride = sizeof(IndirectCommand);
        // no need to change shader to match the padding, but renderdoc would show wrong result without padding
		float padding;
	};

    // Graphics root signature parameter offsets.
    enum GraphicsRootParameters
    {
		Cbv,
		Srv,
		RootConst,
		GraphicsRootParametersCount
    };

    // Compute root signature parameter offsets.
    enum ComputeRootParameters
    {
        SrvUavTable,
        RootConstants,            // Root constants that give the shader information about the triangle vertices and culling planes.
		IndirectArgsCbv,
		ComputeRootParametersCount
    };

    // CBV/SRV/UAV descriptor heap offsets.
    enum HeapOffsets
    {
        CbvSrvOffset = 0,                                                   // SRV that points to the constant buffers used by the rendering thread.
        CommandsOffset = CbvSrvOffset + 1,                                  // SRV that points to all of the indirect commands.
		ProcessedCommandsOffset = CommandsOffset + 1,                       // UAV that records the commands we actually want to execute.
        CbvSrvUavDescriptorCountPerFrame = ProcessedCommandsOffset + 1        // 2 SRVs + 1 UAV for the compute shader.
    };

    // Each triangle gets its own constant buffer per frame.
    std::vector<SceneConstantBuffer> m_constantBufferData;
    UINT8* m_pCbvDataBegin;

    CSRootConstants m_csRootConstants;    // Constants for the compute shader.
    bool m_enableCulling;                // Toggle whether the compute shader pre-processes the indirect commands.

    // Pipeline objects.
    CD3DX12_VIEWPORT m_viewport;
    CD3DX12_RECT m_scissorRect;
    D3D12_RECT m_cullingScissorRect;
    ComPtr<IDXGISwapChain3> m_swapChain;
    ComPtr<ID3D12Device> m_device;
    ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
    ComPtr<ID3D12CommandAllocator> m_commandAllocators[FrameCount];
    ComPtr<ID3D12CommandAllocator> m_computeCommandAllocators[FrameCount];
    ComPtr<ID3D12CommandQueue> m_commandQueue;
    ComPtr<ID3D12CommandQueue> m_computeCommandQueue;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12RootSignature> m_computeRootSignature;
    ComPtr<ID3D12CommandSignature> m_commandSignature;
    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
    ComPtr<ID3D12DescriptorHeap> m_cbvSrvUavHeap;
    UINT m_rtvDescriptorSize;
    UINT m_cbvSrvUavDescriptorSize;
    UINT m_frameIndex;

    // Synchronization objects.
    ComPtr<ID3D12Fence> m_fence;
    ComPtr<ID3D12Fence> m_computeFence;
    UINT64 m_fenceValues[FrameCount];
    HANDLE m_fenceEvent;

    // Asset objects.
    ComPtr<ID3D12PipelineState> m_pipelineState;
	ComPtr<ID3D12PipelineState> m_computeState;
	ComPtr<ID3D12PipelineState> m_textureState;
    ComPtr<ID3D12GraphicsCommandList> m_commandList;
    ComPtr<ID3D12GraphicsCommandList> m_computeCommandList;
	ComPtr<ID3D12Resource> m_constantBuffer;
	ComPtr<ID3D12Resource> m_indirectArgsConstantBuffer;
    ComPtr<ID3D12Resource> m_depthStencil;
    ComPtr<ID3D12Resource> m_commandBuffer;
    ComPtr<ID3D12Resource> m_processedCommandBuffers[FrameCount];
    ComPtr<ID3D12Resource> m_processedCommandBufferCounterReset;

	const static uint32_t kAssetCount = 2;
	struct MeshData
    {
		ComPtr<ID3D12Resource>      vertexBuffer;
		D3D12_VERTEX_BUFFER_VIEW    vertexBufferView;
		ComPtr<ID3D12Resource>      indexBuffer;
		D3D12_INDEX_BUFFER_VIEW     indexBufferView;
    };

	MeshData m_mesh[kAssetCount]; // 0: quad, 1: triangle

    struct TextureData
    {
		ComPtr<ID3D12Resource> texture;
		ComPtr<ID3D12Resource> buffer;
		uint32_t heapOffset;
		uint32_t width;
		uint32_t height;
    };
    TextureData m_textureData[kAssetCount];

	ComPtr<ID3D12RootSignature> m_textureRootSignature;
    uint32_t m_indirectArgsCbvHeapOffset;


    void LoadPipeline();
    void LoadAssets();
    void RestoreD3DResources();
    void ReleaseD3DResources();
    float GetRandomFloat(float min, float max);
    void PopulateCommandLists();
    void WaitForGpu();
    void MoveToNextFrame();

    // We pack the UAV counter into the same buffer as the commands rather than create
    // a separate 64K resource/heap for it. The counter must be aligned on 4K boundaries,
    // so we pad the command buffer (if necessary) such that the counter will be placed
    // at a valid location in the buffer.
    static inline UINT AlignForUavCounter(UINT bufferSize)
    {
        const UINT alignment = D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT;
        return (bufferSize + (alignment - 1)) & ~(alignment - 1);
    }
};
