
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
// inputbox.cpp
//
// Inputbox object.  Created by D.Nuttall.
//
///////////////////////////////////////////////////////////////////////////////


// Includes
#include "StdAfx.h"								// Pre-compiled headers

#ifndef _MSC_VER								// Includes for non-MS compilers
	#include <stdio.h>
	#include <windows.h>
#endif

#include "AutoIt.h"								// Autoit values, macros and config options

#include "inputbox.h"
#include "globaldata.h"

template <class T>
inline void swap(T &v1, T &v2) {
	T tmp=v1;
	v1=v2;
	v2=tmp;
}

void Normalize(RECT &rCheck)
{
	if (rCheck.left > rCheck.right)
		swap(rCheck.left, rCheck.right);
	if (rCheck.top > rCheck.bottom)
		swap(rCheck.top, rCheck.bottom);
}

CInputBox *(CInputBox::CurrInputBox) = NULL;

/////////////////////////////////////////////////////////////////////////////
// CInputBox dialog

CInputBox::CInputBox(void)
{
	m_hWnd			= NULL;
	m_password		= '\0';
	m_title			= "";
	m_left			= -1;
	m_top			= -1;
	m_width			= -1;
	m_height		= -1;
	m_timeout		= -1.0;
	m_timer			= 0;
	m_maxlen		= 0;
	m_flags			= 0;
	m_strInputText	= "";
	m_strPrompt		= "";
}

/////////////////////////////////////////////////////////////////////////////
// CInputBox message handlers

BOOL CInputBox::OnInitDialog()
{
	int nWidth, nHeight, nLeft, nTop;
	int nWidthDT, nHeightDT;
	HWND hControl=NULL;
	RECT rSize;

	// Set application icon
	SendMessage(m_hWnd, WM_SETICON, ICON_SMALL, (LPARAM)LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_MAIN)) );

	// Title the message box
	if (m_title.length()>0)
		SetWindowText(m_hWnd, m_title.c_str());

	// Prompt
	if (m_strPrompt.length()>0) {
		hControl = GetDlgItem(m_hWnd, IDC_INPUTPROMPT);
		SetWindowText(hControl, m_strPrompt.c_str());
	}

	// Default Value?
	if (m_strInputText.length()>0) {
		hControl = GetDlgItem(m_hWnd, IDC_INPUTEDIT);
		SetWindowText(hControl, m_strInputText.c_str());
	}

	// Check for password character
	if (m_password != '\0')
		SendDlgItemMessage(m_hWnd, IDC_INPUTEDIT, EM_SETPASSWORDCHAR, m_password, 0);

 	// If Max length is specified, set max length
 	if (m_maxlen > 0)
 		SendDlgItemMessage(m_hWnd, IDC_INPUTEDIT, EM_LIMITTEXT, (WPARAM)m_maxlen, 0);

	// How big is the default box?
	GetWindowRect(m_hWnd, &rSize);
	Normalize(rSize);
	nLeft = rSize.left;
	nTop = rSize.top;
	nWidth = rSize.right-rSize.left;
	nHeight = rSize.bottom-rSize.top;

	// Check for extra parameters
	if (m_left>=0)
		nLeft=m_left;
	if (m_top>=0)
		nTop=m_top;
	if (m_width>0)
		nWidth=m_width;
	if (m_height>0)
		nHeight=m_height;
	if (m_title != "")
		SetWindowText(m_hWnd, m_title.c_str());

  	GetWindowRect(GetDesktopWindow(), &rSize);
	Normalize(rSize);
 	if (m_left<0) {
 		// centre the box horizontally
  		// size of desktop
 		nWidthDT = rSize.right-rSize.left;
 		nLeft = (nWidthDT-nWidth)/2;
 	}
 	if (m_top<0) {
 		// centre the box vertically
 		// size of desktop
  		nHeightDT = rSize.bottom-rSize.top;
  		nTop = (nHeightDT-nHeight)/2;
  	}

 	// Position everything
 	MoveWindow(m_hWnd, nLeft, nTop, nWidth, nHeight, FALSE);

	// Post the resize message to make sure everything gets drawn.
	GetClientRect(m_hWnd, &rSize);
	Normalize(rSize);
	nWidth = rSize.right-rSize.left;
	nHeight = rSize.bottom-rSize.top;
	PostMessage(m_hWnd, WM_SIZE, SIZE_RESTORED, nWidth + (nHeight<<16));

 	// If timeout is specified, set the timer
 	if (m_timeout>0)
 		m_timer = SetTimer(m_hWnd, (UINT_PTR)TimerConst, UINT(m_timeout*1E3), NULL);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CInputBox::OnSize(UINT nType, int cx, int cy)
{
	int lastheight=0, nWidth, nHeight;
	HWND hbtOk=NULL;
	HWND hbtCancel=NULL;
	HWND hstPrompt=NULL;
	HWND hedText=NULL;
	RECT rTmp;

	const int XMargin=10, YMargin=10;

	// don't try moving controls if minimized
	if (nType == SIZE_MINIMIZED)
		return;

	// start at the bottom - OK button

	hbtOk = GetDlgItem(m_hWnd, IDOK);
	if (hbtOk != NULL) {
		// how big is the control?
		GetWindowRect(hbtOk, &rTmp);
		if (rTmp.left > rTmp.right)
			swap(rTmp.left, rTmp.right);
		if (rTmp.top > rTmp.bottom)
			swap(rTmp.top, rTmp.bottom);
		nWidth=rTmp.right-rTmp.left;
		nHeight=rTmp.bottom-rTmp.top;
		lastheight=cy-YMargin-nHeight;
		// where to put the control?
		MoveWindow(hbtOk, cx/4+(XMargin-nWidth)/2, lastheight, nWidth, nHeight, FALSE);
	}

	// Cancel Button
	hbtCancel = GetDlgItem(m_hWnd, IDCANCEL);
	if (hbtCancel != NULL) {
		// how big is the control?
		GetWindowRect(hbtCancel, &rTmp);
		if (rTmp.left > rTmp.right)
			swap(rTmp.left, rTmp.right);
		if (rTmp.top > rTmp.bottom)
			swap(rTmp.top, rTmp.bottom);
		nWidth=rTmp.right-rTmp.left;
		nHeight=rTmp.bottom-rTmp.top;
		// where to put the control?
		MoveWindow(hbtCancel, cx*3/4-(XMargin+nWidth)/2, lastheight, nWidth, nHeight, FALSE);
	}

	// Edit Box
	hedText = GetDlgItem(m_hWnd, IDC_INPUTEDIT);
	if (hedText != NULL) {
		// how big is the control?
		GetWindowRect(hedText, &rTmp);
		if (rTmp.left > rTmp.right)
			swap(rTmp.left, rTmp.right);
		if (rTmp.top > rTmp.bottom)
			swap(rTmp.top, rTmp.bottom);
		nWidth=rTmp.right-rTmp.left;
		nHeight=rTmp.bottom-rTmp.top;
		lastheight -= 5 + nHeight;
		// where to put the control?
		MoveWindow(hedText,XMargin, lastheight, cx-XMargin*2, nHeight, FALSE);
	}

	// Static Box (Prompt)
	hstPrompt = GetDlgItem(m_hWnd, IDC_INPUTPROMPT);
	if (hstPrompt != NULL) {
		lastheight -= 5;
		// where to put the control?
		MoveWindow(hstPrompt, XMargin, YMargin, cx-XMargin*2, lastheight, FALSE);
	}
	InvalidateRect(m_hWnd, NULL, TRUE);	// force window to be redrawn
}

