/**
 * @file Renderer.h
 * @author LinhengXilan
 * @version 0.0.0.7
 * @date 2026-11-19
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
	~Renderer();

public:
	void Initialize(HWND hwnd);
	void Shutdown();

	void BeginFrame(float deltaTime);
	void EndFrame();
	bool WaitForGpu();
	bool WaitForFence(uint64 value);

private:
	static constexpr uint8 FRAME_COUNT = 3;

	ComPtr<IDXGIFactory7> mFactory;
	ComPtr<IDXGIAdapter4> mAdapter;
	ComPtr<ID3D12Device9> mDevice;
	ComPtr<ID3D12CommandQueue> mQueue;
	ComPtr<IDXGISwapChain3> mSwapChain;
	ComPtr<ID3D12DescriptorHeap> mRtvHeap;
	ComPtr<ID3D12Resource2> mRenderTargets[FRAME_COUNT];
	ComPtr<ID3D12CommandAllocator> mAllocators[FRAME_COUNT];
	ComPtr<ID3D12GraphicsCommandList6> mLists[FRAME_COUNT];
	ComPtr<ID3D12Fence1> mFence;
	uint64 mFenceValue{};
	uint64 mFenceValues[FRAME_COUNT]{};
	HANDLE mFenceEvent{ nullptr };
	uint32 mRtvDescriptorSize{ 0 };
	uint32 mFrameIndex{ 0 };
	float mElapsedSecond{ 0.0f };
	bool mFrameActive{ false };
};

#endif