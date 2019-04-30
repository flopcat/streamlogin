#include <winsock2.h>
#include <windows.h>
#include <windowsx.h>
#include <shlwapi.h>
#include <commctrl.h>
#include <winhttp.h>
#include <functional>
#include <array>
#include <map>
#include <deque>
#include <vector>
#include <memory>
#include <thread>
#include <curl/curl.h>


enum HttpRequest { HttpRequestGet, HttpRequestPost };

const WCHAR CLASS_NAME[] = L"MainWindow";
const WCHAR APP_TITLE[] = L"ServerLogin Desktop Client";
std::map<HttpRequest,std::wstring> httpRequestToString {
    { HttpRequestGet, L"GET" },
    { HttpRequestPost, L"POST" }
};

//---------------------------------------------------------

class CriticalSection {
public:
    CriticalSection() {
        InitializeCriticalSection(&cs);
    }
    void lock() {
        EnterCriticalSection(&cs);
    }
    void unlock() {
        LeaveCriticalSection(&cs);
    }
    ~CriticalSection() {
        DeleteCriticalSection(&cs);
    }
private:
    CRITICAL_SECTION cs;
};

//---------------------------------------------------------

class CurlRequest {
public:
    CurlRequest() = default;
    CurlRequest(const std::string &url, HttpRequest requestType = HttpRequestGet)
        : url(url)
        , requestType(requestType) {};
    ~CurlRequest() {
        if (hnd) {
            curl_easy_cleanup(hnd);
            hnd = nullptr;
        }
        if (slist1) {
            curl_slist_free_all(slist1);
            slist1 = nullptr;
        }
    }
    void setupRequest() {
        hnd = curl_easy_init();
        curl_easy_setopt(hnd, CURLOPT_BUFFERSIZE, 102400L);
        curl_easy_setopt(hnd, CURLOPT_URL, url.data());
        curl_easy_setopt(hnd, CURLOPT_NOPROGRESS, 1L);
        //curl_easy_setopt(hnd, CURLOPT_USERAGENT, "curl/7.64.1");
        curl_easy_setopt(hnd, CURLOPT_MAXREDIRS, 50L);
        //curl_easy_setopt(hnd, CURLOPT_HTTP_VERSION, (long)CURL_HTTP_VERSION_2TLS);
        //curl_easy_setopt(hnd, CURLOPT_HTTP09_ALLOWED, 1L);
        curl_easy_setopt(hnd, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(hnd, CURLOPT_SSL_VERIFYHOST, 0L);
        //curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, httpRequestToString[requestType]);
        //curl_easy_setopt(hnd, CURLOPT_TCP_KEEPALIVE, 1L);
        curl_easy_setopt(hnd, CURLOPT_VERBOSE, 1);
    }
    void setupJson() {
        slist1 = nullptr;
        slist1 = curl_slist_append(slist1, "Content-Type: application/json");
        curl_easy_setopt(hnd, CURLOPT_POSTFIELDS, postData.data());
        curl_easy_setopt(hnd, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t)77);
        curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, slist1);
    }
    void setupResponse() {
        curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, curlWriteFunction);
        curl_easy_setopt(hnd, CURLOPT_WRITEDATA, this);
        responseData.clear();
    }
    void appendResponse(char *ptr, size_t bytes) {
        keyhole.lock();
        responseData.insert(postData.end(), ptr, ptr + bytes);
        keyhole.unlock();
    }
    void perform() {
        CURLcode res = curl_easy_perform(hnd);
        if (res != CURLE_OK) {
            MessageBoxA(NULL, curl_easy_strerror(res), "Curl failed", MB_OK);
        }
    }
    std::string writeOutString() {
        return std::string(responseData.begin(), responseData.end());
    }
    static size_t curlWriteFunction(char *ptr, size_t size1, size_t size2, CurlRequest *self) {
        self->appendResponse(ptr, size1*size2);
        return size1*size2;
    }
    std::string url;
    HttpRequest requestType = HttpRequestGet;
    std::vector<char> postData = {};
    std::vector<char> responseData = {};
private:
    CriticalSection keyhole;
    CURL *hnd = nullptr;
    curl_slist *slist1 = nullptr;
};

//---------------------------------------------------------

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void initExeFolder();
void findIniFiles();
void setupInternet();
void closeInternet();

