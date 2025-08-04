#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <d2d1.h>
#include <dwrite.h>
#include <string>
#include <chrono>
#include <iostream>
#include <fstream>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")


extern int CheckHWComposeSupport();

IDXGISwapChain* swapChain = nullptr;
ID3D11Device* device = nullptr;
ID3D11DeviceContext* context = nullptr;
ID3D11RenderTargetView* rtv = nullptr;
ID2D1Factory* d2dFactory = nullptr;
ID2D1RenderTarget* d2dRenderTarget = nullptr;
IDWriteFactory* dwFactory = nullptr;
IDWriteTextFormat* textFormat = nullptr;
ID2D1SolidColorBrush* textBrush = nullptr;
HWND hWnd = nullptr;

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_RBUTTONDBLCLK:
	{
		int flag = CheckHWComposeSupport();
		::MessageBox(NULL, std::to_wstring(flag).c_str(), L"CheckHWComposeSupport", MB_OK);
		return 0;
	}
		 
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

bool InitWindow(HINSTANCE hInstance, int width, int height) {
	WNDCLASSEX wc = {};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
#ifndef MY_VSYNC
	wc.lpszClassName = L"Render1000-Tearing";
#else
	wc.lpszClassName = L"Render1000-VSync";
#endif
	RegisterClassEx(&wc);

#ifndef MY_VSYNC
	hWnd = CreateWindow(L"Render1000-Tearing", L"Render1000-Tearing", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		0, 0, width, height, nullptr, nullptr, hInstance, nullptr);
#else
	hWnd = CreateWindow(L"Render1000-VSync", L"Render1000-VSync", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		0, 500, width, height, nullptr, nullptr, hInstance, nullptr);
#endif

	if (!hWnd) return false;

	ShowWindow(hWnd, SW_SHOW);
	SetForegroundWindow(hWnd);
	return true;
}

bool InitDX11(int width, int height) {
	DXGI_SWAP_CHAIN_DESC swapDesc = {};
	swapDesc.BufferDesc.Width = width;
	swapDesc.BufferDesc.Height = height;
#ifndef MY_VSYNC
	swapDesc.BufferDesc.RefreshRate.Numerator = 1000;
	swapDesc.BufferDesc.RefreshRate.Denominator = 1;
#endif
	swapDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapDesc.BufferCount = 5;
	swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_BACK_BUFFER;
	swapDesc.OutputWindow = hWnd;
	swapDesc.SampleDesc.Count = 1;
	swapDesc.Windowed = 1;
	swapDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapDesc.Flags = 0;
#ifndef MY_VSYNC
	swapDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
#endif

	HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
		nullptr, 0, D3D11_SDK_VERSION, &swapDesc, &swapChain, &device, nullptr, &context);
	if (FAILED(hr)) {
		std::cerr << "Failed to create device and swap chain: " << std::hex << hr << std::endl;
		return false;
	}

	// Create render target view
	ID3D11Texture2D* backBuffer = nullptr;
	hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
	if (FAILED(hr)) {
		std::cerr << "Failed to get back buffer: " << std::hex << hr << std::endl;
		return false;
	}

	hr = device->CreateRenderTargetView(backBuffer, nullptr, &rtv);
	if (FAILED(hr)) {
		std::cerr << "Failed to create render target view: " << std::hex << hr << std::endl;
		backBuffer->Release();
		return false;
	}

	// Initialize Direct2D
	D2D1_FACTORY_OPTIONS options = {};
	options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION; // Enable debug info for troubleshooting
	hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory), &options, (void**)&d2dFactory);
	if (FAILED(hr)) {
		std::cerr << "Failed to create D2D1 factory: " << std::hex << hr << std::endl;
		backBuffer->Release();
		return false;
	}

	// Query DXGI surface from back buffer
	IDXGISurface* dxgiSurface = nullptr;
	hr = backBuffer->QueryInterface(__uuidof(IDXGISurface), (void**)&dxgiSurface);
	backBuffer->Release(); // Release back buffer as soon as possible
	if (FAILED(hr)) {
		std::cerr << "Failed to query DXGI surface: " << std::hex << hr << std::endl;
		return false;
	}

	// Create Direct2D render target
	D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
		D2D1_RENDER_TARGET_TYPE_DEFAULT,
		D2D1::PixelFormat(DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
		0.0f, 0.0f, // Use default DPI
		D2D1_RENDER_TARGET_USAGE_NONE,
		D2D1_FEATURE_LEVEL_DEFAULT
	);
	hr = d2dFactory->CreateDxgiSurfaceRenderTarget(dxgiSurface, &props, &d2dRenderTarget);
	dxgiSurface->Release();
	if (FAILED(hr)) {
		std::cerr << "Failed to create DXGI surface render target: " << std::hex << hr << std::endl;
		return false;
	}

	// Initialize DirectWrite
	hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&dwFactory);
	if (FAILED(hr)) {
		std::cerr << "Failed to create DirectWrite factory: " << std::hex << hr << std::endl;
		return false;
	}

	// Create text format
	hr = dwFactory->CreateTextFormat(L"Arial", nullptr, DWRITE_FONT_WEIGHT_NORMAL,
		DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 64.0f, L"en-us", &textFormat);
	if (FAILED(hr)) {
		std::cerr << "Failed to create text format: " << std::hex << hr << std::endl;
		return false;
	}

	textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

	// Create text brush
	hr = d2dRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &textBrush);
	if (FAILED(hr)) {
		std::cerr << "Failed to create text brush: " << std::hex << hr << std::endl;
		return false;
	}

	return true;
}

