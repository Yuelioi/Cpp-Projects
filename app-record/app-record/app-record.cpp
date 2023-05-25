#include "app-record.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    PWSTR pCmdLine, int nCmdShow) {
  // 创建窗口
  g_hwnd = CreateMainWindow(hInstance);

  if (g_hwnd == nullptr) {
    return 0;
  }

  // 创建组件
  CreateComponents(g_hwnd, hInstance);

  // 注册托盘
  CreateTrayIcon(g_hwnd, hInstance);

  // 设置定时器，每隔1秒触发一次WM_TIMER消息 用于查看当前应用
  SetTimer(g_hwnd, 1, 1000, nullptr);

  ShowWindow(g_hwnd, nCmdShow);
  UpdateWindow(g_hwnd);

  // 消息循环
  MSG nMsg = {};
  while (GetMessage(&nMsg, NULL, 0, 0)) {
    TranslateMessage(&nMsg);  // 翻译消息
    DispatchMessage(&nMsg);  // 派发消息(谁处理消息 就派发给谁 -> 窗口处理函数)
  }

  return 0;
}

HWND CreateMainWindow(HINSTANCE hInstance) {
  const wchar_t CLASS_NAME[] = L"ActivityMonitor";
  WNDCLASS wc = {};

  wc.lpfnWndProc = WndProc;
  wc.hInstance = hInstance;
  wc.lpszClassName = CLASS_NAME;
  wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
  wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPICON));

  RegisterClass(&wc);

  // 初始化在屏幕中心
  int screenWidth = GetSystemMetrics(SM_CXSCREEN);
  int screenHeight = GetSystemMetrics(SM_CYSCREEN);

  int windowX = (screenWidth - WINDOW_WIDTH) / 2;
  int windowY = (screenHeight - 300) / 2;

  HWND hWnd = CreateWindowEx(
      0, CLASS_NAME, L"应用时长记录", WS_OVERLAPPEDWINDOW, windowX, windowY,
      WINDOW_WIDTH, 300, nullptr, nullptr, hInstance, nullptr);

  return hWnd;
}

void CreateComponents(HWND hWnd, HINSTANCE hInstance) {
  // 创建按钮
  g_startRecordButton =
      CreateWindow(L"BUTTON", L"开始记录",
                   WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 10, 10,
                   100, 30, hWnd, (HMENU)RUN_BUTTON, hInstance, nullptr);

  g_clearRecordsButton =
      CreateWindow(L"BUTTON", L"清除记录",
                   WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 310, 10,
                   100, 30, hWnd, (HMENU)CLEAR_BUTTON, hInstance, nullptr);

  // 创建列表视图
  INITCOMMONCONTROLSEX icex;
  icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
  icex.dwICC = ICC_LISTVIEW_CLASSES;
  InitCommonControlsEx(&icex);

  g_hListView = CreateWindowEx(
      0, WC_LISTVIEW, L"", WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_EDITLABELS,
      10, 50, 460, 200, hWnd, (HMENU)ID_LISTVIEW, hInstance, nullptr);

  // 添加列
  LVCOLUMN lvc;
  lvc.mask = LVCF_TEXT | LVCF_WIDTH;

  lvc.pszText = const_cast<LPWSTR>(L"应用");
  lvc.cx = 300;
  ListView_InsertColumn(g_hListView, 0, &lvc);

  lvc.mask = LVCF_TEXT | LVCF_WIDTH;
  lvc.pszText = const_cast<LPWSTR>(L"持续时间");
  lvc.cx = 150;
  ListView_InsertColumn(g_hListView, 1, &lvc);
}

/**
 * 创建托盘图标
 */
void CreateTrayIcon(HWND hWnd, HINSTANCE hInstance) {
  NOTIFYICONDATA nid = {};
  nid.cbSize = sizeof(NOTIFYICONDATA);
  nid.hWnd = hWnd;
  nid.uID = 1;
  nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
  nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPICON));
  nid.uCallbackMessage = WM_TRAYICON;
  wcscpy_s(nid.szTip, _countof(nid.szTip), L"应用名称");
  Shell_NotifyIcon(NIM_ADD, &nid);
}

/**
 * 移除托盘图标
 */
void RemoveTrayIcon(HWND hWnd) {
  NOTIFYICONDATA nid = {};
  nid.cbSize = sizeof(NOTIFYICONDATA);
  nid.hWnd = hWnd;
  nid.uID = 1;
  Shell_NotifyIcon(NIM_DELETE, &nid);
}

/**
 * 创建托盘图标消息
 */
void RemoveTrayIconMessage(HWND hWnd) {
  // 从消息循环中移除处理系统托盘图标消息的注册
  UINT uTaskbarRestartMsg = RegisterWindowMessageW(L"TaskbarCreated");
  if (uTaskbarRestartMsg != 0) {
    ChangeWindowMessageFilter(uTaskbarRestartMsg, MSGFLT_REMOVE);
    ChangeWindowMessageFilter(WM_TRAYICON, MSGFLT_REMOVE);
  }
}

