#pragma once

#include <string>
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include<thread>
#include"screenrecording.h"
#include"SystemRecorder.h"
#include "Muxer.h"
#include "Mic_WAPI.h"
#include <shlobj.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
// ImGui 头文件
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_internal.h"
#include "Httpnet.h"


// 链接库
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "msimg32.lib")
#include <chrono>
HWND hwnd;
bool Fenxiao = false;
extern std::string g_LoginAccount;

// 推广链接
extern std::string g_InviteLink;
// 你的D3D11设备/设备上下文指针（直接用你项目里已有的全局变量）
extern ID3D11Device* g_pd3dDevice;
bool is_login_success = false;
static char user[32] = { 0 };
// --------------------------
// 开机动画：Logo + 渐入渐出 + 两行文字
// 直接用！路径：exe 同目录 logo.png
#include <gdiplus.h>
#pragma comment(lib,"gdiplus.lib")
//using namespace Gdiplus;

static ID3D11ShaderResourceView* g_LogoSRV = nullptr;


void ShowSplashFinal()
{
    static float t = 0;
    static bool end = false;
    if (end) return;

    t += ImGui::GetIO().DeltaTime;
    if (t > 5.0f) { end = true; return; }

    float alpha = 1.0f;
    if (t < 1.5f) alpha = t / 1.5f;
    if (t > 3.5f) alpha = 1.0f - (t - 3.5f) / 1.5f;

    ImVec2 scr = ImGui::GetIO().DisplaySize;
    // 👉 宽度不变，上下高度压窄：420宽 / 200高（原来280）
    ImVec2 winSz = ImVec2(420, 200);
    ImVec2 pos = ImVec2((scr.x - winSz.x) / 2.f, (scr.y - winSz.y) / 2.f);

    ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(winSz);

    ImGui::Begin("SplashOK", nullptr,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove);

    ImDrawList* dl = ImGui::GetWindowDrawList();

    // 渐变背景
    ImU32 c1 = IM_COL32(60, 40, 100, (int)(alpha * 240));
    ImU32 c2 = IM_COL32(20, 80, 60, (int)(alpha * 240));
    dl->AddRectFilledMultiColor(ImVec2(0, 0), winSz, c1, c1, c2, c2);

    // 三色文字
    ImU32 color_title = IM_COL32(255, 215, 0, (int)(alpha * 255)); // 金黄
    ImU32 color_line1 = IM_COL32(0, 255, 220, (int)(alpha * 255)); // 亮青
    ImU32 color_line2 = IM_COL32(200, 160, 255, (int)(alpha * 255)); // 淡紫

    // 👉 文字间距缩小，适配窄框
    ImGui::PushStyleColor(ImGuiCol_Text, color_title);
    ImGui::SetCursorPosX((420 - ImGui::CalcTextSize(u8"魔音录屏").x) / 2.0f);
    ImGui::SetCursorPosY(40);   // 上间距收窄
    ImGui::Text(u8"魔音录屏");
    ImGui::PopStyleColor();

    ImGui::PushStyleColor(ImGuiCol_Text, color_line1);
    ImGui::SetCursorPosX((420 - ImGui::CalcTextSize(u8"别家软件都在控制你的时间").x) / 2.0f);
    ImGui::SetCursorPosY(95);   // 中间紧凑
    ImGui::Text(u8"别家软件都在控制你的时间");
    ImGui::PopStyleColor();

    ImGui::PushStyleColor(ImGuiCol_Text, color_line2);
    ImGui::SetCursorPosX((420 - ImGui::CalcTextSize(u8"我把时间完完整整的还给你").x) / 2.0f);
    ImGui::SetCursorPosY(140);  // 底部收窄
    ImGui::Text(u8"我把时间完完整整的还给你");
    ImGui::PopStyleColor();

    ImGui::End();
}
using namespace std::chrono;

static bool         g_timer_running = false;
static bool         g_timer_paused = false;
static long long    g_total_sec = 0;
static steady_clock::time_point g_last_time;

static int g_hh = 0, g_mm = 0, g_ss = 0;
void TimerThreadLoop()
{
    while (g_timer_running)
    {
        // 每 10ms 跑一次，绝对不受UI影响
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        if (!g_timer_paused)
        {
            auto now = steady_clock::now();
            auto diff = duration_cast<seconds>(now - g_last_time);

            if (diff.count() > 0)
            {
                g_total_sec += diff.count();
                g_last_time = now;

                g_hh = (int)(g_total_sec / 3600);
                g_mm = (int)((g_total_sec % 3600) / 60);
                g_ss = (int)(g_total_sec % 60);
            }
        }
    }
}
using namespace std;
static int s_click_count = 0;

void UpdateRecordingTimer()
{
    
}

void StartRecordingTimer()
{
    g_timer_running = true;
    g_timer_paused = false;
    g_total_sec = 0;
    g_last_time = steady_clock::now();

    // 启动独立计时线程！
    std::thread t(TimerThreadLoop);
    t.detach();
}
void PauseRecordingTimer()
{
    g_timer_paused = true;
}
void ResumeRecordingTimer()
{
    g_timer_paused = false;
    g_last_time = steady_clock::now();
}
void StopRecordingTimer()
{
    g_timer_running = false;
    g_timer_paused = false;
    g_total_sec = 0;
    g_hh = g_mm = g_ss = 0;
}

void DrawRecordingTimer()
{
    if (g_timer_paused)
    {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.1f, 1.0f),
            u8"录制时间: %02d:%02d:%02d  [已暂停]", g_hh, g_mm, g_ss);
    }
    else
    {
        ImGui::TextColored(ImVec4(0.8f, 1.0f, 0.8f, 1.0f),
            u8"录制时间: %02d:%02d:%02d", g_hh, g_mm, g_ss);
    }
}


// 只留这一个计时变量！



ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
ID3D11BlendState* g_blendState = nullptr;
UINT g_ResizeWidth = 0, g_ResizeHeight = 0;
bool g_SwapChainOccluded = false;
//extern GlobalVoiceChanger g_globalVoice;
//GlobalVoiceChanger g_globalVoice;
// 菜单专用全局变量（仅定义一次，无嵌套）
namespace MenuConfig {
    bool show_menu_window = true;       // 菜单显示开关
    ImVec2 menu_pos = ImVec2(50, 50);   // 菜单位置
    bool F1KeyPressed = false;          // F1键状态
    bool quit_app = false;  
    bool is_vip = false;
 // 退出程序标志
}
atomic<bool> g_bPaused = false;
// ===================== 函数声明区 =====================
// D3D相关
void CreateRenderTarget();
bool CreateDeviceD3D(HWND hWnd);
void CleanupRenderTarget();
void CleanupDeviceD3D();

// 菜单相关
void InitMenuStyle();          // 初始化菜单样式
void RenderDraggableMenu();    // 绘制可拖动菜单
void HandleMenuShortcut(UINT msg, WPARAM wParam); // 处理菜单快捷键

// 窗口/系统相关
bool InitTransparentWindow(HWND& hwnd, WNDCLASSEXW& wc);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
void RunMainLoop(HWND hwnd);
void InitImGuiFonts();
#include <Windows.h>
#include <string>

// 全局路径
extern std::string g_audioFilePath;
std::string g_aviPath;
std::string g_micPath;
std::string g_outPath;
std::string g_output;


