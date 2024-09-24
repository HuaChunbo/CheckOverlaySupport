#include <d3d11.h>
#include <dxgi1_3.h>
#include <iostream>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

const char* DxgiFormatToString(DXGI_FORMAT format)
{
    switch (format) {
    case DXGI_FORMAT_R10G10B10A2_UNORM:
        return "RGB10A2";
    case DXGI_FORMAT_B8G8R8A8_UNORM:
        return "BGRA";
    case DXGI_FORMAT_YUY2:
        return "YUY2";
    case DXGI_FORMAT_NV12:
        return "NV12";
    case DXGI_FORMAT_P010:
        return "P010";
    default:
        return "UNKNOWN DXGI format";
    }
}

void CheckOverlaySupport(IDXGIAdapter* adapter, ID3D11Device* device, DXGI_FORMAT format)
{
    IDXGIOutput* output = nullptr;
    HRESULT hr = adapter->EnumOutputs(0, &output);
    if (FAILED(hr))
    {
        std::cout << "Failed to enumerate outputs.\n";
        return;
    }

    IDXGIOutput3* output3 = nullptr;
    hr = output->QueryInterface(__uuidof(IDXGIOutput3), (void**)&output3);
    if (FAILED(hr))
    {
        std::cout << "IDXGIOutput3 interface is not supported.\n";
        output->Release();
        return;
    }

    std::cout << "Start IDXGIOutput3::CheckOverlaySupport for " << DxgiFormatToString(format) << "-----------------" << std::endl;

    UINT flags = 0;
    hr = output3->CheckOverlaySupport(format, device, &flags);
    if (FAILED(hr))
    {
        std::cout << "Failed to check overlay support for format.\n";
    }
    else
    {
        std::cout << "IDXGIOutput3::CheckOverlaySupport returns " << flags << std::endl;
        if (flags & DXGI_OVERLAY_SUPPORT_FLAG_DIRECT)
        {
            std::cout << "Parsing CheckOverlaySupport::Overlay support is available (Direct).\n";
        }
        else if (flags & DXGI_OVERLAY_SUPPORT_FLAG_SCALING)
        {
            std::cout << "Parsing CheckOverlaySupport::Overlay support is available (Scaling).\n";
        }
        else
        {
            std::cout << "Parsing CheckOverlaySupport::Overlay support is not available.\n";
        }
    }

    std::cout << "End IDXGIOutput3::CheckOverlaySupport for " << DxgiFormatToString(format) << "-----------------" << std::endl;

    output3->Release();
    output->Release();
}

int main()
{
    HRESULT hr = S_OK;

    // Create a DXGI factory
    IDXGIFactory1* factory = nullptr;
    hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&factory);
    if (FAILED(hr))
    {
        std::cout << "Failed to create DXGIFactory1.\n";
        return -1;
    }

    // Create a D3D11 device
    IDXGIAdapter* adapter = nullptr;
    D3D_FEATURE_LEVEL featureLevel;
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;

    hr = factory->EnumAdapters(0, &adapter);
    if (FAILED(hr))
    {
        std::cout << "Failed to enumerate adapters.\n";
        factory->Release();
        return -1;
    }

    hr = D3D11CreateDevice(
        adapter,
        D3D_DRIVER_TYPE_UNKNOWN,
        NULL,
        0,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &device,
        &featureLevel,
        &context
    );

    if (FAILED(hr))
    {
        std::cout << "Failed to create D3D11 device.\n";
        adapter->Release();
        factory->Release();
        return -1;
    }

    // Check overlay support for P010 format - 104
    CheckOverlaySupport(adapter, device, DXGI_FORMAT_P010);

    // Check overlay support for R10G10B10A2_UNORM format - 24
    CheckOverlaySupport(adapter, device, DXGI_FORMAT_R10G10B10A2_UNORM);

    // Check overlay support for B8G8R8A8_UNORM format - 87
    CheckOverlaySupport(adapter, device, DXGI_FORMAT_B8G8R8A8_UNORM);

    // Check overlay support for NV12 format - 103
    CheckOverlaySupport(adapter, device, DXGI_FORMAT_NV12);

    // Check overlay support for YUY2 format - 107
    CheckOverlaySupport(adapter, device, DXGI_FORMAT_YUY2);

    // Cleanup
    context->Release();
    device->Release();
    adapter->Release();
    factory->Release();

    return 0;
}
