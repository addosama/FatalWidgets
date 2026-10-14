// Dear ImGui: standalone example application for Windows API + DirectX 12

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"
#include <d3d12.h>
#include <dxgi1_5.h>
#include <tchar.h>

#include "font_iconfont.h"
#include "imgui_internal.h"

#ifdef _DEBUG
#define DX12_ENABLE_DEBUG_LAYER
#endif

#ifdef DX12_ENABLE_DEBUG_LAYER
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif

// Config for example app
static const int APP_NUM_FRAMES_IN_FLIGHT = 2;
static const int APP_NUM_BACK_BUFFERS = 2;
static const int APP_SRV_HEAP_SIZE = 64;

struct FrameContext
{
    ID3D12CommandAllocator* CommandAllocator;
    UINT64                      FenceValue;
};

// Simple free list based allocator
struct ExampleDescriptorHeapAllocator
{
    ID3D12DescriptorHeap* Heap = nullptr;
    D3D12_DESCRIPTOR_HEAP_TYPE  HeapType = D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
    D3D12_CPU_DESCRIPTOR_HANDLE HeapStartCpu;
    D3D12_GPU_DESCRIPTOR_HANDLE HeapStartGpu;
    UINT                        HeapHandleIncrement;
    ImVector<int>               FreeIndices;

    void Create(ID3D12Device* device, ID3D12DescriptorHeap* heap)
    {
        IM_ASSERT(Heap == nullptr && FreeIndices.empty());
        Heap = heap;
        D3D12_DESCRIPTOR_HEAP_DESC desc = heap->GetDesc();
        HeapType = desc.Type;
        HeapStartCpu = Heap->GetCPUDescriptorHandleForHeapStart();
        HeapStartGpu = Heap->GetGPUDescriptorHandleForHeapStart();
        HeapHandleIncrement = device->GetDescriptorHandleIncrementSize(HeapType);
        FreeIndices.reserve((int)desc.NumDescriptors);
        for (int n = desc.NumDescriptors; n > 0; n--)
            FreeIndices.push_back(n - 1);
    }
    void Destroy()
    {
        Heap = nullptr;
        FreeIndices.clear();
    }
    void Alloc(D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_desc_handle)
    {
        IM_ASSERT(FreeIndices.Size > 0);
        int idx = FreeIndices.back();
        FreeIndices.pop_back();
        out_cpu_desc_handle->ptr = HeapStartCpu.ptr + (idx * HeapHandleIncrement);
        out_gpu_desc_handle->ptr = HeapStartGpu.ptr + (idx * HeapHandleIncrement);
    }
    void Free(D3D12_CPU_DESCRIPTOR_HANDLE out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE out_gpu_desc_handle)
    {
        int cpu_idx = (int)((out_cpu_desc_handle.ptr - HeapStartCpu.ptr) / HeapHandleIncrement);
        int gpu_idx = (int)((out_gpu_desc_handle.ptr - HeapStartGpu.ptr) / HeapHandleIncrement);
        IM_ASSERT(cpu_idx == gpu_idx);
        FreeIndices.push_back(cpu_idx);
    }
};

// Data
static FrameContext                 g_frameContext[APP_NUM_FRAMES_IN_FLIGHT] = {};
static UINT                         g_frameIndex = 0;

static ID3D12Device* g_pd3dDevice = nullptr;
static ID3D12DescriptorHeap* g_pd3dRtvDescHeap = nullptr;
static ID3D12DescriptorHeap* g_pd3dSrvDescHeap = nullptr;
static ExampleDescriptorHeapAllocator g_pd3dSrvDescHeapAlloc;
static ID3D12CommandQueue* g_pd3dCommandQueue = nullptr;
static ID3D12GraphicsCommandList* g_pd3dCommandList = nullptr;
static ID3D12Fence* g_fence = nullptr;
static HANDLE                       g_fenceEvent = nullptr;
static UINT64                       g_fenceLastSignaledValue = 0;
static IDXGISwapChain3* g_pSwapChain = nullptr;
static bool                         g_SwapChainTearingSupport = false;
static bool                         g_SwapChainOccluded = false;
static HANDLE                       g_hSwapChainWaitableObject = nullptr;
static ID3D12Resource* g_mainRenderTargetResource[APP_NUM_BACK_BUFFERS] = {};
static D3D12_CPU_DESCRIPTOR_HANDLE  g_mainRenderTargetDescriptor[APP_NUM_BACK_BUFFERS] = {};

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
void WaitForPendingOperations();
FrameContext* WaitForNextFrameContext();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// declar FatalWidgets test function
void loadFonts();
void drawFatalWidgets();
void shadowTest();

