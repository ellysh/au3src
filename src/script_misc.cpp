
///////////////////////////////////////////////////////////////////////////////
//
// AutoIt v3
//
// Copyright (C)1999-2005:
//		- Jonathan Bennett <jon at hiddensoft dot com>
//		- See "AUTHORS.txt" for contributors.
//
// This file is part of AutoIt.
//
// AutoIt source code is copyrighted software distributed under the terms of the
// AutoIt source code license.
//
// You may:
//
// - Customize the design and operation of the AutoIt source code to suit
// the internal needs of your organization except to the extent not
// permitted in this Agreement
//
// You may not:
//
// - Distribute the AutoIt source code and/or compiled versions of AutoIt
// created with the AutoIt source code.
// - Create derivative works based on the AutoIt source code for distribution
// or usage outside your organisation.
// - Modify and/or remove any copyright notices or labels included in the
// AutoIt source code.
//
// AutoIt is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
// See the LICENSE.txt file that accompanies the AutoIt source
// code for more details.
//
///////////////////////////////////////////////////////////////////////////////
//
// script_misc.cpp
//
// Contains routines that don't belong anywhere else.  Part of script.cpp
//
///////////////////////////////////////////////////////////////////////////////


// Includes
#include "StdAfx.h"								// Pre-compiled headers

#ifndef _MSC_VER								// Includes for non-MS compilers
	#include <stdio.h>
	#include <windows.h>
	#include <math.h>
	#include <limits.h>
	#include <olectl.h>
	#include <commctrl.h>
	#include <ctype.h>
	#include <wininet.h>
	#include <process.h>
#endif

// MinGW is missing some internet headers from wininet.h so declare them here
#ifndef INTERNET_STATE_CONNECTED
    typedef struct {
        DWORD dwConnectedState;
        DWORD dwFlags;
    } INTERNET_CONNECTED_INFO, * LPINTERNET_CONNECTED_INFO;

    #define INTERNET_STATE_CONNECTED                0x00000001  // connected state (mutually exclusive with disconnected)
    #define INTERNET_STATE_DISCONNECTED             0x00000002  // disconnected from network
    #define INTERNET_STATE_DISCONNECTED_BY_USER     0x00000010  // disconnected by user request
    #define INTERNET_STATE_IDLE                     0x00000100  // no network requests being made (by Wininet)
    #define INTERNET_STATE_BUSY                     0x00000200  // network requests being made (by Wininet)

    #define INTERNET_OPTION_CONNECTED_STATE         50
#endif


#include "AutoIt.h"								// Autoit values, macros and config options

#include "script.h"
#include "globaldata.h"
#include "resources\resource.h"
#include "utility.h"
#include "mt19937ar-cok.h"
#include "inputbox.h"
#include "regexp.h"


///////////////////////////////////////////////////////////////////////////////
// AdlibDisable()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_AdlibDisable(VectorVariant &vParams, Variant &vResult)
{
	m_bAdlibEnabled	= false;
	return AUT_OK;

} // AdlibDisable()


///////////////////////////////////////////////////////////////////////////////
// AdlibEnable()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_AdlibEnable(VectorVariant &vParams, Variant &vResult)
{
	int		nTemp1, nTemp2, nTemp3, nTemp4;
	m_bAdlibEnabled	= false;			// Disable by default in case of error

	// Check that this user function exists
	if (Parser_FindUserFunction(vParams[0].szValue(), nTemp1, nTemp2, nTemp3, nTemp4) == false)
	{
		FatalError(IDS_AUT_E_UNKNOWNUSERFUNC);
		return AUT_ERR;
	}
	else
	{
		m_sAdlibFuncName	= vParams[0].szValue();
		m_tAdlibTimerStarted= timeGetTime();

		if (vParams.size() == 2 && vParams[1].nValue() > 0)
			m_nAdlibTimeout = vParams[1].nValue();
		else
			m_nAdlibTimeout = AUT_ADLIB_DELAY;

		m_bAdlibEnabled		= true;
	}

	return AUT_OK;

} // AdlibEnable()


///////////////////////////////////////////////////////////////////////////////
// AutoItWinSetTitle()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_AutoItWinSetTitle(VectorVariant &vParams, Variant &vResult)
{
	SetWindowText(g_hWnd, vParams[0].szValue());
	return AUT_OK;

} // AutoItWinSetTitle()


///////////////////////////////////////////////////////////////////////////////
// AutoItWinGetTitle()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_AutoItWinGetTitle(VectorVariant &vParams, Variant &vResult)
{
	char szTemp1[AUT_WINTEXTBUFFER+1];

	GetWindowText(g_hWnd, szTemp1, AUT_WINTEXTBUFFER);
	vResult = szTemp1;
	return AUT_OK;

} // AutoItWinGetTitle()


///////////////////////////////////////////////////////////////////////////////
// BlockInput()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_BlockInput(VectorVariant &vParams, Variant &vResult)
{

typedef void (CALLBACK *BlockInput)(BOOL);

	// Must be running 2000/ win98 for this function to be successful
	// We must dynamically load the function to retain compatibility with Win95

    // Get a handle to the DLL module that contains BlockInput
	HINSTANCE hinstLib = LoadLibrary("user32.dll");

    // If the handle is valid, try to get the function address.
	if (hinstLib != NULL)
	{
		BlockInput lpfnDLLProc = (BlockInput)GetProcAddress(hinstLib, "BlockInput");

		if (lpfnDLLProc != NULL)
		{
			if (vParams[0].nValue() == 0)
				(*lpfnDLLProc)(FALSE);
			else
				(*lpfnDLLProc)(TRUE);
		}

		// Free the DLL module.
		FreeLibrary(hinstLib);
	}

	return AUT_OK;

} // BlockInput()


///////////////////////////////////////////////////////////////////////////////
// EnvGet()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_EnvGet(VectorVariant &vParams, Variant &vResult)
{
	char	szEnvValue[AUT_MAX_ENVSIZE+1];		// Env variable value

	szEnvValue[0] = '\0';						// Terminate in case the Get function fails
	GetEnvironmentVariable(vParams[0].szValue(), szEnvValue, AUT_MAX_ENVSIZE);

	vResult = szEnvValue;

	return AUT_OK;

} // EnvGet()


///////////////////////////////////////////////////////////////////////////////
// EnvSet()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_EnvSet(VectorVariant &vParams, Variant &vResult)
{
	if (vParams.size() >= 2)
		SetEnvironmentVariable(vParams[0].szValue(), vParams[1].szValue());
	else
		SetEnvironmentVariable(vParams[0].szValue(), NULL);

	return AUT_OK;

} // EnvSet()


///////////////////////////////////////////////////////////////////////////////
// EnvUpdate()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_EnvUpdate(VectorVariant &vParams, Variant &vResult)
{
    ULONG	nResult;

	if(SendMessageTimeout(HWND_BROADCAST,WM_SETTINGCHANGE,0,(LPARAM)"Environment",SMTO_BLOCK,15000,&nResult)==0)
		SetFuncErrorCode(1);

	return AUT_OK;

} // EnvUpdate()

///////////////////////////////////////////////////////////////////////////////
// ClipGet()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_ClipGet(VectorVariant &vParams, Variant &vResult)
{
	// $var = ClipGet()

	if (IsClipboardFormatAvailable(CF_TEXT))
	{
		OpenClipboard(g_hWnd);
		HGLOBAL hClipMem = GetClipboardData(CF_TEXT);
		if (hClipMem == NULL)
		{
			SetFuncErrorCode(1);				// Default is 0
			return AUT_OK;
		}

		PSTR pClipMem = (char *)GlobalLock(hClipMem);
		if (pClipMem == NULL)
		{
			CloseClipboard();
			SetFuncErrorCode(1);				// Default is 0
			return AUT_OK;
		}

		vResult = pClipMem;						// Copy the text to result
		GlobalUnlock(hClipMem);
		CloseClipboard();
	}
	else
		SetFuncErrorCode(1);					// Default is 0

	return AUT_OK;

} // ClipGet()


///////////////////////////////////////////////////////////////////////////////
// ClipPut()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_ClipPut(VectorVariant &vParams, Variant &vResult)
{
	// ClipPut(<text>)

	HGLOBAL hClipMem = GlobalAlloc(GMEM_MOVEABLE, strlen(vParams[0].szValue())+1);
	if (hClipMem == NULL)
	{
		vResult = 0;							// Default is 1
		return AUT_OK;
	}

	PSTR pClipMem = (char *)GlobalLock(hClipMem);
	if (pClipMem == NULL)
	{
		vResult = 0;							// Default is 1
		return AUT_OK;
	}

	strcpy( pClipMem, vParams[0].szValue() );	// Store the data
	GlobalUnlock(hClipMem);

	OpenClipboard(g_hWnd);
	EmptyClipboard();
	SetClipboardData(CF_TEXT, hClipMem);
	CloseClipboard();

	return AUT_OK;

} // ClipPut()


