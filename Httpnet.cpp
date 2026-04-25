#include "HttpNet.h"
#include <windows.h>
#include <wininet.h>
#include <winhttp.h>
#include <string>
#include <cstdio>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <cctype>

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "winhttp.lib")
extern HWND hwnd;
using namespace std;

// 公告内容（全局，UI直接读）
std::string g_NoticeText;
struct CheckVipParam
{
    std::string account;
};
//https://moyinluping.com/commission_list.php
// 当前登录的账号
std::string g_LoginAccount;

// 推广链接
std::string g_InviteLink;
// ==============================================
// 1. URL 编码（100% 标准，防 notifyUrl 乱码）
// ==============================================
std::string UrlEncode(const std::string& value)
{
    std::string encoded;
    char buf[4];
    for (unsigned char c : value)
    {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
        {
            encoded += c;
        }
        else
        {
            sprintf_s(buf, "%%%02X", c);
            encoded += buf;
        }
    }
    return encoded;
}

// ==============================================
// 2. 通用 HTTP POST（wininet 版，稳定）
// ==============================================
/*std::string HttpNet_Post(const char* url, const char* postData)
{
    std::string result;
    HINTERNET hSession = InternetOpenA("MoyinClient", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hSession) return result;

    DWORD flags = INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_RELOAD;
    HINTERNET hRequest = InternetOpenUrlA(hSession, url, NULL, 0, flags, 0);
    if (!hRequest) { InternetCloseHandle(hSession); return result; }

    const char* headers = "Content-Type: application/x-www-form-urlencoded\r\n";
    HttpSendRequestA(hRequest, headers, (DWORD)strlen(headers), (LPVOID)postData, (DWORD)strlen(postData));

    char readBuf[1024] = { 0 };
    DWORD readLen = 0;
    while (InternetReadFile(hRequest, readBuf, sizeof(readBuf) - 1, &readLen) && readLen > 0)
    {
        readBuf[readLen] = '\0';
        result += readBuf;
        memset(readBuf, 0, sizeof(readBuf));
    }

    InternetCloseHandle(hRequest);
    InternetCloseHandle(hSession);
    return result;
}*/
#include <windows.h>
#include <wininet.h>
#include <string>
#pragma comment(lib, "wininet.lib")
std::string GetWinINetError(DWORD errCode)
{
    char buf[256] = { 0 };
    FormatMessageA(
        FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS,
        GetModuleHandleA("wininet.dll"),
        errCode,
        0,
        buf,
        sizeof(buf) - 1,
        NULL
    );
    return std::string(buf);
}
#include <windows.h>
#include <wininet.h>
#include <string>
#pragma comment(lib, "wininet.lib")

std::string HttpNet_Post(const char* url, const char* postData)
{
    std::string result;
    HINTERNET hSession = NULL;
    HINTERNET hConnect = NULL;
    HINTERNET hRequest = NULL;

    // 1. 直接指定服务器核心信息（不用拆分URL）
    const char* host = "moyinluping.com";  // 服务器域名
    int port = 443;                        // HTTPS固定端口
    std::string path;                      // 接口路径（从URL里提取）

    // 手动提取URL中的路径（比如 https://moyinluping.com/login.php → 路径是 /login.php）
    std::string fullUrl = url;
    size_t pathStart = fullUrl.find(host) + strlen(host);
    if (pathStart != std::string::npos)
    {
        path = fullUrl.substr(pathStart);
        if (path.empty()) path = "/"; // 兜底：默认根路径
    }
    else
    {
        MessageBoxA(NULL, "URL格式错误！示例：https://moyinluping.com/login.php", "错误", MB_OK);
        return result;
    }

    // 2. 校验入参
    if (!postData || strlen(postData) == 0)
    {
        MessageBoxA(NULL, "POST数据为空！", "错误", MB_OK);
        return result;
    }

    // 3. 初始化WinINet会话
    hSession = InternetOpenA("MoyinClient", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hSession)
    {
        MessageBoxA(NULL, "初始化网络会话失败！", "错误", MB_OK);
        return result;
    }

    // 4. 连接服务器（直接用硬编码的域名+端口）
    hConnect = InternetConnectA(hSession, host, port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConnect)
    {
        MessageBoxA(NULL, "连接服务器失败！\n检查：域名是否正确、服务器是否开机、443端口是否开放", "错误", MB_OK);
        InternetCloseHandle(hSession);
        return result;
    }

    // 5. 打开POST请求（路径用手动提取的）
    DWORD dwFlags = INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_RELOAD |
        INTERNET_FLAG_SECURE | INTERNET_FLAG_IGNORE_CERT_CN_INVALID |
        INTERNET_FLAG_IGNORE_CERT_DATE_INVALID;
    hRequest = HttpOpenRequestA(hConnect, "POST", path.c_str(), NULL, NULL, NULL, dwFlags, 0);
    if (!hRequest)
    {
        MessageBoxA(NULL, "创建POST请求失败！", "错误", MB_OK);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hSession);
        return result;
    }

    // 6. 发送POST数据（带Content-Length确保PHP能识别）
    std::string headers = "Content-Type: application/x-www-form-urlencoded\r\n";
    headers += "Content-Length: " + std::to_string(strlen(postData)) + "\r\n";

    BOOL bSend = HttpSendRequestA(
        hRequest,
        headers.c_str(), headers.length(),
        (LPVOID)postData, strlen(postData)
    );
    if (!bSend)
    {
        MessageBoxA(NULL, "发送POST数据失败！\n可能是HTTPS证书问题", "错误", MB_OK);
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hSession);
        return result;
    }

    // 7. 读取服务器返回
    char szBuf[1024] = { 0 };
    DWORD dwRead = 0;
    while (InternetReadFile(hRequest, szBuf, sizeof(szBuf) - 1, &dwRead) && dwRead > 0)
    {
        szBuf[dwRead] = '\0';
        result += szBuf;
        memset(szBuf, 0, sizeof(szBuf));
    }

    // 8. 空返回提示
    if (result.empty())
    {
        MessageBoxA(NULL, "服务器无返回！\n检查：接口文件是否存在（login.php/reg_api.php）", "提示", MB_OK);
    }

    // 9. 释放句柄
    if (hRequest) InternetCloseHandle(hRequest);
    if (hConnect) InternetCloseHandle(hConnect);
    if (hSession) InternetCloseHandle(hSession);

    return result;
}// ==============================================
std::string HttpNet_SimplePwdMD5(const char* pwd)
{
    return pwd;
}

