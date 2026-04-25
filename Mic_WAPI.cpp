/*#include "Mic_WAPI.h"
#include <fstream>
#include <process.h>
#include <cstring>
#include <avrt.h>
#include <functiondiscoverykeys.h>
#include <vector>
#include <Windows.h>
#include <mmdeviceapi.h>
#include <audiopolicy.h>
#include <cmath>
#include <atomic>
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "mmdevapi.lib")
#pragma comment(lib, "avrt.lib")
extern std::atomic<bool> g_bPaused;
#define MAX_SIZE (128 * 1024 * 1024)
#define REFTIMES_PER_SEC 10000000
#define REFTIMES_PER_MILLISEC 10000
#define DEFAULT_CHUNK_SIZE 480
// 你记忆里的老版极简变声（只改音调，变速无所谓，效果极强、永不失效）
// 参数：pcm数据、总采样数、音调步进(1.0原声，>1升调，<1降调)
static void SimpleVoiceChange(short* pcm, int sampleCount, float pitch)
{
    // 无效值直接返回
    if (sampleCount <= 0 || fabs(pitch - 1.0f) < 0.01f)
        return;

    // 老版经典逻辑：单循环 + 线性插值，改一点就明显变声
    float pos = 0.0f;
    for (int i = 0; i < sampleCount; i++)
    {
        int idx = (int)pos;
        float frac = pos - idx;

        // 防止越界
        if (idx >= sampleCount - 1)
            idx = sampleCount - 2;

        // 最简单插值（当年就是这行代码）
        pcm[i] = (short)(pcm[idx] * (1.0f - frac) + pcm[idx + 1] * frac);

        // 核心步进：改这个值就变声（当年你只改这一个参数）
        pos += pitch;
    }
}
// ==============================================
// 【最终修复版】真正能用的 变调不变速 立体声通用
// ==============================================
// ==============================
// 纯净原声录音类实现 - 修复Windows音频增强问题
// ==============================

MicRecorder::MicRecorder()
    : m_running(false)
    , m_micEnable(false)
    , m_pcmBuf(nullptr)
    , m_pos(0)
    , m_maxSize(MAX_SIZE)
    , m_wf(nullptr)
    , m_audioClient(nullptr)
    , m_captureClient(nullptr)
    , m_eventHandle(nullptr)
    , m_threadHandle(nullptr)
    , m_hnsBufferDuration(0)
    , m_sampleRate(0)
    , m_channels(0)
    , m_bitsPerSample(0)
    , m_blockAlign(0)
{
    m_pcmBuf = (unsigned char*)VirtualAlloc(0, MAX_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (m_pcmBuf) memset(m_pcmBuf, 0, MAX_SIZE);
}

MicRecorder::~MicRecorder()
{
    Stop();
    if (m_pcmBuf) VirtualFree(m_pcmBuf, 0, MEM_RELEASE);
    if (m_wf) CoTaskMemFree(m_wf);
}

void MicRecorder::EnableMic(bool enable)
{
    m_micEnable = enable;
}

bool MicRecorder::Start()
{
    if (m_running) return false;

    if (m_pcmBuf) memset(m_pcmBuf, 0, m_maxSize);
    m_pos = 0;
    m_running = true;
    m_micEnable = false;

    m_threadHandle = (HANDLE)_beginthreadex(0, 0, Thread, this, 0, 0);
    return m_threadHandle != nullptr;
}

void MicRecorder::Stop()
{
    m_running = false;
    if (m_threadHandle)
    {
        WaitForSingleObject(m_threadHandle, 20);
        CloseHandle(m_threadHandle);
        m_threadHandle = nullptr;
    }
}


void MicRecorder::WriteSilent(int samples)
{
    if (!m_pcmBuf || !m_wf) return;
    int size = samples * m_blockAlign;
    if (m_pos + size >= m_maxSize) return;

    memset(m_pcmBuf + m_pos, 0, size);
    m_pos += size;
}

unsigned __stdcall MicRecorder::Thread(void* arg)
{
    MicRecorder* p = (MicRecorder*)arg;
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    IMMDeviceEnumerator* penum = nullptr;
    IMMDevice* pdev = nullptr;
    IAudioClient* pAudioClient = nullptr;
    IAudioCaptureClient* pCap = nullptr;
    HANDLE hEvent = nullptr;

    CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&penum));
    penum->GetDefaultAudioEndpoint(eCapture, eConsole, &pdev);
    pdev->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&pAudioClient);
    pAudioClient->GetMixFormat(&p->m_wf);

    p->m_sampleRate = p->m_wf->nSamplesPerSec;
    p->m_channels = p->m_wf->nChannels;
    p->m_bitsPerSample = p->m_wf->wBitsPerSample;
    p->m_blockAlign = p->m_wf->nBlockAlign;

    IAudioClient3* pAudioClient3 = nullptr;
    if (SUCCEEDED(pAudioClient->QueryInterface(__uuidof(IAudioClient3), (void**)&pAudioClient3)))
    {
        pAudioClient3->Release();
    }

    hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    REFERENCE_TIME hnsRequestedDuration = 5000000;
    HRESULT hr = pAudioClient->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
        hnsRequestedDuration,
        0,
        p->m_wf,
        nullptr
    );

    if (FAILED(hr))
    {
        hr = pAudioClient->Initialize(
            AUDCLNT_SHAREMODE_SHARED,
            0,
            hnsRequestedDuration,
            0,
            p->m_wf,
            nullptr
        );
    }

    if (FAILED(hr))
    {
        CoUninitialize();
        return 0;
    }

    pAudioClient->SetEventHandle(hEvent);
    pAudioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&pCap);

    p->m_audioClient = pAudioClient;
    p->m_captureClient = pCap;
    p->m_eventHandle = hEvent;

    pAudioClient->Start();
    p->RecordingLoop(pAudioClient, pCap, hEvent);

    pAudioClient->Stop();
    pCap->Release();
    pAudioClient->Release();
    pdev->Release();
    penum->Release();
    CloseHandle(hEvent);

    CoUninitialize();
    return 0;
}

/*void MicRecorder::RecordingLoop(IAudioClient* pAudioClient, IAudioCaptureClient* pCaptureClient, HANDLE hEvent)
{
    HRESULT hr = S_OK;
    BYTE* pData = nullptr;
    UINT32 numFramesAvailable = 0;
    DWORD dwFlags = 0;

    while (m_running && m_pos < m_maxSize - 8192)
    {
        DWORD waitResult = WaitForSingleObject(hEvent, 100);
        if (g_bPaused)
        {
            Sleep(10);  // 只空转，不写任何数据，时长不增加
            continue; // 只写静音，不录任何声音
                                   // 直接跳过本轮，不进任何录音逻辑
        }
        if (m_micEnable)
        {
            if (waitResult == WAIT_OBJECT_0)
            {
                hr = pCaptureClient->GetBuffer(&pData, &numFramesAvailable, &dwFlags, nullptr, nullptr);

                if (SUCCEEDED(hr) && pData && numFramesAvailable > 0)
                {
                    ProcessAudioData(pData, numFramesAvailable, numFramesAvailable * m_blockAlign, dwFlags);
                    pCaptureClient->ReleaseBuffer(numFramesAvailable);
                }
                else
                {
                    WriteSilent(DEFAULT_CHUNK_SIZE);
                }
            }
            else
            {
                WriteSilent(DEFAULT_CHUNK_SIZE);
            }
        }
        else
        {
            WriteSilent(DEFAULT_CHUNK_SIZE);
            Sleep(1);
        }
    }
}


void MicRecorder::ProcessAudioData(BYTE* pData, UINT32 numFrames, int dataSize, DWORD flags)
{
    if (flags & AUDCLNT_BUFFERFLAGS_SILENT)
    {
        WriteSilent(numFrames);
        return;
    }

    if (!m_wf)
    {
        WriteSilent(numFrames);
        return;
    }

    if (m_wf->wFormatTag == WAVE_FORMAT_PCM)
    {
        if (m_pos + dataSize < m_maxSize)
        {
            memcpy(m_pcmBuf + m_pos, pData, dataSize);
            m_pos += dataSize;
        }
    }
    else if (m_wf->wFormatTag == WAVE_FORMAT_IEEE_FLOAT || m_wf->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
    {
        float* src = (float*)pData;
        short* dst = (short*)(m_pcmBuf + m_pos);
        int count = numFrames * m_channels;

        int requiredSpace = count * sizeof(short);
        if (m_pos + requiredSpace >= m_maxSize)
        {
            WriteSilent(numFrames);
            return;
        }

        for (int i = 0; i < count; i++)
        {
            float s = src[i];
            if (s > 1.0f)  s = 1.0f;
            if (s < -1.0f) s = -1.0f;
            dst[i] = (short)(s * 32767.0f);
        }
        m_pos += requiredSpace;
    }
    else
    {
        WriteSilent(numFrames);
    }
}
// 只存 pitch，16 个档位，从1.0往上排，纯数值列表

bool MicRecorder::SaveToFile(const std::string& path)
{
   
    if (!m_pcmBuf || m_pos <= 0)
        return false;

    // 强制覆盖旧文件（解决你刚才不覆盖的问题）
    DeleteFileA(path.c_str());
    Sleep(10);
  
    int sr = m_sampleRate > 0 ? m_sampleRate : 44100;
    int ch = m_channels > 0 ? m_channels : 1;
    int bits = 16;
    int align = ch * 2;
    int byteRate = sr * align;
    int dataSize = m_pos;
    int fileSize = 36 + dataSize;

    short* tempPcm = new short[dataSize / 2];
    memcpy(tempPcm, m_pcmBuf, dataSize);
    int sampleCount = dataSize / 2;

    // ====================== 你记忆里只改这一个参数 ======================
  // 1.0原声 | 1.25萝莉 | 0.8大叔
    // ====================================================================

    // 老版调用方式：只传 采样数 + 音调 两个参数
    if (g_voice.enable)
    {
        SimpleVoiceChange(tempPcm, sampleCount, g_voice.pitch);
    }

    std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
    if (!ofs)
    {
        delete[] tempPcm;
        return false;
    }

    ofs.write("RIFF", 4);
    ofs.write((char*)&fileSize, 4);
    ofs.write("WAVE", 4);
    ofs.write("fmt ", 4);

    int fmtSize = 16;
    short fmtTag = 1;
    ofs.write((char*)&fmtSize, 4);
    ofs.write((char*)&fmtTag, 2);
    ofs.write((char*)&ch, 2);
    ofs.write((char*)&sr, 4);
    ofs.write((char*)&byteRate, 4);
    ofs.write((char*)&align, 2);
    ofs.write((char*)&bits, 2);

    ofs.write("data", 4);
    ofs.write((char*)&dataSize, 4);
    ofs.write((char*)tempPcm, dataSize);

    ofs.flush();
    ofs.close();
    delete[] tempPcm;

    return true;
}*/
#include "Mic_WAPI.h"
#include <fstream>
#include <process.h>
#include <cstring>
#include <avrt.h>
#include <functiondiscoverykeys.h>
#include <vector>
#include <Windows.h>
#include <mmdeviceapi.h>
#include <audiopolicy.h>
#include <cmath>
#include <atomic>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "mmdevapi.lib")
#pragma comment(lib, "avrt.lib")
#include <mmsystem.h>
#pragma comment(lib,"winmm.lib")
extern std::atomic<bool> g_bPaused;
extern std::atomic<UINT64> g_audioTotalFrames;
extern std::atomic<int> g_audioSampleRate;

