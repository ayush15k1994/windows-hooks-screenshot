// WindowsGUIwithHooks.cpp : Defines the entry point for the application.
//

#ifndef UNICODE
#define UNICODE
#endif

#define WINDOW_HEIGHT 400
#define WINDOW_WIDTH 500
#define BUTTON_WIDTH 100		
#define BUTTON_HEIGHT 30
#define TEXTBOX_WIDTH 400
#define TEXTBOX_HEIGHT 30

// constants for hooks
#define IDM_MOUSE 0
#define IDM_KEYBOARD 1
#define IDM_MOUSE_LL 2
#define IDM_KEYBOARD_LL 3


#include "framework.h"
#include "WindowsGUIwithHooks.h"
#include <iostream>
#include <strsafe.h>
#include <string>

HWND hwndButton, hwndTextBox, hwndReadOnlyText, hwndHookOutput;
HWND gh_hwndMain;
HHOOK hMouseHook{ NULL }, hKeyboardHook{ NULL };

enum Keys {
	ShiftKeys = 16,
	Capital = 20,
};

int shift_active() {
	return GetKeyState(VK_LSHIFT) < 0 || GetKeyState(VK_RSHIFT) < 0;
}

int capital_active() {
	// why this not works
	return (GetKeyState(VK_CAPITAL & 1) == 1);
}

int num_active() {
	// why this not works
	return (GetKeyState(VK_NUMLOCK & 1) == 1);
}

std::wstring getKeyFromVKCode(UINT);

LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
BOOLEAN AddOtherControls(HWND);
void ShowHookMessage(LPWSTR);
void takeWindowScreenshot();

typedef struct _MYHOOKDATA {
	int nType;
	HOOKPROC hkprc;
	HHOOK hhook;
} MYHOOKDATA;

MYHOOKDATA myhookdata[5];

// Hook Procedures
LRESULT WINAPI KeyboardProc(int, WPARAM, LPARAM);
LRESULT WINAPI MouseProc(int, WPARAM, LPARAM);

int WINAPI wWinMain(
	_In_ HINSTANCE hInstance, 
	_In_opt_ HINSTANCE, 
	_In_ PWSTR pCmdLine, 
	_In_ int nCmdShow
) {
	// Register the window class
	const wchar_t CLASS_NAME[] = L"Sample Window Class";

	WNDCLASS wc = {};
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;

	RegisterClass(&wc);

	// Create the Window - hwnd = Window handle
	HWND hwnd = CreateWindowEx(
		0,
		CLASS_NAME,
		L"Windows Hooks Program",
		// WS_OVERLAPPEDWINDOW,
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT, 500, 400,
		NULL,
		NULL,
		hInstance,
		NULL
	);

	if (!hwnd) {
		MessageBox(
			NULL,
			_T("Could not create the window"),
			_T("Windows Hooks Program"),
			NULL
		);
		return 0;
	}

	if (!AddOtherControls(hwnd)) {
		MessageBox(
			NULL,
			_T("Could not create the window"),
			_T("Windows Hooks Program"),
			NULL
		);
		return 0;
	}

	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);

	// Run the message loop
	MSG msg = {};
	while (GetMessage(&msg, NULL, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);	// this call leads to invocation of WindowProc procedure which handles the event.
	}

	return 0;
}

