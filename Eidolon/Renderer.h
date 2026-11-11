/**
 * @file Renderer.h
 * @author LinhengXilan
 * @version 0.0.0.6
 * @date 2026-11-11
 */

#ifndef RENDERER_H
#define RENDERER_H

#include <Types.h>
#include <Windows.h>

using Microsoft::WRL::ComPtr;

class Renderer
{
public:
	Renderer() = default;
	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;
	~Renderer() = default;

public:
	void Initialize(HWND hwnd);
	void Shutdown();

	void BeginFrame(float deltaTime);
	bool WaitForGpu();
	void EndFrame();

private:
	static constexpr uint8 mFrameCount = 3;
	uint32 mRtvDescriptorSize = 0;
	uint64 mFenceValue = 0;
	HANDLE mFenceEvent = nullptr;

	ComPtr<IDXGIFactory7> mFactory;
	ComPtr<IDXGIAdapter4> mAdapter;
	ComPtr<ID3D12Device9> mDevice;
	ComPtr<ID3D12CommandQueue> mQueue;
	ComPtr<IDXGISwapChain3> mSwapChain;
	ComPtr<ID3D12DescriptorHeap> mRtvHeap;
	ComPtr<ID3D12Resource2> mRenderTargets[mFrameCount];
	ComPtr<ID3D12CommandAllocator> mAllocator;
	ComPtr<ID3D12GraphicsCommandList6> mList;
	ComPtr<ID3D12Fence1> mFence;

	float mElapsedSecond = 0.0f;
};

#endif