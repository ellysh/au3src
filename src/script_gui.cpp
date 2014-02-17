
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
// script_gui.cpp
//
// Contains GUI routines.  Part of script.cpp
//
///////////////////////////////////////////////////////////////////////////////


// Includes
#include "StdAfx.h"								// Pre-compiled headers

#ifndef _MSC_VER								// Includes for non-MS compilers
	#include <stdio.h>
	#include <windows.h>
#endif

#include "AutoIt.h"								// Autoit values, macros and config options

#ifdef AUT_CONFIG_GUI							// Is GUI enabled?

#include "script.h"
#include "globaldata.h"
#include "resources\resource.h"
#include "utility.h"


///////////////////////////////////////////////////////////////////////////////
// GUISwitch()
//
// GuiSwitch( guihandle )
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_GUISwitch(VectorVariant &vParams, Variant &vResult)
{
	vResult = g_oGUI.SwitchGUI(vParams[0].hWnd());
	return AUT_OK;

} // GUISwitch


///////////////////////////////////////////////////////////////////////////////
// GUIGetMsg()
//
// GuiGetMsg( [1] )
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_GUIGetMsg(VectorVariant &vParams, Variant &vResult)
{
	GUIEVENT	Event;
	Variant		*pvTemp;
	Variant		vTemp;


	// If we are in event mode then

	// Don't try and get the message if the OnEvent mode is active
	if ( g_oGUI.m_bGuiEventEnabled == true || g_oGUI.GetMsg(Event) == false)
	{
		// No events - add default values
		Event.nGlobalID	= 0;
		Event.sCallback	= "";
		Event.hWnd		= NULL;
		Event.hCtrl		= NULL;
		Event.nCursorX	= 0;
		Event.nCursorY	= 0;
	}
	if ( g_oGUI.m_bGuiEventEnabled == true)
		SetFuncErrorCode(1);

	if (vParams.size() > 0 && vParams[0].nValue() == 1)
	{
		// Advanced return

		// Setup vResult as an Array to hold the 3 values we want to return
		Util_VariantArrayDim(&vResult, 5);

		pvTemp = Util_VariantArrayGetRef(&vResult, 0);	// First element
		*pvTemp = Event.nGlobalID;

		pvTemp = Util_VariantArrayGetRef(&vResult, 1);
		*pvTemp = Event.hWnd;

		pvTemp = Util_VariantArrayGetRef(&vResult, 2);
		*pvTemp = Event.hCtrl;

		pvTemp = Util_VariantArrayGetRef(&vResult, 3);
		*pvTemp = Event.nCursorX;

		pvTemp = Util_VariantArrayGetRef(&vResult, 4);
		*pvTemp = Event.nCursorY;
	}
	else
	{
		// Simple return
		vResult = Event.nGlobalID;
	}

	return AUT_OK;

} // GUIGetMsg()


///////////////////////////////////////////////////////////////////////////////
// GUICreate(Title [,x [,y [,w [,h [,style [,exStyle]]]]]])
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_GUICreate(VectorVariant &vParams, Variant &vResult)
{
	uint	iNumParams = vParams.size();

	int		nStyle		= -1;
	int		nExStyle	= -1;
	int		nW			= -1;
	int		nH			= -1;
	int		nX			= -1;
	int		nY			= -1;
	HWND hParent		= NULL;

	if (iNumParams > 7) hParent = vParams[7].hWnd();
	if (iNumParams > 6) nExStyle = vParams[6].nValue();
	if (iNumParams > 5) nStyle = vParams[5].nValue();
	if (iNumParams > 4) nY = vParams[4].nValue();
	if (iNumParams > 3) nX = vParams[3].nValue();
	if (iNumParams > 2) nH = vParams[2].nValue();
	if (iNumParams > 1) nW = vParams[1].nValue();

	HWND hWnd= g_oGUI.CreateGUI(vParams[0].szValue(), nX, nY,nW, nH, nStyle, nExStyle, hParent);

	if (hWnd == NULL)
	{
		SetFuncErrorCode(1);
		vResult = (HWND)NULL;					// window can not be created
	}
	else
		vResult = hWnd;

	return AUT_OK;

} // GUICreate()


///////////////////////////////////////////////////////////////////////////////
// GUISetState([state [,winhandle]])
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_GUISetState(VectorVariant &vParams, Variant &vResult)
{
	int nState = SW_SHOW;
	HWND hWnd = NULL;
 	if (vParams.size() > 1) hWnd = vParams[1].hWnd();
	if (vParams.size() > 0) nState = vParams[0].nValue();

	vResult = g_oGUI.SetState(nState, hWnd);

	return AUT_OK;

} // GUISetState()


