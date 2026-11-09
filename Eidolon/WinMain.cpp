/**
 * @file WinMain.cpp
 * @author LinhengXilan
 * @version 0.0.0.5
 * @date 2026-11-9
 */

#define WIN32_LEAN_AND_MEAN
#define NOGDICAPMASKS
#define NOSYSMETRICS
#define NOMENUS
#define NOICONS
#define NOKEYSTATES
#define NOSYSCOMMANDS
#define NORASTEROPS
#define OEMRESOURCE
#define NOATOM
#define NOCLIPBOARD
#define NOCOLOR
#define NOCTLMGR
#define NODRAWTEXT
#define NOKERNEL
#define NONLS
#define NOMEMMGR
#define NOMETAFILE
#define NOMINMAX
#define NOOPENFILE
#define NOSCROLL
#define NOSERVICE
#define NOSOUND
#define NOTEXTMETRIC
#define NOWH
#define NOWINOFFSETS
#define NOCOMM
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS
#define NOMCX

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif

#include <chrono>
#include <cstdint>
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <Types.h>

using Microsoft::WRL::ComPtr;
using handle = HANDLE;

constexpr uint8 EIDOLON_FRAME_COUNT = 3;
/* vvv 全局变量 vvv */
ComPtr<ID3D12Device> gDevice;
ComPtr<ID3D12CommandQueue> gQueue;
ComPtr<ID3D12CommandAllocator> gAllocator;
ComPtr<ID3D12GraphicsCommandList> gList;
ComPtr<ID3D12Fence> gFence;
ComPtr<IDXGIFactory6> gFactory;
ComPtr<IDXGISwapChain3> gSwapChain;
ComPtr<ID3D12DescriptorHeap> gRtvHeap;
ComPtr<ID3D12Resource> gRenderTargets[EIDOLON_FRAME_COUNT];
uint32 gRtvDescriptorSize = 0;
uint64 gFenceValue = 0;
handle gFenceEvent = nullptr;
float gElapsedSecond = 0.0f;

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void LogHresult(const wchar* what, HRESULT hr);
static bool IsWindows11OrGreater();
static bool CreateD3D12Device(ComPtr<ID3D12Device>& outDevice);
static bool CreateCommandObjects();
static bool CreateSwapChain(HWND hwnd);
static bool CreateRenderTargets();
static void RenderFrame(float deltaTime);
static bool WaitForGpu();

namespace Logger
{
	static void Log(const wchar* format, ...)
	{
		wchar buffer[1024]{};
		va_list args;
		va_start(args, format);
		vswprintf_s(buffer, _countof(buffer), format, args);

		va_end(args);
		OutputDebugStringW(buffer);
		OutputDebugStringW(L"\n");
	}
};

int WINAPI wWinMain(HINSTANCE hInstance, [[maybe_unused]] HINSTANCE hPrevInstance, [[maybe_unused]] LPWSTR lpCmdLine, int nShowCmd)
{
	IsWindows11OrGreater();

	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	WNDCLASSEX wndClass{};
	wndClass.cbSize = sizeof(wndClass);
	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc = WndProc;
	wndClass.hInstance = hInstance;
	wndClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
	wndClass.hbrBackground = nullptr;
	wndClass.lpszClassName = L"EidolonWindow";
	if (RegisterClassExW(&wndClass) == 0)
	{
		MessageBoxW(nullptr, L"注册窗口类失败", L"错误", MB_ICONERROR);
		return 1;
	}

	// 计算窗口大小
	RECT work{};
	SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
	const int workW = work.right - work.left;
	const int workH = work.bottom - work.top;
	const int clientW = (workW - 80 < 1920) ? (workW - 80) : 1920;
	const int clientH = (workH - 80 < 1080) ? (workH - 80) : 1080;
	RECT rect{ 0, 0, clientW, clientH };
	AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, FALSE, 0);

	HWND hwnd = CreateWindowExW(
		0,
		L"EidolonWindow",
		L"Eidolon Editor",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		rect.right - rect.left, rect.bottom - rect.top,
		nullptr,
		nullptr,
		hInstance,
		nullptr
	);
	if (hwnd == nullptr)
	{
		MessageBoxW(nullptr, L"创建窗口失败", L"错误", MB_ICONERROR);
		return 1;
	}

	ShowWindow(hwnd, nShowCmd);
	UpdateWindow(hwnd);

	if (!CreateD3D12Device(gDevice))
	{
		MessageBoxW(nullptr, L"创建 D3D12 设备失败，详情看“输出”窗口", L"错误", MB_ICONERROR);
		return 1;
	}

	CreateCommandObjects();
	CreateSwapChain(hwnd);
	CreateRenderTargets();

	MSG msg{};
	bool running = true;
	auto prevTime = std::chrono::steady_clock::now();
	int frames = 0;
	double accum = 0.0;
	while (running)
	{
		while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				running = false;
				break;
			}
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}

		if (!running)
		{
			break;
		}

		const auto nowTime = std::chrono::steady_clock::now();
		float deltaTime = std::chrono::duration<float>(nowTime - prevTime).count();
		prevTime = nowTime;

		/* vvv vvv */
		RenderFrame(gElapsedSecond);
		gElapsedSecond += 0.001f;
		WaitForGpu();
		/* ^^^ ^^^ */

		frames++;
		accum += deltaTime;
		if (accum >= 0.5)
		{
			RECT clientRect{};
			GetClientRect(hwnd, &clientRect);
			WCHAR title[160];
			swprintf_s(title, _countof(title), L"Eidolon Editor - %ld * %ld - %.0f fps", clientRect.right - clientRect.left, clientRect.bottom - clientRect.top, frames / accum);
			SetWindowTextW(hwnd, title);
			frames = 0;
			accum = 0.0;
		}
	}

	WaitForGpu();
	CloseHandle(gFenceEvent);
	return 0;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
		{
			DestroyWindow(hwnd);
		}
		return 0;
	}
	return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static void LogHresult(const wchar* what, HRESULT hr)
{
	WCHAR buffer[256];
	swprintf_s(buffer, _countof(buffer), L"[DX12] %s 失败: 0x%08x\n", what, hr);
	OutputDebugStringW(buffer);
}

