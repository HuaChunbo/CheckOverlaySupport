#include <d3d11.h>
#include <d3d11_1.h>
#include <d3d11_4.h>
#include <dcomp.h>
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

const char* DxgiColorSpaceToString(DXGI_COLOR_SPACE_TYPE colorSpace)
{
    switch (colorSpace) {
    case DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P709:
        return "\"YUV Studio G22 Left BT.709\"";
    case DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020:
        return "\"RGB Full G2084 None BT.2020\"";
    case DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P709:
        return "\"RGB Studio G22 None BT.709\"";
    default:
        return "Unaccepted DXGI color space";
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

        if (flags & DXGI_OVERLAY_SUPPORT_FLAG_SCALING)
        {
            std::cout << "Parsing CheckOverlaySupport::Overlay support is available (Scaling).\n";
        }

        if (!flags)
        {
            std::cout << "Parsing CheckOverlaySupport::Overlay support is not available.\n";
        }
    }

    std::cout << "End IDXGIOutput3::CheckOverlaySupport for " << DxgiFormatToString(format) << "-----------------" << std::endl;

    output3->Release();
    output->Release();
}

void TestSwapChainSetColorSpace1(IDXGIAdapter* adapter, ID3D11Device* device, DXGI_FORMAT format, DXGI_COLOR_SPACE_TYPE colorSpace)
{
    HRESULT hr = S_OK;
    // Get IDXGIFactoryMedia instance
    IDXGIFactoryMedia* mediaFactory = nullptr;
    hr = adapter->GetParent(IID_PPV_ARGS(&mediaFactory));
    if (FAILED(hr))
    {
        std::cout << "Failed to get DXGIFactoryMedia.\n";
        return;
    }

    // Setup DXGI_SWAP_CHAIN_DESC1
    DXGI_SWAP_CHAIN_DESC1 desc = {};
    desc.Width = 1920;
    desc.Height = 1080;
    desc.Format = format;
    desc.Stereo = FALSE;
    desc.SampleDesc.Count = 1;
    desc.BufferCount = 2;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.Scaling = DXGI_SCALING_STRETCH;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    desc.Flags =
        DXGI_SWAP_CHAIN_FLAG_YUV_VIDEO | DXGI_SWAP_CHAIN_FLAG_FULLSCREEN_VIDEO |
        DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING | DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
    desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

    // Different flags required for RBG format
    if (format != DXGI_FORMAT_NV12 && format != DXGI_FORMAT_YUY2 && format != DXGI_FORMAT_P010)
    {
        desc.Flags = DXGI_SWAP_CHAIN_FLAG_FULLSCREEN_VIDEO | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING |
            DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
    }

    // Create surface handle for IDXGISwapChain1
    HANDLE handle = INVALID_HANDLE_VALUE;
    using PFN_DCOMPOSITION_CREATE_SURFACE_HANDLE =
        HRESULT(WINAPI*)(DWORD, SECURITY_ATTRIBUTES*, HANDLE*);
    static PFN_DCOMPOSITION_CREATE_SURFACE_HANDLE create_surface_handle_function =
        nullptr;
    if (!create_surface_handle_function) {
        HMODULE dcomp = ::LoadLibraryEx(L"dcomp.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (!dcomp) {
            std::cout << "Failed to get handle for dcomp.dll" << std::endl;
            return;
        }
        create_surface_handle_function =
            reinterpret_cast<PFN_DCOMPOSITION_CREATE_SURFACE_HANDLE>(
                ::GetProcAddress(dcomp, "DCompositionCreateSurfaceHandle"));
        if (!create_surface_handle_function) {
            std::cout << "Failed to get address for DCompositionCreateSurfaceHandle" << std::endl;
            return;
        }
    }
    hr = create_surface_handle_function(COMPOSITIONOBJECT_ALL_ACCESS, nullptr, &handle);

    // Create IDXGISwapChain1 instance
    IDXGISwapChain1* swapchain1 = nullptr;
    hr = mediaFactory->CreateSwapChainForCompositionSurfaceHandle(
        device, handle, &desc, nullptr, &swapchain1);
    if (SUCCEEDED(hr))
    {
        std::cout << "Successfully created IDXGISwapChain1 with " << DxgiFormatToString(format) << std::endl;
    }
    else
    {
        std::cout << "Failed to create IDXGISwapChain1 with " << DxgiFormatToString(format) << " error 0x" << std::hex << hr << std::endl;
        return;
    }

    // QI IDXGISwapChain3 for SetColorSpace1 API
    IDXGISwapChain3* swapchain3 = nullptr;
    hr = swapchain1->QueryInterface(__uuidof(IDXGISwapChain3), (void**)&swapchain3);
    if (SUCCEEDED(hr))
    {
        // ?? Fail with E_INVALIDARG (0x80070057) if the swap chain does not support the
        // DXGI color space or something goes wrong internally.
        hr = swapchain3->SetColorSpace1(colorSpace);
        if (FAILED(hr)) {
            std::cout << "Failed to SetColorSpace1 with color space: " << DxgiColorSpaceToString(colorSpace) << " error: 0x" << std::hex << hr << std::endl;
        }
        else
        {
            std::cout << "Successfully SetColorSpace1 with color space: " << DxgiColorSpaceToString(colorSpace) << std::endl;
        }

        UINT flags = 0;
        hr = swapchain3->CheckColorSpaceSupport(colorSpace, &flags);
        if (FAILED(hr))
        {
            std::cout << "Failed to check color space support for " << DxgiColorSpaceToString(colorSpace) << std::endl;
        }
        else
        {
            std::cout << "IDXGISwapChain3::CheckColorSpaceSupport returns " << flags << std::endl;
            if (flags & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT)
            {
                std::cout << "Parsing IDXGISwapChain3::CheckColorSpaceSupport::PRESENT support is available (Non-Overlay).\n";
            }

            if (flags & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_OVERLAY_PRESENT)
            {
                std::cout << "Parsing IDXGISwapChain3::CheckColorSpaceSupport::OVERLAY_PRESENT support is available (Overlay).\n";
            }

            if (!flags)
            {
                std::cout << "Parsing IDXGISwapChain3::CheckColorSpaceSupport support is not available. \n";
            }
        }
    }

    swapchain3->Release();
    swapchain1->Release();
    ::CloseHandle(handle);
    mediaFactory->Release();
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

    // 1. IDXGIOutput3::CheckOverlaySupport cap test
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

    // 2. IDXGISwapChain3::SetColorSpace1 cap test
    // 8-bit SDR: BGRA8 + BI.709
    TestSwapChainSetColorSpace1(adapter, device, DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P709);
    // 8-bit SDR: NV12 + BT.709
    TestSwapChainSetColorSpace1(adapter, device, DXGI_FORMAT_NV12, DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P709);
    // 10-bit HDR: RGB10A2 + BT.2020
    TestSwapChainSetColorSpace1(adapter, device, DXGI_FORMAT_R10G10B10A2_UNORM, DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020);

    // Cleanup
    context->Release();
    device->Release();
    adapter->Release();
    factory->Release();

    return 0;
}