///////////////////////////////////////////////////////////////////////////////
// GUISetOnEvent(eventid, "callback" [,winhandle])
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_GUISetOnEvent(VectorVariant &vParams, Variant &vResult)
{
	int		nTemp1, nTemp2, nTemp3, nTemp4;

	// Check that this user function exists
	if (Parser_FindUserFunction(vParams[1].szValue(), nTemp1, nTemp2, nTemp3, nTemp4) == false)
	{
		FatalError(IDS_AUT_E_UNKNOWNUSERFUNC);
		return AUT_ERR;
	}
	else
	{
		HWND hWnd = NULL;
		if (vParams.size() > 2) hWnd = vParams[2].hWnd();
		vResult = g_oGUI.SetOnEvent(vParams[0].nValue(), vParams[1].szValue(), hWnd);

		return AUT_OK;
	}

} // GUISetOnEvent()


//////////////////////////////////////////////////////////////////////////////
// GUISetBkColor( color [,winhandle] )
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_GUISetBkColor(VectorVariant &vParams, Variant &vResult)
{
	HWND hWnd = NULL;
	if (vParams.size() > 1) hWnd = vParams[1].hWnd();

	vResult = g_oGUI.CreateGuiEx(NULL, vParams[0].nValue(),NULL, -1, hWnd);
	return AUT_OK;

} // GUISetBkColor()


//////////////////////////////////////////////////////////////////////////////
// GUISetIcon(filename [,iconid [,winhandle]] )
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_GUISetIcon(VectorVariant &vParams, Variant &vResult)
{
	int		nIcon	= -1;
	HWND	hWnd = NULL;

	if (vParams.size() > 2) hWnd = vParams[2].hWnd();
	if (vParams.size() > 1) nIcon = vParams[1].nValue();

	vResult = g_oGUI.CreateGuiEx(NULL, -1, vParams[0].szValue(), nIcon, hWnd);
	return AUT_OK;

} // GUISetIcon()


//////////////////////////////////////////////////////////////////////////////
// GUISetHelp(filename [,winhandle])
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_GUISetHelp(VectorVariant &vParams, Variant &vResult)
{
	HWND hWnd = NULL;
	if (vParams.size() > 1) hWnd = vParams[1].hWnd();

	vResult = g_oGUI.CreateGuiEx(vParams[0].szValue(), -1,NULL, -1, hWnd);
	return AUT_OK;

} // GUISetHelp()


///////////////////////////////////////////////////////////////////////////////
// GUISetCursor( [CursorID [,nOverride [,winhandle]]] )
//
// Change the mouse cursor to CursorID, or revert mouse to original cursor.
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_GUISetCursor(VectorVariant &vParams, Variant &vResult)
{
	uint	iNumParams = vParams.size();
	int		n = 2;								// Default is ID 2
	int		nOverride = 0;
	HWND	hWnd = NULL;

	if (iNumParams > 0)	n = vParams[0].nValue();
	if (iNumParams > 1)	nOverride = vParams[1].nValue();
	if (iNumParams > 2) hWnd = vParams[2].hWnd();

	g_oGUI.SetCursor(n, nOverride, hWnd);

	return AUT_OK;

} // GUISetCursor()


//////////////////////////////////////////////////////////////////////////////
// GUISetFont(size, [weight [,attribute [,font]]])
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_GUISetFont(VectorVariant &vParams, Variant &vResult)
{
	uint	iNumParams	= vParams.size();
	int		nWeight		= -1;
	AString sFont;								// Blank by default
	int		nAttribute	= -1;
	HWND	hWnd		= NULL;

	if (iNumParams > 4) hWnd = vParams[4].hWnd();
	if (iNumParams > 3) sFont = vParams[3].szValue();
	if (iNumParams > 2) nAttribute = vParams[2].nValue();
	if (iNumParams > 1) nWeight = vParams[1].nValue();

	vResult = g_oGUI.GuiSetFont(vParams[0].fValue(), nWeight, sFont.c_str(), nAttribute, hWnd);

	return AUT_OK;

} // GUISetFont()


///////////////////////////////////////////////////////////////////////////////
// GUICtrlDelete(controlref)
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_GUICtrlDelete(VectorVariant &vParams, Variant &vResult)
{
	vResult = g_oGUI.CtrlDelete(vParams[0].nValue());

	return AUT_OK;

} // GUICtrlDelete()