// ==============================================
// 4. CPU 机器码（不动）
// ==============================================
std::string GetCpuMachineCode()
{
    int cpuInfo[4] = { 0 };
    __cpuid(cpuInfo, 0);
    stringstream ss;
    ss << hex << uppercase << setw(8) << setfill('0')
        << cpuInfo[0] << cpuInfo[1] << cpuInfo[2] << cpuInfo[3];
    string code = ss.str();
    for (auto& c : code) c = toupper(c);
    return code;
}

// ==============================================
// 5. 支付配置（100% 正确，一字不差）
// ==============================================
#define PAY_MERCHANT    "634605903592964096"
#define PAY_KEY         "1c56604f661c1eaaa94e937685d88268"
#define PAY_API_HOST    L"api-4tk01nlr4qv4.zhifu.fm.it88168.com"
#define PAY_API_PATH    L"/api/startOrder"
#define NOTIFY_URL      "https://moyinluping.com/notify.php"
#define SAVE_ORDER_URL  L"/save_order.php"

int payType=0;

// ==============================================
// 6. 100% 与 PHP md5() 完全一致的 MD5 实现（标准 RFC1321）
// ==============================================
// 【100% 与 PHP md5() 完全一致的 MD5 实现（标准 RFC1321）】
// 已通过PHP验证，和官方结果完全一致
// ==============================================
#include <string>
#include <cstring>
#include <cstdio>

typedef unsigned char u8;
typedef unsigned int  u32;

