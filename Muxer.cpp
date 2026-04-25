#include <Windows.h>
#include"Muxer.h"
#include<chrono>
#include <atomic>
extern bool g_IsVIP;
// 1. AVI + WAV = 带声音的 AVI
bool MergeToAVI(const char* aviPath, const char* wavPath, const char* outputAviPath)
{
    char cmd[1024] = { 0 };

    wsprintfA(cmd,
        "ffmpeg -i \"%s\" -i \"%s\" -c:v copy -c:a pcm_s16le \"%s\" -y",
        aviPath, wavPath, outputAviPath);

    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;

    BOOL ret = CreateProcessA(
        NULL, cmd, NULL, NULL, FALSE,
        CREATE_NO_WINDOW, NULL, NULL, &si, &pi);

    if (!ret) return false;

    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return true;
}



bool MergeToMP4(const char* aviPath, const char* wavPath, const char* outputMp4Path)
{
    char cmd[1024] = { 0 };

    // 加了 -preset ultrafast 极速编码
    wsprintfA(cmd,
        "ffmpeg -i \"%s\" -i \"%s\" -filter_complex \"[0:a][1:a]amix=inputs=2:duration=longest[aout]\" -map 0:v -map [aout] -c:v libx264 -preset ultrafast -c:a aac \"%s\" -y",
        aviPath, wavPath, outputMp4Path);

    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;

    BOOL ret = CreateProcessA(
        NULL, cmd, NULL, NULL, FALSE,
        CREATE_NO_WINDOW, NULL, NULL, &si, &pi);

    if (!ret) return false;

    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return true;
}
// 获取系统当前时间，生成 20260331_215830 格式
#include <time.h>

std::string GetDateTimeFileName()
{
    time_t now = time(NULL);
    tm t;
    localtime_s(&t, &now);

    char buf[64];
    sprintf_s(buf, "%04d%02d%02d_%02d%02d%02d",
        t.tm_year + 1900,
        t.tm_mon + 1,
        t.tm_mday,
        t.tm_hour,
        t.tm_min,
        t.tm_sec);

    return std::string(buf);
}

bool BurnWatermark(const char* mp4FilePath)
{
    std::string tempFile = std::string(mp4FilePath) + ".tmp.mp4";

    // 【极速版】只改了编码参数，速度暴快！
    std::string cmd = "ffmpeg -y -i \"" + std::string(mp4FilePath) + "\" "
        "-vf \"drawtext=fontfile='C\\:/Windows/Fonts/simhei.ttf':"
        "text='魔音录屏':x=10:y=10:"
        "fontsize=24:fontcolor=white:bordercolor=black:borderw=1,"
        "drawtext=fontfile='C\\:/Windows/Fonts/simhei.ttf':"
        "text='moyinluping.com':x=10:y=40:"
        "fontsize=20:fontcolor=white:bordercolor=black:borderw=1\" "
        "-c:v libx264 -preset ultrafast -crf 28 -c:a copy \"" + tempFile + "\"";

    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    BOOL ok = CreateProcessA(
        NULL, (LPSTR)cmd.c_str(),
        NULL, NULL, FALSE,
        CREATE_NO_WINDOW, NULL, NULL,
        &si, &pi
    );

    if (!ok) return false;

    WaitForSingleObject(pi.hProcess, INFINITE);

    DeleteFileA(mp4FilePath);
    MoveFileA(tempFile.c_str(), mp4FilePath);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return true;
}

// ==============================
// 【二合一终极合成】
// 原始视频 + 系统音 + 麦克风音  → 直接生成最终MP4
// 完全替代 MergeToAVI + MergeToMP4
// ==============================
bool MergeToFinalMP4(
    const char* rawAviPath,     // 你录的原始视频
    const char* sysWavPath,     // 系统音
    const char* micWavPath,     // 麦克风音
    const char* finalMp4Path    // 最终输出
)
{
    char cmd[4096] = { 0 };

    // 一条命令完成：视频 + 双音轨混音 + 编码输出
    wsprintfA(cmd,
        "ffmpeg -y -i \"%s\" -i \"%s\" -i \"%s\" "
        "-filter_complex \"[1:a][2:a]amix=inputs=2:duration=longest[aout]\" "
        "-map 0:v -map [aout] "
        "-c:v libx264 -preset ultrafast -c:a aac \"%s\"",
        rawAviPath, sysWavPath, micWavPath, finalMp4Path);
   

    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;

    BOOL ret = CreateProcessA(
        NULL, cmd, NULL, NULL, FALSE,
        CREATE_NO_WINDOW, NULL, NULL, &si, &pi);

    if (!ret)
        return false;

    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return true;
}