///////////////////////////////////////////////////////////////////////////////
// GUICtrlSetFont(controlref, size, [weight [,attribute [,font]]])
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_GUICtrlSetFont(VectorVariant &vParams, Variant &vResult)
{
	uint	iNumParams	= vParams.size();
	int		nWeight		= -1;
	AString sFont;								// Blank by default
	int		nAttribute	= -1;


	if (iNumParams > 4) sFont = vParams[4].szValue();
	if (iNumParams > 3) nAttribute = vParams[3].nValue();
	if (iNumParams > 2) nWeight = vParams[2].nValue();

	vResult = g_oGUI.CtrlSetFont(vParams[0].nValue(), vParams[1].fValue(), nWeight, sFont.c_str(), nAttribute);

	return AUT_OK;

} // GUICtrlSetFont()


///////////////////////////////////////////////////////////////////////////////
// GUICtrlSetOnEvent( control, callback )
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_GUICtrlSetOnEvent(VectorVariant &vParams, Variant &vResult)
{
	int		nTemp1, nTemp2, nTemp3, nTemp4;

	// Check that this user function exists
	if (Parser_FindUserFunction(vParams[1].szValue(), nTemp1, nTemp2, nTemp3, nTemp4) == false)
	{
		FatalError(IDS_AUT_E_UNKNOWNUSERFUNC);
		return AUT_ERR;
	}
	else
		vResult = g_oGUI.CtrlSetOnEvent(vParams[0].nValue(), vParams[1].szValue());

	return AUT_OK;

} // GUICtrlSetOnEvent()


///////////////////////////////////////////////////////////////////////////////
// GUICtrlSetColor(controlid, textcolor)
//
///////////////////////////////////////////////////////////////////////////////
AUT_RESULT AutoIt_Script::F_GUICtrlSetColor(VectorVariant &vParams, Variant &vResult)
{
	vResult = g_oGUI.CtrlSetColor(vParams[0].nValue(), vParams[1].nValue());

	return AUT_OK;

} // GUICtrlSetColor()


///////////////////////////////////////////////////////////////////////////////
// GUICtrlSetCursor()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_GUICtrlSetCursor(VectorVariant &vParams, Variant &vResult)
{
	vResult = g_oGUI.CtrlSetCursor(vParams[0].nValue(), vParams[1].nValue());

	return AUT_OK;

} // GUICtrlSetCursor()


///////////////////////////////////////////////////////////////////////////////
// GUICtrlSetBkColor(controlid, backgroundcolor)
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_GUICtrlSetBkColor(VectorVariant &vParams, Variant &vResult)
{
	vResult = g_oGUI.CtrlSetBkColor(vParams[0].nValue(), vParams[1].nValue());

	return AUT_OK;

} // GUICtrlSetBkColor()