#define MAX_SIZE (256 * 1024 * 1024)
#define DEFAULT_CHUNK_SIZE 480

static void SimpleVoiceChange(short* pcm, int sampleCount, float pitch)
{
    if (sampleCount <= 0 || fabs(pitch - 1.0f) < 0.01f)
        return;

    float pos = 0.0f;
    for (int i = 0; i < sampleCount; i++)
    {
        int idx = (int)pos;
        float frac = pos - idx;

        if (idx >= sampleCount - 1)
            idx = sampleCount - 2;

        pcm[i] = (short)(pcm[idx] * (1.0f - frac) + pcm[idx + 1] * frac);
        pos += pitch;
    }
}

MicRecorder::MicRecorder()
    : m_running(false)
    , m_micEnable(false)
    , m_pcmBuf(nullptr)
    , m_pos(0)
    , m_maxSize(MAX_SIZE)
    , m_wf(nullptr)
    , m_audioClient(nullptr)
    , m_captureClient(nullptr)
    , m_eventHandle(nullptr)
    , m_threadHandle(nullptr)
    , m_hnsBufferDuration(0)
    , m_sampleRate(0)
    , m_channels(0)
    , m_bitsPerSample(0)
    , m_blockAlign(0)
{
    m_pcmBuf = (unsigned char*)VirtualAlloc(0, MAX_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (m_pcmBuf)
        memset(m_pcmBuf, 0, MAX_SIZE);
}

MicRecorder::~MicRecorder()
{
    Stop();
    if (m_pcmBuf) VirtualFree(m_pcmBuf, 0, MEM_RELEASE);
    if (m_wf) CoTaskMemFree(m_wf);
}

void MicRecorder::EnableMic(bool enable)
{
    m_micEnable = enable;
}

bool MicRecorder::Start()
{
    if (m_running) return false;

    if (m_pcmBuf)
        memset(m_pcmBuf, 0, m_maxSize);
    m_pos = 0;
    m_running = true;

    m_threadHandle = (HANDLE)_beginthreadex(0, 0, Thread, this, 0, 0);
    return m_threadHandle != nullptr;
}

void MicRecorder::Stop()
{
    m_running = false;
    if (m_threadHandle)
    {
        WaitForSingleObject(m_threadHandle, 1000);
        CloseHandle(m_threadHandle);
        m_threadHandle = nullptr;
    }
}



void MicRecorder::WriteSilent(int samples)
{
    if (!m_pcmBuf || !m_wf) return;
    int size = samples * m_blockAlign;
    if (m_pos + size >= m_maxSize)
        return;

    memset(m_pcmBuf + m_pos, 0, size);
    m_pos += size;

    // ======================
    // 关键：上报音频总帧数（视频靠这个对齐）
    // ======================
    g_audioTotalFrames += samples;
}

unsigned __stdcall MicRecorder::Thread(void* arg)
{
    MicRecorder* p = (MicRecorder*)arg;
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    timeBeginPeriod(1);
    IMMDeviceEnumerator* penum = nullptr;
    IMMDevice* pdev = nullptr;
    IAudioClient* pAudioClient = nullptr;
    IAudioCaptureClient* pCap = nullptr;
    HANDLE hEvent = nullptr;

    CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&penum));
    penum->GetDefaultAudioEndpoint(eCapture, eConsole, &pdev);
    pdev->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&pAudioClient);
    pAudioClient->GetMixFormat(&p->m_wf);

    p->m_sampleRate = p->m_wf->nSamplesPerSec;
    p->m_channels = p->m_wf->nChannels;
    p->m_bitsPerSample = p->m_wf->wBitsPerSample;
    p->m_blockAlign = p->m_wf->nBlockAlign;

    hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    REFERENCE_TIME hnsRequestedDuration = 5000000;
    HRESULT hr = pAudioClient->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
        hnsRequestedDuration,
        0,
        p->m_wf,
        nullptr
    );

    if (FAILED(hr))
    {
        hr = pAudioClient->Initialize(
            AUDCLNT_SHAREMODE_SHARED,
            0,
            hnsRequestedDuration,
            0,
            p->m_wf,
            nullptr
        );
    }

    if (FAILED(hr))
    {
        CoUninitialize();
        return 0;
    }

    pAudioClient->SetEventHandle(hEvent);
    pAudioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&pCap);

    p->m_audioClient = pAudioClient;
    p->m_captureClient = pCap;
    p->m_eventHandle = hEvent;

    pAudioClient->Start();
    p->RecordingLoop(pAudioClient, pCap, hEvent);

    pAudioClient->Stop();
    pCap->Release();
    pAudioClient->Release();
    pdev->Release();
    penum->Release();
    CloseHandle(hEvent);
    timeEndPeriod(1);
    CoUninitialize();
    return 0;
}