static bool IsWindows11OrGreater()
{
	using Fn = LONG(WINAPI*)(PRTL_OSVERSIONINFOW);
	HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
	auto fn = reinterpret_cast<Fn>(GetProcAddress(ntdll, "RtlGetVersion"));
	if (!fn)
	{
		return false;
	}

	RTL_OSVERSIONINFOW vi{};
	vi.dwOSVersionInfoSize = sizeof(vi);
	if (fn(&vi) != 0)
		return false;

	return vi.dwBuildNumber >= 22000;   // 22000 = Win11 21H2
}

static bool CreateD3D12Device(ComPtr<ID3D12Device>& outDevice)
{
	// 调试层
#ifdef EIDOLON_DEBUG
	{
		ComPtr<ID3D12Debug> debug;
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
	// DXGI工厂
	
	HRESULT hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&gFactory));
	if (FAILED(hr))
	{
		LogHresult(L"CreateDXGIFactory2", hr);
		return false;
	}

	// 枚举显卡
	ComPtr<IDXGIAdapter1> chosenAdapter;
	for (uint32 i = 0;; ++i)
	{
		ComPtr<IDXGIAdapter1> adapter;
		if (gFactory->EnumAdapters1(i, adapter.GetAddressOf()) == DXGI_ERROR_NOT_FOUND)
		{
			break;
		}
		DXGI_ADAPTER_DESC1 desc{};
		adapter->GetDesc1(&desc);
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			continue;
		}
		wchar buffer[256];
		swprintf_s(buffer, _countof(buffer), L"[D3D12] 适配器 %u: %s | 显存 %llu MB\n", i, desc.Description, desc.DedicatedVideoMemory / 1024 / 1024);
		OutputDebugStringW(buffer);

		// 查询是否支持DirectX12
		if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), nullptr)))
		{
			chosenAdapter = adapter;
			break;
		}
	}

	if (!chosenAdapter)
	{
		OutputDebugStringW(L"[DX12] 未找到支持DirectX12的显示适配器\n");
		return false;
	}

	hr = D3D12CreateDevice(chosenAdapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&outDevice));
	if (FAILED(hr))
	{
		LogHresult(L"[D3D12] D3D12CreateDevice\n", hr);
		return false;
	}
	OutputDebugStringW(L"[D3D12] 设备创建成功\n");
	return true;
}

static bool CreateCommandObjects()
{
	D3D12_COMMAND_QUEUE_DESC desc{};
	desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask = 0;

	// 创建Queue
	HRESULT hr = gDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&gQueue));
	if (FAILED(hr))
	{
		LogHresult(L"CreateCommandQueue", hr);
		return false;
	}
	gQueue->SetName(L"MainQueue");

	// 创建Allocator
	hr = gDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&gAllocator));
	if (FAILED(hr))
	{
		LogHresult(L"CreateCommandAllocator", hr);
		return false;
	}

	hr = gDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, gAllocator.Get(), nullptr, IID_PPV_ARGS(&gList));
	if (FAILED(hr))
	{
		LogHresult(L"CreateCommandList", hr);
		return false;
	}
	gList->Close(); // 默认开启，创建后立即关闭。

	hr = gDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&gFence));
	if (FAILED(hr))
	{
		LogHresult(L"CreateFence", hr);
		return false;
	}
	gFenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);

	if (!gFenceEvent)
	{
		LogHresult(L"CreateEventW", hr);
		return false;
	}

	Logger::Log(L"[Eidolon] CommandObjects创建成功");
}

