#include "SystemRecorder.h"
#include "screenrecording.h"
#include <Windows.h>
#include <atomic>
#include <string>
#include <cstring>
#include <stdio.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl.h>
#include <chrono>

using namespace Microsoft::WRL;
using namespace std;
extern bool g_IsVIP;
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "Shlwapi.lib")
extern HWND hwnd;
atomic<bool> g_isRecording = false;
extern atomic<bool> g_bPaused;
static HANDLE g_hRecordThread = NULL;
static PROCESS_INFORMATION g_ffmpegPI = { 0 };
static UINT64 s_videoConsumedAudioFrames = 0;
// 录制时长 & 合成倒计时用
std::chrono::steady_clock::time_point g_RecordStartTime;
atomic<double> g_TotalDurationSec = 0.0;   // 总时长(秒)
atomic<bool>  g_IsMerging = false;         // 是否正在合成
atomic<double> g_MergeProgressSec = 0.0;    // 合成已完成秒数
// ==========================
// 全局 VIP 配置（自动切换）
// ==========================


// 内部抓屏固定抓全屏1920x1080（保证永远录全屏）
#define CAPTURE_WIDTH  1920
#define CAPTURE_HEIGHT 1080 

float GetScreenRefreshRate()
{
    DEVMODE dm = { 0 };
    dm.dmSize = sizeof(DEVMODE);

    // 获取当前显示设置
    if (!EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm))
    {
        MessageBoxA(hwnd, "获取失败，默认60Hz", "提示", MB_OK);
        return 60.0f;
    }

    // 刷新率（整数）
    DWORD hz = dm.dmDisplayFrequency;

    // 只显示整数，绝对不会出 F 这种乱码
    char buf[128];
    wsprintfA(buf, "屏幕刷新率：%u Hz", hz);

    MessageBoxA(hwnd, buf, "刷新率", MB_OK | MB_ICONINFORMATION);

    // 按你的要求返回 float 类型
    return (float)hz;
}

// 获取当前主显示器刷新率（单位：Hz）
string GetExeDir()
{
    char path[MAX_PATH] = { 0 };
    GetModuleFileNameA(NULL, path, MAX_PATH);
    char* pos = strrchr(path, '\\');
    if (pos) *pos = 0;
    return path;
}

std::string GetSysTempPath()
{
    char path[MAX_PATH] = { 0 };
    GetTempPathA(MAX_PATH, path);
    return std::string(path);
}
#include <Shlobj.h>
#pragma comment(lib, "shell32.lib")

// 获取系统“视频”文件夹
std::string GetSystemVideosDir()
{
    char path[MAX_PATH] = { 0 };
    SHGetFolderPathA(NULL, CSIDL_MYVIDEO, NULL, 0, path);
    return std::string(path) + "\\";
}

#include <wrl.h>
#include <d3d11.h>
#include <dxgi.h>
#include <memory.h>
using namespace Microsoft::WRL;

#include <wrl.h>
#include <d3d11.h>
#include <dxgi.h>
#include <memory.h>
using namespace Microsoft::WRL;


class ScreenCaptureDDA
{
public:
    ID3D11Device* pDevice = nullptr;
    ID3D11DeviceContext* pContext = nullptr;
    IDXGIOutputDuplication* pDupl = nullptr;
    D3D11_TEXTURE2D_DESC desc{};

    // 复用纹理（不重复创建，根治显存泄漏卡死）
    ComPtr<ID3D11Texture2D> m_stagingTex;

    bool Init()
    {
        HRESULT hr;

        hr = D3D11CreateDevice(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
            0,
            nullptr, 0, D3D11_SDK_VERSION,
            &pDevice, nullptr, &pContext
        );
        if (FAILED(hr)) return false;

        ComPtr<IDXGIDevice> pDxgiDev;
        pDevice->QueryInterface(IID_PPV_ARGS(&pDxgiDev));

        ComPtr<IDXGIAdapter> pAdapter;
        pDxgiDev->GetParent(IID_PPV_ARGS(&pAdapter));

        ComPtr<IDXGIOutput> pOutput;
        pAdapter->EnumOutputs(0, &pOutput);

        ComPtr<IDXGIOutput1> pOutput1;
        pOutput->QueryInterface(IID_PPV_ARGS(&pOutput1));
        hr = pOutput1->DuplicateOutput(pDevice, &pDupl);

        return SUCCEEDED(hr);
    }