extern atomic<bool> g_isAudioRecording;
extern atomic<bool> g_isRecording;
MicRecorder micRecorder;
void OnStartBtnClick()
{
    
    std::string TEMPdir = GetSysTempPath();
    std::string dir = GetSystemVideosDir();
   
    g_aviPath = TEMPdir + "\\temp.avi";
    g_audioFilePath = TEMPdir + "\\sys.wav";
    g_micPath = TEMPdir + "\\mic.wav";
    g_outPath = TEMPdir + "\\output.avi";
    std::string fileName = GetDateTimeFileName();
    g_output = dir + "\\魔音录屏\\" + fileName + ".mp4";
    
    DeleteFileA(g_aviPath.c_str());
    DeleteFileA(g_audioFilePath.c_str());
  //  DeleteFileA(g_micPath.c_str()); // 把旧麦克风也删了

    if (!g_isRecording)
    {
        // 第一次点 → 开始录
        g_isRecording = true;
        g_bPaused = false;
        micRecorder.Start();
        //Sleep(150); 
        StartScreenRecord(g_aviPath.c_str());
        RecordSystemSound_Start();

        StartRecordingTimer(); // 时钟启动
    }
    else
    {
        // 暂停 / 继续
        g_bPaused = !g_bPaused;

        if (g_bPaused)
            PauseRecordingTimer();
        else
            ResumeRecordingTimer();
    }
}
extern bool g_IsVIP;
bool g_IsVIP=true ;


void OnStopBtnClick()
{
    g_isRecording = false;
    g_isAudioRecording = false;
    

    // 停止采集
    RecordSystemSound_Stop();
    micRecorder.Stop();
    StopScreenRecord();

    // 简短等一下，保证文件落盘
    Sleep(300);

    // 保存麦克风音频
    micRecorder.SaveToFile(g_micPath);

    // 停止UI计时
    StopRecordingTimer();

    // 丢后台合成，界面秒响应
    CreateThread(NULL, 0, BackMergeThread, NULL, 0, NULL);
}
bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };

    if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
        featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();

    // 创建Alpha混合状态（实现透明）
    if (g_blendState == nullptr)
    {
        D3D11_BLEND_DESC blendDesc = {};
        blendDesc.AlphaToCoverageEnable = FALSE;
        blendDesc.RenderTarget[0].BlendEnable = TRUE;
        blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        g_pd3dDevice->CreateBlendState(&blendDesc, &g_blendState);
    }
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
    if (g_blendState) { g_blendState->Release(); g_blendState = nullptr; }
}

// ===================== 菜单实现区 =====================
// 初始化菜单样式（可同步调整边框样式，可选）
void InitMenuStyle()
{
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    ImGuiIO& io = ImGui::GetIO();

    // 全局样式优化（保留圆角、间距等）
    style.WindowRounding = 12.0f;
    style.ChildRounding = 8.0f;
    style.FrameRounding = 8.0f;
    style.PopupRounding = 8.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding = 6.0f;

    style.WindowPadding = ImVec2(16, 16);
    style.FramePadding = ImVec2(12, 8);
    style.ItemSpacing = ImVec2(10, 8);
    style.ItemInnerSpacing = ImVec2(8, 6);

    // 恢复原生边框样式（可选：如需隐藏原生边框，设为0）
    style.WindowBorderSize = 1.0f;          // 原生窗口边框（1px）
    style.FrameBorderSize = 0.0f;
    style.PopupBorderSize = 1.0f;

    // 紫蓝绿渐变背景（保留）
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.18f, 0.15f, 0.30f, 0.95f);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.12f, 0.20f, 0.28f, 0.95f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.25f, 0.22f, 0.95f);

    // 标题栏样式
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.25f, 0.20f, 0.40f, 0.95f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.28f, 0.25f, 0.45f, 0.95f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.22f, 0.18f, 0.35f, 0.95f);

    // 按钮样式
    style.Colors[ImGuiCol_Button] = ImVec4(0.20f, 0.45f, 0.40f, 0.90f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.50f, 0.45f, 1.0f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.15f, 0.40f, 0.35f, 1.0f);

    // 文字样式
    style.Colors[ImGuiCol_Text] = ImVec4(0.95f, 0.98f, 0.95f, 1.0f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.70f, 0.75f, 0.70f, 1.0f);

    style.MouseCursorScale = 1.1f;
}



//可选：添加自定义颜色文字辅助函数（让文字更丰富）
void DrawColoredText(const char* text, ImVec4 color)
{
    ImGui::PushStyleColor(ImGuiCol_Text, color);
    ImGui::Text("%s", text);
    ImGui::PopStyleColor();
}

// 处理菜单快捷键
void HandleMenuShortcut(UINT msg, WPARAM wParam)
{
    switch (msg)
    {
    case WM_KEYDOWN:
        if (wParam == VK_F1 && !MenuConfig::F1KeyPressed)
        {
            MenuConfig::show_menu_window = !MenuConfig::show_menu_window;
            MenuConfig::F1KeyPressed = true;
        }
        break;
    case WM_KEYUP:
        if (wParam == VK_F1)
        {
            MenuConfig::F1KeyPressed = false;
        }
        break;
    }
}

void InitImGuiFonts()
{
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear(); // 清空原有字体

    // 字体配置
    ImFontConfig fontConfig;
    fontConfig.MergeMode = false;
    fontConfig.PixelSnapH = true; // 像素对齐，避免文字模糊

    // 方案1：直接使用宽字符路径（推荐，无需_T宏）
    const char* msyhPath = "C:\\Windows\\Fonts\\msyh.ttc";
    ImFont* mainFont = io.Fonts->AddFontFromFileTTF(
        msyhPath,
        16.0f,
        &fontConfig,
        io.Fonts->GetGlyphRangesChineseFull() // 加载完整中文字符集
    );

    // 兜底：加载宋体
    if (mainFont == nullptr)
    {
        const char* simsunPath = "C:\\Windows\\Fonts\\simsun.ttc";
        mainFont = io.Fonts->AddFontFromFileTTF(
            simsunPath,
            16.0f,
            &fontConfig,
            io.Fonts->GetGlyphRangesChineseFull()
        );
    }

    // 构建字体纹理（必须调用）
    io.Fonts->Build();
    if (mainFont != nullptr)
    {
        io.FontDefault = mainFont;
    }
}
// 先添加必要的头文件（确保编译通过）

// 兼容宏：如果编译器不支持C++17的u8字面量，手动定义
#if defined(_MSC_VER) && _MSC_VER < 1910 // VS2017以下版本
#define U8(x) (L##x)
#else
#define U8(x) u8##x
#endif

// UTF-8转GBK（终极版：兼容所有场景）
std::string UTF8ToGBK(const std::string& utf8_str)
{
    if (utf8_str.empty()) return "";

    // 1. UTF-8转宽字符（WCHAR）：添加错误处理，兼容无效字符
    int wchar_size = MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS | MB_PRECOMPOSED, // 严格校验+预组合字符
        utf8_str.c_str(),
        static_cast<int>(utf8_str.length()), // 明确长度，避免-1截断问题
        NULL,
        0
    );
    if (wchar_size == 0) {
        // 转换失败时，尝试直接返回原字符串（降级处理）
        return utf8_str;
    }

    std::wstring wchar_buf(wchar_size, L'\0');
    MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS | MB_PRECOMPOSED,
        utf8_str.c_str(),
        static_cast<int>(utf8_str.length()),
        &wchar_buf[0],
        wchar_size
    );

    // 2. 宽字符转GBK（多字节）：指定默认字符集
    int gbk_size = WideCharToMultiByte(
        CP_ACP,
        0,
        wchar_buf.c_str(),
        static_cast<int>(wchar_buf.length()),
        NULL,
        0,
        "?", // 无效字符替换为?（避免崩溃）
        NULL
    );
    if (gbk_size == 0) {
        return utf8_str;
    }

    std::string gbk_buf(gbk_size, '\0');
    WideCharToMultiByte(
        CP_ACP,
        0,
        wchar_buf.c_str(),
        static_cast<int>(wchar_buf.length()),
        &gbk_buf[0],
        gbk_size,
        "?",
        NULL
    );

    return gbk_buf;
}