///////////////////////////////////////////////////////////////////////////////
// HttpSetProxy()
//
// HttpSetProxy( mode, ["proxy:port", "user", "password"] )
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_HttpSetProxy(VectorVariant &vParams, Variant &vResult)
{
	uint iNumParams = vParams.size();

	if (vParams[0].nValue() == 1)
		m_nHttpProxyMode = AUT_PROXY_DIRECT;
	else if (vParams[0].nValue() == 2)
		m_nHttpProxyMode = AUT_PROXY_PROXY;
	else
		m_nHttpProxyMode = AUT_PROXY_REGISTRY;

	if (iNumParams > 1)
		m_sHttpProxy = vParams[1].szValue();
	if (iNumParams > 2)
		m_sHttpProxyUser = vParams[2].szValue();
	if (iNumParams > 3)
		m_sHttpProxyPwd = vParams[3].szValue();

	return AUT_OK;

} // HttpSetProxy()


///////////////////////////////////////////////////////////////////////////////
// FtpSetProxy()
//
// FtpSetProxy( mode, ["proxy:port", "user", "password"] )
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_FtpSetProxy(VectorVariant &vParams, Variant &vResult)
{
	uint iNumParams = vParams.size();

	if (vParams[0].nValue() == 1)
		m_nFtpProxyMode = AUT_PROXY_DIRECT;
	else if (vParams[0].nValue() == 2)
		m_nFtpProxyMode = AUT_PROXY_PROXY;
	else
		m_nFtpProxyMode = AUT_PROXY_REGISTRY;

	if (iNumParams > 1)
		m_sFtpProxy = vParams[1].szValue();
	if (iNumParams > 2)
		m_sFtpProxyUser = vParams[2].szValue();
	if (iNumParams > 3)
		m_sFtpProxyPwd = vParams[3].szValue();

	return AUT_OK;

} // FtpSetProxy()


///////////////////////////////////////////////////////////////////////////////
// PixelSearch()
// Finds a pixel of certain color within a RECT of pixels
// PixelSearch(<left>,<top>,<right>,<bottom>,<decimal pixel color> [, variation] [, step] )
//
// Variation code from based on cmallet's post on hiddensoft.com forum.
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_PixelSearch (VectorVariant &vParams, Variant &vResult)
{
	uint		iNumParams = vParams.size();
	int			q,r;
	int			col;
	BYTE		red, green, blue;
	BYTE		red_low, red_high, green_low, green_high, blue_low, blue_high;
	HDC			hdc;
	RECT		relrect;
	int			nVar;
	int			nStep = 1;
	POINT		ptOrigin;

	relrect.left = vParams[0].nValue();
	relrect.top = vParams[1].nValue();
	relrect.right = vParams[2].nValue();
	relrect.bottom = vParams[3].nValue();

	// Convert coords to screen/active window/client
	ConvertCoords(m_nCoordPixelMode, ptOrigin);
	relrect.left += ptOrigin.x;
	relrect.top += ptOrigin.y;
	relrect.right += ptOrigin.x;
	relrect.bottom += ptOrigin.y;


	// Get the search colour and split into components
	col		= vParams[4].nValue();				// Pixel color to find
	if (m_bColorModeBGR == false)
		Util_RGBtoBGR(col);						// Get RGB color into the standard COLORREF BGR format

	red		= GetRValue(col);
	green	= GetGValue(col);
	blue	= GetBValue(col);

	// Variation required?
	if (iNumParams >= 6)
	{
		nVar = vParams[5].nValue();
		if ( nVar < 0 )
			nVar = 0;
		else if ( nVar > 0xff )
			nVar = 0xff;
	}
	else
		nVar = 0;

	// Step required?
	if (iNumParams >= 7 && vParams[6].nValue() > 1)
		nStep = vParams[6].nValue();

	// Config the variation code
	if (nVar == 0)								// Exact match
	{
		red_low = red_high = red;
		green_low = green_high = green;
		blue_low = blue_high = blue;
	}
	else
	{
		// Prevent wrap around
		red_low = (nVar > red) ? 0 : red - nVar;
		green_low = (nVar > green) ? 0 : green - nVar;
		blue_low = (nVar > blue) ? 0 : blue - nVar;
		red_high = (nVar > 0xFF - red) ? 0xFF : red + nVar;
		green_high = (nVar > 0xFF - green) ? 0xFF : green + nVar;
		blue_high = (nVar > 0xFF - blue) ? 0xFF : blue + nVar;
	}


	hdc = GetDC(NULL);

	for( q=relrect.left; q<=relrect.right; q = q + nStep)
	{
		for( r=relrect.top; r<=relrect.bottom; r = r + nStep)
		{
			col		= GetPixel(hdc, q, r);
			red		= GetRValue(col);
			green	= GetGValue(col);
			blue	= GetBValue(col);

			if (red >= red_low && red <= red_high && green >= green_low && green <= green_high
					&& blue >= blue_low && blue <= blue_high)
			{
				// Match!
				// Setup vResult as an Array to hold the 2 values we want to return
				Variant	*pvTemp;

				Util_VariantArrayDim(&vResult, 2);

				// Convert coords to screen/active window/client
				q -= ptOrigin.x;
				r -= ptOrigin.y;

				pvTemp = Util_VariantArrayGetRef(&vResult, 0);	//First element
				*pvTemp = q;					// X

				pvTemp = Util_VariantArrayGetRef(&vResult, 1);
				*pvTemp = r;					// Y

				ReleaseDC(NULL,hdc);

				return AUT_OK;
			}
		}
	}
	ReleaseDC(NULL,hdc);

	SetFuncErrorCode(1);			// Not found
	return AUT_OK;

} // PixelSearch()


///////////////////////////////////////////////////////////////////////////////
// PixelChecksum()
// Does an Alder32 checksum of a region of pixels
// PixelChecksum(<left>,<top>,<right>,<bottom> [,<step>])
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_PixelChecksum (VectorVariant &vParams, Variant &vResult)
{
	int				q,r;
	COLORREF		col;
	HDC				hdc;
	RECT			relrect;
	unsigned long	adler = 1L;
	unsigned long	s1, s2;
	int				nStep = 1;
	POINT			ptOrigin;

	relrect.left = vParams[0].nValue();
	relrect.top = vParams[1].nValue();
	relrect.right = vParams[2].nValue();
	relrect.bottom = vParams[3].nValue();

	// Step required?
	if (vParams.size() >= 5 && vParams[4].nValue() > 1)
		nStep = vParams[4].nValue();

	// Convert coords to screen/active window/client
	ConvertCoords(m_nCoordPixelMode, ptOrigin);
	relrect.left += ptOrigin.x;
	relrect.top += ptOrigin.y;
	relrect.right += ptOrigin.x;
	relrect.bottom += ptOrigin.y;


	hdc = GetDC(NULL);

	for( q=relrect.left; q<=relrect.right; q = q + nStep)
	{
		for( r=relrect.top; r<=relrect.bottom; r = r + nStep)
		{
			col	= GetPixel(hdc, q, r);

			s1 = adler & 0xffff;
			s2 = (adler >> 16) & 0xffff;
			s1 = (s1 + GetRValue(col)) % 65521;
			s2 = (s2 + s1) % 65521;
			adler = (s2 << 16) + s1;

			s1 = adler & 0xffff;
			s2 = (adler >> 16) & 0xffff;
			s1 = (s1 + GetGValue(col)) % 65521;
			s2 = (s2 + s1) % 65521;
			adler = (s2 << 16) + s1;

			s1 = adler & 0xffff;
			s2 = (adler >> 16) & 0xffff;
			s1 = (s1 + GetBValue(col)) % 65521;
			s2 = (s2 + s1) % 65521;
			adler = (s2 << 16) + s1;
		}
	}
	ReleaseDC(NULL,hdc);

	vResult = (double)adler;

	return AUT_OK;

} // PixelChecksum()


///////////////////////////////////////////////////////////////////////////////
// PixelGetColor()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_PixelGetColor(VectorVariant &vParams, Variant &vResult)
{
	HDC		hdc;
	POINT	ptOrigin;

	int relx = vParams[0].nValue();
	int rely = vParams[1].nValue();

	// Convert coords to screen/active window/client
	ConvertCoords(m_nCoordPixelMode, ptOrigin);
	relx += ptOrigin.x;
	rely += ptOrigin.y;

	if ( (hdc=GetDC(NULL)) )
	{
		int nCol = (int)GetPixel(hdc,relx,rely);

		// Convert from BGR to RGB?
		if (m_bColorModeBGR == false)
			Util_BGRtoRGB(nCol);

		vResult = nCol;

		ReleaseDC(NULL,hdc);
	}
	else
		SetFuncErrorCode(1);

	return AUT_OK;

} // PixelGetColor()


///////////////////////////////////////////////////////////////////////////////
// MouseGetPos()
// Returns the coords of the mouse pointer
// $array = MouseGetPos()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_MouseGetPos(VectorVariant &vParams, Variant &vResult)
{
	POINT	pt, ptOrigin;

	GetCursorPos(&pt);

	// Convert coords to screen/active window/client
	ConvertCoords(m_nCoordMouseMode, ptOrigin);
	pt.x -= ptOrigin.x;
	pt.y -= ptOrigin.y;

	// Setup vResult as an Array to hold the 2 values we want to return
	Variant	*pvTemp;

	Util_VariantArrayDim(&vResult, 2);

	pvTemp = Util_VariantArrayGetRef(&vResult, 0);	//First element
	*pvTemp = (int)pt.x;						// X

	pvTemp = Util_VariantArrayGetRef(&vResult, 1);
	*pvTemp = (int)pt.y	;						// Y

	return AUT_OK;

} // MouseGetPos()