HWND hWndMain = 0;
HINSTANCE hInstance;
SIZE windowSize;
std::wstring exeFolder;
std::string clientIpAddress;

int WINAPI WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR lpCmdLine, int nCmdShow)
{
    InitCommonControls();

    initExeFolder();
    findIniFiles();
    setupInternet();

    hInstance = inst;
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = inst;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
    RegisterClass(&wc);
    
    int dx = GetSystemMetrics(SM_CXSIZEFRAME)*2;
    int dy = GetSystemMetrics(SM_CYSIZEFRAME)*2 + GetSystemMetrics(SM_CYCAPTION);

    windowSize = {230 + dx, 175 + dy};

    hWndMain = CreateWindowEx(
        0,                              // Optional window styles.
        CLASS_NAME,                     // Window class
        APP_TITLE,                      // Window text

        // Window style
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,

        // Size and position
        100, 100, windowSize.cx, windowSize.cy,

        NULL,       // Parent window    
        NULL,       // Menu
        hInstance,  // Instance handle
        NULL        // Additional application data
        );

    if (hWndMain == NULL) {
        return 0;
    }
    ShowWindow(hWndMain, nCmdShow);
    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    closeInternet();
    return 0;
}

void initExeFolder()
{
    // Find exe folder
    WCHAR *szExeFolder;
    DWORD size = 8;
    do {
        szExeFolder = new WCHAR[size];
        DWORD nsize = size;
        nsize = GetModuleFileNameW(0, szExeFolder, nsize);
        if (nsize < size - 2)
            break;
        size *= 2;
        delete[] szExeFolder;
    } while(true);
    PathRemoveFileSpec(szExeFolder);
    exeFolder = std::wstring(szExeFolder);
    delete[] szExeFolder;
}


void OnSize(HWND hwnd, UINT flag, int width, int height);
void sendActivation();
void sendDeactivation();

class SubFuncData {
public:
    SubFuncData() = default;
    std::function<void(int)> clicked = nullptr;
    std::function<void(int)> toggled = nullptr;
};

class SubControl { 
public:
    SubControl() = default;
    const WCHAR *className;
    const WCHAR *windowName;
    DWORD style;
    int x, y, w, h;
    SubFuncData funcData;
};

std::map<int,SubControl> childControls {
  { 100, SubControl {L"BUTTON", L"Activate",
                         WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, 10, 10, 210, 25, {} } },
  { 101, SubControl {L"BUTTON", L"Deactivate",
                         WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, 10, 40, 210, 25, {} } },
  { 102, SubControl {L"BUTTON", L"Activate server at startup",
                         WS_CHILD|WS_VISIBLE|BS_CHECKBOX, 10, 80, 210, 30, {} } },
  { 103, SubControl {L"BUTTON", L"Dectivate server at shutdown",
                         WS_CHILD|WS_VISIBLE|BS_CHECKBOX, 10, 110, 210, 30, {} } },
  { 104, SubControl {L"BUTTON", L"Start this program minimized",
                         WS_CHILD|WS_VISIBLE|BS_CHECKBOX, 10, 140, 210, 30, {} } },
};

std::map<int,HWND> childHandles;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
        {
            for (auto it = childControls.begin(); it != childControls.end(); it++) {
                SubControl &c = it->second;
                HWND child = CreateWindow(c.className, c.windowName, c.style, c.x, c.y, c.w, c.h, hwnd, (HMENU)it->first, hInstance, 0);
                childHandles.emplace(it->first, child);
            }
            Button_SetCheck(childHandles[102], BST_CHECKED);
            Button_SetCheck(childHandles[103], BST_CHECKED);
            Button_SetCheck(childHandles[104], BST_CHECKED);
            
            auto toggleFunc = [](int cx) {
                bool checked = Button_GetCheck(childHandles[cx]) != 0;
                Button_SetCheck(childHandles[cx], !checked);
            };
            childControls[102].funcData.toggled = toggleFunc;
            childControls[103].funcData.toggled = toggleFunc;
            childControls[104].funcData.toggled = toggleFunc;

            childControls[100].funcData.clicked = [](int) { sendActivation(); };
            childControls[101].funcData.clicked = [](int) { sendDeactivation(); };
            return 0;
        }

    case WM_SIZE: 
        {
            int width = LOWORD(lParam);  // Macro to get the low-order word.
            int height = HIWORD(lParam); // Macro to get the high-order word.

            // Respond to the message:
            OnSize(hwnd, (UINT)wParam, width, height);


        }
        break;
        /*
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            // All painting occurs here, between BeginPaint and EndPaint.
            FillRect(hdc, &ps.rcPaint, (HBRUSH) (COLOR_WINDOW+1));

            EndPaint(hwnd, &ps);
            return 0;
       }*/
    case WM_COMMAND:
        {
            int id = LOWORD(wParam);
            if (childControls.count(id)) {
                SubControl &c = childControls[id];
                if (HIWORD(wParam) == BN_CLICKED) {
                    if (!!c.funcData.toggled)
                        c.funcData.toggled(id);
                    if (!!c.funcData.clicked)
                        c.funcData.clicked(id);
                }
            }
            return 0;
        }
    case WM_CLOSE:
        {
            DestroyWindow(hwnd);
            return 0;
        }
    case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 0;
        }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void OnSize(HWND hwnd, UINT flag, int width, int height)
{
    // Handle resizing
}