// Main code
int main(int, char**)
{
    // Make process DPI aware and obtain main monitor scale
    ImGui_ImplWin32_EnableDpiAwareness();
    float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    // Create application window
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Dear ImGui DirectX12 Example", WS_OVERLAPPEDWINDOW, 100, 100, (int)(1280 * main_scale), (int)(800 * main_scale), nullptr, nullptr, wc.hInstance, nullptr);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    loadFonts();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;        // Set initial font scale. (in docking branch: using io.ConfigDpiScaleFonts=true automatically overrides this for every window depending on the current monitor)

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);

    ImGui_ImplDX12_InitInfo init_info = {};
    init_info.Device = g_pd3dDevice;
    init_info.CommandQueue = g_pd3dCommandQueue;
    init_info.NumFramesInFlight = APP_NUM_FRAMES_IN_FLIGHT;
    init_info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    init_info.DSVFormat = DXGI_FORMAT_UNKNOWN;
    // Allocating SRV descriptors (for textures) is up to the application, so we provide callbacks.
    // (current version of the backend will only allocate one descriptor, future versions will need to allocate more)
    init_info.SrvDescriptorHeap = g_pd3dSrvDescHeap;
    init_info.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle) { return g_pd3dSrvDescHeapAlloc.Alloc(out_cpu_handle, out_gpu_handle); };
    init_info.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle) { return g_pd3dSrvDescHeapAlloc.Free(cpu_handle, gpu_handle); };
    ImGui_ImplDX12_Init(&init_info);

    // Before 1.91.6: our signature was using a single descriptor. From 1.92, specifying SrvDescriptorAllocFn/SrvDescriptorFreeFn will be required to benefit from new features.
    //ImGui_ImplDX12_Init(g_pd3dDevice, APP_NUM_FRAMES_IN_FLIGHT, DXGI_FORMAT_R8G8B8A8_UNORM, g_pd3dSrvDescHeap, g_pd3dSrvDescHeap->GetCPUDescriptorHandleForHeapStart(), g_pd3dSrvDescHeap->GetGPUDescriptorHandleForHeapStart());

    // Load Fonts
    // - If fonts are not explicitly loaded, Dear ImGui will select an embedded font: either AddFontDefaultVector() or AddFontDefaultBitmap().
    //   This selection is based on (style.FontSizeBase * style.FontScaleMain * style.FontScaleDpi) reaching a small threshold.
    // - You can load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - If a file cannot be loaded, AddFont functions will return a nullptr. Please handle those errors in your code (e.g. use an assertion, display an error and quit).
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use FreeType for higher quality font rendering.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //style.FontSizeBase = 20.0f;
    //io.Fonts->AddFontDefaultVector();
    //io.Fonts->AddFontDefaultBitmap();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf");
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf");
    //IM_ASSERT(font != nullptr);

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Handle window screen locked
        if ((g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) || ::IsIconic(hwnd))
        {
            ::Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        // Start the Dear ImGui frame
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &show_another_window);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

            if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }

        // 3. Show another simple window.
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();
        }

        // FatalWidgets test
        drawFatalWidgets();

        shadowTest();

        // Rendering
        ImGui::Render();

        FrameContext* frameCtx = WaitForNextFrameContext();
        UINT backBufferIdx = g_pSwapChain->GetCurrentBackBufferIndex();
        frameCtx->CommandAllocator->Reset();

        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = g_mainRenderTargetResource[backBufferIdx];
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        g_pd3dCommandList->Reset(frameCtx->CommandAllocator, nullptr);
        g_pd3dCommandList->ResourceBarrier(1, &barrier);

        // Render Dear ImGui graphics
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dCommandList->ClearRenderTargetView(g_mainRenderTargetDescriptor[backBufferIdx], clear_color_with_alpha, 0, nullptr);
        g_pd3dCommandList->OMSetRenderTargets(1, &g_mainRenderTargetDescriptor[backBufferIdx], FALSE, nullptr);
        g_pd3dCommandList->SetDescriptorHeaps(1, &g_pd3dSrvDescHeap);
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), g_pd3dCommandList);
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        g_pd3dCommandList->ResourceBarrier(1, &barrier);
        g_pd3dCommandList->Close();

        g_pd3dCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&g_pd3dCommandList);
        g_pd3dCommandQueue->Signal(g_fence, ++g_fenceLastSignaledValue);
        frameCtx->FenceValue = g_fenceLastSignaledValue;

        // Present
        HRESULT hr = g_pSwapChain->Present(1, 0);   // Present with vsync
        //HRESULT hr = g_pSwapChain->Present(0, g_SwapChainTearingSupport ? DXGI_PRESENT_ALLOW_TEARING : 0); // Present without vsync
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
        g_frameIndex++;
    }

    WaitForPendingOperations();

    // Cleanup
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    // This is a basic setup. Optimally could handle fullscreen mode differently. See #8979 for suggestions.
    DXGI_SWAP_CHAIN_DESC1 sd;
    {
        ZeroMemory(&sd, sizeof(sd));
        sd.BufferCount = APP_NUM_BACK_BUFFERS;
        sd.Width = 0;
        sd.Height = 0;
        sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        sd.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        sd.Scaling = DXGI_SCALING_STRETCH;
        sd.Stereo = FALSE;
    }

    // [DEBUG] Enable debug interface
#ifdef DX12_ENABLE_DEBUG_LAYER
    ID3D12Debug* pdx12Debug = nullptr;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pdx12Debug))))
        pdx12Debug->EnableDebugLayer();
#endif

    // Create device
    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
    if (D3D12CreateDevice(nullptr, featureLevel, IID_PPV_ARGS(&g_pd3dDevice)) != S_OK)
        return false;

    // [DEBUG] Setup debug interface to break on any warnings/errors
#ifdef DX12_ENABLE_DEBUG_LAYER
    if (pdx12Debug != nullptr)
    {
        ID3D12InfoQueue* pInfoQueue = nullptr;
        g_pd3dDevice->QueryInterface(IID_PPV_ARGS(&pInfoQueue));
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);

        // Disable breaking on this warning because of a suspected bug in the D3D12 SDK layer, see #9084 for details.
        const int D3D12_MESSAGE_ID_FENCE_ZERO_WAIT_ = 1424; // not in all copies of d3d12sdklayers.h
        D3D12_MESSAGE_ID disabledMessages[] = { (D3D12_MESSAGE_ID)D3D12_MESSAGE_ID_FENCE_ZERO_WAIT_ };
        D3D12_INFO_QUEUE_FILTER filter = {};
        filter.DenyList.NumIDs = 1;
        filter.DenyList.pIDList = disabledMessages;
        pInfoQueue->AddStorageFilterEntries(&filter);

        pInfoQueue->Release();
        pdx12Debug->Release();
    }