// 自定义圆形勾选框（红色圆点）
bool CustomCircleCheckbox(const char* label, bool* v)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiStyle& style = ImGui::GetStyle();
    ImDrawList* draw_list = window->DrawList;

    float radius = 8.0f;
    float dot_radius = 3.0f;
    ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
    float check_gap = style.ItemInnerSpacing.x;
    ImVec2 pos = window->DC.CursorPos;

    ImRect total_bb(
        pos.x,
        pos.y,
        pos.x + radius * 2 + check_gap + label_size.x,
        pos.y + (radius * 2.0f > label_size.y ? radius * 2.0f : label_size.y)
    );

    ImGui::ItemSize(total_bb, style.FramePadding.y);
    if (!ImGui::ItemAdd(total_bb, ImGui::GetID(label)))
        return false;

    bool hovered = false;
    bool held = false;
    bool pressed = ImGui::ButtonBehavior(total_bb, ImGui::GetID(label), &hovered, &held);
    if (pressed) {
        *v = !*v;
    }

    ImVec2 center;
    center.x = pos.x + radius;
    center.y = pos.y + radius;

    ImU32 col_border = ImGui::GetColorU32(ImGuiCol_Border);
    draw_list->AddCircle(center, radius, col_border, 16, 1.0f);

    if (*v) {
        ImU32 col_dot = IM_COL32(255, 0, 0, 255);
        draw_list->AddCircleFilled(center, dot_radius, col_dot, 16);
    }

    if (label_size.x > 0.0f) {
        ImVec2 text_pos;
        text_pos.x = pos.x + radius * 2 + check_gap;
        text_pos.y = pos.y + (radius * 2 - label_size.y) * 0.5f;
        ImGui::RenderText(text_pos, label);
    }

    return pressed;
}



static bool show_settings = false;
static bool show_about = false;





void RenderAboutWindow()
{

    ImGui::SetNextWindowSize(ImVec2(480, 550), ImGuiCond_FirstUseEver);
    ImGui::Begin(U8("关于"), &show_about, ImGuiWindowFlags_NoResize);

    // 修复：透明度改成 1.0
    ImGui::TextColored(ImVec4(1.0f, 0.84f, 0.0f, 1.0f), U8("魔音录屏 v1.0"));
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextWrapped(U8("使用说明："));
    ImGui::TextWrapped(U8("1. 点击开始/暂停录屏即可开始录制"));
    ImGui::TextWrapped(U8("2. 可开启麦克风与变声功能"));
    ImGui::TextWrapped(U8("3. 变声功能与原声只能二选一重新启动软件选择"));
    ImGui::TextWrapped(U8("4. 本软件默认可以悬浮在所有游戏窗口之上，记得给游戏开无边框窗口模式就可以显示了"));
    ImGui::TextWrapped(U8("5. 视频自动保存到系统视频\\魔音录屏目录"));
    ImGui::TextWrapped(U8("6. 打开输出目录可直接查看录制好的视频"));
    ImGui::TextWrapped(U8("7. 因为资金能力不足，变声功能有限，仅限10种小黄人变声功能，等资金能力充足后续会努力更新"));
    ImGui::TextWrapped(U8("8.本软件音视频合成算法已经申请国家发明专利,专利申请号:2026103350550 "));
    ImGui::TextWrapped(U8("9.因算法原因，开启录屏时会获取麦克风权限属于正常现象，作者本人技术能力有限不会窃取您电脑内的任何隐私 "));
    ImGui::TextWrapped(U8("10. 后续支付系统与分销系统可能出错，因为资金不足没有测试，如遇到充值没到账可加下方微信联系作者"));
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.5f, 1.0f), U8("产品特色："));
    ImGui::TextWrapped(U8("本产品采用独家架构、独家算法，无录屏漂移、无音画不同步，"));
    ImGui::TextWrapped(U8("超低延迟、超流畅，不吃CPU、不吃声卡、不吃配置，低配机也能流畅运行。"));

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), U8("作者介绍："));
    ImGui::TextWrapped(U8("农村出身，初中学历，透析6年，自学5年 C 和 C++。"));
    ImGui::TextWrapped(U8("野路子散修，一个人独立开发完成本软件。"));
    ImGui::TextWrapped(U8("无团队、无 funding、无背景，靠技术死磕到底。"));

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.1f, 1.0f), U8("版权提示："));
    ImGui::TextWrapped(U8("本软件为个人原创作品，持续更新维护。"));
    ImGui::TextWrapped(U8("如果您使用的是盗版，恳请回来补张票，支持作者坚持下去。"));
    ImGui::TextWrapped(U8("您的支持，是我活下去、更新下去的唯一动力。"));

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextWrapped(U8("本软件基于 FFmpeg 实现音视频处理："));
    ImGui::TextWrapped(U8("FFmpeg 遵循 LGPLv2.1 开源协议，版权归 FFmpeg 社区。"));

    ImGui::Spacing();
    ImGui::Text(U8("© 2026 魔音录屏 版权所有"));
    ImGui::Text(U8("独家原创技术 · 禁止盗版"));
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.6f, 1.0f), u8"【唯一官方账号】");
    ImGui::TextWrapped(u8"抖音：FUNNYMOUNDPE  微信：FUNNYMODPE");
    ImGui::TextWrapped(u8"所有真实日常、更新动态、售后咨询均在此号，仅此一号，谨防假冒！");

    ImGui::End();
}

VoicePitchSet g_voice;
bool g_enableVoiceChange = false;

int  g_selectedVoice = 0;
bool show_vip_tip = false;
const char* vip_tip_text = "";
void RenderVipTipWindow()
{
    if (!show_vip_tip) return;

    ImGuiIO& io = ImGui::GetIO();
    ImVec2 size = { 380, 160 };

    ImGui::SetNextWindowSize(size);
    ImGui::SetNextWindowPos(
        { io.DisplaySize.x * 0.5f - size.x * 0.5f,
          io.DisplaySize.y * 0.5f - size.y * 0.5f },
        ImGuiCond_Always
    );

    ImGui::Begin(u8"提示", &show_vip_tip,
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings
    );

    // 文字居中
    ImVec2 textSize = ImGui::CalcTextSize(vip_tip_text);
    ImGui::SetCursorPosX((size.x - textSize.x) * 0.5f);
    ImGui::SetCursorPosY(50);
    ImGui::Text("%s", vip_tip_text);

    // 按钮居中
    ImGui::SetCursorPosX((size.x - 100) * 0.5f);
    ImGui::SetCursorPosY(110);
    if (ImGui::Button(u8"确定", ImVec2(100, 28))) {
        show_vip_tip = false;
    }

    ImGui::End(); // ✅ 绝对配对，永不报错
}




void ShowVipMessage(const char* text)
{
    vip_tip_text = text;
    show_vip_tip = true;
}
bool g_IsLoginOK = false;
#include <windows.h>
#include <string>

// 辅助函数：UTF8 -> 宽字符（解决乱码）
std::wstring UTF8ToWString(const char* utf8)
{
    if (!utf8 || !*utf8) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
    std::wstring wstr(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, &wstr[0], len);
    return wstr;
}

// 新版 ShowVipMessage UTF8 版本
void ShowVipMessageUTF8(const char* text)
{
    std::wstring wtext = UTF8ToWString(text);
    MessageBoxW(hwnd, wtext.c_str(), L"提示", MB_OK);
}
static void CleanStr(std::string& s)
{
    // 删掉换行、回车、空格
    s.erase(std::remove(s.begin(), s.end(), '\n'), s.end());
    s.erase(std::remove(s.begin(), s.end(), '\r'), s.end());
    s.erase(std::remove(s.begin(), s.end(), ' '), s.end());
}

bool Tixain = false;
bool check_vip = false;