std::deque<std::wstring> iniFiles;

void findIniFiles()
{
    const auto accountFolder = L"\\accounts\\";
    const auto iniGlob = L"*.ini";
    const auto noIniFiles = L"No account files found.\n"
                            L"This does not make sense, so not continuing.\n"
                            L"Please read the documentation.";
    iniFiles.clear();

    std::wstring iniFolder = exeFolder;
    iniFolder += accountFolder;
    iniFolder += iniGlob;

    WIN32_FIND_DATAW findData;
    HANDLE findHandle;
    findHandle = FindFirstFileW(iniFolder.data(), &findData);
    if (findHandle == INVALID_HANDLE_VALUE) {
        MessageBoxW(hWndMain, noIniFiles, APP_TITLE, MB_ICONERROR|MB_OK);
        exit(1);
        return;
    }
    do {
        std::wstring iniFile = exeFolder;
        iniFile += accountFolder;
        iniFile += findData.cFileName;
        iniFiles.push_back(iniFile);
    } while (FindNextFileW(findHandle, &findData));
    FindClose(findHandle);
}

HINTERNET hSession = 0;

void setupInternet()
{
    curl_global_init(CURL_GLOBAL_ALL);

    CurlRequest xhr("http://ifconfig.co/ip");
    xhr.setupRequest();
    xhr.setupResponse();
    xhr.perform();
    clientIpAddress = xhr.writeOutString();
    MessageBoxA(NULL, clientIpAddress.data(), "", MB_OK);
    /*
    const auto userAgent = //L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/52.0.2743.116 Safari/537.36 Edge/15.15063";
    L"DesktopClient/1.0";

    // Use WinHttpOpen to obtain a session handle.
    hSession = WinHttpOpen( userAgent,  
                            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                            WINHTTP_NO_PROXY_NAME, 
                            WINHTTP_NO_PROXY_BYPASS, 0);
    
    if (hSession == INVALID_HANDLE_VALUE) {
        MessageBoxW(hWndMain, L"No internet connection", APP_TITLE, MB_ICONERROR|MB_OK);
        exit(1);
    }

    std::vector<char> myIpAddress;
    int statusCode;
    makeRequest(L"ifconfig.co", 80, HttpRequestGet, L"/ip", {}, {}, myIpAddress, statusCode);
    clientIpAddress = std::string(myIpAddress.data(), myIpAddress.size());
    */
}

void closeInternet()
{
    curl_global_cleanup();
    /*
    WinHttpCloseHandle(hSession);
    */
}



std::string getMyIpAddress()
{

}

