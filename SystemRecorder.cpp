#include <Windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <stdio.h>
#include <math.h>
#include "SystemRecorder.h"
#include <atomic>
#include <string>
using namespace std;
// 加在这里！！！
atomic<UINT64> g_audioTotalFrames = 0;
// 音频已经录制的总帧数


// 音频格式信息（给视频用）
atomic<int>    g_audioSampleRate = 48000;
atomic<int>    g_audioChannels = 2;// 音频已写入帧数
extern std::atomic<bool> g_bPaused;

// 音频文件路径 必须和你主程序的 g_sysPath 一致！
string g_audioFilePath ;

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "mmdevapi.lib")

atomic<bool> g_isAudioRecording = false;

static void FloatTo16BitPCM(BYTE* pFloatData, BYTE* p16BitData, UINT32 frameCount, int channels)
{
    float* f = (float*)pFloatData;
    short* s = (short*)p16BitData;

    for (UINT32 i = 0; i < frameCount * channels; i++)
    {
        float sample = f[i];
        if (sample > 1.0f)  sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;
        s[i] = (short)(sample * 32767.0f);
    }
}

/*static DWORD WINAPI AudioRecordThread(LPVOID lpParam)
{
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
    SetProcessPriorityBoost(GetCurrentProcess(), TRUE);
    timeBeginPeriod(1);
    IMMDeviceEnumerator* pEnum = NULL;
    IMMDevice* pDevice = NULL;
    IAudioClient* pAudioClient = NULL;
    IAudioCaptureClient* pCaptureClient = NULL;
    WAVEFORMATEX* pFmt = NULL;
    FILE* f = NULL;

    f = fopen(g_audioFilePath.c_str(), "wb");
    if (!f) { CoUninitialize(); return 0; }

    char fileBuf[65536];
    setvbuf(f, fileBuf, _IOFBF, sizeof(fileBuf));

    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnum);
    if (SUCCEEDED(hr)) hr = pEnum->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
    if (SUCCEEDED(hr)) hr = pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&pAudioClient);
    if (SUCCEEDED(hr)) hr = pAudioClient->GetMixFormat(&pFmt);

    if (SUCCEEDED(hr))
    {
        hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, 20000000, 0, pFmt, NULL);
    }

    if (SUCCEEDED(hr)) hr = pAudioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&pCaptureClient);

    g_audioSampleRate = pFmt->nSamplesPerSec;
    g_audioChannels = pFmt->nChannels;
    char header[44] = { 0 };
    memcpy(header, "RIFF", 4);
    memcpy(header + 8, "WAVEfmt ", 8);
    *(DWORD*)(header + 16) = 16;
    *(WORD*)(header + 20) = 1;
    *(WORD*)(header + 22) = pFmt ? pFmt->nChannels : 2;
    *(DWORD*)(header + 24) = pFmt ? pFmt->nSamplesPerSec : 44100;
    *(DWORD*)(header + 28) = (pFmt ? pFmt->nSamplesPerSec : 44100) * (pFmt ? pFmt->nChannels : 2) * 2;
    *(WORD*)(header + 32) = (pFmt ? pFmt->nChannels : 2) * 2;
    *(WORD*)(header + 34) = 16;
    memcpy(header + 36, "data", 4);
    fwrite(header, 1, 44, f);

    BYTE* pConvertBuffer = NULL;
    UINT32 totalFrames = 0;

    if (SUCCEEDED(hr))
    {
        pAudioClient->Start();
        pConvertBuffer = new BYTE[65536];
        Sleep(200); //
        while (g_isAudioRecording)
        {
            if (g_bPaused)
            {
                Sleep(10);
                continue;
            }
            BYTE* pData = NULL;
            UINT32 frames = 0;
            DWORD flags = 0;

            hr = pCaptureClient->GetBuffer(&pData, &frames, &flags, NULL, NULL);

            if (SUCCEEDED(hr) && pData && frames > 0 && pFmt)
            {
                if (flags & AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY || flags & AUDCLNT_BUFFERFLAGS_SILENT)
                {
                    pCaptureClient->ReleaseBuffer(frames);
                    continue;
                }

                FloatTo16BitPCM(pData, pConvertBuffer, frames, pFmt->nChannels);
                fwrite(pConvertBuffer, 2, frames * pFmt->nChannels, f);
                totalFrames += frames;
                g_audioTotalFrames += frames;
            }
            else
            {
                Sleep(1);
            }

            if (SUCCEEDED(hr))
                pCaptureClient->ReleaseBuffer(frames);
        }

        pAudioClient->Stop();
    }

    if (totalFrames > 0 && pFmt)
    {
        DWORD dataSize = totalFrames * pFmt->nChannels * 2;
        fseek(f, 4, SEEK_SET);
        DWORD fileSize = 36 + dataSize;
        fwrite(&fileSize, 4, 1, f);
        fseek(f, 40, SEEK_SET);
        fwrite(&dataSize, 4, 1, f);
    }

    if (pConvertBuffer) delete[] pConvertBuffer;
    if (f) fclose(f);
    if (pFmt) CoTaskMemFree(pFmt);
    if (pCaptureClient) pCaptureClient->Release();
    if (pAudioClient) pAudioClient->Release();
    if (pDevice) pDevice->Release();
    if (pEnum) pEnum->Release();

    CoUninitialize();
    return 0;
}*/
static DWORD WINAPI AudioRecordThread(LPVOID lpParam)
{
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
    SetProcessPriorityBoost(GetCurrentProcess(), TRUE);
    timeBeginPeriod(1); // 开启高精度时钟

    IMMDeviceEnumerator* pEnum = NULL;
    IMMDevice* pDevice = NULL;
    IAudioClient* pAudioClient = NULL;
    IAudioCaptureClient* pCaptureClient = NULL;
    WAVEFORMATEX* pFmt = NULL;
    FILE* f = NULL;

    f = fopen(g_audioFilePath.c_str(), "wb");
    if (!f) {
        timeEndPeriod(1); // 修正点1：失败时释放高精度时钟
        CoUninitialize();
        return 0;
    }

    char fileBuf[65536];
    setvbuf(f, fileBuf, _IOFBF, sizeof(fileBuf));

    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnum);
    if (SUCCEEDED(hr)) hr = pEnum->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
    if (SUCCEEDED(hr)) hr = pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&pAudioClient);
    if (SUCCEEDED(hr)) hr = pAudioClient->GetMixFormat(&pFmt);

    if (SUCCEEDED(hr))
    {
        hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, 20000000, 0, pFmt, NULL);
    }

    if (SUCCEEDED(hr)) hr = pAudioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&pCaptureClient);

    g_audioSampleRate = pFmt->nSamplesPerSec;
    g_audioChannels = pFmt->nChannels;
    char header[44] = { 0 };
    memcpy(header, "RIFF", 4);
    memcpy(header + 8, "WAVEfmt ", 8);
    *(DWORD*)(header + 16) = 16;
    *(WORD*)(header + 20) = 1;
    *(WORD*)(header + 22) = pFmt ? pFmt->nChannels : 2;
    *(DWORD*)(header + 24) = pFmt ? pFmt->nSamplesPerSec : 44100;
    *(DWORD*)(header + 28) = (pFmt ? pFmt->nSamplesPerSec : 44100) * (pFmt ? pFmt->nChannels : 2) * 2;
    *(WORD*)(header + 32) = (pFmt ? pFmt->nChannels : 2) * 2;
    *(WORD*)(header + 34) = 16;
    memcpy(header + 36, "data", 4);
    fwrite(header, 1, 44, f);

    BYTE* pConvertBuffer = NULL;
    UINT32 totalFrames = 0;

    if (SUCCEEDED(hr))
    {
        pAudioClient->Start();
        pConvertBuffer = new BYTE[65536];
        Sleep(200);
        while (g_isAudioRecording)
        {
            if (g_bPaused)
            {
                Sleep(10);
                continue;
            }
            BYTE* pData = NULL;
            UINT32 frames = 0;
            DWORD flags = 0;

            hr = pCaptureClient->GetBuffer(&pData, &frames, &flags, NULL, NULL);

            if (SUCCEEDED(hr) && pData && frames > 0 && pFmt)
            {
                if (flags & AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY || flags & AUDCLNT_BUFFERFLAGS_SILENT)
                {
                    pCaptureClient->ReleaseBuffer(frames);
                    continue;
                }

                FloatTo16BitPCM(pData, pConvertBuffer, frames, pFmt->nChannels);
                fwrite(pConvertBuffer, 2, frames * pFmt->nChannels, f);
                totalFrames += frames;
                g_audioTotalFrames += frames;
            }
            else
            {
                Sleep(1);
            }

            if (SUCCEEDED(hr))
                pCaptureClient->ReleaseBuffer(frames);
        }

        pAudioClient->Stop();
    }

    if (totalFrames > 0 && pFmt)
    {
        DWORD dataSize = totalFrames * pFmt->nChannels * 2;
        fseek(f, 4, SEEK_SET);
        DWORD fileSize = 36 + dataSize;
        fwrite(&fileSize, 4, 1, f);
        fseek(f, 40, SEEK_SET);
        fwrite(&dataSize, 4, 1, f);
    }

    // 资源释放
    if (pConvertBuffer) delete[] pConvertBuffer;
    if (f) fclose(f);
    if (pFmt) CoTaskMemFree(pFmt);
    if (pCaptureClient) pCaptureClient->Release();
    if (pAudioClient) pAudioClient->Release();
    if (pDevice) pDevice->Release();
    if (pEnum) pEnum->Release();

    timeEndPeriod(1); // 修正点2：线程结束时释放高精度时钟（核心！）
    CoUninitialize();
    return 0;
}
void RecordSystemSound_Start()
{
    // 先复位，防止旧状态干扰
    g_isAudioRecording = false;
    Sleep(100);

    // 再启动
    g_isAudioRecording = true;
    CreateThread(NULL, 0, AudioRecordThread, NULL, 0, NULL);
}

void RecordSystemSound_Stop()
{
    // 先停止循环
    g_isAudioRecording = false;

    // 等线程写完文件头 + 关闭文件（必须等！）
    Sleep(200);
}