#endif

    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        desc.NumDescriptors = APP_NUM_BACK_BUFFERS;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        desc.NodeMask = 1;
        if (g_pd3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3dRtvDescHeap)) != S_OK)
            return false;

        SIZE_T rtvDescriptorSize = g_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = g_pd3dRtvDescHeap->GetCPUDescriptorHandleForHeapStart();
        for (UINT i = 0; i < APP_NUM_BACK_BUFFERS; i++)
        {
            g_mainRenderTargetDescriptor[i] = rtvHandle;
            rtvHandle.ptr += rtvDescriptorSize;
        }
    }

    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors = APP_SRV_HEAP_SIZE;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        if (g_pd3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3dSrvDescHeap)) != S_OK)
            return false;
        g_pd3dSrvDescHeapAlloc.Create(g_pd3dDevice, g_pd3dSrvDescHeap);
    }

    {
        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        desc.NodeMask = 1;
        if (g_pd3dDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&g_pd3dCommandQueue)) != S_OK)
            return false;
    }

    for (UINT i = 0; i < APP_NUM_FRAMES_IN_FLIGHT; i++)
        if (g_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_frameContext[i].CommandAllocator)) != S_OK)
            return false;

    if (g_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_frameContext[0].CommandAllocator, nullptr, IID_PPV_ARGS(&g_pd3dCommandList)) != S_OK ||
        g_pd3dCommandList->Close() != S_OK)
        return false;

    if (g_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_fence)) != S_OK)
        return false;

    g_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (g_fenceEvent == nullptr)
        return false;

    {
        IDXGIFactory5* dxgiFactory = nullptr;
        IDXGISwapChain1* swapChain1 = nullptr;
        if (CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)) != S_OK)
            return false;

        BOOL allow_tearing = FALSE;
        dxgiFactory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allow_tearing, sizeof(allow_tearing));
        g_SwapChainTearingSupport = (allow_tearing == TRUE);
        if (g_SwapChainTearingSupport)
            sd.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

        if (dxgiFactory->CreateSwapChainForHwnd(g_pd3dCommandQueue, hWnd, &sd, nullptr, nullptr, &swapChain1) != S_OK)
            return false;
        if (swapChain1->QueryInterface(IID_PPV_ARGS(&g_pSwapChain)) != S_OK)
            return false;
        if (g_SwapChainTearingSupport)
            dxgiFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);

        swapChain1->Release();
        dxgiFactory->Release();
        g_pSwapChain->SetMaximumFrameLatency(APP_NUM_BACK_BUFFERS);
        g_hSwapChainWaitableObject = g_pSwapChain->GetFrameLatencyWaitableObject();
    }

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->SetFullscreenState(false, nullptr); g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_hSwapChainWaitableObject != nullptr) { CloseHandle(g_hSwapChainWaitableObject); }
    for (UINT i = 0; i < APP_NUM_FRAMES_IN_FLIGHT; i++)
        if (g_frameContext[i].CommandAllocator) { g_frameContext[i].CommandAllocator->Release(); g_frameContext[i].CommandAllocator = nullptr; }
    if (g_pd3dCommandQueue) { g_pd3dCommandQueue->Release(); g_pd3dCommandQueue = nullptr; }
    if (g_pd3dCommandList) { g_pd3dCommandList->Release(); g_pd3dCommandList = nullptr; }
    if (g_pd3dRtvDescHeap) { g_pd3dRtvDescHeap->Release(); g_pd3dRtvDescHeap = nullptr; }
    if (g_pd3dSrvDescHeap) { g_pd3dSrvDescHeap->Release(); g_pd3dSrvDescHeap = nullptr; }
    if (g_fence) { g_fence->Release(); g_fence = nullptr; }
    if (g_fenceEvent) { CloseHandle(g_fenceEvent); g_fenceEvent = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }

#ifdef DX12_ENABLE_DEBUG_LAYER
    IDXGIDebug1* pDebug = nullptr;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug))))
    {
        pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY);
        pDebug->Release();
    }
#endif
}

void CreateRenderTarget()
{
    for (UINT i = 0; i < APP_NUM_BACK_BUFFERS; i++)
    {
        ID3D12Resource* pBackBuffer = nullptr;
        g_pSwapChain->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer));
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, g_mainRenderTargetDescriptor[i]);
        g_mainRenderTargetResource[i] = pBackBuffer;
    }
}

void CleanupRenderTarget()
{
    WaitForPendingOperations();

    for (UINT i = 0; i < APP_NUM_BACK_BUFFERS; i++)
        if (g_mainRenderTargetResource[i]) { g_mainRenderTargetResource[i]->Release(); g_mainRenderTargetResource[i] = nullptr; }
}

void WaitForPendingOperations()
{
    g_pd3dCommandQueue->Signal(g_fence, ++g_fenceLastSignaledValue);

    g_fence->SetEventOnCompletion(g_fenceLastSignaledValue, g_fenceEvent);
    ::WaitForSingleObject(g_fenceEvent, INFINITE);
}

FrameContext* WaitForNextFrameContext()
{
    FrameContext* frame_context = &g_frameContext[g_frameIndex % APP_NUM_FRAMES_IN_FLIGHT];
    if (g_fence->GetCompletedValue() < frame_context->FenceValue)
    {
        g_fence->SetEventOnCompletion(frame_context->FenceValue, g_fenceEvent);
        HANDLE waitableObjects[] = { g_hSwapChainWaitableObject, g_fenceEvent };
        ::WaitForMultipleObjects(2, waitableObjects, TRUE, INFINITE);
    }
    else
        ::WaitForSingleObject(g_hSwapChainWaitableObject, INFINITE);

    return frame_context;
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            DXGI_SWAP_CHAIN_DESC1 desc = {};
            g_pSwapChain->GetDesc1(&desc);
            HRESULT result = g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), desc.Format, desc.Flags);
            IM_ASSERT(SUCCEEDED(result) && "Failed to resize swapchain.");
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

#include "FatalWidgets.h"
#include "font_extrabold.h"
#include "font_medium.h"
#include "menu/Menu.h"