BOOLEAN AddOtherControls(HWND parentHwnd) {	
	HWND hwndTextBoxLabel = CreateWindow(
		TEXT("STATIC"),
		TEXT("Input text here:"),
		WS_VISIBLE | WS_CHILD,
		WINDOW_WIDTH / 2 - TEXTBOX_WIDTH / 2, WINDOW_HEIGHT / 3 - TEXTBOX_HEIGHT - 10,
		180, 20,
		parentHwnd, 
		NULL,
		(HINSTANCE) GetWindowLongPtr(parentHwnd, GWLP_HINSTANCE),
		NULL
	);

	if (!hwndTextBoxLabel) {
		return false;
	}

	hwndHookOutput = CreateWindow(
		TEXT("STATIC"),
		TEXT("Hook Output"),
		WS_VISIBLE | WS_CHILD,
		300, 10,
		180, 100,
		parentHwnd,
		NULL,
		(HINSTANCE)GetWindowLongPtr(parentHwnd, GWLP_HINSTANCE),
		NULL
	);

	if (!hwndHookOutput) {
		return false;
	}

	hwndButton = CreateWindow(
		L"Button", L"OK",	
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 
		WINDOW_WIDTH / 2 - BUTTON_WIDTH / 2, WINDOW_HEIGHT / 3 * 2, 
		BUTTON_WIDTH, BUTTON_HEIGHT,
		parentHwnd, (HMENU) 1, 
		(HINSTANCE) GetWindowLongPtr(parentHwnd, GWLP_HINSTANCE),
		NULL
	);

	if (!hwndButton) {
		return false;
	}

	hwndTextBox = CreateWindow(
		L"EDIT",
		L"",
		WS_VISIBLE | WS_CHILD | WS_BORDER,
		WINDOW_WIDTH/2 - TEXTBOX_WIDTH/2, WINDOW_HEIGHT/3 - TEXTBOX_HEIGHT/2,
		TEXTBOX_WIDTH, TEXTBOX_HEIGHT,
		parentHwnd,
		(HMENU) 2,
		NULL, NULL
	);

	if (!hwndTextBox) {
		return false;
	}

	hwndReadOnlyText = CreateWindow(
		L"EDIT",
		L"",
		WS_VISIBLE | WS_CHILD | WS_BORDER | ES_READONLY,
		WINDOW_WIDTH / 2 - TEXTBOX_WIDTH / 2, WINDOW_HEIGHT / 2 - TEXTBOX_HEIGHT / 2,
		TEXTBOX_WIDTH, TEXTBOX_HEIGHT,
		parentHwnd,
		(HMENU)3,
		NULL, NULL
	);

	if (!hwndReadOnlyText) {
		return false;
	}

	return true;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	HDC hdc;
	PAINTSTRUCT ps;
	LPWSTR text;
	int len, lenro;

	gh_hwndMain = hwnd;
	
	switch (uMsg) {
	
	case WM_CREATE:
		myhookdata[IDM_MOUSE].nType = WH_MOUSE;
		myhookdata[IDM_MOUSE].hkprc = MouseProc;

		myhookdata[IDM_KEYBOARD].nType = WH_KEYBOARD;
		myhookdata[IDM_KEYBOARD].hkprc = KeyboardProc;

		myhookdata[IDM_MOUSE].hhook = SetWindowsHookEx(
			myhookdata[IDM_MOUSE].nType,
			myhookdata[IDM_MOUSE].hkprc,
			(HINSTANCE) NULL,
			GetCurrentThreadId()
		);

		myhookdata[IDM_KEYBOARD].hhook = SetWindowsHookEx(
			myhookdata[IDM_KEYBOARD].nType,
			myhookdata[IDM_KEYBOARD].hkprc,
			(HINSTANCE) NULL,
			GetCurrentThreadId()
		);

		break;

	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);
		FillRect(hdc, &ps.rcPaint, (HBRUSH) (COLOR_WINDOW+1));
		EndPaint(hwnd, &ps);
		break;

	case WM_DESTROY:
		UnhookWindowsHookEx(myhookdata[IDM_MOUSE].hhook);
		UnhookWindowsHookEx(myhookdata[IDM_KEYBOARD].hhook);

		PostQuitMessage(0);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case 1: 
			OutputDebugString(L"Button Input\n");
			len = GetWindowTextLength(hwndTextBox) + 1;
			lenro = GetWindowTextLength(hwndReadOnlyText) + 1;
			text = new TCHAR[len];
			GetWindowText(hwndTextBox, text, len);
			OutputDebugString(text);

			SendMessage(hwndReadOnlyText, EM_SETSEL, 0, lenro);		// set selection
			SendMessage(hwndReadOnlyText, EM_REPLACESEL, 0, (LPARAM)text);	// replace selection

			SetWindowText(hwndTextBox, L"");

			break;
		default:
			break;
		}
		break;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void ShowHookMessage(LPWSTR msg) {
	SetWindowText(hwndHookOutput, msg);
}

