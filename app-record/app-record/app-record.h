#pragma once

#include "framework.h"

// Windows 控件（如按钮、列表框等）和通用功能的库文件
#pragma comment(lib, "comctl32.lib")

#define WM_TRAYICON (WM_USER + 1)
#define ID_LISTVIEW 100
#define WINDOW_WIDTH 460

HWND g_hwnd;
HWND g_startRecordButton;
HWND g_clearRecordsButton;
HWND g_hListView;
bool g_isRecording = false;
std::unordered_map<std::wstring, std::chrono::steady_clock::duration>
g_appUsage;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
std::wstring GetWindowProcessName(HWND hWnd);

// 创建
HWND CreateMainWindow(HINSTANCE hInstance);
void CreateComponents(HWND hWnd, HINSTANCE hInstance);
void CreateTrayIcon(HWND hWnd, HINSTANCE hInstance);

// 方法
void StartRecording();
void StopRecording();
void UpdateListView();
