#pragma once

#include <windows.h>
#include <string>
#include <mmreg.h>
#include <atomic>
#include <mmdeviceapi.h>
#include <audioclient.h>

class MicRecorder
{
public:
    MicRecorder();
    ~MicRecorder();

    bool Start();
    void Stop();
    bool SaveToFile(const std::string& path);
    void EnableMic(bool enable);

private:
    static unsigned __stdcall Thread(void* arg);
    void RecordingLoop(IAudioClient* pAudioClient, IAudioCaptureClient* pCaptureClient, HANDLE hEvent);
    void ProcessAudioData(BYTE* pData, UINT32 numFrames, int dataSize, DWORD flags);
    void WriteSilent(int samples);

private:
    bool      m_running;
    bool      m_micEnable;

    unsigned char* m_pcmBuf;
    int       m_pos;
    int       m_maxSize;

    WAVEFORMATEX* m_wf;
    IAudioClient* m_audioClient;
    IAudioCaptureClient* m_captureClient;
    HANDLE    m_eventHandle;
    HANDLE    m_threadHandle;

    REFERENCE_TIME m_hnsBufferDuration;
    int       m_sampleRate;
    int       m_channels;
    int       m_bitsPerSample;
    int       m_blockAlign;

private:
    // 新增：记录音频时间戳
    std::atomic<UINT64> m_totalFramesRecorded;
    std::atomic<UINT64> m_lastWriteTime;
    LARGE_INTEGER m_frequency;
    LARGE_INTEGER m_startTime;
  

    // 新增：计算精确的采样点数
    UINT64 CalculateExpectedFramesSinceLast();
};

struct VoicePitchSet
{
    // 在 main.cpp 顶部，其他 include 之后添加
// ==================== 变声全局控制结构 ====================
   
        bool  enable = false;   // 变声总开关
        int   type = 0;         // 音效类型 (0=原声, 1=萝莉音...)
        float pitch = 1.0f;     // 对应的音调系数
   
};

// 全局实例，按钮直接改它
extern struct VoicePitchSet g_voice;