///////////////////////////////////////////////////////////////////////////////
// GUICtrlCreate(Type, Text, x, y [,w [,h [,style [,exStyle]]]])
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::GUICtrlCreate(int nType, VectorVariant &vParams, Variant &vResult)
{
	int i = 0;
	int iNumParams = vParams.size();
	char *szText = NULL;
	int W		= -1;
	int H		= -1;
	int X		= -1;
	int Y		= -1;
	int Style	= -1;
	int exStyle	= -1;
	int id		= -1;
	int nSubType=  0;

	if ((nType == AUT_GUI_DUMMY ) && iNumParams != 0)
	{
		FatalError(IDS_AUT_E_FUNCTIONNUMPARAMS);
		return AUT_ERR;
	}

	switch (nType)
	{
		case AUT_GUI_TREEVIEWITEM:
		case AUT_GUI_LISTVIEWITEM:
			nSubType |= AUT_GUI_NORESIZE | AUT_GUI_NOTEXTSIZE;
			i = 0;
			szText = Util_StrCpyAlloc(vParams[0].szValue());
			break;

		case AUT_GUI_TABITEM:
		case AUT_GUI_MENU:
		case AUT_GUI_TRAYMENU:
		case AUT_GUI_MENUITEM:
		case AUT_GUI_DUMMY:
			nSubType |= AUT_GUI_NORESIZE | AUT_GUI_NOFONT | AUT_GUI_NOTEXTSIZE;

		case AUT_GUI_PIC:
			nSubType |= AUT_GUI_NOFONT | AUT_GUI_NOTEXTSIZE;

		case AUT_GUI_GROUP:
		case AUT_GUI_COMBO:
		case AUT_GUI_LIST:
		case AUT_GUI_EDIT:
		case AUT_GUI_INPUT:
		case AUT_GUI_DATE:
		case AUT_GUI_LISTVIEW:
			nSubType |= AUT_GUI_NOTEXTSIZE;

		case AUT_GUI_LABEL:
		case AUT_GUI_BUTTON:
		case AUT_GUI_CHECKBOX:
		case AUT_GUI_RADIO:
			i = 0;
			szText = Util_StrCpyAlloc(vParams[0].szValue());
			break;

		case AUT_GUI_AVI:
		case AUT_GUI_ICON:
			nSubType |= AUT_GUI_NOFONT | AUT_GUI_NOTEXTSIZE;
			i = 1;
			id = vParams[1].nValue();
			if (id<0) id=0;
			szText = Util_StrCpyAlloc(vParams[0].szValue());
			break;

		case AUT_GUI_PROGRESS:
		case AUT_GUI_SLIDER:
		case AUT_GUI_TAB:
		case AUT_GUI_CONTEXTMENU:
		case AUT_GUI_TREEVIEW:
			nSubType |= AUT_GUI_NOFONT | AUT_GUI_NOTEXTSIZE;
			i = -1;
			break;

		case AUT_GUI_UPDOWN:
			nSubType |= AUT_GUI_NORESIZE | AUT_GUI_NOFONT | AUT_GUI_NOTEXTSIZE;
			char szTemp[10];
			sprintf(szTemp,"%d",vParams[0].nValue());
			szText = Util_StrCpyAlloc(szTemp);
			i = 0;
			break;
	}

	if (iNumParams > i+6) exStyle	= vParams[i+6].nValue();
	if (iNumParams > i+5) Style		= vParams[i+5].nValue();
	if (iNumParams > i+4) H			= vParams[i+4].nValue();
	if (iNumParams > i+3) W			= vParams[i+3].nValue();
	if (iNumParams > i+2) Y			= vParams[i+2].nValue();
	if (iNumParams > i+1) X			= vParams[i+1].nValue();

	vResult = g_oGUI.CtrlCreate(nType, szText, X, Y, W, H, Style, exStyle, id, nSubType);

	if (vResult.nValue() == 0)
		SetFuncErrorCode(1);

	delete [] szText;
	return AUT_OK;


} // GUICtrlCreate()


///////////////////////////////////////////////////////////////////////////////
// GUICtrlCreateXXX()
//
//	first parameter send to GUICtrlCreate() must/should be correspondent to GuiBox
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_GUICtrlCreateAvi(VectorVariant &vParams, Variant &vResult)
{
	return GUICtrlCreate(AUT_GUI_AVI,vParams,vResult);
} // GUICtrlCreateAvi()



AUT_RESULT AutoIt_Script::F_GUICtrlCreateLabel(VectorVariant &vParams, Variant &vResult)
{
	return GUICtrlCreate(AUT_GUI_LABEL,vParams,vResult);
} // GUICtrlCreateLabel()


AUT_RESULT AutoIt_Script::F_GUICtrlCreateButton(VectorVariant &vParams, Variant &vResult)
{
	return GUICtrlCreate(AUT_GUI_BUTTON,vParams,vResult);
} // GUICtrlCreateButton()


AUT_RESULT AutoIt_Script::F_GUICtrlCreateCheckbox(VectorVariant &vParams, Variant &vResult)
{
	return GUICtrlCreate(AUT_GUI_CHECKBOX,vParams,vResult);
} // GUICtrlCreateCheckbox()


AUT_RESULT AutoIt_Script::F_GUICtrlCreateCombo(VectorVariant &vParams, Variant &vResult)
{
	return GUICtrlCreate(AUT_GUI_COMBO,vParams,vResult);
} // GUICtrlCreateCombo()


AUT_RESULT AutoIt_Script::F_GUICtrlCreateContextMenu(VectorVariant &vParams, Variant &vResult)
{
	return GUICtrlCreate(AUT_GUI_CONTEXTMENU,vParams,vResult);
} // GUICtrlCreateContextMenu()


AUT_RESULT AutoIt_Script::F_GUICtrlCreateDate(VectorVariant &vParams, Variant &vResult)
{
	return GUICtrlCreate(AUT_GUI_DATE,vParams,vResult);
} // GUICtrlCreateDate()


AUT_RESULT AutoIt_Script::F_GUICtrlCreateEdit(VectorVariant &vParams, Variant &vResult)
{
	return GUICtrlCreate(AUT_GUI_EDIT,vParams,vResult);
} // GUICtrlCreateEdit()


AUT_RESULT AutoIt_Script::F_GUICtrlCreateGroup(VectorVariant &vParams, Variant &vResult)
{
	return GUICtrlCreate(AUT_GUI_GROUP,vParams,vResult);
} // GUICtrlCreateGroup()


