/**
 * @file WinMain.cpp
 * @author LinhengXilan
 * @version 0.0.0.3
 * @date 2026-11-3
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

using Microsoft::WRL::ComPtr;
using wchar = wchar_t;
using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;
using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;
using usize = uint64;


LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void LogHresult(const wchar* what, HRESULT hr);
static bool CreateD3D12Device(ComPtr<ID3D12Device>& outDevice);

int WINAPI wWinMain(HINSTANCE hInstance, [[maybe_unused]] HINSTANCE hPrevInstance, [[maybe_unused]] LPWSTR lpCmdLine, int nShowCmd)
{
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	WNDCLASSEX wndClass{};
	wndClass.cbSize = sizeof(wndClass);
	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc = WndProc;
	wndClass.hInstance = hInstance;
	wndClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wndClass.lpszClassName = L"EidolonWindow";
	if (RegisterClassEx(&wndClass) == 0)
	{
		MessageBox(nullptr, L"注册窗口类失败", L"错误", MB_ICONERROR);
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

	HWND hwnd = CreateWindowEx(
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
		MessageBox(nullptr, L"创建窗口失败", L"错误", MB_ICONERROR);
		return 1;
	}

	ShowWindow(hwnd, nShowCmd);
	UpdateWindow(hwnd);

	ComPtr<ID3D12Device> gDevice;
	if (!CreateD3D12Device(gDevice))
	{
		MessageBox(nullptr, L"创建 D3D12 设备失败，详情看“输出”窗口", L"错误", MB_ICONERROR);
		return 1;
	}

	MSG msg{};
	bool running = true;
	auto prevTime = std::chrono::steady_clock::now();
	int frames = 0;
	double accum = 0.0;
	while (running)
	{
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				running = false;
				break;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (!running)
		{
			break;
		}

		const auto nowTime = std::chrono::steady_clock::now();
		float deltaTime = std::chrono::duration<float>(nowTime - prevTime).count();
		prevTime = nowTime;
		if (deltaTime >= 0.1f)
		{
			deltaTime = 0.1f;
		}

		frames++;
		accum += deltaTime;
		if (accum >= 0.5)
		{
			RECT clientRect{};
			GetClientRect(hwnd, &clientRect);
			WCHAR title[160];
			swprintf_s(title, _countof(title), L"Eidolon Editor - %ld * %ld - %.0f fps", clientRect.right - clientRect.left, clientRect.bottom - clientRect.top, frames / accum);
			SetWindowText(hwnd, title);
			frames = 0;
			accum = 0.0;
		}
		Sleep(1);
	}
	
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
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

static void LogHresult(const wchar* what, HRESULT hr)
{
	WCHAR buffer[256];
	swprintf_s(buffer, _countof(buffer), L"[DX12] %s 失败: 0x%08x\n", what, hr);
	OutputDebugString(buffer);
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
			OutputDebugString(L"[D3D12] 调试层已启用\n");
		}
		else
		{
			OutputDebugString(L"[D3D12] 调试层不可用\n");
		}
	}
#endif
	// DXGI工厂
	ComPtr<IDXGIFactory7> factory;
	HRESULT hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&factory));
	if (FAILED(hr))
	{
		LogHresult(L"CreateDXGIFactory2", hr);
		return false;
	}

	// 枚举显卡
	ComPtr<IDXGIAdapter1> chosenAdapter;
	for (usize i = 0;; ++i)
	{
		ComPtr<IDXGIAdapter1> adapter;
		if (factory->EnumAdapters1(i, adapter.GetAddressOf()) == DXGI_ERROR_NOT_FOUND)
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
		swprintf_s(buffer, _countof(buffer), L"[D3D12] 适配器 %llu: %s | 显存 %llu MB\n", i, desc.Description, static_cast<uint64>(desc.DedicatedVideoMemory / 1024 / 1024));
		OutputDebugString(buffer);

		// 查询是否支持DirectX12
		if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), nullptr)))
		{
			chosenAdapter = adapter;
			break;
		}
	}

	if (!chosenAdapter)
	{
		OutputDebugString(L"[DX12] 未找到支持DirectX12的显示适配器\n");
		return false;
	}

	hr = D3D12CreateDevice(chosenAdapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&outDevice));
	if (FAILED(hr))
	{
		LogHresult(L"[D3D12] D3D12CreateDevice\n", hr);
		return false;
	}
	OutputDebugString(L"[D3D12] 设备创建成功\n");
	return true;
}