string g_current_userid = user;
void ShowVipInfoWindow(const std::string& loginAccount)
{
    ImGui::SetNextWindowSize(ImVec2(480, 550), ImGuiCond_FirstUseEver);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
    ImGui::Begin(u8"VIP 信息", &check_vip, flags);

    static int vipStart = 0;
    static int vipEnd = 0;
    static bool loaded = false;

    if (!loaded && !loginAccount.empty()) {
        if (GetVipTimeForImGui(loginAccount, vipStart, vipEnd)) {
            loaded = true;
        }
        else {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), u8"获取VIP信息失败，请检查网络");
            ImGui::End();
            return;
        }
    }

    std::string startText = FormatVipDate(vipStart);
    std::string endText = FormatVipDate(vipEnd);

    ImGui::Text(u8"VIP 开始时间：");
    // 关键：这里必须用 %s 配合 u8 前缀
    ImGui::TextColored(ImVec4(0.2f, 1, 0.5f, 1), u8"%s", startText.c_str());

    ImGui::Spacing();
    ImGui::Text(u8"VIP 到期时间：");
    ImGui::TextColored(ImVec4(1, 0.6f, 0.2f, 1), u8"%s", endText.c_str());

    ImGui::Spacing();
    bool isVip = (vipEnd > time(NULL));
    if (isVip) {
        ImGui::TextColored(ImVec4(0, 1, 0.3f, 1), u8"状态：VIP 正常使用中 ");
    }
    else {
        ImGui::TextColored(ImVec4(1, 0.2f, 0.2f, 1), u8"状态：VIP 已过期 ");
    }

    ImGui::End();
}// 全局变量：保存支付宝账号
char g_alipay_account[64] = "";
// 界面文件里的支付宝缓存（只在本文件用，不用全局）

// 本文件内部用，不用 extern、不用全局

// 提现窗口（带绑定按钮）
void ShowWithdrawWindow(const char* current_account)
{
    ImGui::SetNextWindowSize(ImVec2(480, 580), ImGuiCond_Always);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;

    ImGui::Begin(u8"分销提现", &Tixain, flags);

    ImGui::Text(u8"提现账号：%s", current_account);
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ==========================
    // 支付宝输入 + 绑定按钮
    // ==========================
    ImGui::Text(u8"支付宝账号：");
    ImGui::InputText(u8"##alipay_input", g_alipay_account, sizeof(g_alipay_account));

    // 绑定按钮（点一下就永久保存到服务器）
    if (ImGui::Button(u8"绑定支付宝", ImVec2(150, 35))) {
        if (strlen(g_alipay_account) < 5) {
            MessageBoxW(hwnd, L"请输入有效的支付宝账号", L"提示", MB_OK | MB_ICONWARNING);
        }
        else {
            string msg = BindAlipayToServer(current_account, g_alipay_account);
            std::wstring wMsg = UTF8ToWString(msg.c_str());
            MessageBoxW(hwnd, wMsg.c_str(), L"绑定结果", MB_OK);
        }
    }
    ImGui::TextColored(ImVec4(0.4f, 1, 0.4f, 1), u8"绑定后，提现自动使用此账号");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text(u8"选择提现金额");
    ImGui::Spacing();

    float btnW = 130;
    float btnH = 40;

    // 50
    if (ImGui::Button(u8"提现 50 元", ImVec2(btnW, btnH))) {
        string msg = SendWithdrawRequest(current_account, 50, g_alipay_account);
        MessageBoxW(hwnd, UTF8ToWString(msg.c_str()).c_str(), L"提现结果", MB_OK);
    }
    ImGui::SameLine();

    // 100
    if (ImGui::Button(u8"提现 100 元", ImVec2(btnW, btnH))) {
        string msg = SendWithdrawRequest(current_account, 100, g_alipay_account);
        MessageBoxW(hwnd, UTF8ToWString(msg.c_str()).c_str(), L"提现结果", MB_OK);
    }
    ImGui::SameLine();

    // 150
    if (ImGui::Button(u8"提现 150 元", ImVec2(btnW, btnH))) {
        string msg = SendWithdrawRequest(current_account, 150, g_alipay_account);
        MessageBoxW(hwnd, UTF8ToWString(msg.c_str()).c_str(), L"提现结果", MB_OK);
    }

    ImGui::Spacing();
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - btnW - 5);

    // 200
    if (ImGui::Button(u8"提现 200 元", ImVec2(btnW, btnH))) {
        string msg = SendWithdrawRequest(current_account, 200, g_alipay_account);
        MessageBoxW(hwnd, UTF8ToWString(msg.c_str()).c_str(), L"提现结果", MB_OK);
    }
    ImGui::SameLine();

    // 300
    if (ImGui::Button(u8"提现 300 元", ImVec2(btnW, btnH))) {
        string msg = SendWithdrawRequest(current_account, 300, g_alipay_account);
        MessageBoxW(hwnd, UTF8ToWString(msg.c_str()).c_str(), L"提现结果", MB_OK);
    }

    ImGui::End();
}
bool Fenxiao1 = false;
void DrawDistributeCenter()
{
   // if (!Fenxiao) return; // 开关控制

    // 窗口带关闭按钮：点×自动把Fenxiao设为false
    ImGui::SetNextWindowSize(ImVec2(500, 350), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(u8"分销中心（全民代理）", &Fenxiao))
    {
        ImGui::Text(u8"当前账号：%s", user);
        ImGui::Separator();

        if (ImGui::Button(u8"一键生成我的推广链接", ImVec2(460, 36))) {
            GetMyInviteLink(user);
        }
        ImGui::Spacing();

        ImGui::Text(u8"你的专属推广链接：");
        ImGui::TextWrapped(u8"%s", g_InviteLink.c_str());
        ImGui::Spacing();

        if (ImGui::Button(u8"一键复制链接", ImVec2(460, 36))) {
            CopyToClipboard(g_InviteLink);
        }
        ImGui::Spacing();

        if (ImGui::Button(u8"一键打开官网查询", ImVec2(460, 36))) {
            OpenCommissionUrl();
        }
        ImGui::Spacing();
        if (ImGui::Button(u8"申请提现", ImVec2(460, 36))) {
            Tixain = true;
          //  ShowWithdrawWindow();
        }
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1, 0.8f, 0.2f, 1), u8"提示：【纯推广返利・无层级・无代理・谁推谁赚 45%%】！");
        ImGui::TextColored(ImVec4(1, 0.8f, 0.2f, 1),u8"提示：任何人通过你的链接注册付款，你自动拿45%%佣金！");
      
       if(Tixain) ShowWithdrawWindow(user);
    }
  //  ShowWithdrawWindow();
    ImGui::End();
}

// 放在顶部
extern int payType ;
static bool showPaySelect = false;

