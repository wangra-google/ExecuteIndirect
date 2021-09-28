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

#include "stdafx.h"
#include "D3D12ExecuteIndirect.h"
#include <wincodec.h>

const UINT D3D12ExecuteIndirect::CommandSizePerFrame = TriangleCount * sizeof(IndirectCommand);
const UINT D3D12ExecuteIndirect::CommandBufferCounterOffset = AlignForUavCounter(D3D12ExecuteIndirect::CommandSizePerFrame);
const float D3D12ExecuteIndirect::TriangleHalfWidth = 0.05f;
const float D3D12ExecuteIndirect::TriangleDepth = 1.0f;
const float D3D12ExecuteIndirect::CullingCutoff = 0.5f;

DXGI_FORMAT GetDXGIFormatFromWICFormat(const WICPixelFormatGUID& wicFormatGUID)
{
	if (wicFormatGUID == GUID_WICPixelFormat128bppRGBAFloat) return DXGI_FORMAT_R32G32B32A32_FLOAT;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBAHalf) return DXGI_FORMAT_R16G16B16A16_FLOAT;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBA) return DXGI_FORMAT_R16G16B16A16_UNORM;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppRGBA) return DXGI_FORMAT_R8G8B8A8_UNORM;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppBGRA) return DXGI_FORMAT_B8G8R8A8_UNORM;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppBGR) return DXGI_FORMAT_B8G8R8X8_UNORM;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppRGBA1010102XR) return DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM;

	else if (wicFormatGUID == GUID_WICPixelFormat32bppRGBA1010102) return DXGI_FORMAT_R10G10B10A2_UNORM;
	else if (wicFormatGUID == GUID_WICPixelFormat16bppBGRA5551) return DXGI_FORMAT_B5G5R5A1_UNORM;
	else if (wicFormatGUID == GUID_WICPixelFormat16bppBGR565) return DXGI_FORMAT_B5G6R5_UNORM;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppGrayFloat) return DXGI_FORMAT_R32_FLOAT;
	else if (wicFormatGUID == GUID_WICPixelFormat16bppGrayHalf) return DXGI_FORMAT_R16_FLOAT;
	else if (wicFormatGUID == GUID_WICPixelFormat16bppGray) return DXGI_FORMAT_R16_UNORM;
	else if (wicFormatGUID == GUID_WICPixelFormat8bppGray) return DXGI_FORMAT_R8_UNORM;
	else if (wicFormatGUID == GUID_WICPixelFormat8bppAlpha) return DXGI_FORMAT_A8_UNORM;

	else return DXGI_FORMAT_UNKNOWN;
}

WICPixelFormatGUID GetConvertToWICFormat(const WICPixelFormatGUID& wicFormatGUID)
{
	if (wicFormatGUID == GUID_WICPixelFormatBlackWhite) return GUID_WICPixelFormat8bppGray;
	else if (wicFormatGUID == GUID_WICPixelFormat1bppIndexed) return GUID_WICPixelFormat32bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat2bppIndexed) return GUID_WICPixelFormat32bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat4bppIndexed) return GUID_WICPixelFormat32bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat8bppIndexed) return GUID_WICPixelFormat32bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat2bppGray) return GUID_WICPixelFormat8bppGray;
	else if (wicFormatGUID == GUID_WICPixelFormat4bppGray) return GUID_WICPixelFormat8bppGray;
	else if (wicFormatGUID == GUID_WICPixelFormat16bppGrayFixedPoint) return GUID_WICPixelFormat16bppGrayHalf;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppGrayFixedPoint) return GUID_WICPixelFormat32bppGrayFloat;
	else if (wicFormatGUID == GUID_WICPixelFormat16bppBGR555) return GUID_WICPixelFormat16bppBGRA5551;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppBGR101010) return GUID_WICPixelFormat32bppRGBA1010102;
	else if (wicFormatGUID == GUID_WICPixelFormat24bppBGR) return GUID_WICPixelFormat32bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat24bppRGB) return GUID_WICPixelFormat32bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppPBGRA) return GUID_WICPixelFormat32bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppPRGBA) return GUID_WICPixelFormat32bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat48bppRGB) return GUID_WICPixelFormat64bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat48bppBGR) return GUID_WICPixelFormat64bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppBGRA) return GUID_WICPixelFormat64bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppPRGBA) return GUID_WICPixelFormat64bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppPBGRA) return GUID_WICPixelFormat64bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat48bppRGBFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
	else if (wicFormatGUID == GUID_WICPixelFormat48bppBGRFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBAFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppBGRAFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBHalf) return GUID_WICPixelFormat64bppRGBAHalf;
	else if (wicFormatGUID == GUID_WICPixelFormat48bppRGBHalf) return GUID_WICPixelFormat64bppRGBAHalf;
	else if (wicFormatGUID == GUID_WICPixelFormat128bppPRGBAFloat) return GUID_WICPixelFormat128bppRGBAFloat;
	else if (wicFormatGUID == GUID_WICPixelFormat128bppRGBFloat) return GUID_WICPixelFormat128bppRGBAFloat;
	else if (wicFormatGUID == GUID_WICPixelFormat128bppRGBAFixedPoint) return GUID_WICPixelFormat128bppRGBAFloat;
	else if (wicFormatGUID == GUID_WICPixelFormat128bppRGBFixedPoint) return GUID_WICPixelFormat128bppRGBAFloat;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppRGBE) return GUID_WICPixelFormat128bppRGBAFloat;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppCMYK) return GUID_WICPixelFormat32bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppCMYK) return GUID_WICPixelFormat64bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat40bppCMYKAlpha) return GUID_WICPixelFormat64bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat80bppCMYKAlpha) return GUID_WICPixelFormat64bppRGBA;

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8) || defined(_WIN7_PLATFORM_UPDATE)
	else if (wicFormatGUID == GUID_WICPixelFormat32bppRGB) return GUID_WICPixelFormat32bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppRGB) return GUID_WICPixelFormat64bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppPRGBAHalf) return GUID_WICPixelFormat64bppRGBAHalf;
#endif

	else return GUID_WICPixelFormatDontCare;
}