void CInputBox::OnTimer(UINT iIdent)
{
 	// What to do when the timer signals
 	if (iIdent == TimerConst) {  // Timeout timer
 		m_strInputText = "";
 		EndDialog(m_hWnd, IDABORT);
 	}
}

void CInputBox::OnGetMinMaxInfo(LPMINMAXINFO lpMMI)
{
	lpMMI->ptMinTrackSize.x=190;
	lpMMI->ptMinTrackSize.y=114;
}

void CInputBox::OnOK()
{
	HWND hControl=NULL;
	char szInputText[256];

	// Make sure that the string is not empty.
	hControl = GetDlgItem(m_hWnd, IDC_INPUTEDIT);
	if (hControl != NULL) {
		GetWindowText(hControl, szInputText, 256);
 		if ((m_flags & ibf_notnull) && *szInputText == '\0')	// empty string
 			MessageBeep(MB_OK);
 		else {
 			if (m_timeout>0.0)
 				KillTimer(m_hWnd, TimerConst);  // Disable previously started timer
			m_strInputText = szInputText;
			EndDialog(m_hWnd, IDOK);
		}
	}
}

void CInputBox::OnCancel()
{
	m_strInputText = "";
	EndDialog(m_hWnd, IDCANCEL);
}

UINT CInputBox::DoModal(HINSTANCE hInstance, HWND hWnd)
{
	int iRetVal;

	CurrInputBox = this;
	iRetVal=(int)DialogBox(hInstance, MAKEINTRESOURCE(IDD), hWnd, ProcHandler);
	CurrInputBox=NULL;

	return iRetVal;
}

BOOL CALLBACK CInputBox::ProcHandler(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	if (CurrInputBox == NULL)
		return FALSE;
	else
		return CurrInputBox->ProcSubHandler(hDlg, iMsg, wParam, lParam);
}

BOOL CInputBox::ProcSubHandler(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	switch (iMsg) {
	case WM_INITDIALOG:
		if (m_hWnd == NULL)
			m_hWnd = hDlg;
		return OnInitDialog();
	case WM_SIZE:
 		OnSize((UINT)wParam, LOWORD(lParam), HIWORD(lParam));
		return TRUE;
	case WM_GETMINMAXINFO:
		OnGetMinMaxInfo((LPMINMAXINFO) lParam);
		return TRUE;
 	case WM_TIMER:
 		OnTimer((UINT)wParam);
  		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			OnOK();
			return TRUE;
		case IDCANCEL:
			OnCancel();
			return TRUE;
		default:
			return FALSE;
		}
	default:
		return FALSE;
	}
}