// 选择支付方式 + 显示二维码
// 全局标记（别动）
static int selectPackage = 0; // 1月 2季 3年
// 支付套餐窗口 + 对接 CreateVipOrder 自动创建订单
void ShowPaySelectWindow()
{
    if (!showPaySelect)
        return;

    ImGui::SetNextWindowBgAlpha(1.0f);
    ImGui::SetNextWindowSize(ImVec2(480, 550), ImGuiCond_FirstUseEver);
    ImGui::Begin(u8"选择支付方式", &showPaySelect);

    ImGui::TextColored(ImVec4(1, 0.85f, 0, 1), u8"请选择您的付款渠道：");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button(u8"微信支付", ImVec2(200, 50)))
    {
        payType = 1;
        selectPackage = 0;
    }
    ImGui::SameLine();
    if (ImGui::Button(u8"支付宝支付", ImVec2(200, 50)))
    {
        payType = 2;
        selectPackage = 0;
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (payType == 1)
    {
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1), u8"—— 微信套餐 ——");
        ImGui::Spacing();

        if (ImGui::Button(u8"月卡 ¥9.9", ImVec2(140, 40))) {
            selectPackage = 1;
            std::string orderNo;
            std::string result =// 微信月卡
                CreateVipOrder(9.9f, 1, user, orderNo);
           
           
        
            ShowVipMessageUTF8(result.c_str());
         
        }
        ImGui::SameLine();
        if (ImGui::Button(u8"季卡 ¥25", ImVec2(140, 40))) {
            selectPackage = 2;
            std::string orderNo;
            std::string result = CreateVipOrder(25.0f, 1, user, orderNo);
            MessageBoxA(hwnd, result.c_str(), u8"订单结果", MB_OK);
        }
        ImGui::SameLine();
        if (ImGui::Button(u8"年卡 ¥88", ImVec2(140, 40))) {
            selectPackage = 3;
            std::string orderNo;
            std::string result = CreateVipOrder(88.0f, 1, user, orderNo);
            MessageBoxA(hwnd, result.c_str(), u8"订单结果", MB_OK);
        }
    }

    if (payType == 2)
    {
        ImGui::TextColored(ImVec4(0.1f, 0.6f, 1, 1), u8"—— 支付宝套餐 ——");
        ImGui::Spacing();

        if (ImGui::Button(u8"月卡 ¥9.9", ImVec2(140, 40))) {
            selectPackage = 1;
            std::string orderNo;
            std::string result = CreateVipOrder(9.9f, 2, user, orderNo);
            MessageBoxA(hwnd, result.c_str(), u8"订单结果", MB_OK);
        }
        ImGui::SameLine();
        if (ImGui::Button(u8"季卡 ¥25", ImVec2(140, 40))) {
            selectPackage = 2;
            std::string orderNo;
            std::string result = CreateVipOrder(25.0f, 2, user, orderNo);
            MessageBoxA(hwnd, result.c_str(), u8"订单结果", MB_OK);
        }
        ImGui::SameLine();
        if (ImGui::Button(u8"年卡 ¥88", ImVec2(140, 40))) {
            selectPackage = 3;
            std::string orderNo;
            std::string result = CreateVipOrder(88.0f, 2, user, orderNo);
            MessageBoxA(hwnd, result.c_str(), u8"订单结果", MB_OK);
        }
    }
    // 绿色大按钮，全屏宽度，点击查询VIP
    if (ImGui::Button(u8"查询VIP是否到账", ImVec2(-1, 50)))
    {
        OnBtnCheckVIP_Click(user); // 直接调用你刚才的查询函数
    }
    ImGui::End();
}
// 你原来的核心函数放在外面就行

static bool showVipWindow = false;
void ShowVipWindow()
{
    if (showVipWindow)
    {
        ImGui::SetNextWindowSize(ImVec2(500, 480));
        ImGui::Begin(u8"会员中心", &showVipWindow);

        // ====================== 顶部：会员中心标题 ======================
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - 120) * 0.5f);
        ImGui::TextColored(ImVec4(1, 0.85f, 0, 1), u8" 魔音录屏 VIP 会员中心 ");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // ====================== 会员权益介绍（从上到下） ======================
        ImGui::SetCursorPosX(30);
        ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.3f, 1), u8"【VIP 会员 全部特权】");
        ImGui::Spacing();

        // 每条权益一行，带颜色
        ImGui::SetCursorPosX(40);
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.6f, 1), u8"1. 视频导出：无水印");
        ImGui::SetCursorPosX(40);
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.6f, 1), u8"2. 画质支持：1080P 全高清");
        ImGui::SetCursorPosX(40);
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.6f, 1), u8"3. 帧率支持：60帧稳定导出");
        ImGui::SetCursorPosX(40);
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.6f, 1), u8"4. 变声算法：全部功能解锁使用");
        ImGui::SetCursorPosX(40);
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.6f, 1), u8"5. 专属客服：优先问题解答");
        ImGui::SetCursorPosX(40);
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.6f, 1), u8"6. 后续更新：所有新功能免费体验");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // ====================== 底部：开通按钮（你原来的逻辑） ======================
        ImGui::SetCursorPosX(30);
        if (ImGui::Button(u8"立即开通 VIP 会员", ImVec2(440, 40)))
        {
            showPaySelect = true;
        }
        ImGui::Spacing();
        ImGui::SetCursorPosX(30);
        if (ImGui::Button(u8"查询VIP信息", ImVec2(440, 40)))
        {
            check_vip = true;
        }

        ImGui::End();
        if(check_vip)ShowVipInfoWindow(user);
        ShowPaySelectWindow();
    }
}



// 全局变量你保留原来的
static BOOL g_bNetWorking = FALSE;
static char g_NetMsg[256] = { 0 };

struct LoginRegParam
{
    char account[32];
    char pwd[32];
    int  type;
};
void MsgBoxU8(const char* u8text)
{
    if (!u8text) u8text = "";

    // 转宽字符
    int wlen = MultiByteToWideChar(CP_UTF8, 0, u8text, -1, NULL, 0);
    std::wstring wstr(wlen, 0);
    MultiByteToWideChar(CP_UTF8, 0, u8text, -1, &wstr[0], wlen);

    // 弹窗
    MessageBoxW(hwnd, wstr.c_str(), L"服务器返回(UTF8)", MB_OK);
}
DWORD WINAPI NetWorkThread(LPVOID lpParam)
{
    LoginRegParam* p = (LoginRegParam*)lpParam;
    g_bNetWorking = TRUE;

    std::string acc = p->account;
    std::string pass = p->pwd;
    std::string machine = GetCpuMachineCode();

    // ✅ 正确 POST 格式（PHP 必认）
    std::string post =
        "account=" + UrlEncode(acc.c_str()) +
        "&password=" + UrlEncode(pass.c_str()) +
        "&machine=" + UrlEncode(machine.c_str());

    std::string res;

    if (p->type == 1)
        res = HttpNet_Post("https://moyinluping.com/login.php", post.c_str());
    else
        res = HttpNet_Post("https://moyinluping.com/reg_api.php", post.c_str());

   // MsgBoxU8(res.c_str());
    strcpy(g_NetMsg, res.c_str());

    delete p;
    g_bNetWorking = FALSE;
    return 0;
}static bool showSettingWin = false;
//  ===================== 自动加的：记住密码配置 =====================
void SaveLoginInfo(const char* account, const char* password)
{
    WritePrivateProfileStringA("Login", "Account", account, "login.ini");
    WritePrivateProfileStringA("Login", "Password", password, "login.ini");
}

void LoadLoginInfo(char* account, char* password, int maxSize)
{
    GetPrivateProfileStringA("Login", "Account", "", account, maxSize, "login.ini");
    GetPrivateProfileStringA("Login", "Password", "", password, maxSize, "login.ini");
}