///////////////////////////////////////////////////////////////////////////////
// MouseMove()
// Moves the mouse
// MouseMove( x, y [,speed(0-100)] )
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_MouseMove(VectorVariant &vParams, Variant &vResult)
{
	int		nSpeed;

	if (vParams.size() == 3)
		nSpeed = vParams[2].nValue();
	else
		nSpeed = -1;							// Force default

	MouseMoveExecute(vParams[0].nValue(), vParams[1].nValue(), nSpeed);

	return AUT_OK;

} // MouseMove()


///////////////////////////////////////////////////////////////////////////////
// MouseClick()
// Performs a mouse click at the requested coords
// MouseClick( "left|middle|right", [x,y] [#clicks] [speed(0-100)] )
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_MouseClick(VectorVariant &vParams, Variant &vResult)
{
	uint	iNumParams = vParams.size();
	int		nSpeed;
	int		nClicks, nX=0, nY=0;

	// Check for x without y
	if (iNumParams == 2)
	{
		FatalError(IDS_AUT_E_FUNCTIONNUMPARAMS);
		return AUT_ERR;
	}

	// Get coords
	if (iNumParams >= 2)
	{
		nX = vParams[1].nValue();
		nY = vParams[2].nValue();
	}

	// Get number of clicks
	if (iNumParams >= 4)
	{
		nClicks = vParams[3].nValue();
		if (nClicks <= 0)
			return AUT_OK;	// 0 or less clicks specified
	}
	else
		nClicks = 1;

	// Get speed
	if (iNumParams >= 5)
		nSpeed = vParams[4].nValue();
	else
		nSpeed = -1;							// -1 = default speed


	// Do we need to move the mouse?
	if (iNumParams > 1)
		MouseMoveExecute(nX, nY, nSpeed);


	for (int i=0; i<nClicks; ++i)
	{
		// Do the click
		if (Util_MouseDown(vParams[0].szValue()) == 0)
		{
			// not a valid click
			vResult = 0;
			return AUT_OK;
		}
		Util_Sleep(m_nMouseClickDownDelay);
		if (Util_MouseUp(vParams[0].szValue()) == 0) {
			// not a valid click
			vResult = 0;
			return AUT_OK;
		}
		Util_Sleep(m_nMouseClickDelay);
	}

	return AUT_OK;

} // MouseClick()


///////////////////////////////////////////////////////////////////////////////
// MouseClickDrag()
// Performs a mouse click drag
// MouseClick( "left|middle|right", x1, y1, x2, y2, [speed(0-100)] )
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_MouseClickDrag(VectorVariant &vParams, Variant &vResult)
{
	int		nSpeed;

	// Get coords
	int nX1 = vParams[1].nValue();
	int nY1 = vParams[2].nValue();
	int nX2 = vParams[3].nValue();
	int nY2 = vParams[4].nValue();

	// Get speed
	if (vParams.size() >= 6)
		nSpeed = vParams[5].nValue();
	else
		nSpeed = -1;							// -1 = default speed

	// Move the mouse to the start position
	MouseMoveExecute(nX1, nY1, nSpeed);

	// The drag operation fails unless speed is now >=2
	if (nSpeed < 2 && nSpeed != -1)
		nSpeed = 2;

	// Do the drag operation
	if (Util_MouseDown(vParams[0].szValue()) == 0)
	{
		vResult = 0;
		return AUT_OK;
	}
	Util_Sleep(m_nMouseClickDragDelay);
	MouseMoveExecute(nX2, nY2, nSpeed);
	Util_Sleep(m_nMouseClickDragDelay);
	if (Util_MouseUp(vParams[0].szValue()) == 0)
	{
		vResult = 0;
		return AUT_OK;
	}
	Util_Sleep(m_nMouseClickDelay);

	return AUT_OK;

} // MouseClickDrag()


///////////////////////////////////////////////////////////////////////////////
// MouseMoveExecute()
// Function to actually move the mouse
///////////////////////////////////////////////////////////////////////////////

void AutoIt_Script::MouseMoveExecute(int x, int y, int nSpeed)
{
	POINT	ptCur, ptOrigin;
	RECT	rect;
	int		xCur, yCur;
	int		delta;
	const	int	nMinSpeed = 32;


	// Convert coords to screen/active window/client
	ConvertCoords(m_nCoordMouseMode, ptOrigin);
	x += ptOrigin.x;
	y += ptOrigin.y;

	// Get size of desktop
	GetWindowRect(GetDesktopWindow(), &rect);

	// Sanity check coords - removed so that it works on multiple monitors where -ve values are OK
/*	if (x < 0)
		x = 0;
	else if (x > rect.right)
		x = rect.right;

	if (y < 0)
		y = 0;
	else if (y > rect.bottom)
		y = rect.bottom; */

	// Convert our coords to mouse_event coords
	x = ((65535 * x) / (rect.right-1)) + 1;
	y = ((65535 * y) / (rect.bottom-1)) + 1;


	// Are we slowly moving or insta-moving?
	if (nSpeed == 0)
	{
		mouse_event(MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE, x, y, 0, 0);
		Util_Sleep(10);							// Hopefully fixes "clicks before moving" bug
		return;
	}

	// Sanity check for speed
	if (nSpeed < 0 || nSpeed > 100)
		nSpeed = 10;							// Default is speed 10


	// So, it's a more gradual speed that is needed :)
	GetCursorPos(&ptCur);
	xCur = ((ptCur.x * 65535) / (rect.right-1)) + 1;
	yCur = ((ptCur.y * 65535) / (rect.bottom-1)) + 1;

	while (xCur != x || yCur != y)
	{
		if (xCur < x)
		{
			delta = (x - xCur) / nSpeed;
			if (delta == 0 || delta < nMinSpeed)
				delta = nMinSpeed;
			if ((xCur + delta) > x)
				xCur = x;
			else
				xCur += delta;
		}
		else
			if (xCur > x)
			{
				delta = (xCur - x) / nSpeed;
				if (delta == 0 || delta < nMinSpeed)
					delta = nMinSpeed;
				if ((xCur - delta) < x)
					xCur = x;
				else
					xCur -= delta;
			}

		if (yCur < y)
		{
			delta = (y - yCur) / nSpeed;
			if (delta == 0 || delta < nMinSpeed)
				delta = nMinSpeed;
			if ((yCur + delta) > y)
				yCur = y;
			else
				yCur += delta;
		}
		else
			if (yCur > y)
			{
				delta = (yCur - y) / nSpeed;
				if (delta == 0 || delta < nMinSpeed)
					delta = nMinSpeed;
				if ((yCur - delta) < y)
					yCur = y;
				else
					yCur -= delta;
			}

		mouse_event(MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE, xCur, yCur, 0, 0);

		Util_Sleep(10);							// 20 ms sleep inbetween moves
	}

} // MouseMoveExecute()


///////////////////////////////////////////////////////////////////////////////
// MouseWheel()
// Performs a Mouse wheel operation
// MouseWheel( "up|down", [#clicks] )
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_MouseWheel(VectorVariant &vParams, Variant &vResult)
{
	int		nClicks = 1;

	// Get number of clicks
	if (vParams.size() >= 2)
	{
		nClicks = vParams[1].nValue();
		if (nClicks <= 0)
			return AUT_OK;	// 0 or less clicks specified
	}


	for (int i=0; i<nClicks; ++i)
	{
		// Do the click
		if (Util_MouseWheel(vParams[0].szValue())==0)
		{
			vResult = 0;
			return AUT_OK;
		}
		Util_Sleep(m_nMouseClickDelay);
	}

	return AUT_OK;

} // MouseWheel()



///////////////////////////////////////////////////////////////////////////////
// MouseDown("button")
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_MouseDown(VectorVariant &vParams, Variant &vResult)
{
	if (Util_MouseDown(vParams[0].szValue()) == 0)
		vResult = 0;

	return AUT_OK;

} // MouseDown()


///////////////////////////////////////////////////////////////////////////////
// MouseUp("button")
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_MouseUp(VectorVariant &vParams, Variant &vResult)
{
	if (Util_MouseUp(vParams[0].szValue()) == 0)
		vResult = 0;

	return AUT_OK;

} // MouseUp()



///////////////////////////////////////////////////////////////////////////////
// IsAdmin()
//
// IsAdmin()
// Returns 1 if current user has admin rights (or is running 9x).
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_IsAdmin(VectorVariant &vParams, Variant &vResult)
{
	vResult = 0;								// Set default

	if (g_oVersion.IsWin9x())
	{
		vResult = 1;
		return AUT_OK;
	}

	// Running NT, check rights

	SC_HANDLE h = OpenSCManager( NULL, NULL, SC_MANAGER_LOCK );
	if ( h )
	{
	    SC_LOCK lock = LockServiceDatabase( h ) ;
	    if ( lock )
	    {
			UnlockServiceDatabase( lock ) ;
			vResult = 1 ;
		}
		else
		{
	        DWORD lastErr = GetLastError() ;
			if ( lastErr == ERROR_SERVICE_DATABASE_LOCKED )
				vResult = 1;
		}
		CloseServiceHandle( h ) ;
	}
	return AUT_OK;

} // IsAdmin()


