#ifndef __INPUTBOX_H
#define __INPUTBOX_H

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
// inputbox.h
//
// Inputbox object.  Created by D.Nuttall.
//
///////////////////////////////////////////////////////////////////////////////


// Includes
#include "AutoIt.h"
#include "resources\resource.h"
#include "astring_datatype.h"


/////////////////////////////////////////////////////////////////////////////
// CInputBox dialog

class CInputBox
{
// Construction
public:
	CInputBox(void);   // standard constructor

// Dialog Data
	enum { IDD = IDD_INPUTBOX, TimerConst=1034 };
	enum InputBoxFlags {
		ibf_none=0,
		ibf_notnull=1
	};

	int m_width;
	int m_height;
	int m_top;
	int m_left;
	uint m_flags;
	int m_maxlen;
	double m_timeout;
	char m_password;
	AString m_title;
	AString	m_strInputText;
	AString	m_strPrompt;

private:
	HWND m_hWnd;
	UINT_PTR m_timer;

// Overrides

// Implementation
public:
	UINT DoModal(HINSTANCE hInstance, HWND hWnd);

protected:

	BOOL OnInitDialog(void);
	void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
	void OnSize(UINT nType, int cx, int cy);
	void OnOK(void);
	void OnCancel(void);
	void OnTimer(UINT iIdent);

private:
	static BOOL CALLBACK ProcHandler(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);
	BOOL ProcSubHandler(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);

protected:
	static CInputBox* CurrInputBox;
};

///////////////////////////////////////////////////////////////////////////////

#endif