AUT_RESULT AutoIt_Script::F_GUICtrlCreateIcon(VectorVariant &vParams, Variant &vResult)
{
	return GUICtrlCreate(AUT_GUI_ICON,vParams,vResult);
} // GUICtrlCreateIcon()


AUT_RESULT AutoIt_Script::F_GUICtrlCreateInput(VectorVariant &vParams, Variant &vResult)
{
	return GUICtrlCreate(AUT_GUI_INPUT,vParams,vResult);
} // GUICtrlCreateInput()



AUT_RESULT AutoIt_Script::F_GUICtrlCreateList(VectorVariant &vParams, Variant &vResult)
{
	return GUICtrlCreate(AUT_GUI_LIST,vParams,vResult);
} // GUICtrlCreateList()


AUT_RESULT AutoIt_Script::F_GUICtrlCreateListView(VectorVariant &vParams, Variant &vResult)
{
	return GUICtrlCreate(AUT_GUI_LISTVIEW,vParams,vResult);
} // GUICtrlCreateListView()


AUT_RESULT AutoIt_Script::F_GUICtrlCreateListViewItem(VectorVariant &vParams, Variant &vResult)
{
	return GUICtrlCreate(AUT_GUI_LISTVIEWITEM,vParams,vResult);
} // GUICtrlCreateListViewItem()


AUT_RESULT AutoIt_Script::F_GUICtrlCreateMenu(VectorVariant &vParams, Variant &vResult)
{
	return GUICtrlCreate(AUT_GUI_MENU,vParams,vResult);
} // GUICtrlCreateMenu()


AUT_RESULT AutoIt_Script::F_GUICtrlCreateMenuItem(VectorVariant &vParams, Variant &vResult)
{
	return GUICtrlCreate(AUT_GUI_MENUITEM,vParams,vResult);
} // GUICtrlCreateMenuItem()


AUT_RESULT AutoIt_Script::F_GUICtrlCreatePic(VectorVariant &vParams, Variant &vResult)
{
	return GUICtrlCreate(AUT_GUI_PIC,vParams,vResult);
} // GUICtrlCreatePic()


AUT_RESULT AutoIt_Script::F_GUICtrlCreateProgress(VectorVariant &vParams, Variant &vResult)
{
	return GUICtrlCreate(AUT_GUI_PROGRESS,vParams,vResult);
} // GUICtrlCreateProgress()


AUT_RESULT AutoIt_Script::F_GUICtrlCreateSlider(VectorVariant &vParams, Variant &vResult)
{
	return GUICtrlCreate(AUT_GUI_SLIDER,vParams,vResult);
} // GUICtrlCreateSlider()


AUT_RESULT AutoIt_Script::F_GUICtrlCreateRadio(VectorVariant &vParams, Variant &vResult)
{
	return GUICtrlCreate(AUT_GUI_RADIO,vParams,vResult);
} // GUICtrlCreateRadio()


AUT_RESULT AutoIt_Script::F_GUICtrlCreateTab(VectorVariant &vParams, Variant &vResult)
{
	return GUICtrlCreate(AUT_GUI_TAB,vParams,vResult);
} // GUICtrlCreateTab()


AUT_RESULT AutoIt_Script::F_GUICtrlCreateTabitem(VectorVariant &vParams, Variant &vResult)
{
	return GUICtrlCreate(AUT_GUI_TABITEM,vParams,vResult);
} // GUICtrlCreateTabitem()

/*
AUT_RESULT AutoIt_Script::F_GUICtrlCreateTrayMenu(VectorVariant &vParams, Variant &vResult)
{
	return GUICtrlCreate(AUT_GUI_TRAYMENU,vParams,vResult);
} // GUICtrlCreateTrayMenu()

*/
AUT_RESULT AutoIt_Script::F_GUICtrlCreateTreeView(VectorVariant &vParams, Variant &vResult)
{
	return GUICtrlCreate(AUT_GUI_TREEVIEW,vParams,vResult);
} // GUICtrlCreateTreeView()


AUT_RESULT AutoIt_Script::F_GUICtrlCreateTreeViewItem(VectorVariant &vParams, Variant &vResult)
{
	return GUICtrlCreate(AUT_GUI_TREEVIEWITEM,vParams,vResult);
} // GUICtrlCreateTreeViewItem()


AUT_RESULT AutoIt_Script::F_GUICtrlCreateUpdown(VectorVariant &vParams, Variant &vResult)
{
	return GUICtrlCreate(AUT_GUI_UPDOWN,vParams,vResult);
} // GUICtrlCreateUpdown()