    bool Capture(BYTE* pBuffer, int targetWidth, int targetHeight)
    {
        if (!pBuffer) return false;

        DXGI_OUTDUPL_FRAME_INFO info{};
        ComPtr<IDXGIResource> pRes;

        // 超时改成 20ms，游戏更稳
        HRESULT hr = pDupl->AcquireNextFrame(20, &info, &pRes);

        // ==========================================
        // 【修复1】拿不到帧 → 直接返回，不写黑！不黑屏！
        // ==========================================
        if (FAILED(hr))
        {
            return false; // 不 memset！不写黑！
        }

        ComPtr<ID3D11Texture2D> pTex;
        pRes.As(&pTex);
        pTex->GetDesc(&desc);

        // ==========================================
        // 【修复2】只创建一次复用，永不显存泄漏
        // ==========================================
        if (!m_stagingTex)
        {
            D3D11_TEXTURE2D_DESC stagingDesc = desc;
            stagingDesc.Usage = D3D11_USAGE_STAGING;
            stagingDesc.BindFlags = 0;
            stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            stagingDesc.MiscFlags = 0;

            pDevice->CreateTexture2D(&stagingDesc, nullptr, &m_stagingTex);
        }

        pContext->CopyResource(m_stagingTex.Get(), pTex.Get());

        D3D11_MAPPED_SUBRESOURCE map{};
        hr = pContext->Map(m_stagingTex.Get(), 0, D3D11_MAP_READ, 0, &map);
        if (FAILED(hr))
        {
            pDupl->ReleaseFrame();
            return false;
        }

        BYTE* pSrc = (BYTE*)map.pData;
        BYTE* pDst = pBuffer;
        int copyW = min((int)desc.Width, targetWidth);
        int copyH = min((int)desc.Height, targetHeight);

        for (int y = 0; y < copyH; y++)
        {
            memcpy(pDst, pSrc, copyW * 4);
            pSrc += map.RowPitch;
            pDst += targetWidth * 4;
        }

        pContext->Unmap(m_stagingTex.Get(), 0);
        pDupl->ReleaseFrame();
        return true;
    }

    ~ScreenCaptureDDA()
    {
        m_stagingTex.Reset();
        if (pDupl) pDupl->Release();
        if (pContext) pContext->Release();
        if (pDevice) pDevice->Release();
    }
};
extern atomic<int>    g_audioSampleRate;
extern  atomic<UINT64> g_audioTotalFrames;

DWORD WINAPI RecordThread(LPVOID lpParam)
{
    const char* outputPath = (const char*)lpParam;
    g_isRecording = true;

    // 重置
    g_audioTotalFrames = 0;
    s_videoConsumedAudioFrames = 0;

    ScreenCaptureDDA cap;
    cap.Init();

    int outW, outH;
    const char* bitrate;

    // 统一 60 帧
    if (g_IsVIP)
    {
        outW = 1920;
        outH = 1080;
        bitrate = "12M";
    }
    else
    {
        outW = 1280;
        outH = 720;
        bitrate = "8M";
    }

    int W = CAPTURE_WIDTH;
    int H = CAPTURE_HEIGHT;
    int fps = 60; // 全局强制60

    BYTE* pFrame = new BYTE[W * H * 4];
    memset(pFrame, 0, W * H * 4);
    double frameInterval = 1.0 / 60.0;

    string exeDir = GetExeDir();
    char cmd[4096];
    sprintf_s(cmd,
        "\"%s\\ffmpeg.exe\" -y -f rawvideo -pix_fmt bgra -video_size 1920x1080 -r 60 -i pipe:0 "
        "-vf scale=%d:%d -c:v libx264 -preset ultrafast -tune zerolatency -pix_fmt yuv420p -b:v %s -vsync 0 \"%s\"",
        exeDir.c_str(), outW, outH, bitrate, outputPath);

    HANDLE hRead, hWrite;
    SECURITY_ATTRIBUTES sa{ sizeof(sa) };
    sa.bInheritHandle = TRUE;
    CreatePipe(&hRead, &hWrite, &sa, 0);

    STARTUPINFOA si{ sizeof(si) };
    PROCESS_INFORMATION pi{};
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = hRead;

    CreateProcessA(NULL, cmd, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, exeDir.c_str(), &si, &pi);
    g_ffmpegPI = pi;
    CloseHandle(hRead);

    LARGE_INTEGER freq, startTick;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&startTick);
    int frameCount = 0;

    // 最简音频同步（无漂移、不卡死）
    double samplesPerFrame = (double)g_audioSampleRate / 60.0;
    double audioAccumulator = 0.0;

    while (g_isRecording)
    {
        if (g_bPaused)
        {
            Sleep(10);
            continue;
        }

        // 最简音频等待
        audioAccumulator += samplesPerFrame;
        UINT64 expectAudio = (UINT64)audioAccumulator;

        DWORD startWait = GetTickCount64();
        while (g_audioTotalFrames < expectAudio && g_isRecording)
        {
            if (GetTickCount64() - startWait > 30) // 最多等30ms，绝不卡死
                break;
            Sleep(1);
        }

        // 抓帧写入
        if (cap.Capture(pFrame, W, H))
        {
            DWORD written;
            WriteFile(hWrite, pFrame, W * H * 4, &written, NULL);
        }

        // 60帧控制
        frameCount++;
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        double elapsed = (double)(now.QuadPart - startTick.QuadPart) / freq.QuadPart;
        double target = frameCount * frameInterval;
        double wait = target - elapsed;
        if (wait > 0) Sleep((DWORD)wait);
    }

    FlushFileBuffers(hWrite);
    CloseHandle(hWrite);
    WaitForSingleObject(pi.hProcess, 2000);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    delete[] pFrame;

    return 0;
}
void StartScreenRecord(const char* path)
{
  
    DeleteFileA(path);
    g_RecordStartTime = std::chrono::steady_clock::now();
    g_hRecordThread = CreateThread(NULL, 0, RecordThread, (LPVOID)path, 0, NULL);
}