void TransformPoints(
    ImDrawList* draws,
    ImDrawIdx begin, ImDrawIdx end,
    const ImVec2& regionMin, const ImVec2& regionMax,
    const ImVec2& p0, const ImVec2& p1, const ImVec2& p2, const ImVec2& p3)
{
	if (ImMax(begin, end) > draws->VtxBuffer.size()) return;
	for (int i = begin; i < end; ++i)
	{
        ImDrawVert& vert = draws->VtxBuffer[i];
        const ImVec2& point = vert.pos;

        // ------------------------------------------------------------
		// 1. 首先判断 point 是否在原始 ImGui 区域内
		// ------------------------------------------------------------

        if (point.x < regionMin.x ||
            point.x > regionMax.x ||
            point.y < regionMin.y ||
            point.y > regionMax.y)
        {
            // 不在区域内：完全不变换
            continue;
        }

        // ------------------------------------------------------------
        // 2. 原始矩形 -> UV
        // ------------------------------------------------------------

        const float width = regionMax.x - regionMin.x;
        const float height = regionMax.y - regionMin.y;

        if (std::abs(width) < 1e-6f ||
            std::abs(height) < 1e-6f)
        {
            continue;
        }

        const float u =
            (point.x - regionMin.x) / width;

        const float v =
            (point.y - regionMin.y) / height;

        // ------------------------------------------------------------
        // 3. UV -> 透视后的四边形
        //
        //       p0 -------- p1
        //       |            |
        //       |            |
        //       p3 -------- p2
        //
        // 这里不能使用普通 bilinear interpolation，
        // 而是使用 Homography。
        // ------------------------------------------------------------

        const float dx1 = p1.x - p2.x;
        const float dx2 = p3.x - p2.x;
        const float dx3 = p0.x - p1.x + p2.x - p3.x;

        const float dy1 = p1.y - p2.y;
        const float dy2 = p3.y - p2.y;
        const float dy3 = p0.y - p1.y + p2.y - p3.y;

        const float denominator =
            dx1 * dy2 - dx2 * dy1;

        float g = 0.0f;
        float h = 0.0f;

        if (std::abs(dx3) > 1e-6f ||
            std::abs(dy3) > 1e-6f)
        {
            if (std::abs(denominator) < 1e-6f)
            {
                continue;
            }

            g =
                (dx3 * dy2 - dx2 * dy3) /
                denominator;

            h =
                (dx1 * dy3 - dx3 * dy1) /
                denominator;
        }

        const float a =
            p1.x - p0.x + g * p1.x;

        const float b =
            p3.x - p0.x + h * p3.x;

        const float c = p0.x;

        const float d =
            p1.y - p0.y + g * p1.y;

        const float e =
            p3.y - p0.y + h * p3.y;

        const float f = p0.y;

        // ------------------------------------------------------------
        // 4. 直接根据 UV 计算变换后的屏幕坐标
        //
        // x = (a*u + b*v + c) / (g*u + h*v + 1)
        // y = (d*u + e*v + f) / (g*u + h*v + 1)
        // ------------------------------------------------------------

        const float denominator2 =
            g * u + h * v + 1.0f;

        if (std::abs(denominator2) < 1e-6f)
        {
            continue;
        }

        const ImVec2 result = {
            (a * u + b * v + c) / denominator2,
            (d * u + e * v + f) / denominator2
        };

        vert.pos = result;
	}
}