// Windows Hooks
LRESULT WINAPI MouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
	TCHAR szBuf[256];	// sz = zero or null-terminated string	
	TCHAR szMsg[32] = L"";
	TCHAR mouseMsg[32] = L"";
	HDC hdc;
	static int c = 0;
	size_t cch;
	HRESULT hResult;

	if (nCode < 0) {	// do not process the message
		return CallNextHookEx(myhookdata[IDM_MOUSE].hhook, nCode, wParam, lParam);
	}

	switch (wParam) {
	case WM_LBUTTONUP: 
		wcscpy_s(mouseMsg, L"Left Mouse Button Up");
		takeWindowScreenshot();
		c++;
		break;
	case WM_LBUTTONDOWN:
		wcscpy_s(mouseMsg, L"Left Mouse Button Down");
		c++;
		break;
	case WM_RBUTTONUP:
		wcscpy_s(mouseMsg, L"Right Mouse Button Up");
		c++;
		break;
	case WM_RBUTTONDOWN:
		wcscpy_s(mouseMsg, L"Right Mouse Button Down");
		c++;
		break;
	default:
		wcscpy_s(mouseMsg, L"Other Mouse Event");
	}

	// convert the message constant to a string and copy it to a buffer
	LPMOUSEHOOKSTRUCT mouseHookStruct = (LPMOUSEHOOKSTRUCT) lParam;
	hResult = StringCchPrintf(
		szMsg, 32/sizeof(TCHAR),
		L"(%d, %d)",
		mouseHookStruct->pt.x, mouseHookStruct->pt.y
	);
	if (FAILED(hResult)) {
		OutputDebugString(L"Error Writing coordinates in Mouse hook procedure.\n");
	}

	hdc = GetDC(gh_hwndMain);
	hResult = StringCchPrintf(
		szBuf, 256/sizeof(TCHAR),
		L"MOUSE: - nCode: %d,\n%s\n(x, y): %s,\n%d",
		nCode, mouseMsg, szMsg, wParam
	);
	if (FAILED(hResult)) {
		OutputDebugString(L"Error writing message in Mouse hook procedure.\n");
	}

	hResult = StringCchLength(szBuf, 256 / sizeof(TCHAR), &cch);
	if (FAILED(hResult)) {
		OutputDebugString(L"Error getting message length in Mouse hook procedure.\n");
	}

	//TextOut(hdc, 2, 95, szBuf, cch);
	ShowHookMessage(szBuf);
	ReleaseDC(gh_hwndMain, hdc);

	return CallNextHookEx(myhookdata[IDM_MOUSE].hhook, nCode, wParam, lParam);
}

LRESULT WINAPI KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
	TCHAR szBuf[256];
	HDC hdc;
	static int d = 0;
	size_t cch;
	HRESULT hResult;
	std::wstring charCode;

	if (nCode < 0) {
		return CallNextHookEx(myhookdata[IDM_KEYBOARD].hhook, nCode, wParam, lParam);
	}

	hdc = GetDC(gh_hwndMain);
	
	charCode = getKeyFromVKCode(wParam);

	if (charCode.length() > 0) {
		hResult = StringCchPrintf(
			szBuf, 256 / sizeof(TCHAR),
			L"KEYBOARD - nCode: %d,\nvk: %s",
			nCode, charCode.c_str()
		);
	}
	else {
		hResult = StringCchPrintf(
			szBuf, 256 / sizeof(TCHAR),
			L"KEYBOARD - nCode: %d,\nvk: [%d]",
			nCode, wParam
		);
	}
	
	if (FAILED(hResult)) {
		OutputDebugString(L"Error writing message in Keyboard procedure.\n");
	}

	hResult = StringCchLength(szBuf, 256 / sizeof(TCHAR), &cch);
	if (FAILED(hResult)) {
		OutputDebugString(L"Error getting message length in Keyboard procedure.\n");
	}

	ShowHookMessage(szBuf);
	ReleaseDC(gh_hwndMain, hdc);

	return CallNextHookEx(myhookdata[IDM_KEYBOARD].hhook, nCode, wParam, lParam);
}