char *makeRequest(const std::wstring &hostName, int hostPort,
                  HttpRequest requestType,
                  const std::wstring &path,
                  const std::vector<std::wstring> &headers,
                  const std::vector<char> &postData,
                  std::vector<char> &returnData,
                  int &statusCode)
{
    
    //FIXME: do this in a different thread!

/*
    
    HANDLE hConnect = 0;
    HANDLE hRequest = 0;
    BOOL bResults = TRUE;
    DWORD dwStatusCode, dwBytesWritten, dwSize, dwDownloaded;

    // Specify an HTTP server.
    if (hSession) {
        hConnect = WinHttpConnect( hSession, hostName.data(), hostPort, 0);
        if (!hConnect) { MessageBoxW(hWndMain, L"Could not connect", L"", MB_OK); }
    }

    // Create an HTTP request handle.
    if (hConnect) {
        hRequest = WinHttpOpenRequest( hConnect, httpRequestToString[requestType].data(),
                                    path.data(), NULL, WINHTTP_NO_REFERER, 
                                    WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
        if (!hRequest) { MessageBoxW(hWndMain, L"Could not open request", L"", MB_OK); }
    }

    if (hRequest)
        for (const auto &h : headers) {
            WinHttpAddRequestHeaders(hRequest, h.data(), -1, WINHTTP_ADDREQ_FLAG_ADD);
            MessageBoxW(hWndMain, h.data(), L"", MB_OK);
        }

    // Send a request.
    if (hRequest) {
        bResults = WinHttpSendRequest(hRequest,
                                    WINHTTP_NO_ADDITIONAL_HEADERS,
                                    0, WINHTTP_NO_REQUEST_DATA, 0, 
                                    0, 0);
        if (!bResults) { MessageBoxW(hWndMain, L"Could not send request", L"", MB_OK); }
    }

    if (bResults && requestType == HttpRequestPost) {
        bResults = WinHttpWriteData( hRequest, postData.data(), 
                                     (DWORD)postData.size(), 
                                     &dwBytesWritten);
        if (!bResults) { MessageBoxW(hWndMain, L"Could not write data", L"", MB_OK); }
    }

    if (bResults) {
        bResults = WinHttpReceiveResponse(hRequest, NULL);   
        if (!bResults) { MessageBoxW(hWndMain, L"Could not receive response", L"", MB_OK); }
    }

  // Use WinHttpQueryHeaders to obtain the header buffer.
    if( bResults ) {
        bResults = WinHttpQueryHeaders( hRequest, 
                    WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                                        NULL, 
                                        &dwStatusCode,
                                        &dwSize,
                                        WINHTTP_NO_HEADER_INDEX );
        if (!bResults) { MessageBoxW(hWndMain, L"Could not query headers", L"", MB_OK); }
    }

    // Keep checking for data until there is nothing left.
    returnData.clear();
    if (bResults)
    {
        do 
        {
            // Check for available data.
            dwSize = 0;
            if (!WinHttpQueryDataAvailable( hRequest, &dwSize)) 
            {
                printf( "Error %u in WinHttpQueryDataAvailable.\n",
                        GetLastError());
                break;
            }
            
            // No more available data.
            if (!dwSize)
                break;

            // Allocate space for the buffer.
            char *pszOutBuffer = new char[dwSize+1];
            if (!pszOutBuffer)
            {
                printf("Out of memory\n");
                break;
            }
            
            // Read the Data.
            ZeroMemory(pszOutBuffer, dwSize+1);

            dwDownloaded = 0;
            if (!WinHttpReadData( hRequest, (LPVOID)pszOutBuffer, 
                                  dwSize, &dwDownloaded))
            {                                  
                printf( "Error %u in WinHttpReadData.\n", GetLastError());
            }
            else
            {
                if (dwSize)
                    returnData.insert(returnData.end(), pszOutBuffer, pszOutBuffer + dwSize);
            }
        
            // Free the memory allocated to the buffer.
            delete[] pszOutBuffer;

            // This condition should never be reached since WinHttpQueryDataAvailable
            // reported that there are bits to read.
            if (!dwDownloaded)
                break;
        } while (dwSize > 0);
    }       

    // Close any open handles.
    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);                           
    */
}

struct PayloadData {
    std::string address;
    std::string port;
    std::string path;
    std::vector<char> data;
};

enum Payload { ActivatePayload, DeactivatePayload };
bool makePayload(const std::wstring fname, PayloadData *pd, Payload payloadType)
{
    std::string hostname;
    std::string clientUrl;
    std::string clientPort;
    std::string clientText;
    // read ini file
    return true;
}

void sendPayload(const PayloadData &p)
{
    // Send HTTP Request
}


void sendFilePayloads(Payload payloadType)
{
    for (auto &f: iniFiles) {
        PayloadData payload;
        if (makePayload(f, &payload, payloadType))
            sendPayload(payload);
    }
}

void sendActivation()
{
    findIniFiles();
    sendFilePayloads(ActivatePayload);
}

void sendDeactivation()
{
    findIniFiles();
    sendFilePayloads(DeactivatePayload);
}
