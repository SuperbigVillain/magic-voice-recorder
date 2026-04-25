#pragma once
#include <Windows.h>
#include <atomic>  // 必须加！std::atomic 定义在这个头文件里
#pragma once
#include <string>
#include "SystemRecorder.h"
#include"Muxer.h"
#pragma once
#include <string>

// 全局录制状态（外部可按需访问）

void OnStopBtnClick();

// ====================== 对外接口（核心调用）======================
// 开始录屏（传入文件名，自动拼到D:\Test\）
void StartScreenRecord(const char* fileName);

// 停止录屏（仅停止，不转码）
void StopScreenRecord();

// 停止录屏并转高清MP4
#pragma once
#include <Windows.h>
#include <atomic>
#include <string>
#include <cstring>

#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "kernel32.lib")

#define PATH_MAX 260

struct RecordPaths
{
    char tempAviPath[PATH_MAX];
    char outputMp4Path[PATH_MAX];
    bool keepAvi = true;
};

extern RecordPaths g_RecordPaths;
extern std::atomic<bool> g_isRecording;

// 纯声明，实现放cpp里，避免重复定义
void StartScreenRecord(const char* aviFileName);
// 新增：带等待的转码函数（按钮用）
void OnStartBtnClick();
struct ThreadParam
{
    std::string aviPath;
    std::string mp4Path;
    HANDLE hEvent; // 事件句柄
};
void CheckVIPAndSetRecordParam();
std::string GetSysTempPath();
std::string GetSystemVideosDir();
std::string AudioCutSyncToVideo(const char* aviVideoPath, const char* micWavPath);
DWORD WINAPI BackMergeThread(LPVOID lpParam);
float GetScreenRefreshRate();