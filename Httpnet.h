#pragma once
#ifndef HTTPNET_H
#define HTTPNET_H

#include <string>

// 引入网络依赖
#include <windows.h>
#include <wininet.h>
#pragma comment(lib, "wininet.lib")

// 普通POST请求 提交表单键值对
std::string HttpNet_Post(const char* url, const char* postData);

// 简易MD5占位（后面你可以换标准MD5）
std::string HttpNet_SimplePwdMD5(const char* pwd);
std::string GetCpuMachineCode();

std::string UrlEncode(const std::string& value);
std::string CreateVipOrder(float money, int uiPayType, const std::string& currentAccount, std::string& outOrderNo);
#endif
bool IsAccountVip(const std::string& account);
void OnBtnCheckVIP_Click(const std::string& currentAccount);
void GetMyInviteLink(const std::string& currentAccount);

void CopyToClipboard(const std::string& text);
void OpenCommissionUrl();
void OpenMoYinHomePage();
void RefreshUserVIP(const std::string& account);
void OpenMoyinPage();
std::string SendWithdrawRequest(const char* account, int amount, const char* alipay);
std::string BindAlipayToServer(const char* account, const char* alipay);
void InitNotice();
bool GetVipTimeForImGui(const std::string& account, int& outStart, int& outEnd);
std::string FormatVipDate(int timestamp);
std::string HttpPost_MySite(const wchar_t* path, const std::string& postData);