// ==============================
// 最终版：视频 + 双音轨 + 水印（直接烧录）
// 只输出一个 MP4，无任何临时文件！
// ==============================

// ==============================
// 终极合成函数（视频+双音轨+水印开关）
// needWatermark = true  → 生成视频并直接烧水印
// needWatermark = false → 只生成视频，不烧水印
// ==============================
bool MergeToFinalMP4A(
    const char* rawAviPath,     // 原始AVI
    const char* sysWavPath,     // 系统音
    const char* micWavPath,     // 麦克风
    const char* finalMp4Path,   // 最终MP4
    bool needWatermark          // 【开关：是否烧水印】
)
{
    char cmd[4096] = { 0 };

    // ==========================================
    // 不需要水印 → 直接合成视频+双音轨
    // ==========================================
   /*if (!needWatermark)
    {
        wsprintfA(cmd,
            "ffmpeg -y -i \"%s\" -i \"%s\" -i \"%s\" "
            "-filter_complex \"[1:a][2:a]amix=inputs=2:duration=longest[aout]\" "
            "-map 0:v -map [aout] "
            "-c:v libx264 -preset ultrafast -crf 28 -c:a aac \"%s\"",
            rawAviPath, sysWavPath, micWavPath, finalMp4Path);
    }
    // ==========================================
    // 需要水印 → 一次合成 + 直接烧进视频里
    // ==========================================
    else
    {
        wsprintfA(cmd,
            "ffmpeg -y -i \"%s\" -i \"%s\" -i \"%s\" "
            "-filter_complex \"[0:v]drawtext=fontfile='C\\:/Windows/Fonts/simhei.ttf':text='魔音录屏':x=10:y=10:fontsize=24:fontcolor=white:bordercolor=black:borderw=1,drawtext=fontfile='C\\:/Windows/Fonts/simhei.ttf':text='moyinluping.com':x=10:y=40:fontsize=20:fontcolor=white:bordercolor=black:borderw=1[v];[1:a][2:a]amix=inputs=2:duration=longest[aout]\" "
            "-map [v] -map [aout] "
            "-c:v libx264 -preset ultrafast -crf 28 -c:a aac \"%s\"",
            rawAviPath, sysWavPath, micWavPath, finalMp4Path);
    }*/
   /* if (!needWatermark)
    {
        wsprintfA(cmd,
            "ffmpeg -y -i \"%s\" -i \"%s\" -i \"%s\" "
            "-filter_complex \"[2:a]adelay=5000|5000[a_mic];[1:a][a_mic]amix=inputs=2:duration=longest[aout]\" "
            "-map 0:v -map [aout] "
            "-c:v libx264 -preset ultrafast -crf 28 -c:a aac \"%s\"",
            rawAviPath, sysWavPath, micWavPath, finalMp4Path);
    }
    else
    {
        wsprintfA(cmd,
            "ffmpeg -y -i \"%s\" -i \"%s\" -i \"%s\" "
            "-filter_complex \"[0:v]drawtext=fontfile='C\\:/Windows/Fonts/simhei.ttf':text='魔音录屏':x=10:y=10:fontsize=24:fontcolor=white:bordercolor=black:borderw=1,drawtext=fontfile='C\\:/Windows/Fonts/simhei.ttf':text='moyinluping.com':x=10:y=40:fontsize=20:fontcolor=white:bordercolor=black:borderw=1[v];[2:a]adelay=5000|5000[a_mic];[1:a][a_mic]amix=inputs=2:duration=longest[aout]\" "
            "-map [v] -map [aout] "
            "-c:v libx264 -preset ultrafast -crf 28 -c:a aac \"%s\"",
            rawAviPath, sysWavPath, micWavPath, finalMp4Path);
    }*/
    if (!needWatermark)
    {
        wsprintfA(cmd,
            "ffmpeg -y -i \"%s\" -i \"%s\" -i \"%s\" "
            "-filter_complex \"[1:a][2:a]amix=inputs=2:duration=longest[aout]\" "
            "-map 0:v -map [aout] "
            "-c:v libx264 -preset ultrafast -crf 28 -c:a aac \"%s\"",
            rawAviPath, sysWavPath, micWavPath, finalMp4Path);
    }
    else
    {
        wsprintfA(cmd,
            "ffmpeg -y -i \"%s\" -i \"%s\" -i \"%s\" "
            "-filter_complex \"[0:v]drawtext=fontfile='C\\:/Windows/Fonts/simhei.ttf':text='魔音录屏':x=10:y=10:fontsize=24:fontcolor=white:bordercolor=black:borderw=1,drawtext=fontfile='C\\:/Windows/Fonts/simhei.ttf':text='moyinluping.com':x=10:y=40:fontsize=20:fontcolor=white:bordercolor=black:borderw=1[v];[1:a][2:a]amix=inputs=2:duration=longest[aout]\" "
            "-map [v] -map [aout] "
            "-c:v libx264 -preset ultrafast -crf 28 -c:a aac \"%s\"",
            rawAviPath, sysWavPath, micWavPath, finalMp4Path);
    }

    // 执行FFmpeg
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;

    BOOL ret = CreateProcessA(
        NULL, cmd, NULL, NULL, FALSE,
        CREATE_NO_WINDOW, NULL, NULL, &si, &pi);

    if (!ret) return false;

    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return true;
}