void Cleanup() {
	if (textBrush) textBrush->Release();
	if (textFormat) textFormat->Release();
	if (dwFactory) dwFactory->Release();
	if (d2dRenderTarget) d2dRenderTarget->Release();
	if (d2dFactory) d2dFactory->Release();
	if (rtv) rtv->Release();
	if (context) context->Release();
	if (device) device->Release();
	if (swapChain) {
		swapChain->SetFullscreenState(FALSE, nullptr);
		swapChain->Release();
	}
	if (hWnd) DestroyWindow(hWnd);
}

int syncInterval = 5;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {

	if (!InitWindow(hInstance, 700, 400)) {
		std::cerr << "Failed to initialize window" << std::endl;
		return -1;
	}

#ifndef MY_VSYNC
	std::ofstream logFile("render_tearing_log.txt", std::ios::app);
	if (!logFile.is_open()) {
		std::cerr << "Failed to open log file: render_tearing_log.txt" << std::endl;
		return -1;
	}
#else
	std::ofstream logFile("render_vsync_log.txt", std::ios::app);
	if (!logFile.is_open()) {
		std::cerr << "Failed to open log file: render_vsync_log.txt" << std::endl;
		return -1;
	}
#endif
	
	if (!InitDX11(700, 400)) {
		std::cerr << "Failed to initialize DX11" << std::endl;
		Cleanup();
		return -1;
	}


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
				// Clear background
				FLOAT clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
				context->OMSetRenderTargets(1, &rtv, nullptr);
				context->ClearRenderTargetView(rtv, clearColor);

				// Get current time in milliseconds
				auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
					now.time_since_epoch()).count();
#ifndef MY_VSYNC
				std::wstring timeStr = L"Tearing-";
#else
				std::wstring timeStr = L"VSync-";
#endif
				timeStr +=std::to_wstring(ms);

				// Render text
				d2dRenderTarget->BeginDraw();
				d2dRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::Black));
				D2D1_RECT_F rect = D2D1::RectF(50, 50, 650, 300);
				d2dRenderTarget->DrawTextW(timeStr.c_str(), timeStr.length(), textFormat,
					rect, textBrush);
				d2dRenderTarget->EndDraw();

				auto begin = std::chrono::high_resolution_clock::now();

#if 0
				switch (syncInterval)
				{
				case 5:
					swapChain->Present(3, /*DXGI_PRESENT_ALLOW_TEARING*/0);
					break;
				case 4:
					swapChain->Present(0, /*DXGI_PRESENT_ALLOW_TEARING*/0);
					break;
				case 3:
					swapChain->Present(0, /*DXGI_PRESENT_ALLOW_TEARING*/0);
					break;
				case 2:
					swapChain->Present(1, /*DXGI_PRESENT_ALLOW_TEARING*/0);
					break;
				case 1:
					swapChain->Present(0, /*DXGI_PRESENT_ALLOW_TEARING*/0);
					break;
				}
				syncInterval--;
				if (syncInterval == 0) {
					syncInterval = 5;
				}
#endif

#ifndef MY_VSYNC
				swapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
#else
				swapChain->Present(1, 0);
#endif

				auto end = std::chrono::high_resolution_clock::now();
				logFile << "current: " << end.time_since_epoch().count()
					<< " present cost: " << (end - begin).count() << std::endl;

				lastTime = now;
			}
		}
	}

	Cleanup();
	return 0;
}