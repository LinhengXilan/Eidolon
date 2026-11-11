/**
 * @file WinMain.cpp
 * @author LinhengXilan
 * @version 0.0.0.6
 * @date 2026-11-11
 */

#include <Pch.h>
#include <Renderer.h>

using Microsoft::WRL::ComPtr;

float gElapsedSecond = 0.0f;

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static bool IsWindows11OrGreater();

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
	const int clientW = workW - 80 < 1920 ? workW - 80 : 1920;
	const int clientH = workH - 80 < 1080 ? workH - 80 : 1080;
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


	Renderer renderer;
	renderer.Initialize(hwnd);

	MSG msg{};
	bool running = true;
	auto prevTime = std::chrono::steady_clock::now();
	int frames = 0;
	double accum = 0.0;
	RECT clientRect{};
	GetClientRect(hwnd, &clientRect);
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
		renderer.BeginFrame(gElapsedSecond);
		gElapsedSecond += 0.001f;
		renderer.WaitForGpu();
		/* ^^^ ^^^ */

		frames++;
		accum += deltaTime;
		if (accum >= 1)
		{
			WCHAR title[160];
			swprintf_s(title, _countof(title), L"Eidolon Editor - %ld * %ld - %.0f fps", clientRect.right - clientRect.left, clientRect.bottom - clientRect.top, frames / accum);
			SetWindowTextW(hwnd, title);
			frames = 0;
			accum = 0.0;
		}
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
	return DefWindowProcW(hwnd, msg, wParam, lParam);
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