AUT_RESULT AutoIt_Script::F_GUICtrlCreateDummy(VectorVariant &vParams, Variant &vResult)
{
	return GUICtrlCreate(AUT_GUI_DUMMY,vParams,vResult);


} // GUICtrlCreateContextMenu()
///////////////////////////////////////////////////////////////////////////////
// GUICtrlSetImage(controlid, filename [,iconid [,mode]])
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_GUICtrlSetImage(VectorVariant &vParams, Variant &vResult)
{
	uint	iNumParams = vParams.size();
	int		nId = 0, nMode = -1;

	if (iNumParams > 3) nMode = vParams[3].nValue();
	if (iNumParams > 2) nId = vParams[2].nValue();

	vResult = g_oGUI.CtrlSetImage(vParams[0].nValue(), vParams[1].szValue(), nId, nMode);

	return AUT_OK;

} // GUICtrlSetImage()


///////////////////////////////////////////////////////////////////////////////
// GUICtrlSetState(controlid, state)
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_GUICtrlSetState(VectorVariant &vParams, Variant &vResult)
{
	vResult = g_oGUI.CtrlSetState(vParams[0].nValue(), vParams[1].nValue());

	return AUT_OK;

} // F_GUICtrlSetState()

/*


///////////////////////////////////////////////////////////////////////////////
// GUISetTrayBalloon(title, text [,timeout [,iconoption]])
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_GUISetTrayBalloon(VectorVariant &vParams, Variant &vResult)
{
	if (g_oGUI.m_bShowTrayIcon == false)
	{
		vResult = 0;
		SetFuncErrorCode(1);
		return AUT_OK;
	}

	uint	iNumParams = vParams.size();
	char	szTitle[64];
	char	szText[256];
	int		nTimeOut = 10;
	int		nOption = 0;

	Util_Strncpy(szTitle,vParams[0].szValue(),64);
	Util_Strncpy(szText,vParams[1].szValue(),256);

	if (iNumParams > 2)
		nTimeOut = vParams[2].nValue();
	if (iNumParams > 3)
		nOption = vParams[3].nValue();

	g_oGUI.CreateTrayBalloon(szTitle,szText,nTimeOut,nOption);

	return AUT_OK;

} // GUISetTrayBalloon()


///////////////////////////////////////////////////////////////////////////////
// GUISetTrayIcon(filename [,iconid])
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_GUISetTrayIcon(VectorVariant &vParams, Variant &vResult)
{
	uint	iNumParams = vParams.size();

	Util_Strncpy(g_oGUI.m_szTrayIconFile,vParams[0].szValue(),_MAX_PATH);

	if (iNumParams > 1 &&  vParams[1].nValue() >= 0)
		g_oGUI.m_nTrayIconId = vParams[1].nValue();

	if (g_oGUI.m_bShowTrayIcon)
		g_oGUI.CreateTrayIcon();

	return AUT_OK;

} // GUISetTrayIcon()


///////////////////////////////////////////////////////////////////////////////
// GUISetTrayTip(tooltip)
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_GUISetTrayTip(VectorVariant &vParams, Variant &vResult)
{
	Util_Strncpy(g_oGUI.m_szTrayToolTip,vParams[0].szValue(),64);

	if (g_oGUI.m_bShowTrayIcon)
		g_oGUI.CreateTrayIcon();

	return AUT_OK;

} // GUISetTrayTip()




*/

///////////////////////////////////////////////////////////////////////////////
// GUIStartGroup()
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_GUIStartGroup(VectorVariant &vParams, Variant &vResult)
{
	HWND hWnd = NULL;

	if (vParams.size() > 0) hWnd = vParams[0].hWnd();

	if (g_oGUI.m_nNumWindows == 0)
	{
		vResult = 0;							// Default is 1
		return AUT_OK;
	}

	g_oGUI.GroupStart(hWnd);

	return AUT_OK;

} // GUIStartGroup()


///////////////////////////////////////////////////////////////////////////////
// GUICtrlSetData(ControlRef,Data [,Default])
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_GUICtrlSetData(VectorVariant &vParams, Variant &vResult)
{
	AString sDefault;

	if (vParams.size() > 2) sDefault = vParams[2].szValue();

	vResult = g_oGUI.CtrlSetData(vParams[0].nValue(), vParams[1].szValue(), sDefault.c_str());

	return AUT_OK;

} // GUICtrlSetData()



///////////////////////////////////////////////////////////////////////////////
// GUICtrlSetLimit(controlref, max [,min])
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_GUICtrlSetLimit(VectorVariant &vParams, Variant &vResult)
{
	int		nMin = -1;

	if (vParams.size() > 2) nMin = vParams[2].nValue();

	vResult = g_oGUI.CtrlSetLimit(vParams[0].nValue(), vParams[1].nValue(), nMin);

	return AUT_OK;

} // GUICtrlSetLimit()