void MicRecorder::RecordingLoop(IAudioClient* pAudioClient, IAudioCaptureClient* pCaptureClient, HANDLE hEvent)
{
    HRESULT hr = S_OK;
    BYTE* pData = nullptr;
    UINT32 numFramesAvailable = 0;
    DWORD dwFlags = 0;

    const int STD_SILENT = 44;

    while (m_running && m_pos < m_maxSize - 8192)
    {
        Sleep(1);
        int captured = 0;

        // 先读麦克风
        if (!g_bPaused && m_micEnable)
        {
            DWORD ret = WaitForSingleObject(hEvent, 15);
            if (ret == WAIT_OBJECT_0)
            {
                hr = pCaptureClient->GetBuffer(&pData, &numFramesAvailable, &dwFlags, nullptr, nullptr);
                if (SUCCEEDED(hr) && pData && numFramesAvailable > 0)
                {
                    ProcessAudioData(pData, numFramesAvailable, numFramesAvailable * m_blockAlign, dwFlags);
                    captured = (int)numFramesAvailable;
                    pCaptureClient->ReleaseBuffer(numFramesAvailable);
                }
            }
        }

        // 最后补静音（只补差额，不硬塞）
        int need = STD_SILENT - captured;
        if (need > 0)
        {
            WriteSilent(need);
        }
    }
}



