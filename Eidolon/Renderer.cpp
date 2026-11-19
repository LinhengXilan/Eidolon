/**
 * @file Renderer.cpp
 * @author LinhengXilan
 * @version 0.0.0.7
 * @date 2026-11-19
 */

#include <Pch.h>
#include <Renderer.h>
#include <Logger.h>
#include <Guid.h>

Renderer::~Renderer()
{
	Shutdown();
}

void Renderer::Initialize(HWND hwnd)
{
	// 启用调试层
#ifdef EIDOLON_DEBUG
	{
		ComPtr<ID3D12Debug6> debug;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))
		{
			debug->EnableDebugLayer();
			OutputDebugStringW(L"[D3D12] 调试层已启用\n");
		}
		else
		{
			OutputDebugStringW(L"[D3D12] 调试层不可用\n");
		}
	}
#endif

	// 创建DXGI Factory
	HRESULT hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&mFactory));
	if (FAILED(hr))
	{
		Logger::LogHresult(L"DXGI", L"CreateDXGIFactory失败", hr);
		return;
	}

	// 获取显示适配器
	for (uint32 index = 0;; ++index)
	{
		ComPtr<IDXGIAdapter4> candidate;
		hr = mFactory->EnumAdapterByGpuPreference(index, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&candidate));
		if (FAILED(hr))
		{
			Logger::Log(L"[GPU] 未找到支持的显卡");
			break;
		}
		DXGI_ADAPTER_DESC3 desc{};
		candidate->GetDesc3(&desc);

		const bool isSoftware = (desc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE) != 0;
		const bool isRemote = (desc.Flags & DXGI_ADAPTER_FLAG3_REMOTE) != 0;

		Logger::Log(L"[GPU] #%u %s | 厂商 0x%04X | 显存 %llu MB%s%s",
			index,
			desc.Description,
			desc.VendorId,
			desc.DedicatedVideoMemory / (1024ull * 1024ull),
			isSoftware ? L"  [软件]" : L"",
			isRemote ? L"  [远程]" : L""
		);
		if (isSoftware || isRemote)
		{
			continue;
		}

		if (FAILED(D3D12CreateDevice(candidate.Get(), D3D_FEATURE_LEVEL_12_2, __uuidof(ID3D12Device), nullptr)))
		{
			Logger::Log(L"[GPU] #%u 不支持 FL 12_2", index);
			continue;
		}
		mAdapter = candidate;
		break;
	}

	// 创建设备。
	hr = D3D12CreateDevice(mAdapter.Get(), D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&mDevice));
	if (FAILED(hr))
	{
		Logger::LogHresult(L"D3D12", L"D3D12CreateDevice", hr);
		return;
	}
	if (!mAdapter)
	{
		Logger::Log(L"没有可用的显示适配器");
		return;
	}
	
	// 创建命令队列。
	D3D12_COMMAND_QUEUE_DESC queueDesc{};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	queueDesc.NodeMask = 0;
	hr = mDevice->CreateCommandQueue1(&queueDesc, Guid::CREATOR_ID, IID_PPV_ARGS(&mQueue));
	if (FAILED(hr))
	{
		Logger::LogHresult(L"D3D12", L"CreateCommandQueue", hr);
		return;
	}
	mQueue->SetName(L"MainQueue");

	// 创建交换链。
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = 0;
	swapChainDesc.Height = 0;
	swapChainDesc.BufferCount = FRAME_COUNT;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo = FALSE;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.Scaling = DXGI_SCALING_NONE;
	swapChainDesc.Flags = 0;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	ComPtr<IDXGISwapChain1> swapChain;
	hr = mFactory->CreateSwapChainForHwnd(mQueue.Get(), hwnd, &swapChainDesc, nullptr, nullptr, &swapChain);
	if (FAILED(hr))
	{
		Logger::LogHresult(L"DXGI", L"CreateSwapChainForHwnd", hr);
		return;
	}
	hr = swapChain.As(&mSwapChain);
	if (FAILED(hr))
	{
		Logger::LogHresult(L"DXGI", L"IDXGISwapChain3 As IDXGISwapChain1", hr);
		return;
	}
	mFactory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_WINDOW_CHANGES);

	// 创建渲染器目标视图(RTV) 描述符堆。
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.NumDescriptors = FRAME_COUNT;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	hr = mDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&mRtvHeap));
	if (FAILED(hr))
	{
		Logger::LogHresult(L"D3D12", L"CreateDescriptorHeap", hr);
		return;
	}
	mRtvDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// 创建帧资源（每个帧的渲染器目标视图）。
	D3D12_CPU_DESCRIPTOR_HANDLE rtv = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
	for (uint32 index = 0; index < FRAME_COUNT; ++index)
	{
		hr = mSwapChain->GetBuffer(index, IID_PPV_ARGS(&mRenderTargets[index]));
		if (FAILED(hr))
		{
			Logger::LogHresult(L"DXGI", L"GetBuffer失败", hr);
			return;
		}
		mDevice->CreateRenderTargetView(mRenderTargets[index].Get(), nullptr, rtv);
		rtv.ptr += mRtvDescriptorSize;

		// 创建命令分配器。
		hr = mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mAllocators[index]));
		if (FAILED(hr))
		{
			Logger::LogHresult(L"D3D12", L"CreateCommandAllocator", hr);
			return;
		}

		// 创建命令列表。
		hr = mDevice->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&mLists[index]));
		if (FAILED(hr))
		{
			Logger::LogHresult(L"D3D12", L"CreateCommandList", hr);
			return;
		}
	}

	// 创建围栏。
	hr = mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence));
	if (FAILED(hr))
	{
		Logger::LogHresult(L"D3D12", L"CreateFence", hr);
		return;
	}
	// 创建事件处理程序。
	mFenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
	if (!mFenceEvent)
	{
		Logger::Log(L"CreateEventW 失败\n");
		return;
	}
}

