#include "core/app_state.hpp"
#include "ui/theme.hpp"

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "implot.h"

#include <d3d11.h>
#include <dwmapi.h>
#include <tchar.h>

// Forward declare Win32 message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace {

ID3D11Device*           g_pd3dDevice           = nullptr;
ID3D11DeviceContext*    g_pd3dDeviceContext    = nullptr;
IDXGISwapChain*         g_pSwapChain           = nullptr;
bool                    g_SwapChainOccluded    = false;
UINT                    g_ResizeWidth          = 0;
UINT                    g_ResizeHeight         = 0;
ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer = nullptr;
    if (SUCCEEDED(g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer))) && pBackBuffer) {
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
        pBackBuffer->Release();
    }
}

void CleanupRenderTarget() {
    if (g_mainRenderTargetView) {
        g_mainRenderTargetView->Release();
        g_mainRenderTargetView = nullptr;
    }
}

bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount                        = 2;
    sd.BufferDesc.Width                   = 0;
    sd.BufferDesc.Height                  = 0;
    sd.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator   = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags                              = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow                       = hWnd;
    sd.SampleDesc.Count                   = 1;
    sd.SampleDesc.Quality                 = 0;
    sd.Windowed                           = TRUE;
    sd.SwapEffect                         = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel{};
    const D3D_FEATURE_LEVEL featureLevelArray[2] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0,
    };

    HRESULT res = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
        featureLevelArray, 2, D3D11_SDK_VERSION, &sd,
        &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);

    if (res == DXGI_ERROR_UNSUPPORTED) {
        res = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags,
            featureLevelArray, 2, D3D11_SDK_VERSION, &sd,
            &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    }

    if (res != S_OK) {
        return false;
    }

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain)        { g_pSwapChain->Release();        g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice)        { g_pd3dDevice->Release();        g_pd3dDevice = nullptr; }
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) {
        return true;
    }

    switch (msg) {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED) {
            return 0;
        }
        g_ResizeWidth  = static_cast<UINT>(LOWORD(lParam));
        g_ResizeHeight = static_cast<UINT>(HIWORD(lParam));
        return 0;

    case WM_SYSCOMMAND:
        if ((wParam & 0xFFF0) == SC_KEYMENU) { // Disable ALT application menu
            return 0;
        }
        break;

    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }

    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

// Temporary Phase 1 bootstrap shell to verify Obsidian Slate theme + ImPlot rendering
void RenderBootstrapShell(core::AppState& state, const ui::theme::FontSet& fonts) {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    constexpr ImGuiWindowFlags kRootFlags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::Begin("##RootWorkspace", nullptr, kRootFlags);

    // Left Sidebar
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ui::theme::kBgSidebar);
    ImGui::BeginChild("##Sidebar", ImVec2(230.0f, 0.0f), ImGuiChildFlags_Borders);
    {
        ImGui::PushFont(fonts.bold);
        ImGui::TextColored(ui::theme::kAccentPrimary, "LEDGER // OS");
        ImGui::PopFont();
        ImGui::TextColored(ui::theme::kTextMuted, "Personal Expense Analytics");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        const char* nav_labels[] = {"Overview", "Deep Analytics", "Transactions", "Budgets"};
        for (int i = 0; i < 4; ++i) {
            const bool selected = (static_cast<int>(state.active_tab) == i);
            if (selected) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.220f, 0.741f, 0.973f, 0.20f));
                ImGui::PushStyleColor(ImGuiCol_Text, ui::theme::kAccentPrimary);
            }
            if (ImGui::Button(nav_labels[i], ImVec2(-1.0f, 38.0f))) {
                state.active_tab = static_cast<core::NavTab>(i);
            }
            if (selected) {
                ImGui::PopStyleColor(2);
            }
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::SameLine();

    // Main Content Canvas
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ui::theme::kBgCanvas);
    ImGui::BeginChild("##MainContent", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None);
    {
        ImGui::PushFont(fonts.bold);
        ImGui::TextUnformatted("Phase 1 Bootstrap: Obsidian Slate Ready");
        ImGui::PopFont();
        ImGui::TextColored(ui::theme::kTextMuted,
            "Win32 + DirectX 11 + Dear ImGui + ImPlot initialized. Ready for Phase 2 SQLite & StatsEngine.");
        ImGui::Spacing();

        // Sample KPI Card
        ImGui::BeginChild("##SampleKpi", ImVec2(320.0f, 110.0f), ImGuiChildFlags_Borders);
        {
            ImGui::TextColored(ui::theme::kTextMuted, "BOOTSTRAP STATUS");
            ImGui::PushFont(fonts.kpi_hero);
            ImGui::TextColored(ui::theme::kAccentEmerald, "100%% Operational");
            ImGui::PopFont();
        }
        ImGui::EndChild();

        ImGui::Spacing();

        // Sample ImPlot Verification Card
        ImGui::BeginChild("##SamplePlotCard", ImVec2(0.0f, 320.0f), ImGuiChildFlags_Borders);
        {
            static constexpr double kDays[7]  = {1, 2, 3, 4, 5, 6, 7};
            static constexpr double kSpend[7] = {24.5, 42.0, 18.75, 65.0, 31.2, 88.4, 52.1};

            if (ImPlot::BeginPlot("Spending Pace Preview (Sample Data)", ImVec2(-1.0f, -1.0f))) {
                ImPlot::SetupAxes("Day", "Spend ($)");
                ImPlot::PlotLine("Daily Outflow", kDays, kSpend, 7);
                ImPlot::EndPlot();
            }
        }
        ImGui::EndChild();
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::End();
}

} // namespace

int main(int, char**) {
    ImGui_ImplWin32_EnableDpiAwareness();
    const float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(
        ::MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY));

    WNDCLASSEXW wc{
        sizeof(wc),
        CS_CLASSDC,
        WndProc,
        0L,
        0L,
        ::GetModuleHandle(nullptr),
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        L"ExpenseTrackerWindowClass",
        nullptr
    };
    ::RegisterClassExW(&wc);

    const int win_width  = static_cast<int>(1360 * main_scale);
    const int win_height = static_cast<int>(840 * main_scale);

    HWND hwnd = ::CreateWindowW(
        wc.lpszClassName,
        L"Expense Tracker // Obsidian Slate",
        WS_OVERLAPPEDWINDOW,
        100, 100, win_width, win_height,
        nullptr, nullptr, wc.hInstance, nullptr);

    // Enable dark native Windows 11 title bar
    const BOOL use_dark_mode = TRUE;
    ::DwmSetWindowAttribute(hwnd, 20 /* DWMWA_USE_IMMERSIVE_DARK_MODE */, &use_dark_mode, sizeof(use_dark_mode));

    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ui::theme::ApplyObsidianSlateTheme(main_scale);
    const ui::theme::FontSet fonts = ui::theme::LoadAppFonts(main_scale);

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    core::AppState app_state{};

    bool done = false;
    while (!done) {
        MSG msg{};
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT) {
                done = true;
            }
        }
        if (done) {
            break;
        }

        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) {
            ::Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        if (g_ResizeWidth != 0 && g_ResizeHeight != 0) {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        RenderBootstrapShell(app_state, fonts);

        ImGui::Render();
        const float clear_color[4] = {
            ui::theme::kBgCanvas.x,
            ui::theme::kBgCanvas.y,
            ui::theme::kBgCanvas.z,
            1.0f
        };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        const HRESULT hr = g_pSwapChain->Present(1, 0); // VSync enabled
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}