///////////////////////////////////////////////////////////////////////////////
// GUICtrlSetPos(controlref , X , Y [,W [,H]])
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_GUICtrlSetPos(VectorVariant &vParams, Variant &vResult)
{
	int nW = -1;
	int nH = -1;

	if (vParams.size() > 4) nH  = vParams[4].nValue();
	if (vParams.size() > 3) nW  = vParams[3].nValue();

	vResult = g_oGUI.CtrlSetPos( vParams[0].nValue(), vParams[1].nValue(), vParams[2].nValue(), nW, nH);

	return AUT_OK;

} // GUICtrlSetPos()


///////////////////////////////////////////////////////////////////////////////
// GUICtrlSetResizing(controlref, state)
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_GUICtrlSetResizing(VectorVariant &vParams, Variant &vResult)
{
	int		nResizing = -1;

	if (vParams.size() > 1) nResizing = vParams[1].nValue();

 	vResult = g_oGUI.CtrlSetResizing(vParams[0].nValue(), nResizing);

	return AUT_OK;

} // GUICtrlSetResizing()


///////////////////////////////////////////////////////////////////////////////
// GUICtrlGetState(controlref, state)
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_GUICtrlGetState(VectorVariant &vParams, Variant &vResult)
{
	vResult= g_oGUI.CtrlGetState(vParams[0].nValue());

	return AUT_OK;

} // GUICtrlGetState()


///////////////////////////////////////////////////////////////////////////////
// GUICtrlSetStyle(controlref, style [,exstyle])
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_GUICtrlSetStyle(VectorVariant &vParams, Variant &vResult)
{
	int		nExStyle = -1;

	if (vParams.size() > 2) nExStyle = vParams[2].nValue();

	vResult = g_oGUI.CtrlSetStyle(vParams[0].nValue(), vParams[1].nValue(), nExStyle);

	return AUT_OK;

} // GUICtrlSetStyle()


///////////////////////////////////////////////////////////////////////////////
// GUICtrlSetTip(controlref, tip)
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_GUICtrlSetTip(VectorVariant &vParams, Variant &vResult)
{
	vResult = g_oGUI.CtrlSetTip( vParams[0].nValue(), vParams[1].szValue());

	return AUT_OK;

} // GUICtrlSetTip()


///////////////////////////////////////////////////////////////////////////////
// GUIRead([Control])
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_GUIRead(VectorVariant &vParams, Variant &vResult)
{
	uint	iNumParams = vParams.size();
	char *szText;
	int nState;
	int nControl = -1;

	if (iNumParams > 0) nControl = vParams[0].nValue();

	szText = g_oGUI.Read(nControl, nState);
	if (nState <0 && szText != NULL)
	{
		vResult = szText;
		delete [] szText;						// We are responsible for deleting memory
	}
	else
		vResult = nState;

	return AUT_OK;

} // GUIRead()


///////////////////////////////////////////////////////////////////////////////
// GUISetCoord(x ,y [,w [,h [,winhandle]]])
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_GUISetCoord(VectorVariant &vParams, Variant &vResult)
{
	uint	iNumParams = vParams.size();
	int nW = -1;
	int nH = -1;
	HWND hWnd = NULL;

	if (iNumParams > 2) nW = vParams[2].nValue();
	if (iNumParams > 3) nH = vParams[3].nValue();
	if (iNumParams > 4) hWnd = vParams[4].hWnd();

	vResult = g_oGUI.SetCoord(vParams[0].nValue(), vParams[1].nValue(), nW, nH, hWnd);

	return AUT_OK;

} // GUISetCoord()


///////////////////////////////////////////////////////////////////////////////
// GUISendToDummy(Controlref [,State])
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_GUISendToDummy(VectorVariant &vParams, Variant &vResult)
{
	Variant vState;

	if (vParams.size() > 1)	vState = vParams[1];

	vResult= g_oGUI.SendToDummy(vParams[0].nValue(), vState);
	return AUT_OK;

} // GUISendToDummy()