ImFont* Medium12;
ImFont* ExtraBold20;
ImFont* IconFont;
void loadFonts() {
    Medium12 = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedBase85TTF(FontMedium_compressed_data_base85, 14);
    ExtraBold20 = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedBase85TTF(FontExtraBold_compressed_data_base85, 24);
    static constexpr ImWchar iconRange[] = { 0xE000, 0xF8FF, 0 };
    IconFont = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedBase85TTF(iconfont_compressed_data_base85, 16, 0, &iconRange[0]);
    ImGui::GetIO().IniFilename = nullptr;
}
void DrawLMB(ImDrawList* draws, ImVec2 pos, ImU32 color, float scale)
{
    auto P = [&](float x, float y) -> ImVec2
        {
            return ImVec2(
                pos.x + x * scale,
                pos.y + y * scale
            );
        };

    draws->PathClear();

    // M 0,10.666668
    draws->PathLineTo(P(0.0f, 10.666668f));

    // C 0,4.77563 4.0933971,0 9.1428576,0
    draws->PathBezierCubicCurveTo(
        P(0.0f, 4.77563f),
        P(4.0933971f, 0.0f),
        P(9.1428576f, 0.0f)
    );

    draws->PathLineTo(P(13.714286f, 0.0f));
    draws->PathLineTo(P(16.0f, 0.0f));
    draws->PathLineTo(P(16.0f, 2.666667f));
    draws->PathLineTo(P(16.0f, 5.0f));

    draws->PathLineTo(P(14.0f, 5.0f));
    draws->PathLineTo(P(14.0f, 19.0f));
    draws->PathLineTo(P(16.0f, 19.0f));

    draws->PathLineTo(P(16.0f, 21.333334f));
    draws->PathLineTo(P(16.0f, 24.0f));

    draws->PathLineTo(P(13.714286f, 24.0f));
    draws->PathLineTo(P(2.2857144f, 24.0f));

    // C 1.0233493,24 0,22.806095 0,21.333334
    draws->PathBezierCubicCurveTo(
        P(1.0233493f, 24.0f),
        P(0.0f, 22.806095f),
        P(0.0f, 21.333334f)
    );

    draws->PathLineTo(P(0.0f, 10.666668f));

    draws->PathFillConcave(color);
}
void DrawRMB(ImDrawList* draws, ImVec2 pos, ImU32 color, float scale)
{
    auto P = [&](float x, float y) -> ImVec2
        {
            return ImVec2(
                pos.x + (16 - x) * scale,
                pos.y + y * scale
            );
        };

    draws->PathClear();

    // Start at A = (0, 10.666668)

    // Reverse of the closing line:
    // D -> A
    draws->PathLineTo(P(0.0f, 21.333334f));

    // Reverse Bézier:
    // D -> C
    //
    // Original:
    // C -> C1 -> C2 -> D
    //
    // Reverse:
    // D -> C2 -> C1 -> C
    draws->PathBezierCubicCurveTo(
        P(0.0f, 22.806095f),     // original C2
        P(1.0233493f, 24.0f),    // original C1
        P(2.2857144f, 24.0f)     // original start
    );

    // Reverse straight segments
    draws->PathLineTo(P(13.714286f, 24.0f));
    draws->PathLineTo(P(16.0f, 24.0f));
    draws->PathLineTo(P(16.0f, 21.333334f));
    draws->PathLineTo(P(16.0f, 19.0f));

    draws->PathLineTo(P(14.0f, 19.0f));
    draws->PathLineTo(P(14.0f, 5.0f));
    draws->PathLineTo(P(16.0f, 5.0f));

    draws->PathLineTo(P(16.0f, 2.666667f));
    draws->PathLineTo(P(16.0f, 0.0f));
    draws->PathLineTo(P(13.714286f, 0.0f));
    draws->PathLineTo(P(9.1428576f, 0.0f));

    // Reverse the upper Bézier:
    //
    // Original:
    // A -> C1 -> C2 -> B
    //
    // Reverse:
    // B -> C2 -> C1 -> A
    draws->PathBezierCubicCurveTo(
        P(4.0933971f, 0.0f),      // original C2
        P(0.0f, 4.77563f),       // original C1
        P(0.0f, 10.666668f)      // original start
    );

    draws->PathFillConcave(color);
}
void drawFatalWidgets()
{
    const auto fDl = ImGui::GetForegroundDrawList();
    DrawLMB(fDl, ImVec2(100, 100), 0xFF0000FF, 2);
    DrawRMB(fDl, ImVec2(140, 100), 0xFF0000FF, 2);

    ImVec2 wPos, wSize;
    {
        static float f = 0.0f;
        static int counter = 0;

        ImGui::Begin("FatalWidgets test", nullptr, ImGuiWindowFlags_NoBackground);
        wPos = ImGui::GetWindowPos();
        wSize = ImGui::GetWindowSize();

        ImGui::PushFont(ExtraBold20);
        float animation = fmodf(ImGui::GetTime(), 1.0f);
        FatalWidgets::Rendering::DrawFatalTitle("FUTALITY", ImGui::GetCursorScreenPos(), animation, 1, 0xFFFFFFFF, 0xFF0000FF, 0xFFFF0000);
        ImGui::Dummy(ImGui::CalcTextSize("FUTALITY") + ImVec2(1, 1));
        ImGui::PopFont();

        static double doubleVal = 1.5;
        static bool boolVal = false;
        static int radioBSelected = 0;
        static int radioLSelected = 0;

        ImGui::PushFont(Medium12);
        
        ImGui::BeginGroup();
        FatalWidgets::RadioButtonB("RadioButtonB_0", "Button0", &radioBSelected, 0);
        ImGui::SameLine();
        FatalWidgets::RadioButtonB("RadioButtonB_1", "But1", &radioBSelected, 1);
        ImGui::SameLine();
        FatalWidgets::RadioButtonB("RadioButtonB_2", "B2", &radioBSelected, 2);
        ImGui::SameLine();
        FatalWidgets::RadioButtonB("RadioButtonB_3", "Button3", &radioBSelected, 3);
        ImGui::EndGroup();

        ImGui::BeginGroup();
        FatalWidgets::RadioButtonL("RadioButtonL_0", "LButton0", &radioLSelected, 0);
        FatalWidgets::RadioButtonL("RadioButtonL_1", "LButton1", &radioLSelected, 1);
        FatalWidgets::RadioButtonL("RadioButtonL_2", "LButton2", &radioLSelected, 2);
        FatalWidgets::RadioButtonL("RadioButtonL_3", "LButton3", &radioLSelected, 3);
        ImGui::EndGroup();

        ImGui::BeginGroup();
        FatalWidgets::IconButton("IconButton0", "0", "IconB0", false, ExtraBold20);
        FatalWidgets::IconButton("IconButton1", "1", "IconB1", true, ExtraBold20);
        FatalWidgets::IconButton("IconButton2", "2", "IconB2", false, ExtraBold20);
        ImGui::EndGroup();

        FatalWidgets::BeginGroup("FataGroup", ImVec2());


        ImGui::Text("Hello Fatal Group!");
        FatalWidgets::DoubleSlider("Slider123", -2, 2, &doubleVal, ImVec2(80, 16));
        ImGui::Text("Draw Fatality Style ClickGui");
        ImGui::SameLine();
        FatalWidgets::CheckBox("FatalCheckBox", &boolVal, ImVec2(14, 14));
        if (boolVal)
        {
            Fatal::Menu::draw();
        }
        if (FatalWidgets::ComboBox("Combo123", "Combo123_Popup", "Hit-chance", ImVec2(80, 22), []() {ImGui::SetNextWindowPos(ImVec2(ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y + 2));})) {
            ImGui::Button("Button in combo popup");
            ImGui::EndPopup();
        }


        FatalWidgets::EndGroup("Group");

        ImGui::PopFont();

        ImGui::End();
    }

    //for (auto list : ImGui::GetDrawListSharedData()->DrawLists)
    //{
    //    TransformPoints(
    //        list,
    //        0, list->VtxBuffer.size(),
    //        wPos, wPos + wSize,
    //        wPos, wPos + ImVec2(wSize.x, wSize.y * 0.25f), wPos + ImVec2(wSize.x, wSize.y * 0.75f), wPos + ImVec2(0, wSize.y));
    //}
}

/*
 *  from https://github.com/Pagghiu/imgui/tree/2017-09-box-shadow
 */
 //The MIT License (MIT)
 //
 //Copyright (c) 2017 Stefano Cristiano
 //
 //Permission is hereby granted, free of charge, to any person obtaining a copy
 //of this software and associated documentation files (the "Software"), to deal
 //in the Software without restriction, including without limitation the rights
 //to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 //copies of the Software, and to permit persons to whom the Software is
 //furnished to do so, subject to the following conditions:
 //
 //The above copyright notice and this permission notice shall be included in all
 //copies or substantial portions of the Software.
 //
 //THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 //IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 //FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 //AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 //LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 //OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 //SOFTWARE.

 // ---------------------------------------------------------------------------
 // Box Shadow for imgui using vertex colors
 // Current Limitations:
 // - Only works for axis aligned non rounded rectangle
 // - Not optimized at all
 // - User needs to figure out how many slices / rings are needed and what 
 //   spacing would look good for a box of given size and shadow sigma.
 //	 Ideally the parameters should be automatically calculated based on some
 //   high, medium, low quality settings
 // ---------------------------------------------------------------------------

ImVec4 operator+(float val, ImVec4 p2) { return ImVec4(val + p2.x, val + p2.y, val + p2.z, val + p2.w); }