int GetDXGIFormatBitsPerPixel(const DXGI_FORMAT& dxgiFormat)
{
	if (dxgiFormat == DXGI_FORMAT_R32G32B32A32_FLOAT) return 128;
	else if (dxgiFormat == DXGI_FORMAT_R16G16B16A16_FLOAT) return 64;
	else if (dxgiFormat == DXGI_FORMAT_R16G16B16A16_UNORM) return 64;
	else if (dxgiFormat == DXGI_FORMAT_R8G8B8A8_UNORM) return 32;
	else if (dxgiFormat == DXGI_FORMAT_B8G8R8A8_UNORM) return 32;
	else if (dxgiFormat == DXGI_FORMAT_B8G8R8X8_UNORM) return 32;
	else if (dxgiFormat == DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM) return 32;

	else if (dxgiFormat == DXGI_FORMAT_R10G10B10A2_UNORM) return 32;
	else if (dxgiFormat == DXGI_FORMAT_B5G5R5A1_UNORM) return 16;
	else if (dxgiFormat == DXGI_FORMAT_B5G6R5_UNORM) return 16;
	else if (dxgiFormat == DXGI_FORMAT_R32_FLOAT) return 32;
	else if (dxgiFormat == DXGI_FORMAT_R16_FLOAT) return 16;
	else if (dxgiFormat == DXGI_FORMAT_R16_UNORM) return 16;
	else if (dxgiFormat == DXGI_FORMAT_R8_UNORM) return 8;
	else if (dxgiFormat == DXGI_FORMAT_A8_UNORM) return 8;
	return 0;
}

void LoadImageDataFromFile(std::vector<UINT8>& imageData, D3D12_RESOURCE_DESC& resourceDescription, const std::string& filename, int& bytesPerRow)
{
	static IWICImagingFactory* wicFactory;

	IWICBitmapDecoder* wicDecoder = nullptr;
	IWICBitmapFrameDecode* wicFrame = nullptr;
	IWICFormatConverter* wicConverter = nullptr;

	bool imageConverted = false;

	if (wicFactory == nullptr)
	{
		ThrowIfFailed(CoInitialize(nullptr));

		ThrowIfFailed(CoCreateInstance(
			CLSID_WICImagingFactory,
			nullptr,
			CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(&wicFactory)
		));

		ThrowIfFailed(wicFactory->CreateFormatConverter(&wicConverter));
	}

	std::wstring stemp = std::wstring(filename.begin(), filename.end());

	ThrowIfFailed(wicFactory->CreateDecoderFromFilename(
		stemp.c_str(),
		nullptr,
		GENERIC_READ,
		WICDecodeMetadataCacheOnLoad,
		&wicDecoder
	));

	ThrowIfFailed(wicDecoder->GetFrame(0, &wicFrame));

	WICPixelFormatGUID pixelFormat;
	ThrowIfFailed(wicFrame->GetPixelFormat(&pixelFormat));

	UINT textureWidth, textureHeight;
	ThrowIfFailed(wicFrame->GetSize(&textureWidth, &textureHeight));

	DXGI_FORMAT dxgiFormat = GetDXGIFormatFromWICFormat(pixelFormat);

	if (dxgiFormat == DXGI_FORMAT_UNKNOWN)
	{
		WICPixelFormatGUID convertToPixelFormat = GetConvertToWICFormat(pixelFormat);

		assert(convertToPixelFormat != GUID_WICPixelFormatDontCare);

		dxgiFormat = GetDXGIFormatFromWICFormat(convertToPixelFormat);

		BOOL canConvert = FALSE;
		ThrowIfFailed(wicConverter->CanConvert(pixelFormat, convertToPixelFormat, &canConvert));

		ThrowIfFailed(wicConverter->Initialize(wicFrame, convertToPixelFormat, WICBitmapDitherTypeErrorDiffusion, 0, 0, WICBitmapPaletteTypeCustom));

		imageConverted = true;
	}

	int bitsPerPixel = GetDXGIFormatBitsPerPixel(dxgiFormat);
	bytesPerRow = (textureWidth * bitsPerPixel) / 8;
	int imageSize = bytesPerRow * textureHeight;
	imageData.resize(imageSize);
	if (imageConverted)
	{
		ThrowIfFailed(wicConverter->CopyPixels(0, bytesPerRow, imageSize, &imageData[0]));
	}
	else
	{
		ThrowIfFailed(wicFrame->CopyPixels(0, bytesPerRow, imageSize, &imageData[0]));
	}

	resourceDescription = {};
	resourceDescription.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDescription.Alignment = 0;
	resourceDescription.Width = textureWidth;
	resourceDescription.Height = textureHeight;
	resourceDescription.DepthOrArraySize = 1;
	resourceDescription.MipLevels = 1;
	resourceDescription.Format = dxgiFormat;
	resourceDescription.SampleDesc.Count = 1;
	resourceDescription.SampleDesc.Quality = 0;
	resourceDescription.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resourceDescription.Flags = D3D12_RESOURCE_FLAG_NONE;
}

D3D12ExecuteIndirect::D3D12ExecuteIndirect(UINT width, UINT height, std::wstring name) :
    DXSample(width, height, name),
    m_frameIndex(0),
    m_viewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)),
    m_scissorRect(0, 0, static_cast<LONG>(width), static_cast<LONG>(height)),
    m_cullingScissorRect(),
    m_rtvDescriptorSize(0),
    m_cbvSrvUavDescriptorSize(0),
    m_csRootConstants(),
    m_enableCulling(true),
    m_fenceValues{}
{
    m_constantBufferData.resize(TriangleCount);

    m_csRootConstants.xOffset = TriangleHalfWidth;
    m_csRootConstants.zOffset = TriangleDepth;
    m_csRootConstants.cullOffset = CullingCutoff;
    m_csRootConstants.commandCount = TriangleCount;

    float center = width / 2.0f;
    m_cullingScissorRect.left = static_cast<LONG>(center - (center * CullingCutoff));
    m_cullingScissorRect.right = static_cast<LONG>(center + (center * CullingCutoff));
    m_cullingScissorRect.bottom = static_cast<LONG>(height);

    ThrowIfFailed(DXGIDeclareAdapterRemovalSupport());
}

void D3D12ExecuteIndirect::OnInit()
{
    LoadPipeline();
    LoadAssets();
}

// Load the rendering pipeline dependencies.
void D3D12ExecuteIndirect::LoadPipeline()
{
    UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
    // Enable the debug layer (requires the Graphics Tools "optional feature").
    // NOTE: Enabling the debug layer after device creation will invalidate the active device.
    {
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();

            // Enable additional debug layers.
            dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }
    }