///////////////////////////////////////////////////////////////////////////////
// SplashTextOn()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_SplashTextOn(VectorVariant &vParams, Variant &vResult)
{
	return Splash(vParams, vParams.size(), 1);

} // SplashTextOn()


///////////////////////////////////////////////////////////////////////////////
// SplashImageOn()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_SplashImageOn(VectorVariant &vParams, Variant &vResult)
{
	return Splash(vParams, vParams.size(), 0);

} // SplashImageOn()


///////////////////////////////////////////////////////////////////////////////
// SplashOff()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_SplashOff(VectorVariant &vParams, Variant &vResult)
{
	return Splash(vParams, vParams.size(), 2);

} // SplashOff()


///////////////////////////////////////////////////////////////////////////////
// Splash()
//
// SplashText and SplashImage handler
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::Splash(VectorVariant &vParams, uint iNumParams, int nFlag)
{
	int		W = 500;
	int		H = 400;
	int		L = -1;
	int		T = -1;
	int		lblstyle = WS_CHILD|WS_VISIBLE|SS_CENTER;
	int		style = WS_DISABLED|WS_POPUP|WS_CAPTION;
	int		xstyle = WS_EX_TOPMOST;
	HWND	hWnd;
	RECT	rect;

	if (g_hWndSplash)				// If it exists, kill it...
	{
		if (g_hSplashBitmap)
		{
			DeleteObject(g_hSplashBitmap);
			g_hSplashBitmap = NULL;
		}
		DestroyWindow(g_hWndSplash);
		g_hWndSplash = NULL;		// Reset to a NULL state
	}

	if (nFlag==2)					// SplashOff()
		return AUT_OK;

	GetWindowRect(GetDesktopWindow(),&rect);	// Get Desktop rect

	if (iNumParams >= 3 && !(vParams[2].nValue()==-1))
		W = vParams[2].nValue();				// Optional width selected
	if (iNumParams >= 4 && !(vParams[3].nValue()==-1))
		H = vParams[3].nValue();				// Optional height selected
	if (iNumParams >= 5 )
		L = vParams[4].nValue();				// Optional left position selected
	if (iNumParams >= 6 )
		T = vParams[5].nValue();				// Optional top position selected
	if (iNumParams >= 7 && vParams[6].nValue() != -1)
	{	////////////////////////// EXTENDED SPLASH OPTIONS
		if ( nFlag == 1 )						// SplashText only (not SplashImage)
		{
			if ( vParams[6].nValue() & 8 )
				lblstyle = WS_CHILD|WS_VISIBLE|SS_RIGHT;		// Right justify text
			if ( vParams[6].nValue() & 4 )
			{
				if ( lblstyle & SS_RIGHT )
					lblstyle = WS_CHILD|WS_VISIBLE|SS_CENTER;	//Chose left and right, so center it
				else
					lblstyle = WS_CHILD|WS_VISIBLE|SS_LEFT;		// Left justify text
			}
		}
		if ( vParams[6].nValue() & 2 )
			xstyle=0;			// Selected a Not topmost window

		if ( vParams[6].nValue() & 1 )					// Selected a window
			style = WS_DISABLED|WS_POPUP|WS_BORDER;		// Without a title bar. (border for aethetics)

		if (vParams[6].nValue() & 16)
			style ^= WS_DISABLED;				// Clear the WS_DISABLED flag
	}
	if ( L == -1 )
		L = (rect.right - W)/2;			// Center splash horizontally
	if ( T == -1 )
		T = (rect.bottom - H)/2;		// Center splash vertically

	SetRect (&rect, 0, 0, W, H);
	AdjustWindowRectEx (&rect, style, FALSE, xstyle);

	// CREATE Main Splash Window
	g_hWndSplash = CreateWindowEx(xstyle,AUT_APPCLASS,vParams[0].szValue(),
		style,L,T,rect.right-rect.left,rect.bottom-rect.top,g_hWnd,NULL,NULL,NULL);

	GetClientRect(g_hWndSplash,&rect);	// get the client size

	if ( nFlag==0 )	// SplashImage()
	{
		HANDLE		hFile;
		HBITMAP		hBitmap;
		DWORD		dwFileSize, dwBytesRead;
		LPPICTURE	gpPicture = NULL;
		LPVOID		pvData;
		LPSTREAM	pstm;
		HGLOBAL		hGlobal;

		// CREATE static full size of client area
		hWnd = CreateWindowEx(0,"static",NULL,WS_CHILD|WS_VISIBLE|SS_BITMAP
			,0,0,rect.right-rect.left,rect.bottom-rect.top,g_hWndSplash,NULL,NULL,NULL);
		// JPG,BMP,WMF,ICO,GIF FILE HERE
		hFile=CreateFile(vParams[1].szValue(),GENERIC_READ,0,NULL,OPEN_EXISTING,0,NULL);

		if ( hFile == INVALID_HANDLE_VALUE )
		{
			//FatalError(IDS_AUT_E_PICFILENOTFOUND);
			return AUT_OK;						// File does NOT exist
		}

		dwFileSize = GetFileSize(hFile,NULL);

		hGlobal = GlobalAlloc(GMEM_MOVEABLE, dwFileSize);					//
		pvData = GlobalLock(hGlobal);										//
		ReadFile(hFile, pvData, dwFileSize, &dwBytesRead, NULL);			// Gnarly picture massaging
		GlobalUnlock(hGlobal);											//
		CloseHandle(hFile);												//
		CreateStreamOnHGlobal(hGlobal,TRUE,&pstm);						//
		OleLoadPicture(pstm,0,FALSE,IID_IPicture,(LPVOID*)&gpPicture);	//

		// Now we have our IPicture OLE object we don't need the stream stuff anymore
		pstm->Release();
		GlobalFree(hGlobal);

		if (gpPicture == NULL)
			return AUT_OK;						// Something went wrong trying to get this image

		gpPicture->get_Handle((OLE_HANDLE*)&hBitmap);

		// Get a copy of the actual bitmap date from the OLE object
		hBitmap = (HBITMAP)CopyImage(hBitmap, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

		// Now we can release the IPicture OLE object
		gpPicture->Release();

		SendMessage(hWnd,STM_SETIMAGE,(WPARAM)IMAGE_BITMAP,(LPARAM)hBitmap);	// Set picture
		g_hSplashBitmap = hBitmap;				// Make a note so we can free the memory later
		SetWindowPos(hWnd,HWND_TOP,0,0,rect.right-rect.left,rect.bottom-rect.top,SWP_DRAWFRAME);							//
	}
	else	// SplashText()
	{
		char	szFont[65];
		int		CyPixels ,nSize = 12, nWeight = 400;
		HFONT	hfFont;
		HDC		h_dc;

		// CREATE static label full size of client area
		hWnd = CreateWindowEx(0,"static",vParams[1].szValue(),lblstyle,
			0,0,rect.right-rect.left,rect.bottom-rect.top,g_hWndSplash,NULL,NULL,NULL);

		h_dc = CreateDC("DISPLAY", NULL, NULL, NULL);					//
		SelectObject(h_dc,(HFONT)GetStockObject(DEFAULT_GUI_FONT));		// Get Default Font Name
		GetTextFace(h_dc, 64, szFont);									//
		CyPixels = GetDeviceCaps(h_dc,LOGPIXELSY);						// For Some Font Size Math
		DeleteDC(h_dc);

		if ( iNumParams>=8 )
		{
			if ( strlen(vParams[7].szValue()) >= 1 )
			strcpy(szFont,vParams[7].szValue());		// Font Name
		}
		if ( iNumParams>=9 )
		{
			if ( vParams[8].nValue() >= 6 )
				nSize = vParams[8].nValue();			// Font Size
		}
		if ( iNumParams>=10 )
		{
			if ( vParams[9].nValue() >= 0 && vParams[9].nValue() <= 1000 )
			nWeight = vParams[9].nValue();				// Font Weight
		}

		hfFont = CreateFont(0-(nSize*CyPixels)/72,0,0,0,nWeight,0,0,0,DEFAULT_CHARSET,
			OUT_TT_PRECIS,CLIP_DEFAULT_PRECIS,PROOF_QUALITY,FF_DONTCARE,szFont);		// Create Font
		SendMessage(hWnd,WM_SETFONT,(WPARAM)hfFont,MAKELPARAM(TRUE,0));					// Do Font
	}

	ShowWindow(g_hWndSplash,SW_SHOWNOACTIVATE);				// Show the Splash

	return AUT_OK;

} // Splash()


///////////////////////////////////////////////////////////////////////////////
// ProgressOn()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_ProgressOn(VectorVariant &vParams, Variant &vResult)
{
	return Progress(vParams, vParams.size(), 0);

} // ProgressOn()


///////////////////////////////////////////////////////////////////////////////
// ProgressOff()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_ProgressOff(VectorVariant &vParams, Variant &vResult)
{
	return Progress(vParams, vParams.size(), 1);

} // ProgressOff()


///////////////////////////////////////////////////////////////////////////////
// ProgressSet()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_ProgressSet(VectorVariant &vParams, Variant &vResult)
{
	uint	iNumParams = vParams.size();

	if (g_hWndProgBar != NULL)
	{
		if ( vParams[0].nValue() >= 0 && vParams[0].nValue() < 101 )
			SendMessage(g_hWndProgBar,PBM_SETPOS,(WPARAM)vParams[0].nValue(),(LPARAM)0);
		if ( iNumParams >= 2 && vParams[1].szValue()[0] != '\0' )
			SendMessage(g_hWndProgLblB,WM_SETTEXT,0,(LPARAM)vParams[1].szValue());
		if (iNumParams > 2)
			SendMessage(g_hWndProgLblA,WM_SETTEXT,0,(LPARAM)vParams[2].szValue());
	}
	return AUT_OK;

} // ProgressSet()


///////////////////////////////////////////////////////////////////////////////
// Progress()
//
// ProgressBar handler
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT	AutoIt_Script::Progress(VectorVariant &vParams, uint iNumParams, int nFlag)
{
	int			L = -1;										// Splash Left
	int			T = -1;										// Splash Top
	int			W, H, CyPixels;								// Window Width and Height
	int			style = WS_DISABLED|WS_POPUP|WS_CAPTION;	// Default window style
	int			xstyle = WS_EX_TOPMOST;
	AString		sLabelB = "";
	RECT		rect;
	HDC			h_dc;
	HFONT		hfFont;
	char		szFont[65];

	if (g_hWndProgress)			// If it exists, kill it...
	{
		DestroyWindow(g_hWndProgress);
		g_hWndProgress = NULL;	// Reset to a NULL state in case we exit below
	}

	if (nFlag==1)				// ProgressOff()
		return AUT_OK;

	H = 100;	// Progress window size (was 78 +)
	W = 300;									//

	if ( iNumParams >= 3 )
		sLabelB = vParams[2].szValue();		// optional sub text
	if ( iNumParams >= 4 )
		L = vParams[3].nValue();			// optional Left position
	if ( iNumParams >= 5 )
		T = vParams[4].nValue();			// optional Top position
	if ( iNumParams >= 6 )
	{
		if ( vParams[5].nValue() & 1 )
			style ^= WS_CAPTION;				// optional No title - remove WS_CAPTION

		if ( vParams[5].nValue() & 2 )			// optional Not on top - remove WS_EX_TOPMOST
			xstyle ^= WS_EX_TOPMOST;

		if ( vParams[5].nValue() & 16 )			// optional moveable - remove WS_DISABLED flag
			style ^= WS_DISABLED;
	}

	SystemParametersInfo(SPI_GETWORKAREA,0,&rect,0);	// Get Desktop rect
	if ( L == -1 )
		L = (rect.right - W)/2;							// Center Horizontally
	if ( T == -1 )
		T = (rect.bottom - H)/2;						// Center Vertically

	SetRect (&rect, 0, 0, W, H);
	AdjustWindowRectEx (&rect, style, FALSE, xstyle);

	// CREATE Main Progress Window
	g_hWndProgress = CreateWindowEx(xstyle,AUT_APPCLASS,vParams[0].szValue(),
		style,L,T,rect.right-rect.left,rect.bottom-rect.top,g_hWnd,NULL,NULL,NULL);

	GetClientRect(g_hWndProgress,&rect);				// for some math

	// CREATE Main label
	g_hWndProgLblA = CreateWindowEx(0,"static",vParams[1].szValue(),WS_CHILD|WS_VISIBLE|SS_LEFT,
		(rect.right-rect.left-281),4,1280,24,g_hWndProgress,NULL,NULL,NULL);

	h_dc = CreateDC("DISPLAY", NULL, NULL, NULL);					//
	SelectObject(h_dc,(HFONT)GetStockObject(DEFAULT_GUI_FONT));		// Get Default Font Name
	GetTextFace(h_dc, 64, szFont);									//
	CyPixels = GetDeviceCaps(h_dc,LOGPIXELSY);			// For Some Font Size Math
	DeleteDC(h_dc);

	hfFont = CreateFont(0-(10*CyPixels)/72,0,0,0,600,0,0,0,DEFAULT_CHARSET,				// Create a bigger
		OUT_TT_PRECIS,CLIP_DEFAULT_PRECIS,PROOF_QUALITY,FF_DONTCARE,szFont);			// bolder default
	SendMessage(g_hWndProgLblA,WM_SETFONT,(WPARAM)hfFont,MAKELPARAM(TRUE,0));			// GUI font.

	// CREATE Progress control
	g_hWndProgBar = CreateWindowEx(WS_EX_CLIENTEDGE,"msctls_progress32",NULL,WS_CHILD|WS_VISIBLE|PBS_SMOOTH,
        (rect.right-rect.left-260)/2,30,260,20, g_hWndProgress, NULL,NULL,NULL);
	SendMessage(g_hWndProgBar,PBM_SETRANGE,0,MAKELONG(0,100));	//
	SendMessage(g_hWndProgBar,PBM_SETSTEP,1,0);					// set some characteristics

	// CREATE Sub label
	g_hWndProgLblB = CreateWindowEx(0,"static",sLabelB.c_str(),WS_CHILD|WS_VISIBLE|SS_LEFT,
		(rect.right-rect.left-280),55,1280,50,g_hWndProgress,NULL,NULL,NULL);
	SendMessage(g_hWndProgLblB,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),MAKELPARAM(TRUE,0));

	ShowWindow(g_hWndProgress,SW_SHOWNOACTIVATE);				// Show the Progress

	return AUT_OK;

} // Progress()