ImVec4 boxGaussianIntegral(ImVec4 x)
{
    const ImVec4 s = ImVec4(x.x > 0 ? 1.0f : -1.0f, x.y > 0 ? 1.0f : -1.0f, x.z > 0 ? 1.0f : -1.0f, x.w > 0 ? 1.0f : -1.0f);
    const ImVec4 a = ImVec4(fabsf(x.x), fabsf(x.y), fabsf(x.z), fabsf(x.w));
    const ImVec4 res = 1.0f + (0.278393f + (0.230389f + 0.078108f * (a * a)) * a) * a;
    const ImVec4 resSquared = res * res;
    return s - s / (resSquared * resSquared);
}

ImVec4 boxLinearInterpolation(ImVec4 x)
{
    const float maxClamp = 1.0f;
    const float minClamp = -1.0f;
    return ImVec4(x.x > maxClamp ? maxClamp : x.x < minClamp ? minClamp : x.x,
        x.y > maxClamp ? maxClamp : x.y < minClamp ? minClamp : x.y,
        x.z > maxClamp ? maxClamp : x.z < minClamp ? minClamp : x.z,
        x.w > maxClamp ? maxClamp : x.w < minClamp ? minClamp : x.w);
}

float boxShadow(ImVec2 lower, ImVec2 upper, ImVec2 point, float sigma, bool linearInterpolation)
{
    const ImVec2 pointLower = point - lower;
    const ImVec2 pointUpper = point - upper;
    const ImVec4 query = ImVec4(pointLower.x, pointLower.y, pointUpper.x, pointUpper.y);
    const ImVec4 pointToSample = query * (sqrtf(0.5f) / sigma);
    const ImVec4 integral = linearInterpolation ? 0.5f + 0.5f * boxLinearInterpolation(pointToSample) : 0.5f + 0.5f * boxGaussianIntegral(pointToSample);
    return (integral.z - integral.x) * (integral.w - integral.y);
}

struct RectangleShadowSettings
{
    // Inputs
    bool    linear = false;
    float   sigma = 3;
    ImVec2  padding = ImVec2(50, 50);
    ImVec2  rectPos = ImVec2(50, 50);
    ImVec2  rectSize = ImVec2(120, 120);
    ImVec2  shadowOffset = ImVec2(0, 0);
    ImVec2  shadowSize = ImVec2(120, 50);
    ImColor shadowColor = ImColor(0.6f, 0.6f, 0.6f, 1.0f);

    int  rings = 3;
    int  spacingBetweenRings = 6;
    int  samplesPerCornerSide = 1;
    int  spacingBetweenSamples = 15;

    // Outputs
    int totalVertices = 0;
    int totalIndices = 0;

    // Visualization
    bool enableDebugVisualization = false;

};