#endif

    ComPtr<IDXGIFactory4> factory;
    ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

    if (m_useWarpDevice)
    {
        ComPtr<IDXGIAdapter> warpAdapter;
        ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

        ThrowIfFailed(D3D12CreateDevice(
            warpAdapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&m_device)
            ));
    }
    else
    {
        ComPtr<IDXGIAdapter1> hardwareAdapter;
        GetHardwareAdapter(factory.Get(), &hardwareAdapter, true);

        ThrowIfFailed(D3D12CreateDevice(
            hardwareAdapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&m_device)
            ));
    }

    // Describe and create the command queues.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));
    NAME_D3D12_OBJECT(m_commandQueue);

    D3D12_COMMAND_QUEUE_DESC computeQueueDesc = {};
    computeQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    computeQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;

    ThrowIfFailed(m_device->CreateCommandQueue(&computeQueueDesc, IID_PPV_ARGS(&m_computeCommandQueue)));
    NAME_D3D12_OBJECT(m_computeCommandQueue);

    // Describe and create the swap chain.
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = FrameCount;
    swapChainDesc.Width = m_width;
    swapChainDesc.Height = m_height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swapChain;
    ThrowIfFailed(factory->CreateSwapChainForHwnd(
        m_commandQueue.Get(),        // Swap chain needs the queue so that it can force a flush on it.
        Win32Application::GetHwnd(),
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain
        ));

    // This sample does not support fullscreen transitions.
    ThrowIfFailed(factory->MakeWindowAssociation(Win32Application::GetHwnd(), DXGI_MWA_NO_ALT_ENTER));

    ThrowIfFailed(swapChain.As(&m_swapChain));
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    // Create descriptor heaps.
    {
        // Describe and create a render target view (RTV) descriptor heap.
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = FrameCount;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

        // Describe and create a depth stencil view (DSV) descriptor heap.
        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
        dsvHeapDesc.NumDescriptors = 1;
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        ThrowIfFailed(m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));

        // Describe and create a constant buffer view (CBV), Shader resource
        // view (SRV), and unordered access view (UAV) descriptor heap.
        D3D12_DESCRIPTOR_HEAP_DESC cbvSrvUavHeapDesc = {};
        cbvSrvUavHeapDesc.NumDescriptors = CbvSrvUavDescriptorCountPerFrame * FrameCount
            + 16; // TODO_RW: 2 * (1 SRV + 1 UAV) + 2 UAV < 16 
        cbvSrvUavHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        cbvSrvUavHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        ThrowIfFailed(m_device->CreateDescriptorHeap(&cbvSrvUavHeapDesc, IID_PPV_ARGS(&m_cbvSrvUavHeap)));
        NAME_D3D12_OBJECT(m_cbvSrvUavHeap);

        m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        m_cbvSrvUavDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }

    // Create frame resources.
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

        // Create a RTV and command allocators for each frame.
        for (UINT n = 0; n < FrameCount; n++)
        {
            ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));
            m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
            rtvHandle.Offset(1, m_rtvDescriptorSize);

            NAME_D3D12_OBJECT_INDEXED(m_renderTargets, n);

            ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocators[n])));
            ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(&m_computeCommandAllocators[n])));
        }
    }
}