///////////////////////////////////////////////////////////////////////////////
// UBound()
// Return number of dimensions or the size of dimensions
// $var = UBound($array, $dim_num)
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_UBound(VectorVariant &vParams, Variant &vResult)
{
	// One of the few cases we need to check if it's an array...
	if (vParams[0].isArray() == false)
	{
		vResult=0;
		SetFuncErrorCode(1);
	}
	else if (vParams.size() == 1)
	{
		vResult = vParams[0].ArrayGetBound(1);	// Get size of 1st dimension
	}
	else
	{ // vParams.size() == 2
		vResult = vParams[0].ArrayGetBound(vParams[1].nValue());
		if (vResult.nValue() == 0)
			SetFuncErrorCode(2);
	}
	return AUT_OK;

} // UBound()


///////////////////////////////////////////////////////////////////////////////
// MouseGetCursor()
//
// Returns the current mouse cursor 1 - 15
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_MouseGetCursor(VectorVariant &vParams, Variant &vResult)
{
	int			a;
	HCURSOR		ciMyCursor;
	HCURSOR		hcCursor[16];
	HWND		hWnd;
	POINT		pTemp;

	hcCursor[0] = LoadCursor(0,IDC_APPSTARTING);
	hcCursor[1] = LoadCursor(0,IDC_ARROW);
	hcCursor[2] = LoadCursor(0,IDC_CROSS);
	hcCursor[3] = LoadCursor(0,IDC_HELP);
	hcCursor[4] = LoadCursor(0,IDC_IBEAM);
	hcCursor[5] = LoadCursor(0,IDC_ICON);
	hcCursor[6] = LoadCursor(0,IDC_NO);
	hcCursor[7] = LoadCursor(0,IDC_SIZE);
	hcCursor[8] = LoadCursor(0,IDC_SIZEALL);
	hcCursor[9] = LoadCursor(0,IDC_SIZENESW);
	hcCursor[10] = LoadCursor(0,IDC_SIZENS);
	hcCursor[11] = LoadCursor(0,IDC_SIZENWSE);
	hcCursor[12] = LoadCursor(0,IDC_SIZEWE);
	hcCursor[13] = LoadCursor(0,IDC_UPARROW);
	hcCursor[14] = LoadCursor(0,IDC_WAIT);

	GetCursorPos(&pTemp);
	hWnd = WindowFromPoint(pTemp);
	AttachThreadInput(GetCurrentThreadId(),GetWindowThreadProcessId(hWnd, NULL), TRUE);
	ciMyCursor = GetCursor();
	AttachThreadInput(GetCurrentThreadId(),GetWindowThreadProcessId(hWnd, NULL), FALSE);

	for ( a=0;a<=14; ++a )
	{
        if( ciMyCursor && ciMyCursor == hcCursor[a] )
			break;
	}
	if (a <= 14 )
		vResult = ++a;
	else
		vResult = 0;

	return AUT_OK;

} // MouseGetCursor()


