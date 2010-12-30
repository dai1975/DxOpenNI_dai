#ifndef MMD_DXOPENNI_VIEW_H
#define MMD_DXOPENNI_VIEW_H

#include <windows.h>

namespace DxOpenNI {

class NISystem;
class View
{
public:
	View(NISystem*);
	virtual ~View();

	bool onAttach(HINSTANCE h);
	bool onDetach(HINSTANCE h);
	bool Setup(HWND parent);
	void Clean();
	LRESULT OnWindowMessage(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
	void RunWindowThread();
	void Repaint();

protected:
	LRESULT OnPaint();
	LRESULT OnCreate(LPCREATESTRUCT);
	LRESULT OnButtonDown(UINT msg);
	LRESULT OnScroll(HWND hWnd, int sb, int nPos);

protected:
	bool CreateMainWindow();
	bool LayoutWidgets();

private:
	bool StartWindowThread();
	bool StopWindowThread();
	HANDLE m_th;
	enum RunState { RUN_INIT, RUN_RUN, RUN_END, RUN_ERROR, };
	RunState m_run;

private:
	NISystem* m_model;

	HINSTANCE m_hInstance;
	ATOM m_atom;
	HWND m_hWnd;
	HFONT m_hFont;

	HWND m_hStat;
	HWND m_hHistoryName, m_hHistoryValue, m_hHistoryBar;
};

}

#endif

