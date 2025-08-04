#include <dxgi1_6.h>
#include <iostream>
#include <comdef.h>

// Function to print HRESULT errors
void PrintError(HRESULT hr) {
	_com_error err(hr);
	std::wcout << L"Error: " << err.ErrorMessage() << std::endl;
}

int CheckHWComposeSupport() {
	HRESULT hr;

	// Create DXGI factory
	IDXGIFactory6* pFactory = nullptr;
	hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&pFactory));
	if (FAILED(hr)) {
		PrintError(hr);
		return -1;
	}

	// Enumerate adapters
	IDXGIAdapter* pAdapter = nullptr;
	hr = pFactory->EnumAdapters(0, &pAdapter);
	if (FAILED(hr)) {
		PrintError(hr);
		pFactory->Release();
		return -1;
	}

	// Get the first output (monitor) from the adapter
	IDXGIOutput* pOutput = nullptr;
	hr = pAdapter->EnumOutputs(0, &pOutput);
	if (FAILED(hr)) {
		PrintError(hr);
		pAdapter->Release();
		pFactory->Release();
		return -1;
	}

	// Query for IDXGIOutput6 interface
	IDXGIOutput6* pOutput6 = nullptr;
	hr = pOutput->QueryInterface(IID_PPV_ARGS(&pOutput6));
	if (FAILED(hr)) {
		PrintError(hr);
		pOutput->Release();
		pAdapter->Release();
		pFactory->Release();
		return -1;
	}

	// Check hardware composition support
	UINT flags = 0;
	hr = pOutput6->CheckHardwareCompositionSupport(&flags);
	if (SUCCEEDED(hr)) {
		std::cout << "Hardware Composition Support Flags: " << flags << std::endl;

		// Check specific support flags
		if (flags & DXGI_HARDWARE_COMPOSITION_SUPPORT_FLAGS::DXGI_HARDWARE_COMPOSITION_SUPPORT_FLAG_FULLSCREEN) {
			std::cout << "Fullscreen composition supported" << std::endl;
		}
		if (flags & DXGI_HARDWARE_COMPOSITION_SUPPORT_FLAGS::DXGI_HARDWARE_COMPOSITION_SUPPORT_FLAG_WINDOWED) {
			std::cout << "Windowed composition supported" << std::endl;
		}
		if (flags & DXGI_HARDWARE_COMPOSITION_SUPPORT_FLAGS::DXGI_HARDWARE_COMPOSITION_SUPPORT_FLAG_CURSOR_STRETCHED) {
			std::cout << "Cursor stretching supported" << std::endl;
		}
	}
	else {
		PrintError(hr);
	}

	// Cleanup
	pOutput6->Release();
	pOutput->Release();
	pAdapter->Release();
	pFactory->Release();

	return flags;
}