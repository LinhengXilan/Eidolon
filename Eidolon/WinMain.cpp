/**
 * @file WinMain.cpp
 * @author LinhengXilan
 * @version 0.0.0.1
 * @date 2026-11-2
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
#include <Windows.h>

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

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

	/* vvv 计算窗口大小 vvv */
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