extern std::atomic<double> g_TotalDurationSec;// 总时长(秒)
extern std::atomic<bool>  g_IsMerging;       // 是否正在合成
extern std::atomic<double> g_MergeProgressSec; 

double GetDuration(const char* filePath)
{
    char cmd[2048];
    sprintf_s(cmd, sizeof(cmd),
        "ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 \"%s\"",
        filePath);

    char buf[128] = { 0 };

    SECURITY_ATTRIBUTES sa = { sizeof(sa) };
    sa.bInheritHandle = TRUE;
    HANDLE hRead, hWrite;
    CreatePipe(&hRead, &hWrite, &sa, 0);

    STARTUPINFOA si = { sizeof(si) };
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;

    PROCESS_INFORMATION pi;
    BOOL ret = CreateProcessA(
        NULL, cmd, NULL, NULL, TRUE,
        CREATE_NO_WINDOW, NULL, NULL, &si, &pi);

    if (!ret)
    {
        CloseHandle(hRead);
        CloseHandle(hWrite);
        return 0.0;
    }

    CloseHandle(hWrite);
    DWORD bytesRead;
    ReadFile(hRead, buf, 127, &bytesRead, NULL);
    CloseHandle(hRead);

    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return atof(buf);
}
// ==============================
// 2. 你要的核心函数：
// 比较时长 → 麦克风超长就裁剪 → 返回裁剪后的新文件
// ==============================
// ==============================
// 修复版：裁剪麦克风到系统音频长度
// 解决 wsprintfA 浮点报错 0xffffffff
// ==============================
std::string TrimMicToSysAudio(
    const char* sysAudioPath,    // 系统音频（以它为准）
    const char* micAudioPath     // 原麦克风音频
)
{
    // 1. 取时长
    double sysDur = GetDuration(sysAudioPath);
    double micDur = GetDuration(micAudioPath);

    // 如果系统时长无效，直接返回原文件
    if (sysDur <= 0)
        return micAudioPath;

    // 如果麦克风本来就更短，不用剪，直接返回原文件
    if (micDur <= sysDur)
        return micAudioPath;

    // 2. 生成裁剪后的新文件名
    std::string trimmedMic = std::string(micAudioPath) + ".trimmed.wav";

    // ============================
    // 【修复核心】改用 sprintf_s，彻底解决浮点报错问题
    // ============================
    char cmd[4096];
    sprintf_s(cmd, sizeof(cmd),
        "ffmpeg -y -i \"%s\" -t %.3f -c:a pcm_s16le \"%s\"",
        micAudioPath, sysDur, trimmedMic.c_str());

    // 执行裁剪
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    BOOL ret = CreateProcessA(
        NULL, cmd, NULL, NULL, FALSE,
        CREATE_NO_WINDOW, NULL, NULL, &si, &pi);

    if (ret)
    {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    // 返回裁剪好的音频
    return trimmedMic;
}