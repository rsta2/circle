//
// uguicpp.h
//
// C++ wrapper for uGUI with mouse and touch screen support
//
#ifndef _ugui_uguicpp_h
#define _ugui_uguicpp_h

#ifdef __cplusplus
extern "C" {
#endif
	#include <ugui/ugui.h>
#ifdef __cplusplus
}
#endif

#include <circle/screen.h>
#include <circle/usb/usbmouse.h>
#include <circle/input/touchscreen.h>

#if DEPTH != 16
	#error DEPTH must be set to 16 in include/circle/screen.h!
#endif

class CUGUI
{
public:
	CUGUI (CScreenDevice *pScreen);
	~CUGUI (void);

	boolean Initialize (void);

	void Update (void);

private:
	static void SetPixel (UG_S16 sPosX, UG_S16 sPosY, UG_COLOR Color);

	void MouseEventHandler (TMouseEvent Event, unsigned nButtons, unsigned nPosX, unsigned nPosY);
	static void MouseEventStub (TMouseEvent Event, unsigned nButtons, unsigned nPosX, unsigned nPosY);

	void TouchScreenEventHandler (TTouchScreenEvent Event,
				      unsigned nID, unsigned nPosX, unsigned nPosY);
	static void TouchScreenEventStub (TTouchScreenEvent Event,
					  unsigned nID, unsigned nPosX, unsigned nPosY);

private:
	CScreenDevice *m_pScreen;

	UG_GUI m_GUI;

	CTouchScreenDevice *m_pTouchScreen;
	unsigned m_nLastUpdate;

	static CUGUI *s_pThis;
};

#endif