static bool CreateSwapChain(HWND hwnd)
{
	DXGI_SWAP_CHAIN_DESC1 desc{};
	desc.Width = 0;	// 0: 跟随客户区
	desc.Height = 0;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.Stereo = FALSE;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.BufferCount = EIDOLON_FRAME_COUNT;
	desc.Scaling = DXGI_SCALING_STRETCH;
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	desc.Flags = 0;

	// 创建SwapChain
	ComPtr<IDXGISwapChain1> swapChain;
	HRESULT hr = gFactory->CreateSwapChainForHwnd(gQueue.Get(), hwnd, &desc, nullptr, nullptr, &swapChain);
	if (FAILED(hr))
	{
		LogHresult(L"CreateSwapChainForHwnd", hr);
		return false;
	}

	// 升级SwapChain
	hr = swapChain.As(&gSwapChain);
	if (FAILED(hr))
	{
		LogHresult(L"QueryInterface SwapChain3", hr); return false;
	}

	gFactory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_WINDOW_CHANGES);

	Logger::Log(L"[Eidolon] 交换链创建成功：%u 缓冲，格式 R8G8B8A8_UNORM", EIDOLON_FRAME_COUNT);
	return true;
}

static bool CreateRenderTargets()
{
	D3D12_DESCRIPTOR_HEAP_DESC desc{};
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	desc.NumDescriptors = EIDOLON_FRAME_COUNT;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	desc.NodeMask = 0;

	HRESULT hr = gDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&gRtvHeap));
	if (FAILED(hr))
	{
		LogHresult(L"CreateDescriptorHeap(RTV)", hr);
		return false;
	}

	gRtvDescriptorSize = gDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_CPU_DESCRIPTOR_HANDLE rtv = gRtvHeap->GetCPUDescriptorHandleForHeapStart();

	for (uint8 i = 0; i < EIDOLON_FRAME_COUNT; ++i)
	{
		hr = gSwapChain->GetBuffer(i, IID_PPV_ARGS(&gRenderTargets[i]));
		if (FAILED(hr))
		{
			LogHresult(L"SwapChain::GetBuffer", hr); return false;
		}
		gDevice->CreateRenderTargetView(gRenderTargets[i].Get(), nullptr, rtv);
		rtv.ptr += gRtvDescriptorSize;
	}
	Logger::Log(L"[Eidolon] RTV 创建成功，增量 = %u 字节", gRtvDescriptorSize);
	return true;
}

static void RenderFrame(float deltaTime)
{
	gAllocator->Reset();
	gList->Reset(gAllocator.Get(), nullptr);

	// 获取当前帧
	const uint32 index = gSwapChain->GetCurrentBackBufferIndex();
	ID3D12Resource* backBuffer = gRenderTargets[index].Get();

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = backBuffer;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	gList->ResourceBarrier(1, &barrier);

	// 找到对应RTV
	D3D12_CPU_DESCRIPTOR_HANDLE rtv = gRtvHeap->GetCPUDescriptorHandleForHeapStart();
	rtv.ptr += static_cast<usize>(gRtvDescriptorSize) * index;

	// 清屏颜色
	const float r = 0.8f + 0.5f * sin(deltaTime * 1.0f);
	const float g = 0.3f + 0.5f * cos(deltaTime * 1.3f);
	const float b = 0.5f + 0.5f * sin(deltaTime * 0.7f);
	const float clearColor[4] = { r, g, b, 1.0f };

	gList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
	gList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	gList->ResourceBarrier(1, &barrier);

	gList->Close();
	ID3D12CommandList* lists[] = { gList.Get() };
	gQueue->ExecuteCommandLists(1, lists);
	gSwapChain->Present(1, 0);
}

static bool WaitForGpu()
{
	const uint64 value = ++gFenceValue;
	HRESULT hr = gQueue->Signal(gFence.Get(), value);	// 向gpu提交一条命令：将gFence置为value
	if (FAILED(hr))
	{
		LogHresult(L"Signal", hr);      // 常见原因：设备已移除
		return false;
	}
	if (gFence->GetCompletedValue() < value)	// gpu没执行到上述命令时执行
	{
		hr = gFence->SetEventOnCompletion(value, gFenceEvent);	// gFence》=value时执行gFenceEvent
		if (FAILED(hr))
		{
			LogHresult(L"SetEventOnCompletion", hr);
			return false;
		}
		if (WaitForSingleObject(gFenceEvent, INFINITE) != WAIT_OBJECT_0)
		{
			Logger::Log(L"[Eidolon] 等待 GPU 超时，设备移除原因 0x%08X",
			            static_cast<unsigned>(gDevice->GetDeviceRemovedReason()));
			return false;
		}
	}
	return true;
}