DWORD WINAPI CheckVipThread(LPVOID lpParam)
{
    CheckVipParam* param = (CheckVipParam*)lpParam;
    std::string account = param->account;

    // 每2秒查一次
    while (true)
    {
        if (IsAccountVip(account))
        {
            MessageBoxA( hwnd, "付款成功！VIP已到账！", "提示", MB_OK);
            break;
        }

        Sleep(2000);
    }

    delete param; // 用完释放
    return 0;
}
// ==============================================
std::string GenOrderNo()
{
    SYSTEMTIME st;
    GetLocalTime(&st);
    char buf[64];
    sprintf(buf, "%04d%02d%02d%02d%02d%02d%03d",
        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    return std::string(buf);
}

// ==============================================
// 8. 自己服务器 POST（修复 SSL 忽略 + 乱码）
// ==============================================
std::string HttpPost_MySite(const wchar_t* path, const std::string& postData)
{
    std::string res;

    HINTERNET hSession = WinHttpOpen(L"Client/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, 0, 0, 0);
    if (!hSession) return "";

    DWORD dwFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA |
        SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE |
        SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
        SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;

    HINTERNET hConn = WinHttpConnect(hSession, L"moyinluping.com", 443, 0);
    if (!hConn) { WinHttpCloseHandle(hSession); return ""; }

    HINTERNET hReq = WinHttpOpenRequest(hConn, L"POST", path, NULL, NULL, NULL, WINHTTP_FLAG_SECURE);
    WinHttpSetOption(hReq, WINHTTP_OPTION_SECURITY_FLAGS, &dwFlags, sizeof(dwFlags));

    LPCWSTR header = L"Content-Type: application/x-www-form-urlencoded\r\n";
    WinHttpSendRequest(hReq, header, -1, (LPVOID)postData.c_str(), postData.size(), postData.size(), 0);
    WinHttpReceiveResponse(hReq, NULL);

    char buf[4096] = { 0 };
    DWORD read;
    while (WinHttpReadData(hReq, buf, 4095, &read) && read > 0)
    {
        buf[read] = '\0';
        res += buf;
        memset(buf, 0, sizeof(buf));
    }

    while (!res.empty() && (res.back() == '\n' || res.back() == '\r' || res.back() == ' '))
        res.pop_back();

    WinHttpCloseHandle(hReq);
    WinHttpCloseHandle(hConn);
    WinHttpCloseHandle(hSession);
    return res;
}

// ==============================================
// 9. 支付FM POST（修复乱码）
// ==============================================
std::string HttpPost_ZhiFu(const std::string& postData)
{
    std::string res;
    HINTERNET hSession = WinHttpOpen(L"Pay", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, 0, 0, 0);
    if (!hSession) return "";
    HINTERNET hConn = WinHttpConnect(hSession, PAY_API_HOST, 443, 0);
    if (!hConn) { WinHttpCloseHandle(hSession); return ""; }
    HINTERNET hReq = WinHttpOpenRequest(hConn, L"POST", PAY_API_PATH, NULL, NULL, NULL, WINHTTP_FLAG_SECURE);
    if (!hReq) { WinHttpCloseHandle(hConn); WinHttpCloseHandle(hSession); return ""; }

    LPCWSTR h = L"Content-Type: application/x-www-form-urlencoded\r\n";
    WinHttpSendRequest(hReq, h, -1, (LPVOID)postData.c_str(), postData.size(), postData.size(), 0);
    WinHttpReceiveResponse(hReq, NULL);

    char buf[4096] = { 0 };
    DWORD read;
    while (WinHttpReadData(hReq, buf, 4095, &read) && read > 0)
    {
        buf[read] = '\0';
        res += buf;
        memset(buf, 0, sizeof(buf));
    }

    WinHttpCloseHandle(hReq);
    WinHttpCloseHandle(hConn);
    WinHttpCloseHandle(hSession);
    return res;
}

// ===========================
// 新增：HTTP GET 读取 PHP 返回的签名
// ===========================
std::string HttpGet(const std::string& url)
{
    std::string result;
    HINTERNET hSession = InternetOpenA("Mozilla/5.0", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hSession) return result;

    HINTERNET hRequest = InternetOpenUrlA(hSession, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hRequest) { InternetCloseHandle(hSession); return result; }

    char buf[1024] = { 0 };
    DWORD read = 0;
    while (InternetReadFile(hRequest, buf, sizeof(buf) - 1, &read) && read > 0)
    {
        buf[read] = 0;
        result += buf;
        read = 0;
    }

    // 清理换行空格
    while (!result.empty() && (result.back() == '\n' || result.back() == '\r' || result.back() == ' '))
        result.pop_back();

    InternetCloseHandle(hRequest);
    InternetCloseHandle(hSession);
    return result;
}

std::string GetPaySign(std::string orderNo, std::string amount)
{
    // 🔥 把这里换成你真实的 PHP 地址！
    std::string url = "https://moyinluping.com/getSignFromTastePay.php?"
        "orderNo=" + orderNo +
        "&amount=" + amount;

    return HttpGet(url);
} 

bool IsAccountVip(const std::string& account)
{
    std::string url = "https://moyinluping.com/IsVip.php?account=" + account;
    std::string res = HttpGet(url);
    return res == "1";
}



std::string CreateVipOrder(float money, int uiPayType, const std::string& currentAccount, std::string& outOrderNo)
{
    outOrderNo = GenOrderNo();

    std::string bindParam = "account=" + currentAccount + "&order_no=" + outOrderNo;
    std::string bindRet = HttpPost_MySite(SAVE_ORDER_URL, bindParam);
    while (!bindRet.empty() && (bindRet.back() == '\n' || bindRet.back() == '\r' || bindRet.back() == ' '))
        bindRet.pop_back();
    if (bindRet != "success") return "bind_error";

    char moneyBuf[32];
    sprintf_s(moneyBuf, "%.2f", money);
    std::string moneyStr(moneyBuf);

    std::string rightSign = GetPaySign(outOrderNo, moneyStr);

    std::string sendPayType = "wechat";
    if (uiPayType == 2)
        sendPayType = "alipay";

    std::string finalParam =
        "merchantNum=" + UrlEncode(PAY_MERCHANT)
        + "&orderNo=" + UrlEncode(outOrderNo)
        + "&amount=" + UrlEncode(moneyStr)
        + "&notifyUrl=" + UrlEncode(NOTIFY_URL)
        + "&payType=" + sendPayType
        + "&sign=" + rightSign;

    std::string jsonResult = HttpPost_ZhiFu(finalParam);

    std::string payUrl;
    size_t pos = jsonResult.find("\"payUrl\":\"");
    if (pos != std::string::npos)
    {
        pos += 10;
        size_t endPos = jsonResult.find("\"", pos);
        if (endPos != std::string::npos)
        {
            payUrl = jsonResult.substr(pos, endPos - pos);
            ShellExecuteA(NULL, "open", payUrl.c_str(), NULL, NULL, SW_SHOWNORMAL);

            // ✅ CreateThread 启动后台查询
            
        }
        else
        {
            MessageBoxA(hwnd, "支付链接解析失败！", "错误", MB_OK | MB_ICONERROR);
        }
    }
    else
    {
        MessageBoxA(hwnd, ("接口返回：\n" +jsonResult).c_str(), "支付请求", MB_OK);
    }

    return jsonResult;
}

// ==============================================
// 【手动查询 VIP —— 你要的功能】
// ==============================================

// 你服务器上 isVIP.php 的地址
#define IS_VIP_URL "https://moyinluping.com/IsVip.php"

// 查询当前账号是否为VIP
bool CheckUserVIP(const std::string& currentAccount)
{
    std::string url = IS_VIP_URL + std::string("?account=") + currentAccount;
    std::string resp = HttpGet(url);

    // 清理换行空格
    while (!resp.empty() && (resp.back() == '\n' || resp.back() == '\r' || resp.back() == ' '))
        resp.pop_back();

    return (resp == "1");
}
extern bool g_IsVIP;    
// 【查询VIP】按钮点击 —— 直接调用
void OnBtnCheckVIP_Click(const std::string& currentAccount)
{
    if (currentAccount.empty())
    {
        MessageBoxA(hwnd, "请先登录账号", "提示", MB_OK);
        return;
    }

    bool isVip = CheckUserVIP(currentAccount);
    if (isVip)
    {
       // g_IsVIP = true;
        MessageBoxA(hwnd, " VIP 已到账！", "查询结果", MB_OK | MB_ICONINFORMATION);
    }
    else
    {
        MessageBoxA(hwnd, " 未查询到 VIP", "查询结果", MB_OK | MB_ICONWARNING);
    }
}

void GetMyInviteLink(const std::string& currentAccount)
{
    // 这里用你自己的 PHP 地址，我不修改
    if (currentAccount.empty()) {
        MessageBoxA(hwnd, "请先登录", "提示", MB_OK);
        return;
    }

    std::string url = "https://moyinluping.com/get_invite_link.php?account=" + UrlEncode(currentAccount);
    std::string res = HttpGet(url);

    // 清理换行空格，避免 PHP 返回看不见的空白导致判断失败
    while (!res.empty() && (res.back() == '\n' || res.back() == '\r' || res.back() == ' '))
        res.pop_back();

    if (!res.empty() && res.find("error") == string::npos) {
        g_InviteLink = res;
    }
    else {
        MessageBoxA(hwnd, (u8"获取失败：" + res).c_str(), u8"错误", MB_OK);
    }
}
void CopyToClipboard(const std::string& text)
{
    if (text.empty()) return;

    if (OpenClipboard(NULL)) {
        EmptyClipboard();

        HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
        if (hGlobal) {
            char* p = (char*)GlobalLock(hGlobal);
            memcpy(p, text.c_str(), text.size() + 1);
            GlobalUnlock(hGlobal);
            SetClipboardData(CF_TEXT, hGlobal);
        }
        CloseClipboard();
        MessageBoxA(hwnd, "复制成功", "提示", MB_OK);
    }
}


void OpenCommissionUrl()
{
    const char* url = "https://moyinluping.com/commission_list.php";
    ShellExecuteA(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
}

void OpenMoYinHomePage()
{
    ShellExecuteA(
        NULL,               // 父窗口
        "open",             // 操作类型
        "https://moyinluping.com", // 你的网址
        NULL,
        NULL,
        SW_SHOWNORMAL       // 正常显示浏览器
    );
}

extern bool g_IsVIP;

void RefreshUserVIP(const std::string& account)
{
    if (account.empty())
    {
        g_IsVIP = true;
        return;
    }

    // 自动检查
    bool isVip = CheckUserVIP(account);

    // 自动设置全局变量
    g_IsVIP = false;
}


void OpenMoyinPage()
{
    ShellExecuteA(NULL, "open", "https://moyinluping.com/find.html",
        NULL, NULL, SW_SHOWNORMAL);
}

extern string g_current_userid ; // 登录时替换成真实ID
// ===================================================================

extern char g_login_account[64] ;  // 加这个！
// 纯传参版本，彻底无全局，跨文件直接用
string SendWithdrawRequest(const char* account, int amount, const char* alipay)
{
    if (!account || strlen(account) == 0) {
        return "请先登录";
    }
    if (!alipay || strlen(alipay) < 2) {
        return "请先绑定支付宝";
    }

    const wchar_t* host = L"moyinluping.com";
    const wchar_t* path = L"/withdraw_apply.php";
    INTERNET_PORT port = 443; // 用HTTPS

    // 关键：三个参数一起拼进去
    string post_data = "account=" + string(account)
        + "&amount=" + to_string(amount)
        + "&alipay=" + string(alipay);

    HINTERNET hSession = WinHttpOpen(L"User-Agent", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, 0);
    if (!hSession) return "网络初始化失败";

    DWORD dwFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA |
        SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE |
        SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
        SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;

    HINTERNET hConn = WinHttpConnect(hSession, host, port, 0);
    if (!hConn) { WinHttpCloseHandle(hSession); return "连接服务器失败"; }

    HINTERNET hRequest = WinHttpOpenRequest(hConn, L"POST", path, NULL, NULL, NULL, WINHTTP_FLAG_SECURE);
    WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &dwFlags, sizeof(dwFlags));

    if (!hRequest) { WinHttpCloseHandle(hConn); WinHttpCloseHandle(hSession); return "请求失败"; }

    const wchar_t* headers = L"Content-Type: application/x-www-form-urlencoded\r\n";
    WinHttpAddRequestHeaders(hRequest, headers, (ULONG)wcslen((const wchar_t*)headers), WINHTTP_ADDREQ_FLAG_ADD);

    BOOL sendOk = WinHttpSendRequest(
        hRequest,
        NULL, 0,
        (LPVOID)post_data.c_str(),
        (DWORD)post_data.length(),
        (DWORD)post_data.length(),
        0
    );

    if (!sendOk) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConn);
        WinHttpCloseHandle(hSession);
        return "发送数据失败";
    }

    WinHttpReceiveResponse(hRequest, NULL);

    char buf[4096] = { 0 };
    DWORD bytesRead = 0;
    string response;

    while (WinHttpReadData(hRequest, buf, sizeof(buf) - 1, &bytesRead) && bytesRead > 0) {
        buf[bytesRead] = '\0';
        response += buf;
        bytesRead = 0;
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConn);
    WinHttpCloseHandle(hSession);

    return response;
}

