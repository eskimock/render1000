#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <random>
#include <chrono>
#include <iostream>
#include <fstream>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")


IDXGISwapChain* swapChain = nullptr;
ID3D11Device* device = nullptr;
ID3D11DeviceContext* context = nullptr;
ID3D11RenderTargetView* rtv = nullptr;
HWND hWnd = nullptr;
BOOL bFull = 1;

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_LBUTTONDBLCLK:
		bFull = !bFull;
		swapChain->SetFullscreenState(bFull, nullptr); 
		return 0;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}


bool InitWindow(HINSTANCE hInstance, int width, int height) {
	WNDCLASSEX wc = {};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = L"Render1000";
	RegisterClassEx(&wc);

	hWnd = CreateWindow(L"Render1000", L"Render1000", WS_POPUP | WS_VISIBLE,
		0, 0, width, height, nullptr, nullptr, hInstance, nullptr);
	if (!hWnd) return false;

	ShowWindow(hWnd, SW_SHOW);
	SetForegroundWindow(hWnd);
	return true;
}

bool InitDX11(int width, int height) {
	DXGI_SWAP_CHAIN_DESC swapDesc = {};
	swapDesc.BufferDesc.Width = width;
	swapDesc.BufferDesc.Height = height;
	swapDesc.BufferDesc.RefreshRate.Numerator = 1000;
	swapDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapDesc.BufferCount = 2;
	swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapDesc.OutputWindow = hWnd;
	swapDesc.SampleDesc.Count = 1;
	swapDesc.Windowed = 1; /*0; */// 全屏模式
	swapDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

	HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
		nullptr, 0, D3D11_SDK_VERSION, &swapDesc, &swapChain, &device, nullptr, &context);
	if (FAILED(hr)) return false;

	// 创建渲染目标视图
	ID3D11Texture2D* backBuffer = nullptr;
	hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
	if (FAILED(hr)) return false;

	hr = device->CreateRenderTargetView(backBuffer, nullptr, &rtv);
	backBuffer->Release();
	if (FAILED(hr)) return false;

	return true;
}

// 清理资源
void Cleanup() {
	if (rtv) rtv->Release();
	if (context) context->Release();
	if (device) device->Release();
	if (swapChain) {
		swapChain->SetFullscreenState(FALSE, nullptr);
		swapChain->Release();
	}
	if (hWnd) DestroyWindow(hWnd);
}


int WINAPI WinMain1(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
	
	if (!InitWindow(hInstance, 200, 200)) {
		std::cerr << "Failed to initialize window" << std::endl;
		return -1;
	}

	std::ofstream logFile("render_log.txt", std::ios::app);
	if (!logFile.is_open()) {
		std::cerr << "Failed to open log file: render_log.txt" << std::endl;
		return -1;
	}


	if (!InitDX11(200, 200)) {
		std::cerr << "Failed to initialize DX11" << std::endl;
		Cleanup();
		return -1;
	}

	// 随机颜色生成
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> dist(0.0f, 1.0f);

	// 控制每秒 1000 次 Present
	const auto targetInterval = std::chrono::microseconds(1000000 / 1000); // 1ms
	auto lastTime = std::chrono::high_resolution_clock::now();

	MSG msg = {};
	while (msg.message != WM_QUIT) {
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			auto now = std::chrono::high_resolution_clock::now();
			if (now - lastTime >= targetInterval) {
				// 生成随机颜色
				FLOAT clearColor[4] = { dist(gen), dist(gen), dist(gen), 1.0f };

				// 渲染
				context->OMSetRenderTargets(1, &rtv, nullptr);
				context->ClearRenderTargetView(rtv, clearColor);

				auto begin = std::chrono::high_resolution_clock::now();;
				swapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING); // 无垂直同步，每秒 1000 次
				auto end = std::chrono::high_resolution_clock::now();;
				/*if (bFull)*/ {
					logFile << "current:" << end.time_since_epoch().count() << " present cost: " << end.time_since_epoch().count() - begin.time_since_epoch().count() << std::endl;
				}			

				lastTime = now;
			}
		}
	}

	Cleanup();
	return 0;
}