void MicRecorder::ProcessAudioData(BYTE* pData, UINT32 numFrames, int dataSize, DWORD flags)
{
    if (flags & AUDCLNT_BUFFERFLAGS_SILENT)
    {
        WriteSilent(numFrames);
        return;
    }

    if (!m_wf)
    {
        WriteSilent(numFrames);
        return;
    }

    // 降噪参数（温和，不吞人声）
    const float noiseGate = 0.001f; // 比它小才清零
    const float gain = 1.4f;        // 适当放大

    if (m_wf->wFormatTag == WAVE_FORMAT_PCM)
    {
        if (m_pos + dataSize < m_maxSize)
        {
            memcpy(m_pcmBuf + m_pos, pData, dataSize);
            m_pos += dataSize;
        }
    }
    else if (m_wf->wFormatTag == WAVE_FORMAT_IEEE_FLOAT || m_wf->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
    {
        float* src = (float*)pData;
        short* dst = (short*)(m_pcmBuf + m_pos);
        int count = numFrames * m_channels;

        int requiredSpace = count * sizeof(short);
        if (m_pos + requiredSpace >= m_maxSize)
        {
            WriteSilent(numFrames);
            return;
        }

        for (int i = 0; i < count; i++)
        {
            float s = src[i];

            // 温和降噪：只压极小底噪
            if (fabsf(s) < noiseGate)
                s = 0.0f;
            else
                s *= gain;

            // 防溢出
            if (s > 1.0f)  s = 1.0f;
            if (s < -1.0f) s = -1.0f;

            dst[i] = (short)(s * 32767.0f);
        }
        m_pos += requiredSpace;
    }
    else
    {
        WriteSilent(numFrames);
    }
}

bool MicRecorder::SaveToFile(const std::string& path)
{
    if (!m_pcmBuf || m_pos <= 0)
    {
        OutputDebugStringA("m_pcmBuf为空或m_pos为0，无法保存文件！");
        return false;
    }

    DeleteFileA(path.c_str());
    Sleep(10);

    int sr = m_sampleRate > 0 ? m_sampleRate : 44100;
    int ch = m_channels > 0 ? m_channels : 1;
    int bits = 16;
    int align = ch * 2;
    int byteRate = sr * align;
    int dataSize = m_pos;
    int fileSize = 36 + dataSize;

    short* tempPcm = new short[dataSize / 2];
    memcpy(tempPcm, m_pcmBuf, dataSize);
    int sampleCount = dataSize / 2;

    if (g_voice.enable)
    {
        SimpleVoiceChange(tempPcm, sampleCount, g_voice.pitch);
    }

    std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
    if (!ofs)
    {
        OutputDebugStringA("文件打开失败！");
        delete[] tempPcm;
        return false;
    }

    ofs.write("RIFF", 4);
    ofs.write((char*)&fileSize, 4);
    ofs.write("WAVE", 4);
    ofs.write("fmt ", 4);

    int fmtSize = 16;
    short fmtTag = 1;
    ofs.write((char*)&fmtSize, 4);
    ofs.write((char*)&fmtTag, 2);
    ofs.write((char*)&ch, 2);
    ofs.write((char*)&sr, 4);
    ofs.write((char*)&byteRate, 4);
    ofs.write((char*)&align, 2);
    ofs.write((char*)&bits, 2);

    ofs.write("data", 4);
    ofs.write((char*)&dataSize, 4);
    ofs.write((char*)tempPcm, dataSize);

    ofs.flush();
    ofs.close();
    delete[] tempPcm;

    OutputDebugStringA("麦克风文件保存成功！");
    return true;
}