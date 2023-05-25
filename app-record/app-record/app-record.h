#pragma once

#include "framework.h"

// Windows �ؼ����簴ť���б��ȣ���ͨ�ù��ܵĿ��ļ�
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

// ����
HWND CreateMainWindow(HINSTANCE hInstance);
void CreateComponents(HWND hWnd, HINSTANCE hInstance);
void CreateTrayIcon(HWND hWnd, HINSTANCE hInstance);

// ����
void StartRecording();
void StopRecording();
void UpdateListView();