///////////////////////////////////////////////////////////////////////////////
// InputBox()
// InputBox("Title", "Prompt" [, "Default" [, "Password" [, Width, Height [, Left, Top]]]]
// Gets user input text
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_InputBox(VectorVariant &vParams, Variant &vResult)
{
	uint		iNumParams = vParams.size();
	int			iRetVal, i, iTmp;
	double		fTmp;
	char		cTmp;
	CInputBox	aDlg;

	switch (iNumParams) {
		case 9: // timeout
 			fTmp = vParams[8].nValue();
 			if (fTmp <= 0.0)
 				fTmp = -1.0;	// sentinal value for invalid value
 			aDlg.m_timeout = fTmp;

		case 8: // with left & top
			aDlg.m_top = vParams[7].nValue();
			aDlg.m_left = vParams[6].nValue();

		case 6: // with width & height
			aDlg.m_height = vParams[5].nValue();
			aDlg.m_width = vParams[4].nValue();

		case 4:	// with password
			cTmp = vParams[3].szValue()[0];
 			if (cTmp != '\0' && !Util_IsSpace(cTmp))
 				aDlg.m_password = cTmp;
			if (cTmp != '\0')
				for (i=1; (cTmp = vParams[3].szValue()[i])!= '\0'; ++i) {
					switch (cTmp) {
					case 'M': case 'm':
						// maditory
						aDlg.m_flags |= aDlg.ibf_notnull;
						break;
					case '0': case '1': case '2': case '3':
					case '4': case '5': case '6': case '7':
					case '8': case '9': // digits for maximum line length
 						iTmp = vParams[3].szValue()[i] - '0'; // store digit value;
 						while (isdigit(vParams[3].szValue()[i+1]))
 							iTmp = iTmp * 10 + vParams[3].szValue()[++i]-'0';
 						aDlg.m_maxlen=iTmp;
						break;
					default:
						SetFuncErrorCode(3);
						return AUT_OK;
					} // switch cTmp
				} // for i

		case 3: // with default
			aDlg.m_strInputText = vParams[2].szValue();

		case 2:	// title and prompt only
			aDlg.m_title = vParams[0].szValue();
			aDlg.m_strPrompt = vParams[1].szValue();
			iRetVal = aDlg.DoModal(g_hInstance, NULL);

			if (iRetVal==IDOK)
				vResult = aDlg.m_strInputText.c_str();
			else if (iRetVal == IDCANCEL) {
				SetFuncErrorCode(1);	// cancel pressed
				vResult = "";
			} else if (iRetVal == IDABORT) { // timeout
 				SetFuncErrorCode(2);
 				vResult = "";
			} else {
				SetFuncErrorCode(3);	// error creating dialog
				vResult = "";
			}
			return AUT_OK;

		default:
 			// Check for x without y, width without height
 			FatalError(IDS_AUT_E_FUNCTIONNUMPARAMS);
 			return AUT_ERR;
	}
} // InputBox()


///////////////////////////////////////////////////////////////////////////////
// SoundSetWaveVolume()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_SoundSetWaveVolume(VectorVariant &vParams, Variant &vResult)
{
	int nVol = vParams[0].nValue();
	if((nVol>=0)&&(nVol<101))
	{
		WORD wVolume = (WORD)(0xFFFF*nVol/100);
		waveOutSetVolume(0,MAKELONG(wVolume,wVolume));
	}
	else
		SetFuncErrorCode(1);

	return AUT_OK;

} // SoundSetWaveVolume()


///////////////////////////////////////////////////////////////////////////////
// TimerInit()
// Returns a floating point value that is a baseline system time.
// Starts tracking a high performance counter allowing for accurate timers.
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_TimerInit(VectorVariant &vParams, Variant &vResult)
{
	__int64 now;

	if (!QueryPerformanceCounter((LARGE_INTEGER *)&now))
		return AUT_OK;

	vResult = (double)now;

	return AUT_OK;

} // TimerInit()

///////////////////////////////////////////////////////////////////////////////
// TimerDiff(Baseline)
// Takes the time difference between now and Baseline and returns the result.
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_TimerDiff(VectorVariant &vParams, Variant &vResult)
{
	__int64 freq, now;

	if (!QueryPerformanceFrequency((LARGE_INTEGER *)&freq))
		return AUT_OK;

	if (!QueryPerformanceCounter((LARGE_INTEGER *)&now))
		return AUT_OK;

	vResult = (((double)now - vParams[0].fValue()) / (double)freq) * 1000.0;

	return AUT_OK;

} // TimerDiff()


///////////////////////////////////////////////////////////////////////////////
// TrayTip("title", "text", timeout, [options])
// Creates a balloon tip near the AutoIt icon with specified text, title and icon on Windows 2000 and later.
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_TrayTip(VectorVariant &vParams, Variant &vResult)
{
	uint	iNumParams = vParams.size();

	if (g_oVersion.IsWin2000orLater()) {

		struct MYNOTIFYICONDATA : public _NOTIFYICONDATAA { // Not wide character compliant.
			DWORD dwState;
		    DWORD dwStateMask;
			CHAR   szTip[64];			// Padding the size, do not use, may cause problems.
			CHAR  szInfo[256];
			union {
				UINT  uTimeout;
				UINT  uVersion;
			};
			CHAR  szInfoTitle[64];
			DWORD dwInfoFlags;
		} nic;

		nic.cbSize	= sizeof(nic);
		nic.uFlags = 0x00000010;	// NIF_INFO
		nic.hWnd = g_hWnd;
		nic.uID = AUT_NOTIFY_ICON_ID;

		strncpy(nic.szInfoTitle, vParams[0].szValue(), 63); // Emtpy title means no title.
		nic.szInfoTitle[63] = '\0';

		strncpy(nic.szInfo, vParams[1].szValue(), 255);	// Empty string doesn't display a balloon, so that's okay.
		nic.szInfo[255] = '\0';

		nic.uTimeout = vParams[2].nValue() * 1000;

		if (iNumParams > 3)
			nic.dwInfoFlags = vParams[3].nValue();
		else
			nic.dwInfoFlags = 0;

		Shell_NotifyIcon(NIM_MODIFY, &nic);
	}
	return AUT_OK;

} // TrayTip()


///////////////////////////////////////////////////////////////////////////////
// Sleep(ms)
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_Sleep(VectorVariant &vParams, Variant &vResult)
{
	// Handle sleep 0 as a special
	if (vParams[0].nValue() <= 0)
		::Sleep(0);
	else
	{
		m_tWinTimerStarted	= timeGetTime();	// Using the WinWait timer - go fig.
		m_nWinWaitTimeout	= vParams[0].nValue();
		m_nCurrentOperation = AUT_SLEEP;
		Execute();
	}

	return AUT_OK;

} // Sleep()