//  ===================== 你原来的代码（已自动集成自动登录） =====================
void RenderSettingsWindow()
{
    // 自动加载上次保存的账号密码
  
    static char pwd[16] = "";
    static bool firstLoad = true;

    // 第一次打开界面 → 自动读保存的账号密码
    if (firstLoad)
    {
        firstLoad = false;
        LoadLoginInfo(user, pwd, 32);
    }

    ImGui::SetNextWindowSize(ImVec2(520, 520));
    ImGui::Begin(u8"设置 - 魔音录屏", &show_settings, ImGuiWindowFlags_NoResize);

    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - 140) * 0.5f);
    ImGui::TextColored(ImVec4(1, 0.85f, 0, 1), u8"账号登录中心");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    static char pwd2[16] = "";

    static float showPwdTime = 0.0f;
    static float showPwd2Time = 0.0f;
    float dt = ImGui::GetIO().DeltaTime;

    if (showPwdTime > 0.f)  showPwdTime -= dt;
    if (showPwd2Time > 0.f) showPwd2Time -= dt;

    bool showPwd1 = showPwdTime > 0.f;
    bool showPwd2 = showPwd2Time > 0.f;

   static std::string g_MachineCode = "";
   // static char g_MachineCode[256] = "";

    // ========== 登录/注册结果提示（硬编码u8，适配你的ShowVipMessage） ==========
    if (strlen(g_NetMsg) > 0 && !g_bNetWorking)
    {
        if (strstr(g_NetMsg, u8"登录成功"))
        {
            g_IsLoginOK = true;
            SaveLoginInfo(user, pwd); // 登录成功 → 自动保存
            ShowVipMessage(u8"登录成功");
        }
        else if (strstr(g_NetMsg, "参数缺失"))
        {
            ShowVipMessage(u8"参数缺失");
        }
        else if (strstr(g_NetMsg, "注册成功"))
        {
            ShowVipMessage(u8"注册成功");
        }
        else if (strstr(g_NetMsg, "账号已存在"))
        {
            ShowVipMessage(u8"账号已存在");
        }
        else if (strstr(g_NetMsg, "密码错误"))
        {
            ShowVipMessage(u8"密码错误");
        }
        else if (strstr(g_NetMsg, "账号不存在"))
        {
            ShowVipMessage(u8"账号不存在");
        }
      
        ZeroMemory(g_NetMsg, sizeof(g_NetMsg));
    }

    if (!g_IsLoginOK)
    {
        ImGui::SetCursorPosX(30);
        ImGui::TextColored(ImVec4(0.6f, 0.8f, 0.6f, 1), u8"提示：账号密码随意好记即可");
        ImGui::Spacing();

        // 账号
        ImGui::SetCursorPosX(30);
        ImGui::TextColored(ImVec4(0.7f, 0.8f, 1, 1), u8"账号");
        ImGui::SetNextItemWidth(440);
        ImGui::InputText("##username", user, IM_ARRAYSIZE(user));

        // 密码1 + 查看按钮（加唯一ID ##pwd1）
        ImGui::SetCursorPosX(30);
        ImGui::TextColored(ImVec4(0.7f, 0.8f, 1, 1), u8"密码");
        ImGui::SetNextItemWidth(360);
        ImGuiInputTextFlags flag1 = showPwd1 ? 0 : ImGuiInputTextFlags_Password;
        ImGui::InputText("##password", pwd, IM_ARRAYSIZE(pwd), flag1);

        ImGui::SameLine();
        if (ImGui::Button(showPwd1 ? u8"已显示##pwd1" : u8"查看(5秒)##pwd1", ImVec2(80, 26)))
        {
            showPwdTime = 5.0f;
        }

        // 密码2 + 查看按钮（加唯一ID ##pwd2）
        ImGui::SetCursorPosX(30);
        ImGui::TextColored(ImVec4(0.7f, 0.8f, 1, 1), u8"再次输入密码");
        ImGui::SetNextItemWidth(360);
        ImGuiInputTextFlags flag2 = showPwd2 ? 0 : ImGuiInputTextFlags_Password;
        ImGui::InputText("##password2", pwd2, IM_ARRAYSIZE(pwd2), flag2);

        ImGui::SameLine();
        if (ImGui::Button(showPwd2 ? u8"已显示##pwd2" : u8"查看(5秒)##pwd2", ImVec2(80, 26)))
        {
            showPwd2Time = 5.0f;
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // 登录 注册
        ImGui::SetCursorPosX(30);
        ImGui::BeginGroup();

        if (ImGui::Button(u8"登录账号", ImVec2(190, 36)) && !g_bNetWorking)
        {
            if (strlen(user) < 2)
            {
                ShowVipMessage(u8"错误：账号没填或太短！");
                return;
            }
            if (strlen(pwd) < 4)
            {
                ShowVipMessage(u8"错误：密码没填或太短！");
                return;
            }
            std::string testCode = GetCpuMachineCode();
            if (testCode.empty())
            {
                ShowVipMessage(u8"错误：获取不到本机机器码！先点【获取机器码】");
                return;
            }

            LoginRegParam* p = new LoginRegParam();
            strcpy(p->account, user);
            strcpy(p->pwd, pwd);
            p->type = 1;
            CreateThread(NULL, 0, NetWorkThread, p, 0, NULL);
        }

        ImGui::SameLine(240);

        if (ImGui::Button(u8"注册账号", ImVec2(190, 36)) && !g_bNetWorking)
        {
            if (strcmp(pwd, pwd2) != 0)
            {
                ShowVipMessage(u8"错误：两次密码不一样！");
                return;
            }
            if (strlen(user) < 2)
            {
                ShowVipMessage(u8"错误：账号没填或太短！");
                return;
            }
            if (strlen(pwd) < 4)
            {
                ShowVipMessage(u8"错误：密码没填或太短！");
                return;
            }
            std::string testCode = GetCpuMachineCode();
            if (testCode.empty())
            {
                ShowVipMessage(u8"错误：获取不到本机机器码！先点【获取机器码】");
                return;
            }

            LoginRegParam* p = new LoginRegParam();
            strcpy(p->account, user);
            strcpy(p->pwd, pwd);
            p->type = 2;
            CreateThread(NULL, 0, NetWorkThread, p, 0, NULL);
        }

        ImGui::EndGroup();
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // 机器码
        ImGui::SetCursorPosX(30);
        ImGui::TextColored(ImVec4(1, 0.85f, 0, 1), u8"本机机器码");
        ImGui::Spacing();

        ImGui::SetNextItemWidth(440);
        ImGui::InputText("##machinecode", (char*)g_MachineCode.c_str(), g_MachineCode.capacity(), ImGuiInputTextFlags_ReadOnly);
        //ImGui::InputText("##machinecode", g_MachineCode, 256, ImGuiInputTextFlags_ReadOnly); // 修
        ImGui::SetCursorPosX(30);
        if (ImGui::Button(u8"获取机器码", ImVec2(215, 26)))
        {
            g_MachineCode = GetCpuMachineCode();
           //strcpy(g_MachineCode, GetCpuMachineCode().c_str());
        }

        ImGui::SameLine();
       if (ImGui::Button(u8"一键复制", ImVec2(215, 26)) && !g_MachineCode.empty())
       // if (ImGui::Button(u8"一键复制", ImVec2(215, 26)) && strlen(g_MachineCode) > 0)
        {
            OpenClipboard(NULL);
            EmptyClipboard();
            HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, g_MachineCode.size() + 1);
            //HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, strlen(g_MachineCode) + 1);
            if (hMem)
            {
                char* pBuf = (char*)GlobalLock(hMem);
                memcpy(pBuf, g_MachineCode.c_str(), g_MachineCode.size() + 1);
               // strcpy(pBuf, g_MachineCode);
                GlobalUnlock(hMem);
                SetClipboardData(CF_TEXT, hMem);
                CloseClipboard();
                ShowVipMessage(u8"机器码已复制到剪贴板");
            }
        }
    }
    else
    {
        ImGui::SetCursorPosX(30);
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.6f, 1), u8"已登录成功，欢迎使用魔音录屏");
        ImGui::Spacing();
    }

    ImGui::SetCursorPosX(30);
    ImGui::Spacing();
    if (ImGui::Button(u8"找回账号密码", ImVec2(460, 40)))
    {
         OpenMoyinPage();
    }
    ImGui::Spacing();
    if (ImGui::Button(u8"会员中心", ImVec2(460, 40)))
    {
        showVipWindow = true;
    }
    ImGui::Spacing();
    if (ImGui::Button(u8"分销中心", ImVec2(460, 40)))
    {
        Fenxiao = true;
   
    }
    ImGui::Spacing();
    
    if (ImGui::Button(u8"关于", ImVec2(460, 40)))
    {
        show_about = true;

    }
    ImGui::Spacing();
    ImGui::End();
    ShowVipWindow();
   if(Fenxiao) DrawDistributeCenter();
   if (show_about) RenderAboutWindow();
}
bool Gonggao = false;   
extern std::string g_NoticeText;
// 显示服务器公告（独立函数）
void ShowNoticeWindow()
{
    if (!Gonggao) return;  // 关闭就不显示

    // 固定窗口大小
    ImGui::SetNextWindowSize(ImVec2(520, 520));
    ImGui::Begin(u8"公告", &Gonggao, ImGuiWindowFlags_NoResize);

    // 公告内容区域（自动滚动）
    ImGui::BeginChild("公告区域", ImVec2(0, 460), true);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextColored(ImVec4(1, 1, 0, 1), u8"===== 服务器公告 =====");
    ImGui::TextWrapped("%s", g_NoticeText.c_str()); // 自动换行
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::EndChild();
    ImGui::End(); // 必须加！
}