void drawRectangleShadowVerticesAdaptive(RectangleShadowSettings& settings)
{
    const int    samplesSpan = settings.samplesPerCornerSide * settings.spacingBetweenSamples;
    const int    halfWidth = static_cast<int>(settings.rectSize.x / 2);
    const int    numSamplesInHalfWidth = (halfWidth / settings.spacingBetweenSamples) == 0 ? 1 : halfWidth / settings.spacingBetweenSamples;
    const int    numSamplesWidth = samplesSpan > halfWidth ? numSamplesInHalfWidth : settings.samplesPerCornerSide;
    const int    halfHeight = static_cast<int>(settings.rectSize.y / 2);
    const int    numSamplesInHalfHeight = (halfHeight / settings.spacingBetweenSamples) == 0 ? 1 : halfHeight / settings.spacingBetweenSamples;
    const int    numSamplesHeight = samplesSpan > halfHeight ? numSamplesInHalfHeight : settings.samplesPerCornerSide;
    const int    numVerticesInARing = numSamplesWidth * 4 + numSamplesHeight * 4 + 4;
    const ImVec2 whiteTexelUV = ImGui::GetIO().Fonts->TexUvWhitePixel;
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const ImVec2 rectangleTopLeft = origin + settings.rectPos;
    const ImVec2 rectangleBottomRight = rectangleTopLeft + settings.rectSize;
    const ImVec2 rectangleTopRight = rectangleTopLeft + ImVec2(settings.rectSize.x, 0);
    const ImVec2 rectangleBottomLeft = rectangleTopLeft + ImVec2(0, settings.rectSize.y);

    ImColor shadowColor = settings.shadowColor;
    settings.totalVertices = numVerticesInARing * settings.rings;
    settings.totalIndices = 6 * (numVerticesInARing) * (settings.rings - 1);

    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    drawList->PrimReserve(settings.totalIndices, settings.totalVertices);
    const ImDrawVert* shadowVertices = drawList->_VtxWritePtr;
    ImDrawVert* vertexPointer = drawList->_VtxWritePtr;

    for (int r = 0; r < settings.rings; ++r)
    {
        const float  adaptiveScale = (r / 2.5f) + 1;
        const ImVec2 ringOffset = ImVec2(adaptiveScale * r * settings.spacingBetweenRings, adaptiveScale * r * settings.spacingBetweenRings);
        for (int j = 0; j < 4; ++j)
        {
            ImVec2      corner;
            ImVec2      direction[2];
            const float spacingBetweenSamplesOnARing = static_cast<float>(settings.spacingBetweenSamples);
            switch (j)
            {
            case 0:
                corner = rectangleTopLeft + ImVec2(-ringOffset.x, -ringOffset.y);
                direction[0] = ImVec2(1, 0) * spacingBetweenSamplesOnARing;
                direction[1] = ImVec2(0, 1) * spacingBetweenSamplesOnARing;
                for (int i = 0; i < numSamplesWidth; ++i)
                {
                    const ImVec2 point = corner + direction[0] * (numSamplesWidth - i);
                    shadowColor.Value.w = boxShadow(rectangleTopLeft, rectangleBottomRight, point - settings.shadowOffset, settings.sigma, settings.linear);
                    vertexPointer->pos = point;
                    vertexPointer->uv = whiteTexelUV;
                    vertexPointer->col = shadowColor;
                    vertexPointer++;
                }

                shadowColor.Value.w = boxShadow(rectangleTopLeft, rectangleBottomRight, corner - settings.shadowOffset, settings.sigma, settings.linear);
                vertexPointer->pos = corner;
                vertexPointer->uv = whiteTexelUV;
                vertexPointer->col = shadowColor;
                vertexPointer++;

                for (int i = 0; i < numSamplesHeight; ++i)
                {
                    const ImVec2 point = corner + direction[1] * (i + 1);
                    shadowColor.Value.w = boxShadow(rectangleTopLeft, rectangleBottomRight, point - settings.shadowOffset, settings.sigma, settings.linear);
                    vertexPointer->pos = point;
                    vertexPointer->uv = whiteTexelUV;
                    vertexPointer->col = shadowColor;
                    vertexPointer++;
                }
                break;
            case 1:
                corner = rectangleBottomLeft + ImVec2(-ringOffset.x, +ringOffset.y);
                direction[0] = ImVec2(1, 0) * spacingBetweenSamplesOnARing;
                direction[1] = ImVec2(0, -1) * spacingBetweenSamplesOnARing;
                for (int i = 0; i < numSamplesHeight; ++i)
                {
                    const ImVec2 point = corner + direction[1] * (numSamplesHeight - i);
                    shadowColor.Value.w = boxShadow(rectangleTopLeft, rectangleBottomRight, point - settings.shadowOffset, settings.sigma, settings.linear);
                    vertexPointer->pos = point;
                    vertexPointer->uv = whiteTexelUV;
                    vertexPointer->col = shadowColor;
                    vertexPointer++;
                }

                shadowColor.Value.w = boxShadow(rectangleTopLeft, rectangleBottomRight, corner - settings.shadowOffset, settings.sigma, settings.linear);
                vertexPointer->pos = corner;
                vertexPointer->uv = whiteTexelUV;
                vertexPointer->col = shadowColor;
                vertexPointer++;

                for (int i = 0; i < numSamplesWidth; ++i)
                {
                    const ImVec2 point = corner + direction[0] * (i + 1);
                    shadowColor.Value.w = boxShadow(rectangleTopLeft, rectangleBottomRight, point - settings.shadowOffset, settings.sigma, settings.linear);
                    vertexPointer->pos = point;
                    vertexPointer->uv = whiteTexelUV;
                    vertexPointer->col = shadowColor;
                    vertexPointer++;
                }
                break;
            case 2:
                corner = rectangleBottomRight + ImVec2(+ringOffset.x, +ringOffset.y);
                direction[0] = ImVec2(-1, 0) * spacingBetweenSamplesOnARing;
                direction[1] = ImVec2(0, -1) * spacingBetweenSamplesOnARing;
                for (int i = 0; i < numSamplesWidth; ++i)
                {
                    const ImVec2 point = corner + direction[0] * (numSamplesWidth - i);
                    shadowColor.Value.w = boxShadow(rectangleTopLeft, rectangleBottomRight, point - settings.shadowOffset, settings.sigma, settings.linear);
                    vertexPointer->pos = point;
                    vertexPointer->uv = whiteTexelUV;
                    vertexPointer->col = shadowColor;
                    vertexPointer++;
                }

                shadowColor.Value.w = boxShadow(rectangleTopLeft, rectangleBottomRight, corner - settings.shadowOffset, settings.sigma, settings.linear);
                vertexPointer->pos = corner;
                vertexPointer->uv = whiteTexelUV;
                vertexPointer->col = shadowColor;
                vertexPointer++;

                for (int i = 0; i < numSamplesHeight; ++i)
                {
                    const ImVec2 point = corner + direction[1] * (i + 1);
                    shadowColor.Value.w = boxShadow(rectangleTopLeft, rectangleBottomRight, point - settings.shadowOffset, settings.sigma, settings.linear);
                    vertexPointer->pos = point;
                    vertexPointer->uv = whiteTexelUV;
                    vertexPointer->col = shadowColor;
                    vertexPointer++;
                }
                break;
            case 3:
                corner = rectangleTopRight + ImVec2(+ringOffset.x, -ringOffset.y);
                direction[0] = ImVec2(-1, 0) * spacingBetweenSamplesOnARing;
                direction[1] = ImVec2(0, 1) * spacingBetweenSamplesOnARing;
                for (int i = 0; i < numSamplesHeight; ++i)
                {
                    const ImVec2 point = corner + direction[1] * (numSamplesHeight - i);
                    shadowColor.Value.w = boxShadow(rectangleTopLeft, rectangleBottomRight, point - settings.shadowOffset, settings.sigma, settings.linear);
                    vertexPointer->pos = point;
                    vertexPointer->uv = whiteTexelUV;
                    vertexPointer->col = shadowColor;
                    vertexPointer++;
                }

                shadowColor.Value.w = boxShadow(rectangleTopLeft, rectangleBottomRight, corner - settings.shadowOffset, settings.sigma, settings.linear);
                vertexPointer->pos = corner;
                vertexPointer->uv = whiteTexelUV;
                vertexPointer->col = shadowColor;
                vertexPointer++;

                for (int i = 0; i < numSamplesWidth; ++i)
                {
                    const ImVec2 point = corner + direction[0] * (i + 1);
                    shadowColor.Value.w = boxShadow(rectangleTopLeft, rectangleBottomRight, point - settings.shadowOffset, settings.sigma, settings.linear);
                    vertexPointer->pos = point;
                    vertexPointer->uv = whiteTexelUV;
                    vertexPointer->col = shadowColor;
                    vertexPointer++;
                }
                break;
            }
        }
    }

    ImDrawIdx idx = (ImDrawIdx)drawList->_VtxCurrentIdx;

    for (int r = 0; r < settings.rings - 1; ++r)
    {
        const ImDrawIdx startOfRingIndex = idx;
        for (int i = 0; i < numVerticesInARing - 1; ++i)
        {
            drawList->_IdxWritePtr[0] = idx + 0;
            drawList->_IdxWritePtr[1] = idx + 1;
            drawList->_IdxWritePtr[2] = idx + numVerticesInARing;
            drawList->_IdxWritePtr[3] = idx + 1;
            drawList->_IdxWritePtr[4] = idx + numVerticesInARing + 1;
            drawList->_IdxWritePtr[5] = idx + numVerticesInARing;

            idx += 1;
            drawList->_IdxWritePtr += 6;
        }

        drawList->_IdxWritePtr[0] = idx + 0;
        drawList->_IdxWritePtr[1] = startOfRingIndex + 0;
        drawList->_IdxWritePtr[2] = startOfRingIndex + numVerticesInARing;
        drawList->_IdxWritePtr[3] = idx + 0;
        drawList->_IdxWritePtr[4] = startOfRingIndex + numVerticesInARing;
        drawList->_IdxWritePtr[5] = idx + numVerticesInARing;

        drawList->_IdxWritePtr += 6;
        idx += 1;
    }
    drawList->_VtxCurrentIdx += settings.totalVertices;
    drawList->_VtxWritePtr = vertexPointer;
    if (settings.enableDebugVisualization)
    {
        const ImColor lineColor(0, 0, 255, 50);
        for (int r = 0; r < settings.rings - 1; ++r)
        {
            const ImDrawIdx startOfRingIndex = r * numVerticesInARing;

            ImVec2 a;
            ImVec2 b;
            for (int i = 0; i < numVerticesInARing - 1; ++i)
            {
                a = shadowVertices[startOfRingIndex + i + 1].pos;
                b = shadowVertices[startOfRingIndex + i + numVerticesInARing].pos;
                drawList->AddLine(a, b, lineColor);
                a = shadowVertices[startOfRingIndex + i + 0].pos;
                b = shadowVertices[startOfRingIndex + i + numVerticesInARing].pos;
                drawList->AddLine(a, b, lineColor);
                a = shadowVertices[startOfRingIndex + i + numVerticesInARing + 1].pos;
                b = shadowVertices[startOfRingIndex + i + numVerticesInARing].pos;
                drawList->AddLine(a, b, lineColor);
            }

            a = shadowVertices[startOfRingIndex + numVerticesInARing - 1].pos;
            b = shadowVertices[startOfRingIndex + numVerticesInARing].pos;
            drawList->AddLine(a, b, lineColor);
            a = shadowVertices[startOfRingIndex + numVerticesInARing + numVerticesInARing - 1].pos;
            b = shadowVertices[startOfRingIndex + numVerticesInARing].pos;
            drawList->AddLine(a, b, lineColor);
        }

        for (int i = 0; i < settings.totalVertices; ++i)
        {
            const ImVec2 bmin = shadowVertices[i].pos - ImVec2(2, 2);
            const ImVec2 bmax = shadowVertices[i].pos + ImVec2(2, 2);
            drawList->AddRectFilled(bmin, bmax, ImColor(255, 0, 0, 50));
        }
    }
}
RectangleShadowSettings shadowSettings;

