#include "View.h"
#include "tc.h"
#include "NISystem.h"

namespace DxOpenNI {

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
	return self->OnWindowMessage(hWnd, msg, wp, lp);
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
LRESULT View::OnWindowMessage(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
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
		return OnCreate(static_cast< LPCREATESTRUCT >((void*)lp));
	case WM_DESTROY:
		break;
	case WM_PAINT:
		return OnPaint();
	case WM_COMMAND:
		//switch (LOWORD(wp)) {
		//}
		break;
	case WM_HSCROLL:
		return OnScroll((HWND)lp, LOWORD(wp), HIWORD(wp));
	}
	return DefWindowProc(m_hWnd, msg, wp, lp);
}

LRESULT View::OnCreate(LPCREATESTRUCT data)
{
	return 0;
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

	m_hFont = CreateFont(24, 0, 0, FW_REGULAR, false, false, false,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS, PROOF_QUALITY, FIXED_PITCH | FF_MODERN, _T(""));

	LayoutWidgets();

	ShowWindow(m_hWnd, SW_SHOWNORMAL);
	return true;
}

//------------------------------------------------------
LRESULT View::OnScroll(HWND hWnd, int sb, int nPos)
{
	HWND hWndLabel = NULL;
	NISystem::CONTROL cid;
	if (hWnd == m_hHistoryBar) {
		cid = NISystem::C_HISTORY_SIZE;
	} else {
		return 0;
	}

	int pos;
	switch (sb) {
		case SB_ENDSCROLL:
			break;
		case SB_LEFT:
			m_model->PlusRangePos(cid, -1); break;
		case SB_RIGHT:
			m_model->PlusRangePos(cid, 1); break;
		case SB_LINELEFT:
			m_model->PlusRangePos(cid, -1); break;
		case SB_LINERIGHT:
			m_model->PlusRangePos(cid, 1); break;
		case SB_PAGELEFT:
			m_model->PlusRangePos(cid, -1); break;
		case SB_PAGERIGHT:
			m_model->PlusRangePos(cid, 1); break;
		case SB_THUMBPOSITION:
			break;
		case SB_THUMBTRACK:
			{
				SCROLLINFO info;
				info.cbSize = sizeof(SCROLLINFO);
				info.fMask = SIF_TRACKPOS;
				GetScrollInfo(m_hHistoryBar, SB_CTL, &info);
				m_model->SetRangePos(cid, info.nTrackPos);
			}
			break;
	}
	return 0;
}

bool View::LayoutWidgets()
{
	DWORD style = WS_CHILD | WS_VISIBLE;
	int y;
	int h = 20;

	{
		y = 10;
		m_hStat = CreateWindow(_T("STATIC"), _T(""),
			style | SS_LEFT, 10,y,200,h, m_hWnd, NULL, m_hInstance, NULL);
	}
	{
		y = 50;
		m_hHistoryName  = CreateWindow(_T("STATIC"), _T("FLAT"),
			style | SS_CENTER, 10,y,50,h, m_hWnd, NULL, m_hInstance, NULL);
		m_hHistoryBar   = CreateWindow(_T("SCROLLBAR"), _T(""),
			style | SBS_HORZ, 70,y,100,h, m_hWnd, NULL, m_hInstance, NULL);
		m_hHistoryValue = CreateWindow(_T("STATIC"), _T(""),
			style | SS_CENTER, 180,y,30,h, m_hWnd, NULL, m_hInstance, NULL);

		int min, max, pos;
		SCROLLINFO info;
		m_model->GetRange(NISystem::C_HISTORY_SIZE, &min, &max, &pos);
		info.cbSize = sizeof(SCROLLINFO);
		info.fMask = SIF_RANGE;
		info.nMin = min;
		info.nMax = max;
		SetScrollInfo(m_hHistoryBar, SB_CTL, &info, true);
	}

	SetWindowPos(m_hWnd, NULL, 0, 0, 300, 200, SWP_NOMOVE | SWP_NOZORDER);
	Repaint();
	return true;
}

void View::Repaint()
{
	if (m_hWnd == NULL) { return; }

	{ //scrollbar
		int min, max, pos;
		SCROLLINFO info;
		m_model->GetRange(NISystem::C_HISTORY_SIZE, &min, &max, &pos);
		info.cbSize = sizeof(SCROLLINFO);
		info.fMask = SIF_POS;
		info.nPos = pos;
		SetScrollInfo(m_hHistoryBar, SB_CTL, &info, true);
	}

	InvalidateRect(m_hWnd, NULL, true);
}

LRESULT View::OnPaint()
{
	if (m_hWnd == 0) { return -1; }

	PAINTSTRUCT pc;
	HDC hdc = BeginPaint(m_hWnd, &pc);
	SelectObject(hdc, m_hFont);
	{
		bool b;
		tc_string str;

		m_model->IsTracking(&b);
		if (b) {
			SetWindowText(m_hStat, _T("tracking"));
		} else {
			SetWindowText(m_hStat, _T("no tracking"));
		}

		str = m_model->GetRangeString(NISystem::C_HISTORY_SIZE);
		SetWindowText(m_hHistoryValue, str.c_str());
	}
	EndPaint(m_hWnd, &pc);

	return  0;
}

}