/**
 * 消息循环处理
 */
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {
    case WM_COMMAND: {
      if (LOWORD(wParam) == RUN_BUTTON) {
        if (!g_isRecording) {
          StartRecording();
          SetWindowText(g_startRecordButton, L"停止记录");
        } else {
          StopRecording();
          SetWindowText(g_startRecordButton, L"开始记录");
        }
      }
      if (LOWORD(wParam) == CLEAR_BUTTON) {
        g_appUsage.clear();
        ListView_DeleteAllItems(g_hListView);
        g_isRecording = false;
        SetWindowText(g_startRecordButton, L"开始记录");
      }
      break;
    }

    case WM_TIMER: {
      if (g_isRecording) {
        // 获取应用名 并追加记录
        std::wstring appName = GetWindowProcessName(GetForegroundWindow());
        if (!appName.empty()) {
          g_appUsage[appName] += std::chrono::seconds(1);
          UpdateListView();
        }
      }
      break;
    }

    case WM_CLOSE:
      // 隐藏窗口 移除系统托盘中的图标 并销毁窗口
      ShowWindow(hWnd, SW_HIDE);
      RemoveTrayIcon(hWnd);
      DestroyWindow(hWnd);
      return 0;

    case WM_DESTROY: {
      // 从消息循环中移除系统托盘图标的消息
      RemoveTrayIconMessage(hWnd);
      PostQuitMessage(0);
      break;
    }

    case WM_NCHITTEST: {
      LRESULT hitTestResult = DefWindowProc(hWnd, msg, wParam, lParam);
      // 如果返回的结果是调整左右边框 (HTLEFT 或 HTRIGHT)，将其替换为上下边框
      // (HTTOP 或 HTBOTTOM)
      if (hitTestResult == HTLEFT || hitTestResult == HTRIGHT)
        hitTestResult = HTTOP;
      return hitTestResult;
    }

    case WM_SYSCOMMAND:
      // 最小化到系统托盘
      if (wParam == SC_MINIMIZE) {
        ShowWindow(hWnd, SW_HIDE);  // 隐藏窗口
        return 0;
      }
      break;

    // 托盘处理
    case WM_TRAYICON:
      switch (LOWORD(lParam)) {
        case WM_LBUTTONDBLCLK:
          // 双击图标时执行的操作
          // 还原窗口
          ShowWindow(hWnd, SW_RESTORE);
          break;
        case WM_RBUTTONDOWN:
          // 右键单击图标时执行的操作
          // 暂时还没写
          break;
      }
      break;
  }

  return DefWindowProc(hWnd, msg, wParam, lParam);
}

std::wstring GetWindowProcessName(HWND hWnd) {
  DWORD processId;
  GetWindowThreadProcessId(hWnd, &processId);

  HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                                FALSE, processId);
  if (hProcess != nullptr) {
    wchar_t exePath[MAX_PATH];
    if (GetModuleFileNameEx(hProcess, nullptr, exePath, MAX_PATH) > 0) {
      std::wstring processName = exePath;
      size_t pos = processName.find_last_of(L"\\");
      if (pos != std::wstring::npos) {
        processName = processName.substr(pos + 1);
      }
      CloseHandle(hProcess);
      return processName;
    }

    CloseHandle(hProcess);
  }

  return L"";
}

void StartRecording() {
  g_isRecording = true;
  // g_appUsage.clear();
  ListView_DeleteAllItems(g_hListView);
}

void StopRecording() { g_isRecording = false; }

void UpdateListView() {
  ListView_DeleteAllItems(g_hListView);

  LVITEM lvItem = {};
  lvItem.mask = LVIF_TEXT;

  for (const auto& pair : g_appUsage) {
    std::wstring appName = pair.first;
    std::chrono::steady_clock::duration duration = pair.second;

    LVITEM lvItem = {};
    lvItem.mask = LVIF_TEXT;

    // 设置应用名称
    lvItem.pszText = const_cast<wchar_t*>(appName.c_str());
    int index = ListView_InsertItem(g_hListView, &lvItem);

    // 计算持续时间的小时、分钟和秒
    std::chrono::seconds durationInSeconds =
        std::chrono::duration_cast<std::chrono::seconds>(duration);
    int hours = durationInSeconds.count() / 3600;
    int minutes = (durationInSeconds.count() % 3600) / 60;
    int seconds = durationInSeconds.count() % 60;

    // 格式化持续时间字符串
    std::wstring durationText = std::to_wstring(hours) + L":" +
                                std::to_wstring(minutes) + L":" +
                                std::to_wstring(seconds);

    // 设置持续时间
    lvItem.iSubItem = 1;
    lvItem.pszText = const_cast<wchar_t*>(durationText.c_str());
    ListView_SetItem(g_hListView, &lvItem);
  }
}