// Load the sample assets.
void D3D12ExecuteIndirect::LoadAssets()
{
    // Create the root signatures.
    {
        D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

        // This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

        if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
        {
            featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
        }

        CD3DX12_ROOT_PARAMETER1 rootParameters[GraphicsRootParametersCount];
		rootParameters[Cbv].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC, D3D12_SHADER_VISIBILITY_ALL);
		rootParameters[Srv].InitAsShaderResourceView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC, D3D12_SHADER_VISIBILITY_PIXEL);

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
        ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
        NAME_D3D12_OBJECT(m_rootSignature);

        // Create compute signature.
        CD3DX12_DESCRIPTOR_RANGE1 ranges[2];
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
        ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE);

        CD3DX12_ROOT_PARAMETER1 computeRootParameters[ComputeRootParametersCount];
        computeRootParameters[SrvUavTable].InitAsDescriptorTable(2, ranges);
        computeRootParameters[RootConstants].InitAsConstants(4, 0);

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC computeRootSignatureDesc;
        computeRootSignatureDesc.Init_1_1(_countof(computeRootParameters), computeRootParameters);

        ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&computeRootSignatureDesc, featureData.HighestVersion, &signature, &error));
        ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_computeRootSignature)));
        NAME_D3D12_OBJECT(m_computeRootSignature);

		/**************************************************************************************************/
        {
			CD3DX12_DESCRIPTOR_RANGE1 ranges[2];
			ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
			ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE);

			CD3DX12_ROOT_PARAMETER1 rootParameters[2] = {};
			rootParameters[0].InitAsConstants(4, 0);
			rootParameters[1].InitAsDescriptorTable(2, ranges);

			CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
			rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters);

			ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
			ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_textureRootSignature)));
        }
	    /**************************************************************************************************/

    }

    // Create the pipeline state, which includes compiling and loading shaders.
    {
        ComPtr<ID3DBlob> vertexShader;
        ComPtr<ID3DBlob> pixelShader;
		ComPtr<ID3DBlob> computeShader;
		ComPtr<ID3DBlob> textureShader;
        ComPtr<ID3DBlob> error;

#if defined(_DEBUG)
        // Enable better shader debugging with the graphics debugging tools.
        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        UINT compileFlags = 0;
#endif

        ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"shaders.hlsl").c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, &error));
        ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"shaders.hlsl").c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, &error));
        ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"compute.hlsl").c_str(), nullptr, nullptr, "CSMain", "cs_5_0", compileFlags, 0, &computeShader, &error));
		/**************************************************************************************************/
		ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"texture.hlsl").c_str(), nullptr, nullptr, "CSMain", "cs_5_0", compileFlags, 0, &textureShader, &error));
		/**************************************************************************************************/

        // Define the vertex input layout.
        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
        {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R16G16_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

        // Describe and create the graphics pipeline state objects (PSO).
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
        psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        psoDesc.SampleDesc.Count = 1;

        ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
        NAME_D3D12_OBJECT(m_pipelineState);

        // Describe and create the compute pipeline state object (PSO).
        D3D12_COMPUTE_PIPELINE_STATE_DESC computePsoDesc = {};
        computePsoDesc.pRootSignature = m_computeRootSignature.Get();
        computePsoDesc.CS = CD3DX12_SHADER_BYTECODE(computeShader.Get());

        ThrowIfFailed(m_device->CreateComputePipelineState(&computePsoDesc, IID_PPV_ARGS(&m_computeState)));
        NAME_D3D12_OBJECT(m_computeState);

		/**************************************************************************************************/
		// Describe and create the compute pipeline state object (PSO).
		D3D12_COMPUTE_PIPELINE_STATE_DESC texturePsoDesc = {};
		computePsoDesc.pRootSignature = m_textureRootSignature.Get();
		computePsoDesc.CS = CD3DX12_SHADER_BYTECODE(textureShader.Get());

		ThrowIfFailed(m_device->CreateComputePipelineState(&computePsoDesc, IID_PPV_ARGS(&m_textureState)));
		NAME_D3D12_OBJECT(m_textureState);
		/**************************************************************************************************/
	}

    // Create the command list.
    ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocators[m_frameIndex].Get(), m_pipelineState.Get(), IID_PPV_ARGS(&m_commandList)));
    ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE, m_computeCommandAllocators[m_frameIndex].Get(), m_computeState.Get(), IID_PPV_ARGS(&m_computeCommandList)));
    ThrowIfFailed(m_computeCommandList->Close());

    NAME_D3D12_OBJECT(m_commandList);
    NAME_D3D12_OBJECT(m_computeCommandList);

    // Note: ComPtr's are CPU objects but these resources need to stay in scope until
    // the command list that references them has finished executing on the GPU.
    // We will flush the GPU at the end of this method to ensure the resources are not
    // prematurely destroyed.
    ComPtr<ID3D12Resource> commandBufferUpload;
	ComPtr<ID3D12Resource> triangleVertexBufferUpload;
	ComPtr<ID3D12Resource> triangleIndexBufferUpload;
	ComPtr<ID3D12Resource> quadVertexBufferUpload;
	ComPtr<ID3D12Resource> quadIndexBufferUpload;

    // Triangles
    {
        // Create the vertex buffer.
        {
            // Define the geometry for a triangle.
            Vertex triangleVertices[] =
            {
                { { 0.0f, TriangleHalfWidth, TriangleDepth }, {0.5f, 0.0f} },
                { { TriangleHalfWidth, -TriangleHalfWidth, TriangleDepth }, {1.f, 1.f} },
                { { -TriangleHalfWidth, -TriangleHalfWidth, TriangleDepth }, {0.0f, 1.f} }
            };

            const UINT vertexBufferSize = sizeof(triangleVertices);

            ThrowIfFailed(m_device->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                D3D12_HEAP_FLAG_NONE,
                &CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
                D3D12_RESOURCE_STATE_COPY_DEST,
                nullptr,
                IID_PPV_ARGS(&m_triangleMesh.vertexBuffer)));

            ThrowIfFailed(m_device->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                D3D12_HEAP_FLAG_NONE,
                &CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&triangleVertexBufferUpload)));

            NAME_D3D12_OBJECT(m_triangleMesh.vertexBuffer);

            // Copy data to the intermediate upload heap and then schedule a copy
            // from the upload heap to the vertex buffer.
            D3D12_SUBRESOURCE_DATA vertexData = {};
            vertexData.pData = reinterpret_cast<UINT8*>(triangleVertices);
            vertexData.RowPitch = vertexBufferSize;
            vertexData.SlicePitch = vertexData.RowPitch;

            UpdateSubresources<1>(m_commandList.Get(), m_triangleMesh.vertexBuffer.Get(), triangleVertexBufferUpload.Get(), 0, 0, 1, &vertexData);
            m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_triangleMesh.vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

            // Initialize the vertex buffer view.
            m_triangleMesh.vertexBufferView.BufferLocation = m_triangleMesh.vertexBuffer->GetGPUVirtualAddress();
            m_triangleMesh.vertexBufferView.StrideInBytes = sizeof(Vertex);
            m_triangleMesh.vertexBufferView.SizeInBytes = sizeof(triangleVertices);
        }

        // Create the Index buffer
        {
            USHORT indices[] = {0, 1, 2};

			const UINT indexBufferSize = sizeof(indices);

			ThrowIfFailed(m_device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize),
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				IID_PPV_ARGS(&m_triangleMesh.indexBuffer)));

			ThrowIfFailed(m_device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&triangleIndexBufferUpload)));

			NAME_D3D12_OBJECT(m_triangleMesh.indexBuffer);

			// Copy data to the intermediate upload heap and then schedule a copy
			// from the upload heap to the vertex buffer.
			D3D12_SUBRESOURCE_DATA indexData = {};
            indexData.pData = reinterpret_cast<UINT8*>(indices);
            indexData.RowPitch = indexBufferSize;
            indexData.SlicePitch = indexData.RowPitch;

			UpdateSubresources<1>(m_commandList.Get(), m_triangleMesh.indexBuffer.Get(), triangleIndexBufferUpload.Get(), 0, 0, 1, &indexData);
			m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_triangleMesh.indexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER));

			// Initialize the index buffer view.
            m_triangleMesh.indexBufferView.BufferLocation = m_triangleMesh.indexBuffer->GetGPUVirtualAddress();
            m_triangleMesh.indexBufferView.SizeInBytes = indexBufferSize;
            m_triangleMesh.indexBufferView.Format = DXGI_FORMAT_R16_UINT;
        }
    }

	// Quad
	{
		// Create the vertex buffer.
		{
			// Define the geometry for a triangle.
			Vertex quadVertices[] =
			{
				{ { -TriangleHalfWidth, TriangleHalfWidth, TriangleDepth }, {0.0f, 0.0f} },
				{ { TriangleHalfWidth, TriangleHalfWidth, TriangleDepth }, {1.f, 0.f} },
				{ { TriangleHalfWidth, -TriangleHalfWidth, TriangleDepth }, {1.f, 1.f} },
				{ { -TriangleHalfWidth, -TriangleHalfWidth, TriangleDepth }, {0.0f, 1.f} }
			};

			const UINT vertexBufferSize = sizeof(quadVertices);

			ThrowIfFailed(m_device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				IID_PPV_ARGS(&m_quadMesh.vertexBuffer)));

			ThrowIfFailed(m_device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&quadVertexBufferUpload)));

			NAME_D3D12_OBJECT(m_quadMesh.vertexBuffer);

			// Copy data to the intermediate upload heap and then schedule a copy
			// from the upload heap to the vertex buffer.
			D3D12_SUBRESOURCE_DATA vertexData = {};
			vertexData.pData = reinterpret_cast<UINT8*>(quadVertices);
			vertexData.RowPitch = vertexBufferSize;
			vertexData.SlicePitch = vertexData.RowPitch;

			UpdateSubresources<1>(m_commandList.Get(), m_quadMesh.vertexBuffer.Get(), quadVertexBufferUpload.Get(), 0, 0, 1, &vertexData);
			m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_quadMesh.vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

			// Initialize the vertex buffer view.
            m_quadMesh.vertexBufferView.BufferLocation = m_quadMesh.vertexBuffer->GetGPUVirtualAddress();
            m_quadMesh.vertexBufferView.StrideInBytes = sizeof(Vertex);
            m_quadMesh.vertexBufferView.SizeInBytes = sizeof(quadVertices);
		}

		// Create the Index buffer
		{
			USHORT indices[] = { 0, 1, 3, 1, 2, 3 };

			const UINT indexBufferSize = sizeof(indices);

			ThrowIfFailed(m_device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize),
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				IID_PPV_ARGS(&m_quadMesh.indexBuffer)));

			ThrowIfFailed(m_device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&quadIndexBufferUpload)));

			NAME_D3D12_OBJECT(m_quadMesh.indexBuffer);

			// Copy data to the intermediate upload heap and then schedule a copy
			// from the upload heap to the vertex buffer.
			D3D12_SUBRESOURCE_DATA indexData = {};
			indexData.pData = reinterpret_cast<UINT8*>(indices);
			indexData.RowPitch = indexBufferSize;
			indexData.SlicePitch = indexData.RowPitch;

			UpdateSubresources<1>(m_commandList.Get(), m_quadMesh.indexBuffer.Get(), quadIndexBufferUpload.Get(), 0, 0, 1, &indexData);
			m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_quadMesh.indexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER));

			// Initialize the index buffer view.
            m_quadMesh.indexBufferView.BufferLocation = m_quadMesh.indexBuffer->GetGPUVirtualAddress();
            m_quadMesh.indexBufferView.SizeInBytes = indexBufferSize;
            m_quadMesh.indexBufferView.Format = DXGI_FORMAT_R16_UINT;
		}
	}
	
    // Create the depth stencil view.
    {
        D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
        depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
        depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

        D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
        depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
        depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
        depthOptimizedClearValue.DepthStencil.Stencil = 0;

        ThrowIfFailed(m_device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, m_width, m_height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &depthOptimizedClearValue,
            IID_PPV_ARGS(&m_depthStencil)
            ));

        NAME_D3D12_OBJECT(m_depthStencil);

        m_device->CreateDepthStencilView(m_depthStencil.Get(), &depthStencilDesc, m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
    }

    // Create the constant buffers.
    {
        const UINT constantBufferDataSize = TriangleResourceCount * sizeof(SceneConstantBuffer);

        ThrowIfFailed(m_device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(constantBufferDataSize),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_constantBuffer)));

        NAME_D3D12_OBJECT(m_constantBuffer);

        // Initialize the constant buffers for each of the triangles.
        for (UINT n = 0; n < TriangleCount; n++)
        {
            m_constantBufferData[n].velocity = XMFLOAT4(GetRandomFloat(0.01f, 0.02f), 0.0f, 0.0f, 0.0f);
            m_constantBufferData[n].offset = XMFLOAT4(GetRandomFloat(-5.0f, -1.5f), GetRandomFloat(-1.0f, 1.0f), GetRandomFloat(0.0f, 2.0f), 0.0f);
            m_constantBufferData[n].color = XMFLOAT4(GetRandomFloat(0.5f, 1.0f), GetRandomFloat(0.5f, 1.0f), GetRandomFloat(0.5f, 1.0f), 1.0f);
            XMStoreFloat4x4(&m_constantBufferData[n].projection, XMMatrixTranspose(XMMatrixPerspectiveFovLH(XM_PIDIV4, m_aspectRatio, 0.01f, 20.0f)));
			m_constantBufferData[n].size = XMFLOAT4(m_viewport.Width, m_viewport.Height, (float)m_textureWidth, (float)m_textureHeight);
		}

        // Map and initialize the constant buffer. We don't unmap this until the
        // app closes. Keeping things mapped for the lifetime of the resource is okay.
        CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
        ThrowIfFailed(m_constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_pCbvDataBegin)));
        memcpy(m_pCbvDataBegin, &m_constantBufferData[0], TriangleCount * sizeof(SceneConstantBuffer));

        // Create shader resource views (SRV) of the constant buffers for the
        // compute shader to read from.
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Buffer.NumElements = TriangleCount;
        srvDesc.Buffer.StructureByteStride = sizeof(SceneConstantBuffer);
        srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

        CD3DX12_CPU_DESCRIPTOR_HANDLE cbvSrvHandle(m_cbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart(), CbvSrvOffset, m_cbvSrvUavDescriptorSize);
        for (UINT frame = 0; frame < FrameCount; frame++)
        {
            srvDesc.Buffer.FirstElement = frame * TriangleCount;
            m_device->CreateShaderResourceView(m_constantBuffer.Get(), &srvDesc, cbvSrvHandle);
            cbvSrvHandle.Offset(CbvSrvUavDescriptorCountPerFrame, m_cbvSrvUavDescriptorSize);
        }
    }

    // Create the command signature used for indirect drawing.
    {
        // Each command consists of a CBV update and a DrawInstanced call.
        D3D12_INDIRECT_ARGUMENT_DESC argumentDescs[3] = {};
        argumentDescs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW;
        argumentDescs[0].ConstantBufferView.RootParameterIndex = Cbv;
		argumentDescs[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW;
		argumentDescs[1].ConstantBufferView.RootParameterIndex = Srv;
        argumentDescs[2].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

        D3D12_COMMAND_SIGNATURE_DESC commandSignatureDesc = {};
        commandSignatureDesc.pArgumentDescs = argumentDescs;
        commandSignatureDesc.NumArgumentDescs = _countof(argumentDescs);
        commandSignatureDesc.ByteStride = sizeof(IndirectCommand);

        ThrowIfFailed(m_device->CreateCommandSignature(&commandSignatureDesc, m_rootSignature.Get(), IID_PPV_ARGS(&m_commandSignature)));
        NAME_D3D12_OBJECT(m_commandSignature);
    }

	/**************************************************************************************************/
	// user data
	m_textureHeapOffset = CbvSrvUavDescriptorCountPerFrame * FrameCount;
	// Create the texture.
	ComPtr<ID3D12Resource> textureStaging; // this has to be valid until the cmd has been finished
	{
		std::vector<UINT8> imageData;
		D3D12_RESOURCE_DESC textureDesc = {};
		int imageBytesPerRow;
		std::string filename = "yeti.png";
		LoadImageDataFromFile(imageData, textureDesc, filename, imageBytesPerRow);

		ThrowIfFailed(m_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&textureDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&m_texture)));

		m_textureWidth = (uint32_t)textureDesc.Width;
		m_textureHeight = (uint32_t)textureDesc.Height;

		const UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_texture.Get(), 0, 1);

		// Create the GPU upload buffer.
		ThrowIfFailed(m_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&textureStaging)));

		D3D12_SUBRESOURCE_DATA textureData = {};
		textureData.pData = &imageData[0];
		textureData.RowPitch = imageBytesPerRow;
		textureData.SlicePitch = textureData.RowPitch * textureDesc.Height;

		UpdateSubresources(m_commandList.Get(), m_texture.Get(), textureStaging.Get(), 0, 0, 1, &textureData);
		m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = textureDesc.Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(m_cbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart(), m_textureHeapOffset, m_cbvSrvUavDescriptorSize);
		m_device->CreateShaderResourceView(m_texture.Get(), &srvDesc, srvHandle);

		// create a buffer to keep the output result
		D3D12_RESOURCE_DESC commandBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(textureDesc.Width * textureDesc.Height * 3 * sizeof(float), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		ThrowIfFailed(m_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&commandBufferDesc,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			nullptr,
			IID_PPV_ARGS(&m_textureBuffer)));

		NAME_D3D12_OBJECT(m_textureBuffer);

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = m_textureWidth * m_textureHeight * 3;
		uavDesc.Buffer.StructureByteStride = 0;
		uavDesc.Buffer.CounterOffsetInBytes = 0;
		uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;

		CD3DX12_CPU_DESCRIPTOR_HANDLE uavHandle(m_cbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart(), m_textureHeapOffset + 1, m_cbvSrvUavDescriptorSize);
		m_device->CreateUnorderedAccessView(
			m_textureBuffer.Get(),
			nullptr,
			&uavDesc,
			uavHandle);
	}
	/**************************************************************************************************/

    // Create the command buffers and UAVs to store the results of the compute work.
    {
        std::vector<IndirectCommand> commands;
        commands.resize(TriangleResourceCount);
        const UINT commandBufferSize = CommandSizePerFrame * FrameCount;

        D3D12_RESOURCE_DESC commandBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(commandBufferSize);
        ThrowIfFailed(m_device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &commandBufferDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&m_commandBuffer)));

        ThrowIfFailed(m_device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(commandBufferSize),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&commandBufferUpload)));

        NAME_D3D12_OBJECT(m_commandBuffer);

        D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = m_constantBuffer->GetGPUVirtualAddress();
        UINT commandIndex = 0;

        for (UINT frame = 0; frame < FrameCount; frame++)
        {
            for (UINT n = 0; n < TriangleCount; n++)
            {
				commands[commandIndex].cbv = gpuAddress;
				commands[commandIndex].srv = m_textureBuffer->GetGPUVirtualAddress();
				commands[commandIndex].drawArguments.BaseVertexLocation = 0;
				commands[commandIndex].drawArguments.IndexCountPerInstance = 6;
                commands[commandIndex].drawArguments.InstanceCount = 1;
                commands[commandIndex].drawArguments.StartIndexLocation = 0;
                commands[commandIndex].drawArguments.StartInstanceLocation = 0;

                commandIndex++;
                gpuAddress += sizeof(SceneConstantBuffer);
            }
        }

        // Copy data to the intermediate upload heap and then schedule a copy
        // from the upload heap to the command buffer.
        D3D12_SUBRESOURCE_DATA commandData = {};
        commandData.pData = reinterpret_cast<UINT8*>(&commands[0]);
        commandData.RowPitch = commandBufferSize;
        commandData.SlicePitch = commandData.RowPitch;

        UpdateSubresources<1>(m_commandList.Get(), m_commandBuffer.Get(), commandBufferUpload.Get(), 0, 0, 1, &commandData);
        m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_commandBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));

        // Create SRVs for the command buffers.
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Buffer.NumElements = TriangleCount;
        srvDesc.Buffer.StructureByteStride = sizeof(IndirectCommand);
        srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

        CD3DX12_CPU_DESCRIPTOR_HANDLE commandsHandle(m_cbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart(), CommandsOffset, m_cbvSrvUavDescriptorSize);
        for (UINT frame = 0; frame < FrameCount; frame++)
        {
            srvDesc.Buffer.FirstElement = frame * TriangleCount;
            m_device->CreateShaderResourceView(m_commandBuffer.Get(), &srvDesc, commandsHandle);
            commandsHandle.Offset(CbvSrvUavDescriptorCountPerFrame, m_cbvSrvUavDescriptorSize);
        }

        // Create the unordered access views (UAVs) that store the results of the compute work.
        CD3DX12_CPU_DESCRIPTOR_HANDLE processedCommandsHandle(m_cbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart(), ProcessedCommandsOffset, m_cbvSrvUavDescriptorSize);
        for (UINT frame = 0; frame < FrameCount; frame++)
        {
            // Allocate a buffer large enough to hold all of the indirect commands
            // for a single frame as well as a UAV counter.
            commandBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(CommandBufferCounterOffset + sizeof(UINT), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
            ThrowIfFailed(m_device->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                D3D12_HEAP_FLAG_NONE,
                &commandBufferDesc,
                D3D12_RESOURCE_STATE_COPY_DEST,
                nullptr,
                IID_PPV_ARGS(&m_processedCommandBuffers[frame])));

            NAME_D3D12_OBJECT_INDEXED(m_processedCommandBuffers, frame);

            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            uavDesc.Format = DXGI_FORMAT_UNKNOWN;
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
            uavDesc.Buffer.FirstElement = 0;
            uavDesc.Buffer.NumElements = TriangleCount;
            uavDesc.Buffer.StructureByteStride = sizeof(IndirectCommand);
            uavDesc.Buffer.CounterOffsetInBytes = CommandBufferCounterOffset;
            uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

            m_device->CreateUnorderedAccessView(
                m_processedCommandBuffers[frame].Get(),
                m_processedCommandBuffers[frame].Get(),
                &uavDesc,
                processedCommandsHandle);

            processedCommandsHandle.Offset(CbvSrvUavDescriptorCountPerFrame, m_cbvSrvUavDescriptorSize);
        }

        // Allocate a buffer that can be used to reset the UAV counters and initialize
        // it to 0.
        ThrowIfFailed(m_device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(sizeof(UINT)),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_processedCommandBufferCounterReset)));

        UINT8* pMappedCounterReset = nullptr;
        CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
        ThrowIfFailed(m_processedCommandBufferCounterReset->Map(0, &readRange, reinterpret_cast<void**>(&pMappedCounterReset)));
        ZeroMemory(pMappedCounterReset, sizeof(UINT));
        m_processedCommandBufferCounterReset->Unmap(0, nullptr);
    }

    // Close the command list and execute it to begin the vertex buffer copy into
    // the default heap.
    ThrowIfFailed(m_commandList->Close());
    ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // Create synchronization objects and wait until assets have been uploaded to the GPU.
    {
        ThrowIfFailed(m_device->CreateFence(m_fenceValues[m_frameIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
        ThrowIfFailed(m_device->CreateFence(m_fenceValues[m_frameIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_computeFence)));
        m_fenceValues[m_frameIndex]++;

        // Create an event handle to use for frame synchronization.
        m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (m_fenceEvent == nullptr)
        {
            ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }

        // Wait for the command list to execute; we are reusing the same command 
        // list in our main loop but for now, we just want to wait for setup to 
        // complete before continuing.
        WaitForGpu();
    }
}

// Get a random float value between min and max.
float D3D12ExecuteIndirect::GetRandomFloat(float min, float max)
{
    float scale = static_cast<float>(rand()) / RAND_MAX;
    float range = max - min;
    return scale * range + min;
}

// Update frame-based values.
void D3D12ExecuteIndirect::OnUpdate()
{
    for (UINT n = 0; n < TriangleCount; n++)
    {
        const float offsetBounds = 2.5f;

        // Animate the triangles.
        m_constantBufferData[n].offset.x += m_constantBufferData[n].velocity.x;
        if (m_constantBufferData[n].offset.x > offsetBounds)
        {
            m_constantBufferData[n].velocity.x = GetRandomFloat(0.01f, 0.02f);
            m_constantBufferData[n].offset.x = -offsetBounds;
        }
    }

    UINT8* destination = m_pCbvDataBegin + (TriangleCount * m_frameIndex * sizeof(SceneConstantBuffer));
    memcpy(destination, &m_constantBufferData[0], TriangleCount * sizeof(SceneConstantBuffer));
}

// Render the scene.
void D3D12ExecuteIndirect::OnRender()
{
    try
    {
        // Record all the commands we need to render the scene into the command list.
        PopulateCommandLists();

        // Execute the compute work.
        if (m_enableCulling)
        {
            PIXBeginEvent(m_commandQueue.Get(), 0, L"Cull invisible triangles");

            ID3D12CommandList* ppCommandLists[] = { m_computeCommandList.Get() };
            m_computeCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

            PIXEndEvent(m_commandQueue.Get());

            m_computeCommandQueue->Signal(m_computeFence.Get(), m_fenceValues[m_frameIndex]);

            // Execute the rendering work only when the compute work is complete.
            m_commandQueue->Wait(m_computeFence.Get(), m_fenceValues[m_frameIndex]);
        }

        PIXBeginEvent(m_commandQueue.Get(), 0, L"Render");

        // Execute the rendering work.
        ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
        m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

        PIXEndEvent(m_commandQueue.Get());

        // Present the frame.
        ThrowIfFailed(m_swapChain->Present(1, 0));

        MoveToNextFrame();
    }
    catch (HrException& e)
    {
        if (e.Error() == DXGI_ERROR_DEVICE_REMOVED || e.Error() == DXGI_ERROR_DEVICE_RESET)
        {
            RestoreD3DResources();
        }
        else
        {
            throw;
        }
    }
}

// Release sample's D3D objects.
void D3D12ExecuteIndirect::ReleaseD3DResources()
{
    m_fence.Reset();
    ResetComPtrArray(&m_renderTargets);
    m_commandQueue.Reset();
    m_swapChain.Reset();
    m_device.Reset();
}

// Tears down D3D resources and reinitializes them.
void D3D12ExecuteIndirect::RestoreD3DResources()
{
    // Give GPU a chance to finish its execution in progress.
    try
    {
        WaitForGpu();
    }
    catch (HrException&)
    {
        // Do nothing, currently attached adapter is unresponsive.
    }
    ReleaseD3DResources();
    OnInit();
}

void D3D12ExecuteIndirect::OnDestroy()
{
    // Ensure that the GPU is no longer referencing resources that are about to be
    // cleaned up by the destructor.
    WaitForGpu();
    CloseHandle(m_fenceEvent);
}

void D3D12ExecuteIndirect::OnKeyDown(UINT8 key)
{
    if (key == VK_SPACE)
    {
        m_enableCulling = !m_enableCulling;
    }
}

template<typename T, typename ALIGNMENT>
T AlignValue(T value, ALIGNMENT alignment)
{
	const T alignMinus1 = (T)alignment - 1;
	return (value + alignMinus1) & ~alignMinus1;
}

// Fill the command list with all the render commands and dependent state.
void D3D12ExecuteIndirect::PopulateCommandLists()
{
    // Command list allocators can only be reset when the associated 
    // command lists have finished execution on the GPU; apps should use 
    // fences to determine GPU execution progress.
    ThrowIfFailed(m_computeCommandAllocators[m_frameIndex]->Reset());
    ThrowIfFailed(m_commandAllocators[m_frameIndex]->Reset());

    // However, when ExecuteCommandList() is called on a particular command 
    // list, that command list can then be reset at any time and must be before 
    // re-recording.
    ThrowIfFailed(m_computeCommandList->Reset(m_computeCommandAllocators[m_frameIndex].Get(), m_computeState.Get()));
    ThrowIfFailed(m_commandList->Reset(m_commandAllocators[m_frameIndex].Get(), m_pipelineState.Get()));

    // Record the compute commands that will cull triangles and prevent them from being processed by the vertex shader.
    if (m_enableCulling)
    {
        UINT frameDescriptorOffset = m_frameIndex * CbvSrvUavDescriptorCountPerFrame;
        D3D12_GPU_DESCRIPTOR_HANDLE cbvSrvUavHandle = m_cbvSrvUavHeap->GetGPUDescriptorHandleForHeapStart();

        m_computeCommandList->SetComputeRootSignature(m_computeRootSignature.Get());

        ID3D12DescriptorHeap* ppHeaps[] = { m_cbvSrvUavHeap.Get() };
        m_computeCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

        m_computeCommandList->SetComputeRootDescriptorTable(
            SrvUavTable,
            CD3DX12_GPU_DESCRIPTOR_HANDLE(cbvSrvUavHandle, CbvSrvOffset + frameDescriptorOffset, m_cbvSrvUavDescriptorSize));

        m_computeCommandList->SetComputeRoot32BitConstants(RootConstants, 4, reinterpret_cast<void*>(&m_csRootConstants), 0);

        // Reset the UAV counter for this frame.
        m_computeCommandList->CopyBufferRegion(m_processedCommandBuffers[m_frameIndex].Get(), CommandBufferCounterOffset, m_processedCommandBufferCounterReset.Get(), 0, sizeof(UINT));

        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_processedCommandBuffers[m_frameIndex].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        m_computeCommandList->ResourceBarrier(1, &barrier);

        m_computeCommandList->Dispatch(static_cast<UINT>(ceil(TriangleCount / float(ComputeThreadBlockSize))), 1, 1);
    }

    ThrowIfFailed(m_computeCommandList->Close());

    // Record the rendering commands.
    {
		ID3D12DescriptorHeap* ppHeaps[] = { m_cbvSrvUavHeap.Get() };
		m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

		/**************************************************************************************************/
        {
			m_commandList->SetComputeRootSignature(m_textureRootSignature.Get());
			D3D12_GPU_DESCRIPTOR_HANDLE cbvSrvUavHandle = m_cbvSrvUavHeap->GetGPUDescriptorHandleForHeapStart();
			m_commandList->SetComputeRootDescriptorTable(
				0,
				CD3DX12_GPU_DESCRIPTOR_HANDLE(cbvSrvUavHandle, m_textureHeapOffset, m_cbvSrvUavDescriptorSize));
            uint32_t rootConsts[2] = {m_textureWidth, m_textureHeight};
            m_commandList->SetComputeRoot32BitConstants(0, 2, reinterpret_cast<void*>(&rootConsts), 0);
            m_commandList->Dispatch(AlignValue(m_textureWidth, 8)/8, AlignValue(m_textureHeight, 8) / 8, 1);
            {
				D3D12_RESOURCE_BARRIER barriers[1] = {
				CD3DX12_RESOURCE_BARRIER::Transition(m_textureBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
				};
				m_commandList->ResourceBarrier(_countof(barriers), barriers);
            }
			
		}
		
		/**************************************************************************************************/

		// Set necessary state.
        m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

        m_commandList->RSSetViewports(1, &m_viewport);
        m_commandList->RSSetScissorRects(1, m_enableCulling ? &m_cullingScissorRect : &m_scissorRect);

        // Indicate that the command buffer will be used for indirect drawing
        // and that the back buffer will be used as a render target.
        D3D12_RESOURCE_BARRIER barriers[2] = {
            CD3DX12_RESOURCE_BARRIER::Transition(
                m_enableCulling ? m_processedCommandBuffers[m_frameIndex].Get() : m_commandBuffer.Get(),
                m_enableCulling ? D3D12_RESOURCE_STATE_UNORDERED_ACCESS : D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT),
            CD3DX12_RESOURCE_BARRIER::Transition(
                m_renderTargets[m_frameIndex].Get(),
                D3D12_RESOURCE_STATE_PRESENT,
                D3D12_RESOURCE_STATE_RENDER_TARGET)
        };

        m_commandList->ResourceBarrier(_countof(barriers), barriers);

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
        CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
        m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

        // Record commands.
        const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
        m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
        m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

        m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        /*m_commandList->IASetVertexBuffers(0, 1, &m_triangleMesh.vertexBufferView);
        m_commandList->IASetIndexBuffer(&m_triangleMesh.indexBufferView);*/
		m_commandList->IASetVertexBuffers(0, 1, &m_quadMesh.vertexBufferView);
		m_commandList->IASetIndexBuffer(&m_quadMesh.indexBufferView);

        if (m_enableCulling)
        {
            PIXBeginEvent(m_commandList.Get(), 0, L"Draw visible triangles");

            // Draw the triangles that have not been culled.
            m_commandList->ExecuteIndirect(
                m_commandSignature.Get(),
                TriangleCount,
                m_processedCommandBuffers[m_frameIndex].Get(),
                0,
                m_processedCommandBuffers[m_frameIndex].Get(),
                CommandBufferCounterOffset);
        }
        else
        {
            PIXBeginEvent(m_commandList.Get(), 0, L"Draw all triangles");

            // Draw all of the triangles.
            m_commandList->ExecuteIndirect(
                m_commandSignature.Get(),
                TriangleCount,
                m_commandBuffer.Get(),
                CommandSizePerFrame * m_frameIndex,
                nullptr,
                0);
        }
        PIXEndEvent(m_commandList.Get());

        // Indicate that the command buffer may be used by the compute shader
        // and that the back buffer will now be used to present.
        barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
        barriers[0].Transition.StateAfter = m_enableCulling ? D3D12_RESOURCE_STATE_COPY_DEST : D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

        m_commandList->ResourceBarrier(_countof(barriers), barriers);

        ThrowIfFailed(m_commandList->Close());
    }
}

// Wait for pending GPU work to complete.
void D3D12ExecuteIndirect::WaitForGpu()
{
    // Schedule a Signal command in the queue.
    ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_fenceValues[m_frameIndex]));

    // Wait until the fence has been processed.
    ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent));
    WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);

    // Increment the fence value for the current frame.
    m_fenceValues[m_frameIndex]++;
}

// Prepare to render the next frame.
void D3D12ExecuteIndirect::MoveToNextFrame()
{
    // Schedule a Signal command in the queue.
    const UINT64 currentFenceValue = m_fenceValues[m_frameIndex];
    ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), currentFenceValue));

    // Update the frame index.
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    // If the next frame is not ready to be rendered yet, wait until it is ready.
    if (m_fence->GetCompletedValue() < m_fenceValues[m_frameIndex])
    {
        ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent));
        WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
    }

    // Set the fence value for the next frame.
    m_fenceValues[m_frameIndex] = currentFenceValue + 1;
}