///////////////////////////////////////////////////////////////////////////////
// AutoItSetOption("option", value)
// Sets various parameters (systray icon, title match modes, mouse modes etc)
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_AutoItSetOption(VectorVariant &vParams, Variant &vResult)
{
	const char *szOption = vParams[0].szValue();
	int			nValue = vParams[1].nValue();

	if ( !stricmp(szOption, "CaretCoordMode") )				// CaretCoordMode
	{
		vResult = (int)m_nCoordCaretMode;	// Store current value
		m_nCoordCaretMode = nValue;
	}
	else if ( !stricmp(szOption, "ColorMode") )				// ColorMode
	{
		vResult = (int)m_bColorModeBGR;

		if (nValue == 0)
		{
			m_bColorModeBGR = false;

			#ifdef AUT_CONFIG_GUI							// Is GUI enabled?
				g_oGUI.m_bColorModeBGR = false;
			#endif
		}
		else
		{
			m_bColorModeBGR = true;
			#ifdef AUT_CONFIG_GUI							// Is GUI enabled?
				g_oGUI.m_bColorModeBGR = true;
			#endif
		}
	}
	else if ( !stricmp(szOption, "FtpBinaryMode") )			// GUICoordMode
	{
		vResult = (int)m_bFtpBinaryMode;	// Store current value

		if (nValue == 0)
			m_bFtpBinaryMode = false;
		else
			m_bFtpBinaryMode = true;
	}
	else if ( !stricmp(szOption, "GUICloseOnESC") )			// GUICloseOnESC
	{
		#ifdef AUT_CONFIG_GUI								// Is GUI enabled?
			vResult = g_oGUI.m_bCloseOnESC;
			if (nValue == 0 )
				g_oGUI.m_bCloseOnESC = false;
			else
				g_oGUI.m_bCloseOnESC = true;
		#endif
	}
	else if ( !stricmp(szOption, "GUICoordMode") )			// GUICoordMode
	{
		#ifdef AUT_CONFIG_GUI							// Is GUI enabled?
			vResult = g_oGUI.m_nGUICoordMode;
			if (nValue >= 0 && nValue <= 2)
				g_oGUI.m_nGUICoordMode = nValue;
			else
				g_oGUI.m_nGUICoordMode = 1;
		#endif
	}
	else if ( !stricmp(szOption, "GUIOnEventMode") )	// GUIOnEventMode
	{
		#ifdef AUT_CONFIG_GUI							// Is GUI enabled?
			vResult = (int)g_oGUI.m_bGuiEventEnabled;
		if (nValue == 0)
			g_oGUI.m_bGuiEventEnabled = false;
		else
			g_oGUI.m_bGuiEventEnabled = true;
		#endif
	}
	else if ( !stricmp(szOption, "GUIResizeMode") )			// GUIResizeMode
	{
		#ifdef AUT_CONFIG_GUI							// Is GUI enabled?
			vResult = g_oGUI.m_nGUIResizeMode;
			if (nValue >= 0 && nValue < 1023)
				g_oGUI.m_nGUIResizeMode = nValue;
			else
				g_oGUI.m_nGUIResizeMode = 0;
		#endif
	}
/*	else if ( !stricmp(szOption, "GuiTaskbarEntry") )		// GuiTaskbarEntry
	{
		#ifdef AUT_CONFIG_GUI							// Is GUI enabled?
			vResult = g_oGUI.m_bGuiTaskbarEntry;
			if (nValue >= 0 && nValue <= 2)
				g_oGUI.m_bGuiTaskbarEntry = (nValue!=0);
			else
				g_oGUI.m_bGuiTaskbarEntry = false;
		#endif
	}
	else if ( !stricmp(szOption, "GuiTrayMode") )			// GuiTrayMode
	{
		#ifdef AUT_CONFIG_GUI							// Is GUI enabled?
			vResult = g_oGUI.m_nGUITrayMode;
			if (nValue >= 1 && nValue <= 4)
			{
				g_oGUI.m_nGUITrayMode = nValue;
				if (g_oGUI.m_bGUIDisplayed && nValue == 4)	// mode 4 only for start in tray
					nValue = 1;
			}
			else
				g_oGUI.m_nGUITrayMode = 0;	// default
		#endif
	}
*/	else if ( !stricmp(szOption, "ExpandEnvStrings") )		// ExpandEnvStrings
	{
		vResult = (int)m_bExpandEnvStrings;	// Store current value

		if (nValue == 0)
			m_bExpandEnvStrings = false;
		else
			m_bExpandEnvStrings = true;
	}
	else if ( !stricmp(szOption, "ExpandVarStrings") )		// ExpandEnvStrings
	{
		vResult = (int)m_bExpandVarStrings;	// Store current value

		if (nValue == 0)
			m_bExpandVarStrings = false;
		else
			m_bExpandVarStrings = true;
	}
	else if ( !stricmp(szOption, "MouseClickDelay") )		// MouseClickDelay
	{
		vResult = (int)m_nMouseClickDelay;	// Store current value

		if (nValue >= 0)
			m_nMouseClickDelay = nValue;
	}
	else if ( !stricmp(szOption, "MouseClickDownDelay") )	// MouseClickDownDelay
	{
		vResult = (int)m_nMouseClickDownDelay;	// Store current value

		if (nValue >= 0)
			m_nMouseClickDownDelay = nValue;
	}
	else if ( !stricmp(szOption, "MouseClickDragDelay") )	// MouseClickDragDelay
	{
		vResult = (int)m_nMouseClickDragDelay;	// Store current value

		if (nValue >= 0)
			m_nMouseClickDragDelay = nValue;
	}
	else if ( !stricmp(szOption, "MouseCoordMode") )		// MouseCoordMode
	{
		vResult = (int)m_nCoordMouseMode;	// Store current value
		m_nCoordMouseMode = nValue;
	}
	else if ( !stricmp(szOption, "MustDeclareVars") )		// MustDeclareVars
	{
		vResult = (int)m_bMustDeclareVars;	// Store current value

		if (nValue == 0)
			m_bMustDeclareVars = false;
		else
			m_bMustDeclareVars = true;
	}
	else if ( !stricmp(szOption, "OnExitFunc") )			// OnExitFunc
	{
		vResult = m_sOnExitFunc.c_str();	// Store current value
		m_sOnExitFunc = vParams[1].szValue();

	}
	else if ( !stricmp(szOption, "PixelCoordMode") )		// PixelCoordMode
	{
		vResult = (int)m_nCoordPixelMode;	// Store current value
		m_nCoordPixelMode = nValue;
	}
	else if ( !stricmp(szOption, "RunErrorsFatal") )		// RunErrorsFatal
	{
		vResult = (int)m_bRunErrorsFatal;	// Store current value

		if (nValue == 0)
			m_bRunErrorsFatal = false;
		else
			m_bRunErrorsFatal = true;
	}
	else if ( !stricmp(szOption, "SendAttachMode") )		// SendAttachMode
	{
		vResult = (int)m_oSendKeys.m_bAttachMode;	// Store current value

		if (nValue == 0)
			m_oSendKeys.SetAttachMode(false);
		else
			m_oSendKeys.SetAttachMode(true);
	}
	else if ( !stricmp(szOption, "SendCapslockMode") )		// SendCapslockMode
	{
		vResult = (int)m_oSendKeys.m_bStoreCapslockMode;	// Store current value

		if (nValue == 0)
			m_oSendKeys.SetStoreCapslockMode(false);
		else
			m_oSendKeys.SetStoreCapslockMode(true);
	}
	else if ( !stricmp(szOption, "SendKeyDelay") )			// SendKeyDelay
	{
		vResult = (int)m_oSendKeys.m_nKeyDelay;	// Store current value

		if (nValue >= -1)
			m_oSendKeys.SetKeyDelay( nValue );
	}
	else if ( !stricmp(szOption, "SendKeyDownDelay") )		// SendKeyDownDelay
	{
		vResult = (int)m_oSendKeys.m_nKeyDownDelay;	// Store current value

		if (nValue >= -1)
			m_oSendKeys.SetKeyDownDelay( nValue );
	}
	else if ( !stricmp(szOption, "TrayIconDebug") )			// TrayIconDebug
	{
		vResult = (int)g_bTrayIconDebug;	// Store current value

		if (nValue == 0)
			g_bTrayIconDebug = false;
		else
			g_bTrayIconDebug = true;
	}
	else if ( !stricmp(szOption, "TrayIconHide") )			// TrayIconHide
	{
		vResult = (int)!g_bTrayIcon;	// Store current value

		if (nValue == 0)
			g_oApplication.CreateTrayIcon();
		else
			g_oApplication.DestroyTrayIcon();
	}
	else if (!stricmp(szOption, "WinSearchChildren"))
	{
		vResult = (int)m_bWinSearchChildren;	// Store current value

		if (nValue == 0)
			m_bWinSearchChildren = false;
		else
			m_bWinSearchChildren = true;
	}
	else if ( !stricmp(szOption, "WinWaitDelay") )			// WinWaitDelay
	{
		vResult = (int)m_nWinWaitDelay;	// Store current value

		if (nValue >= 0)
			m_nWinWaitDelay = nValue;
	}
	else if ( !stricmp(szOption, "WinDetectHiddenText") )	// WinDetectHiddenText
	{
		vResult = (int)m_bDetectHiddenText;	// Store current value

		if ( nValue == 0)
			m_bDetectHiddenText = false;
		else
			m_bDetectHiddenText = true;
	}
	else if ( !stricmp(szOption, "WinTextMatchMode") )		// WinTextMatchMode
	{
		vResult = (int)m_nWindowSearchTextMode;	// Store current value

		if (nValue >= 1 && nValue <= 2)
			m_nWindowSearchTextMode = nValue;
		else
			m_nWindowSearchTextMode = 1;
	}
	else if ( !stricmp(szOption, "WinTitleMatchMode") )		// WinTitleMatchMode
	{
		vResult = (int)m_nWindowSearchMatchMode;	// Store current value

		if (nValue >= 1 && nValue <= 4)
			m_nWindowSearchMatchMode = nValue;
		else
			m_nWindowSearchMatchMode = 1;
	}
	else
	{
		// No matching option if we got here
		FatalError(IDS_AUT_E_BADOPTION);
		return AUT_ERR;
	}

	return AUT_OK;

} // AutoItSetOption()


///////////////////////////////////////////////////////////////////////////////
// HotKeySet(key ([mod]key, function)
//
// Sets a hotkey
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_HotKeySet(VectorVariant &vParams, Variant &vResult)
{
	uint	iNumParams = vParams.size();
	int		nTemp1, nTemp2, nTemp3, nTemp4;
	bool	bShift, bControl, bAlt, bWin;
	UINT	mods = 0;
	UINT	vk;
	int		nFreeHandle;
	LPARAM	lParam;


	// Unless blank, check that the requested user function exists
	if (iNumParams >= 2 && Parser_FindUserFunction(vParams[1].szValue(), nTemp1, nTemp2, nTemp3, nTemp4) == false)
	{
		FatalError(IDS_AUT_E_UNKNOWNUSERFUNC);
		return AUT_ERR;
	}


	// Get the virtual key code and modifiers
	if (m_oSendKeys.GetSingleVKandMods(vParams[0].szValue(), vk, bShift, bControl, bAlt, bWin) == false)
	{
		vResult = 0;							// Error, default is 1
		return AUT_OK;
	}

	if (bShift)
		mods |= MOD_SHIFT;
	if (bControl)
		mods |= MOD_CONTROL;
	if (bAlt)
		mods |= MOD_ALT;
	if (bWin)
		mods |= MOD_WIN;


	// Create an WM_HOTKEY compatible lParam for the modifiers and the VK
	lParam = (vk << 16) | mods;

	// Is this a duplicate or an unset (Function is blank)
	for (nFreeHandle = 0; nFreeHandle < AUT_MAXHOTKEYS; ++nFreeHandle)
	{
		if (m_HotKeyDetails[nFreeHandle] != NULL)
		{
			if (m_HotKeyDetails[nFreeHandle]->lParam == lParam)
			{
				// Are we reusing or unregistering?
				if (iNumParams == 1)
				{
					UnregisterHotKey(g_hWnd, (int)m_HotKeyDetails[nFreeHandle]->wParam);	// Unregister
					delete m_HotKeyDetails[nFreeHandle];	// Delete memory
					m_HotKeyDetails[nFreeHandle] = NULL;	// Mark as empty
				}
				else
					m_HotKeyDetails[nFreeHandle]->sFunction = vParams[1].szValue();	// Reuse

				return AUT_OK;
			}
		}
	}

	// If we are here and numParams == 1 (want to unregister) then we failed to find our
	// key to unregister in the code above == error
	if (iNumParams == 1)
	{
		vResult = 0;
		return AUT_OK;
	}

	// Find a free handle
	for (nFreeHandle = 0; nFreeHandle < AUT_MAXHOTKEYS; ++nFreeHandle)
	{
		if (m_HotKeyDetails[nFreeHandle] == NULL)
		{
			m_HotKeyDetails[nFreeHandle] = new HotKeyDetails;	// Create new entry
			m_HotKeyDetails[nFreeHandle]->wParam = 0x0000 + nFreeHandle;
			m_HotKeyDetails[nFreeHandle]->lParam = lParam;
			m_HotKeyDetails[nFreeHandle]->sFunction = vParams[1].szValue();

			if (!RegisterHotKey(g_hWnd, (int)m_HotKeyDetails[nFreeHandle]->wParam, mods, vk))
			{
				// Failed!
				delete m_HotKeyDetails[nFreeHandle];	// Delete memory
				m_HotKeyDetails[nFreeHandle] = NULL;	// Mark as empty
				vResult = 0;							// Error, default is 1
			}

			return AUT_OK;						// Return (sucessfull or not)
		}
	}

	vResult = 0;								// No free handles, error, default is 1

	return AUT_OK;

} // HotKeySet()