///////////////////////////////////////////////////////////////////////////////
// GUISendMsg(Controlref, Msg ,wParam ,lParam)
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_GUISendMsg(VectorVariant &vParams, Variant &vResult)
{
	int nTemp = 0;
	if (vParams[2].type() == VAR_STRING)
		nTemp = 1;
	if (vParams[3].type() == VAR_STRING)
		nTemp = nTemp + 2;

	switch (nTemp)
	{
		case 0:
			vResult= g_oGUI.SendMsg(vParams[0].nValue(),vParams[1].nValue(),vParams[2].nValue(),vParams[3].nValue());
			break;
		case 1:
			vResult= g_oGUI.SendMsg(vParams[0].nValue(),vParams[1].nValue(),vParams[2].szValue(),vParams[3].nValue());
			break;
		case 2:
			vResult= g_oGUI.SendMsg(vParams[0].nValue(),vParams[1].nValue(),vParams[2].nValue(),vParams[3].szValue());
			break;
		case 3:
			vResult= g_oGUI.SendMsg(vParams[0].nValue(),vParams[1].nValue(),vParams[2].szValue(),vParams[3].szValue());
			break;
	}
	return AUT_OK;

} // GUISendMsg()


///////////////////////////////////////////////////////////////////////////////
// GUIRecvMsg(Controlref, Msg [,wParam [,lParamType]])
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_GUIRecvMsg(VectorVariant &vParams, Variant &vResult)
{
	uint	iNumParams = vParams.size();
	int nWparam = 0;
	int nLparamType = 0;

	if (iNumParams > 3) nLparamType = vParams[3].nValue();
	if (iNumParams > 2) nWparam = vParams[2].nValue();

	int nX, nY, nRight,nBottom;

	union
	{
		WORD nTemp1;
		char szTemp[AUT_MAX_LINESIZE];
	};

	nTemp1 =  AUT_MAX_LINESIZE;		// to avoid overflow return

	int nTemp = g_oGUI.RecvMsg(vParams[0].nValue(), vParams[1].nValue(), nWparam, nLparamType, nX, nY, nRight, nBottom, szTemp);

	if (nTemp == 0)
	{	// bad return from SendMessage
		vResult = 0;
		SetFuncErrorCode(1);
		return AUT_OK;
	}

	if (nLparamType == 1)
	{
		// string to be returned
		vResult = szTemp;
	}
	else
	{
		// Setup vResult as an Array to hold the n values we want to return
		Variant	*pvTemp;

		nLparamType = nLparamType + 2; //  entry in array

		Util_VariantArrayDim(&vResult, nLparamType);

		pvTemp = Util_VariantArrayGetRef(&vResult, 0);	//First element
		*pvTemp = nX;

		pvTemp = Util_VariantArrayGetRef(&vResult, 1);
		*pvTemp = nY;

		if (nLparamType == 4)
		{
			pvTemp = Util_VariantArrayGetRef(&vResult, 2);
			*pvTemp = nRight;

			pvTemp = Util_VariantArrayGetRef(&vResult, 3);
			*pvTemp = nBottom;
		}
	}

	return AUT_OK;

} // GUIRecvMsg()


///////////////////////////////////////////////////////////////////////////////
// GUIDelete()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_GUIDelete(VectorVariant &vParams, Variant &vResult)
{
	HWND	hWnd = NULL;						// NULL will be set to delete current GUI

	if (vParams.size() > 0)	hWnd = vParams[0].hWnd();

	vResult = g_oGUI.DeleteGUI(hWnd);
	return AUT_OK;

} // GUIDelete()



///////////////////////////////////////////////////////////////////////////////
// GUIGetCursorInfo( [winhandle] )
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_GUIGetCursorInfo(VectorVariant &vParams, Variant &vResult)
{
	HWND	hWnd = NULL;
	int		nX, nY, nPrimary, nSecondary, nGlobalID;

	// Setup vResult as an Array to hold the 5 values we want to return
	Variant	*pvTemp;

	Util_VariantArrayDim(&vResult, 5);

	if (vParams.size() > 0)
		hWnd = vParams[0].hWnd();

	if ( g_oGUI.GuiGetCursorInfo(hWnd, nX, nY, nPrimary, nSecondary, nGlobalID) )
	{
		pvTemp = Util_VariantArrayGetRef(&vResult, 0);	//First element
		*pvTemp = nX;					// X

		pvTemp = Util_VariantArrayGetRef(&vResult, 1);
		*pvTemp = nY;					// Y

		pvTemp = Util_VariantArrayGetRef(&vResult, 2);
		*pvTemp = nPrimary;				// Primary down

		pvTemp = Util_VariantArrayGetRef(&vResult, 3);
		*pvTemp = nSecondary;			// Secondary down

		pvTemp = Util_VariantArrayGetRef(&vResult, 4);
		*pvTemp = nGlobalID;			// Ctrl ID that cursor is over
	}
	else
		SetFuncErrorCode(1);

	return AUT_OK;

} // GUIGetCursorPos()



#endif	// AUT_CONFIG_GUI


