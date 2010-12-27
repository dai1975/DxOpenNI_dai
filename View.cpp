#include "View.h"
#include "tc.h"
#include "NISystem.h"

enum {
	ID_BUTTON1 = 0,
	NUM_IDS,
};

namespace {

static LRESULT CALLBACK _WindowProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	View* self;

	if (msg == WM_CREATE) {
		LPCREATESTRUCT p = reinterpret_cast< LPCREATESTRUCT >(lp);
		self = static_cast< View* >(p->lpCreateParams);
		intptr_t v = reinterpret_cast< intptr_t >(self);
		SetWindowLongPtr(hWnd, 0, v);
	} else {
		intptr_t v = GetWindowLongPtr(hWnd, 0);
		self = reinterpret_cast< View* >(v);
	}
	if (self == NULL) {
		return DefWindowProc(hWnd, msg, wp, lp);
	}
	return self->onWindowMessage(hWnd, msg, wp, lp);
}

}

View::View(NISystem* p)
{
	m_model = p;
	m_atom = 0;
	m_hWnd = NULL;
	m_hInstance = NULL;
}

View::~View()
{
}

bool View::onAttach(HINSTANCE hInstance)
{
	if (m_atom != 0) { return false; }

	WNDCLASSEX wc;
	wc.cbSize        = sizeof(WNDCLASSEX);
	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = _WindowProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = sizeof(this);
	wc.hInstance     = hInstance;
	wc.hIcon         = NULL;
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = _T("DxOpenNI");
	wc.hIconSm       = NULL;

	m_atom = RegisterClassEx(&wc);
	if (m_atom == 0) {
		DWORD d = GetLastError();
		return false;
	}

	m_hInstance = hInstance;
	m_hWnd = NULL;
	m_th = NULL;
	return true;
}

bool View::onDetach(HINSTANCE hInstance)
{
	if (m_th != NULL) {
		StopWindowThread();
	}
	if (m_atom != 0) {
		intptr_t atom = m_atom & 0xFFFF;
		void* p = reinterpret_cast< void* >(atom);
		UnregisterClass(static_cast<LPCTSTR>(p), hInstance);
		m_atom = 0;
	}
	return true;
}


bool View::Setup(HWND hParent)
{
	if (m_atom == 0) { return false; }
/*
	if (m_hWnd != NULL) {
		ShowWindow(m_hWnd, SW_SHOW);
		return true;
	}
*/

	return StartWindowThread();
}

void View::Clean()
{
	StopWindowThread();
}

// ---------------------------------
static DWORD WINAPI _Run(LPVOID data)
{
	static_cast< View* >(data)->RunWindowThread();
	return 0;
}

bool View::StartWindowThread()
{
	if (m_th != NULL) { return false; }
	if (m_hWnd != NULL) { return false; }

	m_run = RUN_INIT;
	m_th = CreateThread(NULL, 0, &_Run, this, 0, NULL);
	while (m_run == RUN_INIT) {
		Sleep(100);
	}
	return (m_run != RUN_ERROR);
}
bool View::StopWindowThread()
{
	if (m_th == NULL) { return false; }
	if (m_hWnd == NULL) { return false; }

	m_run = RUN_END;
	WaitForSingleObject(m_th, INFINITE);
	CloseHandle(m_th);
	m_hWnd = NULL;
	m_th = NULL;
	return true;
}

void View::RunWindowThread()
{
	m_run = RUN_INIT;
	if (! CreateMainWindow()) {
		m_run = RUN_ERROR;
		return;
	}
	m_run = RUN_RUN;

	MSG msg;
	while (m_run == RUN_RUN) {
		if (PeekMessage (&msg,m_hWnd,0,0,PM_NOREMOVE)) {
			if (GetMessage (&msg,m_hWnd,0,0) == 0) {
				m_run = RUN_END;
				break;
			}
			//TranslateMessage(&msg);
			DispatchMessage(&msg);
			Sleep(10);
		} else {
			Sleep(100);
		}
	}
	ShowWindow(m_hWnd, SW_HIDE);
	DestroyWindow(m_hWnd);
	m_hWnd = NULL;
}

//----------------------------------------------------------
LRESULT View::onWindowMessage(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{

	if (msg == WM_CREATE) {
		if (m_hWnd != 0) { return -1; }
		m_hWnd = hWnd;
	} else {
		if (m_hWnd == 0) { return -1; }
	}

	switch(msg)
	{   
	case WM_CREATE:
		return onCreate(static_cast< LPCREATESTRUCT >((void*)lp));
	case WM_DESTROY:
		break;
	case WM_PAINT:
		return onPaint();
	case WM_COMMAND:
		switch (LOWORD(wp)) {
			case ID_BUTTON1:
				m_model->toggleTest();
				InvalidateRect(m_hWnd, NULL, true);
				return 0;
		}
		break;
//	case WM_LBUTTONDOWN:
//	case WM_RBUTTONDOWN:
//		return onButtonDown(msg);
	}
	return DefWindowProc(m_hWnd, msg, wp, lp);
}

LRESULT View::onCreate(LPCREATESTRUCT data)
{
	return 0;
}

LRESULT View::onPaint()
{
	if (m_hWnd == 0) { return -1; }

	bool b;
	m_model->IsTracking(&b);
	if (b) {
		if (m_model->isTest()) {
			SetWindowText(m_hButton1, _T("OFF"));
		} else {
			SetWindowText(m_hButton1, _T("ON"));
		}
		EnableWindow(m_hButton1, true);
	} else {
		SetWindowText(m_hButton1, _T("no user"));
		EnableWindow(m_hButton1, false);
	}

	return  0;
}

//-----------------------------------------------------------------------
bool View::CreateMainWindow()
{
	m_run = RUN_INIT;
	intptr_t klass_ = m_atom & 0xFFFF;
	void* klass = reinterpret_cast<void*>(klass_);
	LPCTSTR klassName = static_cast< LPCTSTR >(klass);
	LPCTSTR winName = _T("DxOpenNI");

	int dft = CW_USEDEFAULT;
	CreateWindow(klassName, winName, WS_OVERLAPPEDWINDOW,
		dft, dft, dft, dft, NULL, NULL, m_hInstance, this);
	if (m_hWnd == NULL) { //m_hWnd is set in onCreate()
		return false;
	}
	LayoutWidgets();
	InvalidateRect(m_hWnd, NULL, true);
	ShowWindow(m_hWnd, SW_SHOWNORMAL);
	return true;
}

HWND View::CreateButton(int id, HWND hParent, LPCTSTR str, int x, int y, int w, int h)
{
	DWORD style = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON;
	return CreateWindow(_T("BUTTON"), str, style,
		x,y,w,h, hParent, (HMENU)(INT_PTR)id, m_hInstance, NULL);
}

bool View::LayoutWidgets()
{
	m_hButton1 = CreateButton(ID_BUTTON1, m_hWnd, _T("button"), 10,10,100,30);

	SetWindowPos(m_hWnd, NULL, 0, 0, 200, 100, SWP_NOMOVE | SWP_NOZORDER);
	return true;
}