///////////////////////////////////////////////////////////////////////////////
// MemGetStats()
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_MemGetStats(VectorVariant &vParams, Variant &vResult)
{
	MEMORYSTATUS mem;
	GlobalMemoryStatus(&mem);

	Variant	*pvTemp;

	Util_VariantArrayDim(&vResult, 7);

	// We have to cast all of these because of the overloaded operator= in the Variant class
	pvTemp = Util_VariantArrayGetRef(&vResult, 0);
	*pvTemp = (double)mem.dwMemoryLoad;

	pvTemp = Util_VariantArrayGetRef(&vResult, 1);
	*pvTemp = (double)mem.dwTotalPhys / 1024.0;			// Conv to KB

	pvTemp = Util_VariantArrayGetRef(&vResult, 2);
	*pvTemp = (double)mem.dwAvailPhys / 1024.0;

	pvTemp = Util_VariantArrayGetRef(&vResult, 3);
	*pvTemp = (double)mem.dwTotalPageFile / 1024.0;

	pvTemp = Util_VariantArrayGetRef(&vResult, 4);
	*pvTemp = (double)mem.dwAvailPageFile / 1024.0;

	pvTemp = Util_VariantArrayGetRef(&vResult, 5);
	*pvTemp = (double)mem.dwTotalVirtual / 1024.0;

	pvTemp = Util_VariantArrayGetRef(&vResult, 6);
	*pvTemp = (double)mem.dwAvailVirtual / 1024.0;

	return AUT_OK;

}	// MemGetStats()


///////////////////////////////////////////////////////////////////////////////
// MsgBox( type, "title", "text" [,timeout] )
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_MsgBox(VectorVariant &vParams, Variant &vResult)
{
	if (vParams.size() == 4)
		vResult = Util_MessageBoxEx(NULL, vParams[2].szValue(), vParams[1].szValue(), (UINT)vParams[0].nValue() | MB_SETFOREGROUND, vParams[3].nValue() * 1000);
	else
		vResult = MessageBox(NULL, vParams[2].szValue(), vParams[1].szValue(), (UINT)vParams[0].nValue() | MB_SETFOREGROUND);
	return AUT_OK;

} // F_MsgBox()


///////////////////////////////////////////////////////////////////////////////
// Send()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_Send(VectorVariant &vParams, Variant &vResult)
{
	if (vParams.size() == 2)
	{
		if (vParams[1].nValue() == 0)
			m_oSendKeys.Send(vParams[0].szValue());		// Normal send
		else
			m_oSendKeys.SendRaw(vParams[0].szValue());	// Raw send
	}
	else
		m_oSendKeys.Send(vParams[0].szValue());	// Normal send

	return AUT_OK;

} // Send()


///////////////////////////////////////////////////////////////////////////////
// SetError()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_SetError(VectorVariant &vParams, Variant &vResult)
{
	SetFuncErrorCode(vParams[0].nValue());
	return AUT_OK;

} // SetError()


///////////////////////////////////////////////////////////////////////////////
// SetExtended()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_SetExtended(VectorVariant &vParams, Variant &vResult)
{
	SetFuncExtCode(vParams[0].nValue());
	return AUT_OK;

} // SetError()


///////////////////////////////////////////////////////////////////////////////
// SoundPlay()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_SoundPlay(VectorVariant &vParams, Variant &vResult)
{
	if (vParams.size() == 2 && vParams[1].nValue() == 1)
		Util_SoundPlay(vParams[0].szValue(), true);
	else
		Util_SoundPlay(vParams[0].szValue(), false);
	return AUT_OK;

} // SoundPlay()


///////////////////////////////////////////////////////////////////////////////
// Break()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_Break(VectorVariant &vParams, Variant &vResult)
{
	if (vParams[0].nValue() == 0)
		g_bBreakEnabled = false;
	else
		g_bBreakEnabled = true;
	return AUT_OK;

} // Break()


///////////////////////////////////////////////////////////////////////////////
// Call()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_Call(VectorVariant &vParams, Variant &vResult)
{
	int nTemp1, nTemp2, nTemp3, nTemp4;

	// Check that this user function exists
	if (Parser_FindUserFunction(vParams[0].szValue(), nTemp1, nTemp2, nTemp3, nTemp4) == false)
	{
		SetFuncErrorCode(1);				// Silent error even though function not valid
		return AUT_OK;						// As will probably be used this way
	}
	else
	{
		m_vUserRetVal = 1;					// Default return value is 1
		SaveExecute(nTemp1+1, true, false);	// Run the user function (line after the Func declaration)
		vResult = m_vUserRetVal;			// Get return value (0 = timed out)
		return AUT_OK;
	}

} // Call()


///////////////////////////////////////////////////////////////////////////////
// Eval()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_Eval(VectorVariant &vParams, Variant &vResult)
{
	bool	bConst = false;

 	if (g_oVarTable.isDeclared(vParams[0].szValue()))
 	{
 		Variant *pvTemp;
 		g_oVarTable.GetRef(vParams[0].szValue(), &pvTemp, bConst);
 		vResult = *pvTemp;
 		return AUT_OK;
 	}
 	else
 	{
 		SetFuncErrorCode(1);			// Silent error even though variable not valid
 		vResult = "";
 		return AUT_OK;
 	}

} // Eval()


//////////////////////////////////////////////////////////////////////////
// F_Assign("variable", "data")
//
// Assign( "varname", "value" [,flag])
//
// binary flag: 0=any, 1=local, 2=global, 4=no create
//
// Assigns the variable in the first parameter to the data in the second parameter.
// This is a compliment to Eval()
//////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_Assign(VectorVariant &vParams, Variant &vResult)
{
	Variant *pvTemp;
	int		nReqScope = VARTABLE_ANY;
	bool	bCreate = true;
	bool	bConst = false;

	if (vParams.size() == 3)
	{
		if (vParams[2].nValue() & 1)
			nReqScope = VARTABLE_FORCELOCAL;
		if (vParams[2].nValue() & 2)
			nReqScope = VARTABLE_FORCEGLOBAL;
		if (vParams[2].nValue() & 4)
			bCreate = false;
	}

	// Get a reference to the variable in the requested scope, if it doesn't exist, then create it.
	g_oVarTable.GetRef(vParams[0].szValue(), &pvTemp, bConst, nReqScope);
	if (pvTemp == NULL)
	{
		if (bCreate)
			g_oVarTable.Assign(vParams[0].szValue(), vParams[1], false, nReqScope);
		else
			vResult = 0;						// Default is 1
	}
	else
		*pvTemp = vParams[1];

	return AUT_OK;

}	// F_Assign()


///////////////////////////////////////////////////////////////////////////////
// IsDeclared
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_IsDeclared(VectorVariant &vParams, Variant &vResult)
{
 	vResult = g_oVarTable.isDeclared(vParams[0].szValue());
 	return AUT_OK;

} // IsDeclared()


//////////////////////////////////////////////////////////////////////////
// F_ConsoleWrite("text")
//
// Writes to the console so editors like SciTE can
// read the data
//////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_ConsoleWrite(VectorVariant &vParams, Variant &vResult)
{
	printf("%s", vParams[0].szValue());
	fflush(stdout);

	return AUT_OK;

}	// F_ConsoleWrite()


//////////////////////////////////////////////////////////////////////////
// ConvertCoords()
//
// Helps convert coordinates to screen, active window and client relative
//////////////////////////////////////////////////////////////////////////

void AutoIt_Script::ConvertCoords(int nCoordMode, POINT &pt)
{
	HWND	hFore = GetForegroundWindow();
	RECT	rect;

	if (nCoordMode == AUT_COORDMODE_WINDOW)
	{
		GetWindowRect(hFore, &rect);
		pt.x = rect.left;
		pt.y = rect.top;
	}
	else if (nCoordMode == AUT_COORDMODE_CLIENT)
	{
		pt.x = 0;
		pt.y = 0;
		ClientToScreen(hFore, &pt);
	}
	else
	{
		// Screen mode
		pt.x = 0;
		pt.y = 0;
	}

}	// ConvertCoords()

