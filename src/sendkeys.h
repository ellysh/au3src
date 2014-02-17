#ifndef __HS_SENDKEYS_H
#define __HS_SENDKEYS_H

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
// sendkeys.h
//
// The AutoIt keystroke engine.
//
///////////////////////////////////////////////////////////////////////////////


// Includes
#include "utility.h"


class HS_SendKeys
{
public:
	// Functions
	HS_SendKeys();										// Constructor
	~HS_SendKeys();										// Destructor
	void	Init(void);									// Call this first to init the SendKeys engine

	void	Send(const char *szString, HWND hWnd = NULL);				// Send keystrokes
	void	SendRaw(const char *szSendKeys, HWND hWnd = NULL);			// Send keystrokes without interpretation

	void	SetKeyDelay(int n) { m_nKeyDelay = n; }						// Change delay between keypresses
	void	SetKeyDownDelay(int n) { m_nKeyDownDelay = n; }				// Change delay between keypresses
	void	SetStoreCapslockMode(bool b) {	m_bStoreCapslockMode = b; }	// Set capslock store mode
	void	SetAttachMode(bool b) {	m_bAttachMode = b; }				// Set attach mode
	bool	GetSingleVKandMods(const char *szString, UINT &vk, bool &bShift, bool &bControl, bool &bAlt, bool &bWin);

	// Variables
	int		m_nKeyDelay;								// Time in between keystrokes
	int		m_nKeyDownDelay;							// Delay after pressing the key down before releasing
	bool	m_bStoreCapslockMode;						// Store/restore capslock state
	bool	m_bAttachMode;								// Attach mode for Send()


private:
	// Variables
	int		m_nKeyMod;									// Key modifiers
	HWND	m_hWnd;										// Window to send to (NULL=active window)

	UINT	m_scanCtrl, m_scanAlt, m_scanShift, m_scanLWin;

	// Functions
	void	WinAttach(HWND hWnd, bool bAttach);
	inline void DoKeyDelay(void) { Util_Sleep(m_nKeyDelay); }
	inline void DoKeyDownDelay(void) { Util_Sleep(m_nKeyDownDelay); }
	bool	SetToggleState(UINT vk, bool bState);
	void	SendCh(char ch, int nRep);
	void	SendVk(UINT vk, int nRep, bool bForceExtended = false);
	void	SendSpecialCh(char ch);
	void	ResolveKeyModifiers(UINT vkres);
	bool	IsVKExtended(UINT key);
	void	SimModsUp();
	void	SimModsDown();
	void	SimKeyUp(UINT vk, bool bForceExtended = false);
	void	SimKeyDown(UINT vk, bool bForceExtended = false);
	void	SimKeystroke(UINT vk, bool bForceExtended = false);
	void	SendSpecial(char *szTemp);

	int		ReadToChar(char ch, const char *szLine, char *szResult, int &nPos);
};

///////////////////////////////////////////////////////////////////////////////

#endif