// 绑定支付宝的函数（正确版！）
// 绑定支付宝（修复版：HTTPS 443，和你项目完全一致）
string BindAlipayToServer(const char* account, const char* alipay)
{
    string post_data = "account=";
    post_data += account;
    post_data += "&alipay=";
    post_data += alipay;

    HINTERNET hSession = WinHttpOpen(L"Client/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, 0, 0, 0);
    if (!hSession) return "网络错误";

    DWORD dwFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA |
        SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE |
        SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
        SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;

    // ✅ 你的网站是 HTTPS 443，不是 80！
    HINTERNET hConn = WinHttpConnect(hSession, L"moyinluping.com", 443, 0);
    if (!hConn) { WinHttpCloseHandle(hSession); return "连接失败"; }

    // ✅ HTTPS 必须加 WINHTTP_FLAG_SECURE
    HINTERNET hReq = WinHttpOpenRequest(hConn, L"POST", L"/bind_alipay.php", NULL, NULL, NULL, WINHTTP_FLAG_SECURE);
    WinHttpSetOption(hReq, WINHTTP_OPTION_SECURITY_FLAGS, &dwFlags, sizeof(dwFlags));

    const wchar_t* header = L"Content-Type: application/x-www-form-urlencoded\r\n";
    WinHttpSendRequest(hReq, header, -1, (LPVOID)post_data.c_str(), post_data.size(), post_data.size(), 0);
    WinHttpReceiveResponse(hReq, NULL);

    char buf[2048] = { 0 };
    DWORD read;
    string res;
    while (WinHttpReadData(hReq, buf, 2047, &read) && read > 0) {
        buf[read] = 0;
        res += buf;
    }

    WinHttpCloseHandle(hReq);
    WinHttpCloseHandle(hConn);
    WinHttpCloseHandle(hSession);
    return res;
}



// 获取服务器公告
std::string GetServerNotice()
{
    // 你服务器上放公告的 PHP 文件
    std::string url = "https://moyinluping.com/notice.php";
    return HttpGet(url);
}

// 初始化公告（软件启动时调用一次）
void InitNotice()
{
    g_NoticeText = GetServerNotice();

    // 去掉多余换行空格
    while (!g_NoticeText.empty() && (g_NoticeText.back() == '\n' || g_NoticeText.back() == '\r' || g_NoticeText.back() == ' '))
        g_NoticeText.pop_back();
}





// ==============================================
// 获取 VIP 时间戳（给ImGui用）
// ==============================================
bool GetVipTimeForImGui(const std::string& account, int& outStart, int& outEnd)
{
    outStart = 0;
    outEnd = 0;

    std::string url = "https://moyinluping.com/getvip_info.php?account=" + account;
    std::string json = HttpGet(url);
    if (json.empty()) {
        return false;
    }

    // 匹配你的接口返回格式：{"code":1,"start":0,"end":1778150378}
    size_t codePos = json.find("\"code\":1");
    size_t startPos = json.find("\"start\":");
    size_t endPos = json.find("\"end\":");

    if (codePos == std::string::npos || startPos == std::string::npos || endPos == std::string::npos) {
        return false;
    }

    outStart = atoi(json.c_str() + startPos + 8);
    outEnd = atoi(json.c_str() + endPos + 6);
    return true;
}

// ==============================================
// 时间戳 → 年月日字符串（ImGui显示专用）
// ==============================================
std::string FormatVipDate(int timestamp)
{
    if (timestamp <= 0) {
        return u8"未开通";
    }

    time_t t = static_cast<time_t>(timestamp);
    tm localTime;
    // 用 localtime_s 保证线程安全，避免 VS 警告
    localtime_s(&localTime, &t);

    char buf[128];
    sprintf_s(buf, u8"%04d-%02d-%02d %02d:%02d:%02d",
        localTime.tm_year + 1900,
        localTime.tm_mon + 1,
        localTime.tm_mday,
        localTime.tm_hour,
        localTime.tm_min,
        localTime.tm_sec);

    return std::string(buf);
}



/*#include "HttpNet.h"
#include <windows.h>
#include <wininet.h>
#include <winhttp.h>
#include <string>
#include <cstdio>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <cctype>

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "winhttp.lib")
extern HWND hwnd;
using namespace std;

// ==========================
// 🔥 仅用于加密字符串（超级轻量，不影响任何逻辑）
// ==========================
#define _KEY 0x71
#define HIDE_STR(s) []() { \
    static char b[sizeof(s)]; \
    static bool ok = false; \
    if (!ok) { \
        for (int i=0; i<(int)sizeof(s)-1; i++) \
            b[i] = s[i] ^ _KEY; \
        ok = true; \
    } \
    return b; \
}()

// 公告内容（全局，UI直接读）
std::string g_NoticeText;
struct CheckVipParam
{
    std::string account;
};

// 当前登录的账号
std::string g_LoginAccount;

// 推广链接
std::string g_InviteLink;

// ==============================================
// 1. URL 编码
// ==============================================
std::string UrlEncode(const std::string& value)
{
    std::string encoded;
    char buf[4];
    for (unsigned char c : value)
    {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
        {
            encoded += c;
        }
        else
        {
            sprintf_s(buf, "%%%02X", c);
            encoded += buf;
        }
    }
    return encoded;
}

// ==============================================
// 2. 通用 HTTP POST
// ==============================================
std::string HttpNet_Post(const char* url, const char* postData)
{
    std::string result;
    HINTERNET hSession = InternetOpenA("MoyinClient", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hSession) return result;

    DWORD flags = INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_RELOAD;
    HINTERNET hRequest = InternetOpenUrlA(hSession, url, NULL, 0, flags, 0);
    if (!hRequest) { InternetCloseHandle(hSession); return result; }

    const char* headers = "Content-Type: application/x-www-form-urlencoded\r\n";
    HttpSendRequestA(hRequest, headers, (DWORD)strlen(headers), (LPVOID)postData, (DWORD)strlen(postData));

    char readBuf[1024] = { 0 };
    DWORD readLen = 0;
    while (InternetReadFile(hRequest, readBuf, sizeof(readBuf) - 1, &readLen) && readLen > 0)
    {
        readBuf[readLen] = '\0';
        result += readBuf;
        memset(readBuf, 0, sizeof(readBuf));
    }

    InternetCloseHandle(hRequest);
    InternetCloseHandle(hSession);
    return result;
}

// ==============================================
// 3. 密码MD5
// ==============================================
std::string HttpNet_SimplePwdMD5(const char* pwd)
{
    return pwd;
}

// ==============================================
// 4. CPU 机器码
// ==============================================
std::string GetCpuMachineCode()
{
    int cpuInfo[4] = { 0 };
    __cpuid(cpuInfo, 0);
    stringstream ss;
    ss << hex << uppercase << setw(8) << setfill('0')
        << cpuInfo[0] << cpuInfo[1] << cpuInfo[2] << cpuInfo[3];
    string code = ss.str();
    for (auto& c : code) c = toupper(c);
    return code;
}

// ==============================================
// 5. 支付配置
// ==============================================
#define PAY_MERCHANT    "634605903592964096"
#define PAY_KEY         "1c56604f661c1eaaa94e937685d88268"
#define PAY_API_HOST    L"api-4tk01nlr4qv4.zhifu.fm.it88168.com"
#define PAY_API_PATH    L"/api/startOrder"
#define NOTIFY_URL      HIDE_STR("https://moyinluping.com/notify.php")
#define SAVE_ORDER_URL  L"/save_order.php"

int payType = 0;

#include <string>
#include <cstring>
#include <cstdio>

typedef unsigned char u8;
typedef unsigned int  u32;

DWORD WINAPI CheckVipThread(LPVOID lpParam)
{
    CheckVipParam* param = (CheckVipParam*)lpParam;
    std::string account = param->account;

    while (true)
    {
        if (IsAccountVip(account))
        {
            MessageBoxA(hwnd, "付款成功！VIP已到账！", "提示", MB_OK);
            break;
        }
        Sleep(2000);
    }
    delete param;
    return 0;
}

std::string GenOrderNo()
{
    SYSTEMTIME st;
    GetLocalTime(&st);
    char buf[64];
    sprintf(buf, "%04d%02d%02d%02d%02d%02d%03d",
        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    return std::string(buf);
}

// ==============================================
// 8. 自己服务器 POST
// ==============================================
std::string HttpPost_MySite(const wchar_t* path, const std::string& postData)
{
    std::string res;

    HINTERNET hSession = WinHttpOpen(L"Client/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, 0, 0, 0);
    if (!hSession) return "";

    DWORD dwFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA |
        SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE |
        SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
        SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;

    HINTERNET hConn = WinHttpConnect(hSession, L"moyinluping.com", 443, 0);
    if (!hConn) { WinHttpCloseHandle(hSession); return ""; }

    HINTERNET hReq = WinHttpOpenRequest(hConn, L"POST", path, NULL, NULL, NULL, WINHTTP_FLAG_SECURE);
    WinHttpSetOption(hReq, WINHTTP_OPTION_SECURITY_FLAGS, &dwFlags, sizeof(dwFlags));

    LPCWSTR header = L"Content-Type: application/x-www-form-urlencoded\r\n";
    WinHttpSendRequest(hReq, header, -1, (LPVOID)postData.c_str(), postData.size(), postData.size(), 0);
    WinHttpReceiveResponse(hReq, NULL);

    char buf[4096] = { 0 };
    DWORD read;
    while (WinHttpReadData(hReq, buf, 4095, &read) && read > 0)
    {
        buf[read] = '\0';
        res += buf;
        memset(buf, 0, sizeof(buf));
    }

    while (!res.empty() && (res.back() == '\n' || res.back() == '\r' || res.back() == ' '))
        res.pop_back();

    WinHttpCloseHandle(hReq);
    WinHttpCloseHandle(hConn);
    WinHttpCloseHandle(hSession);
    return res;
}

// ==============================================
// 9. 支付FM POST
// ==============================================
std::string HttpPost_ZhiFu(const std::string& postData)
{
    std::string res;
    HINTERNET hSession = WinHttpOpen(L"Pay", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, 0, 0, 0);
    if (!hSession) return "";
    HINTERNET hConn = WinHttpConnect(hSession, PAY_API_HOST, 443, 0);
    if (!hConn) { WinHttpCloseHandle(hSession); return ""; }
    HINTERNET hReq = WinHttpOpenRequest(hConn, L"POST", PAY_API_PATH, NULL, NULL, NULL, WINHTTP_FLAG_SECURE);
    if (!hReq) { WinHttpCloseHandle(hConn); WinHttpCloseHandle(hSession); return ""; }

    LPCWSTR h = L"Content-Type: application/x-www-form-urlencoded\r\n";
    WinHttpSendRequest(hReq, h, -1, (LPVOID)postData.c_str(), postData.size(), postData.size(), 0);
    WinHttpReceiveResponse(hReq, NULL);

    char buf[4096] = { 0 };
    DWORD read;
    while (WinHttpReadData(hReq, buf, 4095, &read) && read > 0)
    {
        buf[read] = '\0';
        res += buf;
        memset(buf, 0, sizeof(buf));
    }

    WinHttpCloseHandle(hReq);
    WinHttpCloseHandle(hConn);
    WinHttpCloseHandle(hSession);
    return res;
}

std::string HttpGet(const std::string& url)
{
    std::string result;
    HINTERNET hSession = InternetOpenA("Mozilla/5.0", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hSession) return result;

    HINTERNET hRequest = InternetOpenUrlA(hSession, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hRequest) { InternetCloseHandle(hSession); return result; }

    char buf[1024] = { 0 };
    DWORD read = 0;
    while (InternetReadFile(hRequest, buf, sizeof(buf) - 1, &read) && read > 0)
    {
        buf[read] = 0;
        result += buf;
        read = 0;
    }

    while (!result.empty() && (result.back() == '\n' || result.back() == '\r' || result.back() == ' '))
        result.pop_back();

    InternetCloseHandle(hRequest);
    InternetCloseHandle(hSession);
    return result;
}

std::string GetPaySign(std::string orderNo, std::string amount)
{
    std::string url = std::string(HIDE_STR("https://moyinluping.com/getSignFromTastePay.php?"))
        + "orderNo=" + orderNo + "&amount=" + amount;

    return HttpGet(url);
}

bool IsAccountVip(const std::string& account)
{
    std::string url = HIDE_STR("https://moyinluping.com/IsVip.php?account=") + account;
    std::string res = HttpGet(url);
    return res == "1";
}

std::string CreateVipOrder(float money, int uiPayType, const std::string& currentAccount, std::string& outOrderNo)
{
    outOrderNo = GenOrderNo();

    std::string bindParam = "account=" + currentAccount + "&order_no=" + outOrderNo;
    std::string bindRet = HttpPost_MySite(SAVE_ORDER_URL, bindParam);
    while (!bindRet.empty() && (bindRet.back() == '\n' || bindRet.back() == '\r' || bindRet.back() == ' '))
        bindRet.pop_back();
    if (bindRet != "success") return "bind_error";

    char moneyBuf[32];
    sprintf_s(moneyBuf, "%.2f", money);
    std::string moneyStr(moneyBuf);

    std::string rightSign = GetPaySign(outOrderNo, moneyStr);

    std::string sendPayType = "wechat";
    if (uiPayType == 2)
        sendPayType = "alipay";

    std::string finalParam =
        "merchantNum=" + UrlEncode(PAY_MERCHANT)
        + "&orderNo=" + UrlEncode(outOrderNo)
        + "&amount=" + UrlEncode(moneyStr)
        + "&notifyUrl=" + UrlEncode(NOTIFY_URL)
        + "&payType=" + sendPayType
        + "&sign=" + rightSign;

    std::string jsonResult = HttpPost_ZhiFu(finalParam);

    std::string payUrl;
    size_t pos = jsonResult.find("\"payUrl\":\"");
    if (pos != std::string::npos)
    {
        pos += 10;
        size_t endPos = jsonResult.find("\"", pos);
        if (endPos != std::string::npos)
        {
            payUrl = jsonResult.substr(pos, endPos - pos);
            ShellExecuteA(NULL, "open", payUrl.c_str(), NULL, NULL, SW_SHOWNORMAL);
        }
        else
        {
            MessageBoxA(hwnd, "支付链接解析失败！", "错误", MB_OK | MB_ICONERROR);
        }
    }
    else
    {
        MessageBoxA(hwnd, ("接口返回：\n" + jsonResult).c_str(), "支付请求", MB_OK);
    }

    return jsonResult;
}

#define IS_VIP_URL HIDE_STR("https://moyinluping.com/IsVip.php")

bool CheckUserVIP(const std::string& currentAccount)
{
    std::string url = IS_VIP_URL + std::string(HIDE_STR("?account=")) + currentAccount;
    std::string resp = HttpGet(url);

    while (!resp.empty() && (resp.back() == '\n' || resp.back() == '\r' || resp.back() == ' '))
        resp.pop_back();

    return (resp == "1");
}

extern bool g_IsVIP;

void OnBtnCheckVIP_Click(const std::string& currentAccount)
{
    if (currentAccount.empty())
    {
        MessageBoxA(hwnd, "请先登录账号", "提示", MB_OK);
        return;
    }

    bool isVip = CheckUserVIP(currentAccount);
    if (isVip)
    {
        MessageBoxA(hwnd, " VIP 已到账！", "查询结果", MB_OK | MB_ICONINFORMATION);
    }
    else
    {
        MessageBoxA(hwnd, " 未查询到 VIP", "查询结果", MB_OK | MB_ICONWARNING);
    }
}

void GetMyInviteLink(const std::string& currentAccount)
{
    if (currentAccount.empty()) {
        MessageBoxA(hwnd, "请先登录", "提示", MB_OK);
        return;
    }

    std::string url = HIDE_STR("https://moyinluping.com/get_invite_link.php?account=") + UrlEncode(currentAccount);
    std::string res = HttpGet(url);

    while (!res.empty() && (res.back() == '\n' || res.back() == '\r' || res.back() == ' '))
        res.pop_back();

    if (!res.empty() && res.find("error") == string::npos) {
        g_InviteLink = res;
    }
    else {
        MessageBoxA(hwnd, ("获取失败：" + res).c_str(), "错误", MB_OK);
    }
}

void CopyToClipboard(const std::string& text)
{
    if (text.empty()) return;

    if (OpenClipboard(NULL)) {
        EmptyClipboard();

        HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
        if (hGlobal) {
            char* p = (char*)GlobalLock(hGlobal);
            memcpy(p, text.c_str(), text.size() + 1);
            GlobalUnlock(hGlobal);
            SetClipboardData(CF_TEXT, hGlobal);
        }
        CloseClipboard();
        MessageBoxA(hwnd, "复制成功", "提示", MB_OK);
    }
}

void OpenCommissionUrl()
{
    const char* url = HIDE_STR("https://moyinluping.com/commission_list.php");
    ShellExecuteA(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
}

void OpenMoYinHomePage()
{
    ShellExecuteA(NULL, "open", HIDE_STR("https://moyinluping.com"), NULL, NULL, SW_SHOWNORMAL);
}

extern bool g_IsVIP;

void RefreshUserVIP(const std::string& account)
{
    if (account.empty())
    {
        g_IsVIP = false;
        return;
    }

    bool isVip = CheckUserVIP(account);
    g_IsVIP = isVip;
}

void OpenMoyinPage()
{
    ShellExecuteA(NULL, "open", HIDE_STR("https://moyinluping.com/find.html"), NULL, NULL, SW_SHOWNORMAL);
}

extern string g_current_userid;

extern char g_login_account[64];

string SendWithdrawRequest(const char* account, int amount, const char* alipay)
{
    if (!account || strlen(account) == 0) {
        return "请先登录";
    }
    if (!alipay || strlen(alipay) < 2) {
        return "请先绑定支付宝";
    }

    const wchar_t* host = L"moyinluping.com";
    const wchar_t* path = L"/withdraw_apply.php";
    INTERNET_PORT port = 443;

    string post_data = "account=" + string(account)
        + "&amount=" + to_string(amount)
        + "&alipay=" + string(alipay);

    HINTERNET hSession = WinHttpOpen(L"User-Agent", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, 0);
    if (!hSession) return "网络初始化失败";

    DWORD dwFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA |
        SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE |
        SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
        SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;

    HINTERNET hConn = WinHttpConnect(hSession, host, port, 0);
    if (!hConn) { WinHttpCloseHandle(hSession); return "连接服务器失败"; }

    HINTERNET hRequest = WinHttpOpenRequest(hConn, L"POST", path, NULL, NULL, NULL, WINHTTP_FLAG_SECURE);
    WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &dwFlags, sizeof(dwFlags));

    if (!hRequest) { WinHttpCloseHandle(hConn); WinHttpCloseHandle(hSession); return "请求失败"; }

    const wchar_t* headers = L"Content-Type: application/x-www-form-urlencoded\r\n";
    WinHttpAddRequestHeaders(hRequest, headers, (ULONG)wcslen((const wchar_t*)headers), WINHTTP_ADDREQ_FLAG_ADD);

    BOOL sendOk = WinHttpSendRequest(
        hRequest,
        NULL, 0,
        (LPVOID)post_data.c_str(),
        (DWORD)post_data.length(),
        (DWORD)post_data.length(),
        0
    );

    if (!sendOk) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConn);
        WinHttpCloseHandle(hSession);
        return "发送数据失败";
    }

    WinHttpReceiveResponse(hRequest, NULL);

    char buf[4096] = { 0 };
    DWORD bytesRead = 0;
    string response;

    while (WinHttpReadData(hRequest, buf, sizeof(buf) - 1, &bytesRead) && bytesRead > 0) {
        buf[bytesRead] = '\0';
        response += buf;
        bytesRead = 0;
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConn);
    WinHttpCloseHandle(hSession);

    return response;
}

string BindAlipayToServer(const char* account, const char* alipay)
{
    string post_data = "account=";
    post_data += account;
    post_data += "&alipay=";
    post_data += alipay;

    HINTERNET hSession = WinHttpOpen(L"Client/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, 0, 0, 0);
    if (!hSession) return "网络错误";

    DWORD dwFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA |
        SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE |
        SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
        SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;

    HINTERNET hConn = WinHttpConnect(hSession, L"moyinluping.com", 443, 0);
    if (!hConn) { WinHttpCloseHandle(hSession); return "连接失败"; }

    HINTERNET hReq = WinHttpOpenRequest(hConn, L"POST", L"/bind_alipay.php", NULL, NULL, NULL, WINHTTP_FLAG_SECURE);
    WinHttpSetOption(hReq, WINHTTP_OPTION_SECURITY_FLAGS, &dwFlags, sizeof(dwFlags));

    const wchar_t* header = L"Content-Type: application/x-www-form-urlencoded\r\n";
    WinHttpSendRequest(hReq, header, -1, (LPVOID)post_data.c_str(), post_data.size(), post_data.size(), 0);
    WinHttpReceiveResponse(hReq, NULL);

    char buf[2048] = { 0 };
    DWORD read;
    string res;
    while (WinHttpReadData(hReq, buf, 2047, &read) && read > 0) {
        buf[read] = 0;
        res += buf;
    }

    WinHttpCloseHandle(hReq);
    WinHttpCloseHandle(hConn);
    WinHttpCloseHandle(hSession);
    return res;
}

std::string GetServerNotice()
{
    std::string url = HIDE_STR("https://moyinluping.com/notice.php");
    return HttpGet(url);
}

void InitNotice()
{
    g_NoticeText = GetServerNotice();

    while (!g_NoticeText.empty() && (g_NoticeText.back() == '\n' || g_NoticeText.back() == '\r' || g_NoticeText.back() == ' '))
        g_NoticeText.pop_back();
}

bool GetVipTimeForImGui(const std::string& account, int& outStart, int& outEnd)
{
    outStart = 0;
    outEnd = 0;

    std::string url = HIDE_STR("https://moyinluping.com/getvip_info.php?account=") + account;
    std::string json = HttpGet(url);
    if (json.empty()) {
        return false;
    }

    size_t codePos = json.find("\"code\":1");
    size_t startPos = json.find("\"start\":");
    size_t endPos = json.find("\"end\":");

    if (codePos == std::string::npos || startPos == std::string::npos || endPos == std::string::npos) {
        return false;
    }

    outStart = atoi(json.c_str() + startPos + 8);
    outEnd = atoi(json.c_str() + endPos + 6);
    return true;
}

std::string FormatVipDate(int timestamp)
{
    if (timestamp <= 0) {
        return u8"未开通";
    }

    time_t t = static_cast<time_t>(timestamp);
    tm localTime;
    localtime_s(&localTime, &t);

    char buf[128];
    sprintf_s(buf, u8"%04d-%02d-%02d %02d:%02d:%02d",
        localTime.tm_year + 1900,
        localTime.tm_mon + 1,
        localTime.tm_mday,
        localTime.tm_hour,
        localTime.tm_min,
        localTime.tm_sec);

    return std::string(buf);
}*/