std::wstring getKeyFromVKCode(UINT vkCode) {
	std::wstring charCode;

	if (vkCode >= 0x41 && vkCode <= 0x5A) {
		if (capital_active() || shift_active())
			charCode = std::wstring(1, 0x41 + (vkCode - 0x41));
		else
			charCode = std::wstring(1, 0x61 + (vkCode - 0x41));
	}
	else if (vkCode>=0x30 && vkCode<=0x39 && !shift_active()) {
		charCode = std::wstring(1, 0x30 + (vkCode - 0x30));
	}
	else if (vkCode >= 0x60 && vkCode <= 0x69) {
		if (num_active() && !shift_active()) 
			charCode = std::wstring(1, 0x30 + (vkCode - 0x60));
		else
			charCode = L"NUMPAD " + std::wstring(1, 0x30 + (vkCode - 0x60));
	}
	
	else {
		charCode = L"";
	}

	return charCode;
}

void takeWindowScreenshot() {
	HWND prevFgWnd = GetForegroundWindow();
	RECT window_rect = { 0 };
	GetWindowRect(gh_hwndMain, &window_rect);

	int width = window_rect.right - window_rect.left;
	int height = window_rect.bottom - window_rect.top;

	SetForegroundWindow(gh_hwndMain);

	HDC hdcScreen = GetDC(HWND_DESKTOP);
	HDC hdc = CreateCompatibleDC(hdcScreen);
	HBITMAP hbmp = CreateCompatibleBitmap(hdcScreen, width, height);
	SelectObject(hdc, hbmp);

	BOOL err = BitBlt(
		hdc, 0, 0,
		width, height, hdcScreen, 
		window_rect.left, window_rect.top, SRCCOPY
	);

	if (err == 0) {
		OutputDebugString(L"Error taking screenshot");
	}

	// save Bitmap
	BITMAPINFO bmp_info = { 0 };
	bmp_info.bmiHeader.biSize = sizeof(bmp_info.bmiHeader);
	bmp_info.bmiHeader.biWidth = width;
	bmp_info.bmiHeader.biHeight = height;
	bmp_info.bmiHeader.biPlanes = 1;
	bmp_info.bmiHeader.biBitCount = 24;
	bmp_info.bmiHeader.biCompression = BI_RGB;

	int bmp_padding = (width * 3) % 4;
	if (bmp_padding != 0) bmp_padding = 4 - bmp_padding;

	BYTE* bmp_pixels = new BYTE[(width*3 + bmp_padding) * height];
	GetDIBits(hdc, hbmp, 0, height, bmp_pixels, &bmp_info, DIB_RGB_COLORS);

	BITMAPFILEHEADER bmfHeader;
	HANDLE bmp_file_handle = CreateFile(
		L"test.bmp", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL
	);

	// Total file size = size of bitmap + size of headers
	DWORD dwSizeOfDIB = (width * 3 + bmp_padding) * height + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	// offset to where actual image bits start
	bmfHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER);

	// size of file
	bmfHeader.bfSize = dwSizeOfDIB;

	bmfHeader.bfType = 0x4D42;

	DWORD dwBytesWritten = 0;
	WriteFile(bmp_file_handle, (LPSTR)&bmfHeader, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL);
	WriteFile(bmp_file_handle, (LPSTR)&bmp_info.bmiHeader, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL);
	WriteFile(bmp_file_handle, (LPSTR)bmp_pixels, (width*3 + bmp_padding) * height, &dwBytesWritten, NULL);

	CloseHandle(bmp_file_handle);

	// cleanup 
	DeleteDC(hdc);
	DeleteObject(hbmp);
	ReleaseDC(HWND_DESKTOP, hdcScreen);
	SetForegroundWindow(prevFgWnd);
}