// ===================== 你的主界面（已修复乱码）=====================
void RenderDraggableMenu()
{
    if (GetAsyncKeyState(VK_F2) & 1)
    {
        MenuConfig::show_menu_window = !MenuConfig::show_menu_window;
    }

    if (!MenuConfig::show_menu_window)
        return;

    static bool is_voice_menu_collapsed = false;
    ImVec2 menu_size(460, is_voice_menu_collapsed ? 320 : 580);

    int window_flags = 0;
    window_flags |= ImGuiWindowFlags_NoResize;
    window_flags |= ImGuiWindowFlags_NoTitleBar;

    ImGui::SetNextWindowPos(MenuConfig::menu_pos, ImGuiCond_Once);
    ImGui::SetNextWindowSize(menu_size);

    if (ImGui::Begin("##功能菜单", &MenuConfig::show_menu_window, window_flags))
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 wp = ImGui::GetWindowPos();
        ImVec2 ws = ImGui::GetWindowSize();

        ImU32 bg_top = IM_COL32(80, 60, 120, 240);
        ImU32 bg_bot = IM_COL32(40, 100, 80, 240);
        dl->AddRectFilledMultiColor(wp, ImVec2(wp.x + ws.x, wp.y + ws.y), bg_top, bg_top, bg_bot, bg_bot);

        // 标题行
        ImGui::SetCursorPosY(10);
        ImGui::TextColored(ImVec4(1.0f, 0.84f, 0.0f, 1.0f), U8("魔音录屏"));
        ImGui::SameLine(ImGui::GetWindowWidth() - 160);
        if (ImGui::Button(U8(" 设置 "))) {
            show_settings = true;
        }
        ImGui::SameLine(ImGui::GetWindowWidth() - 220);
        if (ImGui::Button(U8(" 公告"))) {
            Gonggao = true;
        }
        if (Gonggao)ShowNoticeWindow();
        ImGui::SameLine(ImGui::GetWindowWidth() - 90);
        ImGui::TextColored(ImVec4(0.0f, 0.8f, 0.6f, 1.0f), U8("F2 显示/隐藏"));

        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

        UpdateRecordingTimer(); // 时钟自己走
        ImGui::TextColored(ImVec4(0.8f, 1.0f, 0.8f, 1.0f), U8("录屏控制"));
        ImGui::Spacing();
        if (ImGui::Button(U8("开始 / 暂停录屏"), ImVec2(menu_size.x - 160, 32))) {
            OnStartBtnClick();
            
        }
        ImGui::SameLine();
        if (ImGui::Button(U8("停止"), ImVec2(130, 32))) {
            OnStopBtnClick();
           
        }DrawRecordingTimer();
        ImGui::Spacing();
       // ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), U8("状态：未录制"));
        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

        // 音频设置
        ImGui::TextColored(ImVec4(0.8f, 1.0f, 0.8f, 1.0f), U8("音频设置"));
        ImGui::Spacing();

        static bool enable_mic = false;
        float r = 7.0f;
        
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton("##mic", ImVec2(r * 3, r * 2));
        if (ImGui::IsItemClicked())
        {
           
            enable_mic = !enable_mic;
            micRecorder.EnableMic(enable_mic);
        }
        dl->AddCircle(ImVec2(p.x + r * 1.5f, p.y + r + 1), r, IM_COL32(200, 200, 200, 255), 0, 1.5f);
        if (enable_mic)
            dl->AddCircleFilled(ImVec2(p.x + r * 1.5f, p.y + r + 1), r - 2.5f, IM_COL32(255, 60, 60, 255));
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.9f, 1, 0.9f, 1), u8"录制麦克风");

        // 变声总开关
        ImGui::SameLine(230);
        p = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton("##voice_toggle", ImVec2(r * 3, r * 2));
        if (ImGui::IsItemClicked())
        {
            g_voice.enable = !g_voice.enable;
        }
        dl->AddCircle(ImVec2(p.x + r * 1.5f, p.y + r + 1), r, IM_COL32(200, 200, 200, 255), 0, 1.5f);
        if (g_voice.enable)
            dl->AddCircleFilled(ImVec2(p.x + r * 1.5f, p.y + r + 1), r - 3.0f, IM_COL32(255, 60, 60, 255));
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.9f, 1, 0.9f, 1), U8("开启变声"));

        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

        // 存储路径
        ImGui::TextColored(ImVec4(0.6f, 1.0f, 0.6f, 1.0f), U8("视频默认保存到：系统【视频】\\魔音录屏"));
        if (ImGui::Button(U8("打开输出目录"), ImVec2(menu_size.x - 20, 26))) {
        
            // 👇 下面这一整段直接放这里，完美运行
            wchar_t videoPath[MAX_PATH] = { 0 };

            // 获取系统【视频】文件夹路径
            SHGetFolderPathW(NULL, CSIDL_MYVIDEO, NULL, SHGFP_TYPE_CURRENT, videoPath);

            // 拼接成：视频\魔音录屏
            wchar_t fullPath[MAX_PATH] = { 0 };
            PathCombineW(fullPath, videoPath, L"魔音录屏");

            // 如果文件夹不存在，自动创建
            CreateDirectoryW(fullPath, NULL);

            // 打开文件夹并选中（资源管理器效果）
            ShellExecuteW(NULL, L"open", fullPath, NULL, NULL, SW_SHOWNORMAL);
        
        }
        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

        // ===================== 变声音色按钮：4×4 网格布局，全部可见 =====================
        ImGui::TextColored(ImVec4(0.8f, 1.0f, 0.8f, 1.0f), U8("变声音色选择"));
        ImGui::Spacing();
        if (ImGui::Button(is_voice_menu_collapsed ? U8("展开音色列表") : U8("收起音色列表"), ImVec2(menu_size.x - 20, 26)))
            is_voice_menu_collapsed = !is_voice_menu_collapsed;

        if (!is_voice_menu_collapsed)
        {
            ImGui::Spacing();
            const char* voiceNames[] = { u8"原声", u8"小黄人[1]", u8"小黄人[2]", u8"小黄人[3]", u8"小黄人[4]", u8"小黄人[5]", u8"小黄人[6]", u8"小黄人[7]", u8"小黄人[8]", u8"小黄人[9]", u8"小黄人[10]" };
            int currentType = g_voice.type;
            if (currentType >= 0 && currentType < 16) {
                ImGui::TextColored(ImVec4(0, 1, 0, 1), u8"当前使用: %s", voiceNames[currentType]);
            }
            ImGui::Spacing();
            ImGui::Spacing();

            // 按钮宽度适配4个一排
            ImVec2 vb_size(100, 28);
            
            // ========== 第 1 行：4个 ==========
            if (ImGui::Button(U8("原声"), vb_size)) {
                g_voice.type = 0;
                g_voice.pitch = 1.0f;
                g_voice.enable = false;
            }
            ImGui::SameLine();
            if (ImGui::Button(U8("小黄人[1]"), vb_size)) {
                g_voice.type = 1;
                g_voice.pitch = 1.1f;
                g_voice.enable = true;
            }
            ImGui::SameLine();
            if (ImGui::Button(U8("小黄人[2]"), vb_size)) {
                g_voice.type = 2;
                if (g_IsVIP == false)
                {
                   
                    g_voice.pitch = 1.0f;
                    ShowVipMessage(u8"您还不是VIP用户，请充值VIP");
                }
                else {
                   
                    g_voice.pitch = 1.2f;
                }
                g_voice.enable = true;
            }
            ImGui::SameLine();
            if (ImGui::Button(U8("小黄人[3]"), vb_size)) {
                g_voice.type = 3;
                if (g_IsVIP == false)
                {
                    g_voice.pitch = 1.0f;
                    ShowVipMessage(u8"您还不是VIP用户，请充值VIP");
                   
                }
                else {
                    g_voice.pitch = 1.3f;
                }
                g_voice.enable = true;
            }

            // ========== 第 2 行：4个 ==========
            ImGui::Spacing();
            if (ImGui::Button(U8("小黄人[4]"), vb_size)) {
                g_voice.type = 4;
                if (g_IsVIP == false)
                {
                    g_voice.pitch = 1.0f;
                    ShowVipMessage(u8"您还不是VIP用户，请充值VIP");
                    
                }
                else {
                    g_voice.pitch = 1.4f;
                }
                g_voice.enable = true;
            }
            ImGui::SameLine();
            if (ImGui::Button(U8("小黄人[5]"), vb_size)) {
                g_voice.type = 5;
                if (g_IsVIP == false)
                {
                    g_voice.pitch = 1.0f;
                    ShowVipMessage(u8"您还不是VIP用户，请充值VIP");
                   
                }
                else {
                    g_voice.pitch = 1.5f;
                }
                g_voice.enable = true;
            }
            ImGui::SameLine();
            if (ImGui::Button(U8("小黄人[6]"), vb_size)) {
                g_voice.type = 6;
                if (g_IsVIP == false)
                {
                 
                    g_voice.pitch = 1.0f;
                    ShowVipMessage(u8"您还不是VIP用户，请充值VIP");
                }
                else {
                    g_voice.pitch = 1.6f;
                }
                g_voice.enable = true;
            }
            ImGui::SameLine();
            if (ImGui::Button(U8("小黄人[7]"), vb_size)) {
                g_voice.type = 7;
                if (g_IsVIP == false)
                {
                   
                    g_voice.pitch = 1.0f;
                    ShowVipMessage(u8"您还不是VIP用户，请充值VIP");
                }
                else {
                    g_voice.pitch = 1.7f;
                }
                g_voice.enable = true;
            }

            // ========== 第 3 行：4个 ==========
            ImGui::Spacing();
            if (ImGui::Button(U8("小黄人[8]"), vb_size)) {
                g_voice.type = 8;
                if (g_IsVIP == false)
                {
                   
                    g_voice.pitch = 1.0f;
                    ShowVipMessage(u8"您还不是VIP用户，请充值VIP");
                }
                else {
                    g_voice.pitch = 1.8f;
                }
                g_voice.enable = true;
            }
            ImGui::SameLine();
            if (ImGui::Button(U8("小黄人[9]"), vb_size)) {
                g_voice.type = 9;
                if (g_IsVIP == false)
                {
                   
                    g_voice.pitch = 1.0f;
                    ShowVipMessage(u8"您还不是VIP用户，请充值VIP");
                }
                else {
                    g_voice.pitch = 1.9f;
                }
                g_voice.enable = true;
            }
            ImGui::SameLine();
            if (ImGui::Button(U8("小黄人[10]"), vb_size)) {
                g_voice.type = 10;
                if (g_IsVIP == false)
                {
                   
                    g_voice.pitch = 1.0f;
                    ShowVipMessage(u8"您还不是VIP用户，请充值VIP");
                }
                else {
                   
                    g_voice.pitch = 2.0f;
                }
                g_voice.enable = true;
            
            }
            
        }

        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
        if (ImGui::Button(U8("打开官网"), ImVec2(menu_size.x - 20, 28))) {
            OpenMoYinHomePage();
        }
        ImGui::Spacing();
        if (ImGui::Button(U8("退出程序"), ImVec2(menu_size.x - 20, 28))) {
            MenuConfig::quit_app = true;
        }
        ImGui::Spacing();
        ImGui::End();
    }
   
    if (show_settings) RenderSettingsWindow();
   
}

IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    // 调用菜单快捷键处理
    HandleMenuShortcut(msg, wParam);

    switch (msg)
    {
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    case WM_SIZE:
        g_ResizeWidth = (UINT)LOWORD(lParam);
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    default:
        return ::DefWindowProcW(hWnd, msg, wParam, lParam);
    }
}

// 初始化透明窗口（全局函数，无嵌套）
bool InitTransparentWindow(HWND& hwnd, WNDCLASSEXW& wc)
{
    // 1. 注册窗口类
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = L"TransparentOverlayWindow";

    if (!::RegisterClassExW(&wc))
        return false;

    // 2. 获取屏幕尺寸（全屏）
    const int screen_width = ::GetSystemMetrics(SM_CXSCREEN);
    const int screen_height = ::GetSystemMetrics(SM_CYSCREEN);

    // 3. 创建全屏、置顶、透明窗口
    hwnd = ::CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED,  // 置顶 + 分层（透明关键）
        wc.lpszClassName,
        L"全屏透明覆盖窗口",
        WS_POPUP | WS_VISIBLE,          // 无边框全屏
        0, 0, screen_width, screen_height,
        nullptr, nullptr, wc.hInstance, nullptr
    );

    if (!hwnd)
        return false;

    // 4. 设置窗口透明（颜色键为黑色，完全透明）
    ::SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);

    // 5. 初始化D3D
    if (!CreateDeviceD3D(hwnd))
    {
       
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return false;
    }
    
   
    // 6. 初始化ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // 初始化字体
    InitImGuiFonts();

    // 初始化菜单样式
    InitMenuStyle();
    
    // 7. 绑定ImGui到Win32和DX11
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    return true;
}
extern atomic<double> g_TotalDurationSec;
extern atomic<bool>  g_IsMerging;
extern atomic<double> g_MergeProgressSec;
void RenderMergeOverlay()
{
    // ✅ 修复：atomic 读取用 .load()
    if (!g_IsMerging.load()) return;

    double total = g_TotalDurationSec.load();
    double curr = g_MergeProgressSec.load();
    int remaining = (int)max(0.0, total - curr);

    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 220, 30));
    ImGui::SetNextWindowSize(ImVec2(200, 90));
    ImGui::Begin(u8"合成中", nullptr,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoInputs |
        ImGuiWindowFlags_AlwaysAutoResize
    );

    ImGui::TextColored(ImVec4(1, 0.75f, 0, 1), u8"视频合成中");
    ImGui::Text(u8"剩余时间: %d 秒", remaining);

    float progress = 0.0f;
    if (total > 0)
        progress = (float)(curr / total);

    ImGui::ProgressBar(progress, ImVec2(-1, 22));
    ImGui::End();
}
void RunMainLoop(HWND hwnd)
{

    
    bool done = false;
    while (!done && !MenuConfig::quit_app)
    {
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

        // ... 你原来的交换链、Resize 代码 ...

        // ImGui帧开始
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
      //  RefreshUserVIP(user);  开通vip
        ShowSplashFinal();
        RenderDraggableMenu();
        RenderVipTipWindow();
        RenderMergeOverlay();
        // 验证码倒计时刷新
       

        ImGui::Render();

        // ==============================================
        // VIP 欢迎语音（只播1次）
        // ==============================================
        static bool hasPlayedWelcome = false;
        if (!hasPlayedWelcome)
        {
            MenuConfig::is_vip = true;
            if (MenuConfig::is_vip)
            {
                std::string wavPath = "启动声音.wav";
                // 强制绝对路径 + 异步
                PlaySoundA(wavPath.c_str(), NULL, SND_FILENAME | SND_ASYNC| SND_NODEFAULT);
            //    PlaySoundA("moyin_sweet_girl.wav", NULL, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
                hasPlayedWelcome = true;
            }
        }
        // ==============================================

        const float clear_color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);

        float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        g_pd3dDeviceContext->OMSetBlendState(g_blendState, blendFactor, 0xFFFFFFFF);

        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        HRESULT hr = g_pSwapChain->Present(0, 0);
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);

        if (MenuConfig::quit_app)
            done = true;
    }
}

// ===================== 主函数 =====================
int main(int, char**)
{
    WNDCLASSEXW wc;
   
    InitNotice();
    // 初始化全屏透明窗口
    if (!InitTransparentWindow(hwnd, wc))
    {
        MessageBoxA( hwnd, "初始化失败！", "错误", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    // 运行主循环
    RunMainLoop(hwnd);
    ShowWindow(GetConsoleWindow(), SW_HIDE);
    // 清理资源
   
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}