void Renderer::Shutdown()
{
	if (mQueue && mFence && mFenceEvent)
	{
		const HRESULT reason = mDevice ? mDevice->GetDeviceRemovedReason() : S_OK;
		if (SUCCEEDED(reason))
		{
			WaitForGpu();
		}
	}
	if (mFenceEvent)
	{
		CloseHandle(mFenceEvent);
		mFenceEvent = nullptr;
	}

	for (uint32 i = 0; i < FRAME_COUNT; ++i)
	{
		mRenderTargets[i].Reset();
		mLists[i].Reset();
		mAllocators[i].Reset();

		mFenceValues[i] = 0;
	}

	mRtvHeap.Reset();
	mSwapChain.Reset();
	mFence.Reset();
	mQueue.Reset();
#ifdef EIDOLON_DEBUG
	if (mDevice)
	{
		ComPtr<ID3D12DebugDevice2> debugDevice;
		if (SUCCEEDED(mDevice.As(&debugDevice)))
		{
			debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
		}
	}
#endif
	mDevice.Reset();
	mAdapter.Reset();
	mFactory.Reset();
	mFrameActive = false;
	mFrameIndex = 0;
	mFenceValue = 0;
	mElapsedSecond = 0;
	mRtvDescriptorSize = 0;
}

void Renderer::BeginFrame(float deltaTime)
{
	mElapsedSecond += deltaTime;
	mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();
	if (!WaitForFence(mFenceValues[mFrameIndex]))
	{
		return;
	}

	auto* allocator = mAllocators[mFrameIndex].Get();
	auto* list = mLists[mFrameIndex].Get();
	auto* backBuffer = mRenderTargets[mFrameIndex].Get();

	HRESULT hr = allocator->Reset();
	if (FAILED(hr))
	{
		Logger::LogHresult(L"D3D12", L"mAllocator->Reset", hr);
		return;
	}
	hr = list->Reset(allocator, nullptr);
	if (FAILED(hr))
	{
		Logger::LogHresult(L"D3D12", L"mList->Reset", hr);
		return;
	}

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = backBuffer;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	list->ResourceBarrier(1, &barrier);

	// 找到对应RTV
	D3D12_CPU_DESCRIPTOR_HANDLE rtv = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
	rtv.ptr += static_cast<usize>(mRtvDescriptorSize) * mFrameIndex;

	// 清屏颜色
	const float r = 0.8f + 0.5f * sin(mElapsedSecond * 1.0f);
	const float g = 0.3f + 0.5f * cos(mElapsedSecond * 1.3f);
	const float b = 0.5f + 0.5f * sin(mElapsedSecond * 0.7f);
	const float clearColor[4] = { r, g, b, 1.0f };

	list->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
	list->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
	
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	list->ResourceBarrier(1, &barrier);

	mFrameActive = true;
}

void Renderer::EndFrame()
{
	if (!mFrameActive)
	{
		return;
	}
	mFrameActive = false;
	auto* list = mLists[mFrameIndex].Get();
	HRESULT hr = list->Close();
	if (FAILED(hr))
	{
		Logger::LogHresult(L"D3D12", L"mList->Close", hr);
		return;
	}
	ID3D12CommandList* lists[] = { list };
	mQueue->ExecuteCommandLists(1, lists);
	hr = mSwapChain->Present(1, 0);
	if (FAILED(hr))
	{
		Logger::LogHresult(L"D3D12", L"mSwapChain->Present", hr);
		return;
	}
	const uint64 value = ++mFenceValue;
	mFenceValues[mFrameIndex] = value;
	hr = mQueue->Signal(mFence.Get(), value);
	if (FAILED(hr))
	{
		Logger::LogHresult(L"D3D12", L"mQueue->Signal()", hr);
	}
}

/**
 * @brief 等待GPU执行完已提交的所有指令。
 *		  向GPU提交一条命令：修改gFence的值，当gFence中的值为新值时，表示该命令前所有命令已执行完毕。
 * @return 
 */
bool Renderer::WaitForGpu()
{
	const uint64 value = ++mFenceValue;
	HRESULT hr = mQueue->Signal(mFence.Get(), value); // 向gpu执行队列提交一条命令：将gFence置为value
	if (FAILED(hr))
	{
		Logger::LogHresult(L"D3D12", L"Signal", hr); // 常见原因：设备已移除
		return false;
	}
	return WaitForFence(value);
}

/**
 * @brief 等待围栏
 *		  检查mFence的值是否为value
 * @param value -> uint64
 * @return 
 */
bool Renderer::WaitForFence(uint64 value)
{
	if (value == 0) // 尚未提交
	{
		return true;
	}
	if (mFence->GetCompletedValue() >= value) // 已完成
	{
		return true;
	}

	HRESULT hr = mFence->SetEventOnCompletion(value, mFenceEvent); // 当mFence为value时，调用mFenceEvent
	if (FAILED(hr))
	{
		Logger::LogHresult(L"D3D12", L"SetEventOnCompletion", hr);
		return false;
	}

	if (WaitForSingleObject(mFenceEvent, 5000) != WAIT_OBJECT_0)
	{
		Logger::Log(L"[D3D12] 等待围栏超时，原因：%08X", static_cast<uint32>(mDevice->GetDeviceRemovedReason()));
		return false;
	}
	return true;
}