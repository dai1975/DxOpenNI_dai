#ifndef MMD_DXOPENNI_VIEW_H
#define MMD_DXOPENNI_VIEW_H

#include <windows.h>

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
	LRESULT onWindowMessage(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
	void RunWindowThread();

protected:
	LRESULT onPaint();
	LRESULT onCreate(LPCREATESTRUCT);
	LRESULT onButtonDown(UINT msg);

protected:
	bool CreateMainWindow();
	bool LayoutWidgets();
	HWND CreateButton(int id, HWND hParent, LPCTSTR str, int x, int y, int w, int h);

private:
	bool StartWindowThread();
	bool StopWindowThread();
	HANDLE m_th;
	enum RunState { RUN_INIT, RUN_RUN, RUN_END, RUN_ERROR, };
	RunState m_run;

private:
	HINSTANCE m_hInstance;
	HWND m_hWnd;
	ATOM m_atom;

	NISystem* m_model;
	HWND m_hButton1;
};



#endif