void drawShadowTestExampleWindow()
{
    ImGui::SetNextWindowSize(ImVec2(400, 800), ImGuiCond_Once);
    ImGui::Begin("Test Shadows");
    static bool showRectangles = true;
    static bool calculateMinimumPadding = false;
    static ImColor backgroundColor(255, 255, 255, 255);
    static ImColor rectangleColor(240, 240, 240, 255);
    ImGui::Checkbox("Wireframe", &shadowSettings.enableDebugVisualization);
    ImGui::Checkbox("Draw Rectangle", &showRectangles);
    ImGui::Checkbox("Recalculate Padding", &calculateMinimumPadding);
    ImGui::Checkbox("Linear Falloff", &shadowSettings.linear);
    ImGui::SliderFloat2("Rectangle Size", &shadowSettings.rectSize.x, 10, 256);
    ImGui::SliderFloat("Shadow Sigma", &shadowSettings.sigma, 0, 50);
    ImGui::SliderFloat2("Shadow Offset", &shadowSettings.shadowOffset.x, -10, 10);
    ImGui::SliderInt("Rings number", &shadowSettings.rings, 1, 10);
    ImGui::SliderInt("Rings spacing", &shadowSettings.spacingBetweenRings, 1, 20);
    ImGui::SliderInt("Corner samples", &shadowSettings.samplesPerCornerSide, 1, 20);
    ImGui::SliderInt("Corner spacing", &shadowSettings.spacingBetweenSamples, 1, 20);
    ImGui::ColorPicker3("Shadow Color", &shadowSettings.shadowColor.Value.x, ImGuiColorEditFlags_PickerHueWheel);
    ImGui::ColorPicker3("Rectangle Color", &rectangleColor.Value.x, ImGuiColorEditFlags_PickerHueWheel);
    ImGui::ColorPicker3("Background Color", &backgroundColor.Value.x, ImGuiColorEditFlags_PickerHueWheel);
    ImGui::End();

    if (calculateMinimumPadding)
    {
        shadowSettings.padding = ImVec2(3 * shadowSettings.sigma, 3 * shadowSettings.sigma);
    }
    shadowSettings.rectPos = shadowSettings.padding;
    shadowSettings.shadowSize = shadowSettings.rectSize + shadowSettings.padding * 2;

    ImGui::SetNextWindowPos(ImVec2(700, 0), ImGuiCond_Once);
    ImGui::Begin("Shadow Outputs");
    ImGui::PushStyleColor(ImGuiCol_ChildBg, (ImU32)backgroundColor);
    ImGui::Text("rings:%d, spacingH:%d, spacingR:%d\ntotalVertices:%d totalIndices:%d", shadowSettings.rings, shadowSettings.spacingBetweenRings, shadowSettings.spacingBetweenSamples, shadowSettings.totalVertices, shadowSettings.totalIndices);
    if (ImGui::BeginChild("Adaptive Rectangle Shadow", shadowSettings.shadowSize))
    {
        drawRectangleShadowVerticesAdaptive(shadowSettings);
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const ImVec2 origin(ImGui::GetCursorScreenPos());
        if (showRectangles)
            drawList->AddRectFilled(origin + shadowSettings.rectPos, origin + shadowSettings.rectPos + shadowSettings.rectSize, rectangleColor);
        drawList->AddRect(origin, origin + shadowSettings.shadowSize, ImColor(255, 0, 0, 255));
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::End();
}

void shadowTest()
{
    drawShadowTestExampleWindow();
}