void StopScreenRecord()
{
    g_isRecording = false;
    auto endTime = std::chrono::steady_clock::now();
    std::chrono::duration<double> dur = endTime - g_RecordStartTime;

    // ✅ 修复：atomic 必须用 .store()
    g_TotalDurationSec.store(dur.count());

    if (g_hRecordThread != NULL)
    {
        WaitForSingleObject(g_hRecordThread, INFINITE);
        CloseHandle(g_hRecordThread);
        g_hRecordThread = NULL;
    }

    ZeroMemory(&g_ffmpegPI, sizeof(g_ffmpegPI));
}
#include <Windows.h>
#include <string>
#include <cstdio>
#include <TlHelp32.h>

// 工具：读取媒体文件时长(秒)，靠ffmpeg探针
double GetMediaDuration(const char* filePath)
{
    std::string exePath = GetExeDir(); // 复用你现有获取程序目录函数
    char cmd[2048] = { 0 };

    // ffprobe 读取时长（静默输出）
    sprintf_s(cmd, "\"%s\\ffprobe.exe\" -v error -show_entries stream=duration -of default=noprint_wrappers=1:nokey=1 \"%s\"",
        exePath.c_str(), filePath);

    FILE* pipe = _popen(cmd, "r");
    if (!pipe) return 0.0;

    char buf[64] = { 0 };
    fgets(buf, 63, pipe);
    _pclose(pipe);

    double dur = 0.0;
    sscanf_s(buf, "%lf", &dur);
    return dur;
}


extern std::string g_aviPath;
extern std::string g_micPath;
extern std::string g_outPath;
extern std::string g_output;
extern string g_audioFilePath;



DWORD WINAPI BackMergeThread(LPVOID lpParam)
{
    // ✅ 修复：atomic 写入
    g_IsMerging.store(true);
    g_MergeProgressSec.store(0.0);

    // 1. 裁剪麦克风
    std::string trimmedMic = TrimMicToSysAudio(
       g_audioFilePath.c_str(),
        g_micPath.c_str()
   );

    // 2. 合成视频
    MergeToFinalMP4A(
        g_aviPath.c_str(),
        g_audioFilePath.c_str(),
        trimmedMic.c_str(),
        g_output.c_str(),
        !g_IsVIP
    );

    // ✅ 修复：进度拉满
    g_MergeProgressSec.store(g_TotalDurationSec.load());
    Sleep(300);

    // 清理临时文件
    Sleep(200);
    DeleteFileA(g_aviPath.c_str());
    DeleteFileA(g_audioFilePath.c_str());
     DeleteFileA(g_micPath.c_str());
    DeleteFileA(g_outPath.c_str());
    DeleteFileA(trimmedMic.c_str());

    // ✅ 修复：结束合成
    g_IsMerging.store(false);

    MessageBoxA(hwnd, "录制完成！临时文件已清理", "魔音录屏", MB_OK | MB_TOPMOST);
    return 0;
}