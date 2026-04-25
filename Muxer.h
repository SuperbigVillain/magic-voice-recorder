#pragma once
#include <Windows.h>
#include <cstring>
#include <Shlwapi.h>
#include <cstdio>
#include <string>

#pragma comment(lib, "Shlwapi.lib")

#include <string>

#define PATH_MAX 260

struct MuxParam
{
    char videoFile[PATH_MAX];
    char audioFile[PATH_MAX];
    char outputPath[PATH_MAX];
    char outputName[PATH_MAX];
    char outputFile[PATH_MAX];
    bool keepAvi = false;
};

extern MuxParam g_Mux;

static bool IsFileValid(const char* filePath);
bool MergeToAVI(const char* aviPath, const char* wavPath, const char* outputAviPath);
bool MergeToMP4(const char* aviPath, const char* wavPath, const char* outputMp4Path);
std::string GetExeDir();
std::string GetDateTimeFileName();
bool BurnWatermark(const char* mp4FilePath);
bool MergeToFinalMP4(
    const char* rawAviPath,     // 你录的原始视频
    const char* sysWavPath,     // 系统音
    const char* micWavPath,     // 麦克风音
    const char* finalMp4Path    // 最终输出
);

bool MergeToFinalMP4A(
    const char* rawAviPath,     // 原始AVI
    const char* sysWavPath,     // 系统音
    const char* micWavPath,     // 麦克风
    const char* finalMp4Path,   // 最终MP4
    bool needWatermark          // 【开关：是否烧水印】
);

std::string TrimMicToSysAudio(
    const char* sysAudioPath,    // 系统音频（以它为准）
    const char* micAudioPath     // 原麦克风音频
);
