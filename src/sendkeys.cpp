
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
// sendkeys.cpp
//
// The AutoIt keystroke engine.
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// SendTo()
//
// This sends a limited set of keys directly to a window/control using
// PostMessages rather than the keybd_event() APIs.
//
// WM_KEYDOWN
// 31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
//  0  0  0  0  0  0  0 |  |---- scan code ------| |---- Repeat count -----------------|
//                      | Extended key 1/0               Usually 1
//
// WM_KEYUP
// 31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
//  1  1  0  0  0  0  0 |  |---- scan code ------| |---- Repeat count -----------------|
//                      | Extended key 1/0               Usually 1
//
// WM_CHAR
// 31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
//  0  0  0  0  0  0  0 |  |---- scan code ------| |---- Repeat count -----------------|
//                      | Extended key 1/0               Usually 1
//
///////////////////////////////////////////////////////////////////////////////


// Includes
#include "StdAfx.h"								// Pre-compiled headers

#ifndef _MSC_VER								// Includes for non-MS compilers
	#include <windows.h>
#endif

#include "AutoIt.h"								// Autoit values, macros and config options

#include "sendkeys.h"
#include "globaldata.h"
#include "utility.h"


// Keys that we can use
#define NUMKEYS 93
typedef enum {CTRLDOWN, CTRLUP, ALTDOWN, ALTUP, SHIFTDOWN, SHIFTUP,
				LWINDOWN, LWINUP, RWINDOWN, RWINUP, ASC, NUMPADENTER, ALT} SK_SpecialKeys;

#define SK_LOOKUP	0
#define SK_SPECIAL	1

// The key modifiers are used in binary ops
#define NONEMOD			0
#define ALTMOD			1
#define SHIFTMOD		2
#define CTRLMOD			4
#define LWINMOD			8
#define ALTPERMMOD		16
#define SHIFTPERMMOD	32
#define CTRLPERMMOD		64
#define LWINPERMMOD		128
#define RWINPERMMOD		256
#define RESETMOD		ALTPERMMOD+CTRLPERMMOD+SHIFTPERMMOD+LWINPERMMOD+RWINPERMMOD


// Extra VK_CODES not defined in headers (well, not defined in 95/NT4 headers :) )
#ifndef VK_SLEEP
	#define VK_SLEEP 0x5F
#endif

#ifndef VK_BROWSER_BACK
	#define VK_BROWSER_BACK			0xA6
	#define VK_BROWSER_FORWARD		0xA7
	#define VK_BROWSER_REFRESH		0xA8
	#define VK_BROWSER_STOP			0xA9
	#define VK_BROWSER_SEARCH		0xAA
	#define VK_BROWSER_FAVORTIES	0xAB
	#define VK_BROWSER_HOME			0xAC
	#define VK_VOLUME_MUTE			0xAD
	#define	VK_VOLUME_DOWN			0xAE
	#define VK_VOLUME_UP			0xAF
	#define VK_MEDIA_NEXT_TRACK		0xB0
	#define VK_MEDIA_PREV_TRACK		0xB1
	#define VK_MEDIA_STOP			0xB2
	#define VK_MEDIA_PLAY_PAUSE		0xB3
	#define VK_LAUNCH_MAIL			0xB4
	#define VK_LAUNCH_MEDIA_SELECT	0xB5
	#define VK_LAUNCH_APP1			0xB6
	#define VK_LAUNCH_APP2			0xB7
#endif


// Macros
//#define IsPhysicallyDown(vk) (GetKeyState(vk) & 0x8000)	// Both GetKeyState and GetAsyncKeyState are unreliable :(
//#define IsPhysicallyDown(vk) (GetAsyncKeyState(vk) & 0x80000000)	// Both GetKeyState and GetAsyncKeyState are unreliable :(
#define IsPhysicallyDown(vk) ( (GetAsyncKeyState(vk) & 0x80000000) || ((GetKeyState(vk) & 0x8000)) )

char *g_szKeyTable[NUMKEYS] =
/*1 */	{"ALT", "BACKSPACE", "BS", "DEL", "DELETE", "DOWN", "END", "ENTER",
/*2 */	"ESC", "ESCAPE", "F1", "F2", "F3", "F4", "F5", "F6",
/*3 */	"F7", "F8", "F9", "F10", "F11", "F12", "HOME", "INS",
/*4 */	"INSERT", "LEFT", "PGDN", "PGUP", "RIGHT", "SPACE", "TAB", "UP",
/*5 */	"PRINTSCREEN", "LWIN", "RWIN", "SCROLLLOCK", "NUMLOCK", "CTRLBREAK", "PAUSE", "CAPSLOCK",
/*6 */	"NUMPAD0", "NUMPAD1", "NUMPAD2", "NUMPAD3", "NUMPAD4", "NUMPAD5", "NUMPAD6", "NUMPAD7",
/*7 */	"NUMPAD8", "NUMPAD9", "NUMPADMULT", "NUMPADADD", "NUMPADSUB", "NUMPADDOT", "NUMPADDIV", "APPSKEY",
/*8 */	"LCTRL", "RCTRL", "LALT", "RALT", "LSHIFT", "RSHIFT", "SLEEP", "NUMPADENTER",
/*9 */	"BROWSER_BACK", "BROWSER_FORWARD", "BROWSER_REFRESH", "BROWSER_STOP",
/*10*/	"BROWSER_SEARCH", "BROWSER_FAVORTIES", "BROWSER_HOME", "VOLUME_MUTE",
/*11*/	"VOLUME_DOWN", "VOLUME_UP", "MEDIA_NEXT", "MEDIA_PREV",
/*12*/	"MEDIA_STOP", "MEDIA_PLAY_PAUSE", "LAUNCH_MAIL", "LAUNCH_MEDIA",
/*13*/	"LAUNCH_APP1", "LAUNCH_APP2",
/*14*/	"CTRLDOWN", "CTRLUP",
/*15*/	"ALTDOWN", "ALTUP",
/*16*/	"SHIFTDOWN", "SHIFTUP",
/*17*/	"LWINDOWN", "LWINUP",
/*18*/	"RWINDOWN", "RWINUP",
/*19*/	"ASC"};

// The following must be in the same order as the szKeyTable
char g_cKeyLookupType[NUMKEYS] =
/*1 */	{SK_SPECIAL, SK_LOOKUP, SK_LOOKUP, SK_LOOKUP, SK_LOOKUP, SK_LOOKUP, SK_LOOKUP, SK_LOOKUP,
/*2 */	SK_LOOKUP, SK_LOOKUP, SK_LOOKUP, SK_LOOKUP, SK_LOOKUP, SK_LOOKUP, SK_LOOKUP, SK_LOOKUP,
/*3 */	SK_LOOKUP, SK_LOOKUP, SK_LOOKUP, SK_LOOKUP, SK_LOOKUP, SK_LOOKUP, SK_LOOKUP, SK_LOOKUP,
/*4 */	SK_LOOKUP, SK_LOOKUP, SK_LOOKUP, SK_LOOKUP, SK_LOOKUP, SK_LOOKUP, SK_LOOKUP, SK_LOOKUP,
/*5 */	SK_LOOKUP, SK_LOOKUP, SK_LOOKUP, SK_LOOKUP,	SK_LOOKUP, SK_LOOKUP, SK_LOOKUP, SK_LOOKUP,
/*6 */	SK_LOOKUP, SK_LOOKUP, SK_LOOKUP, SK_LOOKUP,	SK_LOOKUP, SK_LOOKUP, SK_LOOKUP, SK_LOOKUP,
/*7 */	SK_LOOKUP, SK_LOOKUP, SK_LOOKUP, SK_LOOKUP,	SK_LOOKUP, SK_LOOKUP, SK_LOOKUP, SK_LOOKUP,
/*8 */	SK_LOOKUP, SK_LOOKUP, SK_LOOKUP, SK_LOOKUP,	SK_LOOKUP, SK_LOOKUP, SK_LOOKUP, SK_SPECIAL,
/*9 */	SK_LOOKUP, SK_LOOKUP, SK_LOOKUP, SK_LOOKUP,
/*10*/	SK_LOOKUP, SK_LOOKUP, SK_LOOKUP, SK_LOOKUP,
/*11*/	SK_LOOKUP, SK_LOOKUP, SK_LOOKUP, SK_LOOKUP,
/*12*/	SK_LOOKUP, SK_LOOKUP, SK_LOOKUP, SK_LOOKUP,
/*13*/	SK_LOOKUP, SK_LOOKUP,
/*14 */	SK_SPECIAL, SK_SPECIAL,
/*15*/	SK_SPECIAL, SK_SPECIAL,
/*16*/	SK_SPECIAL, SK_SPECIAL,
/*17*/	SK_SPECIAL, SK_SPECIAL,
/*18*/	SK_SPECIAL, SK_SPECIAL,
/*19*/	SK_SPECIAL};

// The following must be in the same order as the szKeyTable
int g_nKeyCodes[NUMKEYS] =
/*1 */	{ALT, VK_BACK, VK_BACK, VK_DELETE, VK_DELETE, VK_DOWN, VK_END, VK_RETURN,
/*2 */	VK_ESCAPE, VK_ESCAPE, VK_F1, VK_F2, VK_F3, VK_F4, VK_F5, VK_F6,
/*3 */	VK_F7, VK_F8, VK_F9, VK_F10, VK_F11, VK_F12, VK_HOME, VK_INSERT,
/*4 */	VK_INSERT, VK_LEFT, VK_NEXT, VK_PRIOR, VK_RIGHT, VK_SPACE, VK_TAB, VK_UP,
/*5 */	VK_SNAPSHOT, VK_LWIN, VK_RWIN, VK_SCROLL, VK_NUMLOCK, VK_CANCEL, VK_PAUSE, VK_CAPITAL,
/*6 */	VK_NUMPAD0, VK_NUMPAD1, VK_NUMPAD2, VK_NUMPAD3, VK_NUMPAD4, VK_NUMPAD5, VK_NUMPAD6, VK_NUMPAD7,
/*7 */	VK_NUMPAD8, VK_NUMPAD9, VK_MULTIPLY, VK_ADD, VK_SUBTRACT, VK_DECIMAL, VK_DIVIDE, VK_APPS,
/*8 */	VK_LCONTROL, VK_RCONTROL, VK_LMENU, VK_RMENU, VK_LSHIFT, VK_RSHIFT, VK_SLEEP, NUMPADENTER,
/*9 */	VK_BROWSER_BACK, VK_BROWSER_FORWARD, VK_BROWSER_REFRESH, VK_BROWSER_STOP,
/*10*/	VK_BROWSER_SEARCH, VK_BROWSER_FAVORTIES, VK_BROWSER_HOME, VK_VOLUME_MUTE,
/*11*/	VK_VOLUME_DOWN, VK_VOLUME_UP, VK_MEDIA_NEXT_TRACK, VK_MEDIA_PREV_TRACK,
/*12*/	VK_MEDIA_STOP, VK_MEDIA_PLAY_PAUSE, VK_LAUNCH_MAIL, VK_LAUNCH_MEDIA_SELECT,
/*13*/	VK_LAUNCH_APP1, VK_LAUNCH_APP2,
/*14*/	CTRLDOWN, CTRLUP,
/*15*/	ALTDOWN, ALTUP,
/*16*/	SHIFTDOWN, SHIFTUP,
/*17*/	LWINDOWN, LWINUP,
/*18*/	RWINDOWN, RWINUP,
/*19*/	ASC};

// List of Diadic characters will be updated according to keyboard layout
//                     0   1   2   3   4   5   6   7
char g_cDiadic [8] = {' ',' ','´','^','~','¨','`',' '};


///////////////////////////////////////////////////////////////////////////////
// Constructor()
///////////////////////////////////////////////////////////////////////////////

HS_SendKeys::HS_SendKeys()
{
	Init();

} // Constructor()


///////////////////////////////////////////////////////////////////////////////
// Destructor()
///////////////////////////////////////////////////////////////////////////////

HS_SendKeys::~HS_SendKeys()
{

} // Destructor()


///////////////////////////////////////////////////////////////////////////////
// Init()
// Call this function directly if constructor not used (e.g. DLL)
///////////////////////////////////////////////////////////////////////////////

void HS_SendKeys::Init(void)
{
	m_nKeyDelay				= 5;				// Default time in between keystrokes
	m_nKeyDownDelay			= 1;				// Default time before a pressed key is released (needs >0 for SendTo to be reliable)
	m_nKeyMod				= NONEMOD;			// Key modifiers
	m_bStoreCapslockMode	= true;				// Store/restore caps ON

	m_bAttachMode			= false;			// Don't attach to foreground for Send()

	// Key scan codes for frequently used keys (mods) to save space later
	m_scanLWin = MapVirtualKey(VK_LWIN, 0);
	m_scanShift = MapVirtualKey(VK_SHIFT, 0);
	m_scanCtrl = MapVirtualKey(VK_CONTROL, 0);
	m_scanAlt = MapVirtualKey(VK_MENU, 0);

	char szKLID[KL_NAMELENGTH];
	GetKeyboardLayoutName(szKLID );		// Get input locale identifier name

	//	update Diadics char according to keyboard possibility
	for (int i=1; i<=7; ++i)
	{	// check VK code to check if diadic char can be sent
		// English keyboard cannot sent diadics char
		if ( VkKeyScan(g_cDiadic[i]) == -1 || (strcmp(&szKLID[6], "09") == 0) )
			g_cDiadic[i] = ' ';				// reset Diadic setting
	}

	// need to check if a German keyboard in use
	// because ~ does not work as a diadic char
	if (strcmp(&szKLID[6], "07") == 0)	// german keyboard
		g_cDiadic[4] = ' ';	// ~

	// need to check if a italian keyboard  in use
	// because ^ does not work as a diadic char
	else if (strcmp(&szKLID[6], "10") == 0)	// italian keyboard
		g_cDiadic[3] = ' ';	// ^

/* I cannot have them working (JPM)
	// need to check if a hungarian keyboard  in use
	// because ~^`¨ does not work as a diadic char
	else if (strcmp(&szKLID[6], "0E") == 0)	// hungarian keyboard
	{
		g_cDiadic[3] = ' ';	// ^
		g_cDiadic[4] = ' ';	// ~
		g_cDiadic[5] = ' ';	// ¨
		g_cDiadic[6] = ' ';	// `
	}

	// need to check if a czech keyboard  in use
	// because ~ does not work as a diadic char
	else if (strcmp(&szKLID[6], "05") == 0)	// czech keyboard
	{
		g_cDiadic[2] = ' ';	// ´
		g_cDiadic[4] = ' ';	// ~
		g_cDiadic[5] = ' ';	// ¨
		g_cDiadic[6] = ' ';	// `
	}
*/
} // Constructor()


///////////////////////////////////////////////////////////////////////////////
// Send()
///////////////////////////////////////////////////////////////////////////////

void HS_SendKeys::Send(const char *szSendKeys, HWND hWnd)
{
#ifdef _DEBUG
	Util_DebugMsg("==> HS_SendKeys::Send : szSendKeys=%s)\n", szSendKeys);
#endif
	char	*szTemp;
	char	ch;
	int		nPos = 0;
	int		nPosTemp;
	bool	bCapsWasOn = false;

	// Store window (or NULL if not specified)
	m_hWnd = hWnd;

	// Attach to the window so that we can add things to the input queues
	WinAttach(m_hWnd, true);

	// Allocate some temporary memory for szTemp
	szTemp =  new char[strlen(szSendKeys)+1];

	// First, store the state of capslock, then turn off
	if ( m_bStoreCapslockMode == true )
		bCapsWasOn = SetToggleState(VK_CAPITAL, false);

	// Send the keys, but watch out for the main script pausing/closing
	while ( (ch = szSendKeys[nPos]) != '\0' )
	{
		nPos++;									// Next key

		// is it a special?
		switch (ch)
		{
			case '!':
				m_nKeyMod = m_nKeyMod | ALTMOD;
				break;
			case '^':
				m_nKeyMod = m_nKeyMod | CTRLMOD;
				break;
			case '+':
				m_nKeyMod = m_nKeyMod | SHIFTMOD;
				break;
			case '#':
				m_nKeyMod = m_nKeyMod | LWINMOD;
				break;

			case '{':
				nPosTemp = nPos;
				if ( ReadToChar('}', szSendKeys, szTemp, nPos) )
				{
					// NO CLOSE BRACKET!!?!
					nPos = nPosTemp;
					SendCh('{', 1);
					break;
				}
				else
				{
					SendSpecial(szTemp);
					m_nKeyMod &= RESETMOD;		// reset modifiers
					break;
				}

			default:
				SendCh(ch, 1);
				m_nKeyMod &= RESETMOD;			// reset modifiers
				break;

		} // End Switch

	} // End While


	// If required, restore caps lock state
	if ( m_bStoreCapslockMode == true )
		SetToggleState(VK_CAPITAL, bCapsWasOn);

	// Free temp string memory
	delete [] szTemp;


	// Detach
	WinAttach(m_hWnd, false);

} // Send()


///////////////////////////////////////////////////////////////////////////////
// SendRaw()
//
// Similar to Send() but sends text as is, without any special code
//
///////////////////////////////////////////////////////////////////////////////

void HS_SendKeys::SendRaw(const char *szSendKeys, HWND hWnd)
{
#ifdef _DEBUG
	Util_DebugMsg("==> HS_SendKeys::SendRaw : szSendKeys=%s, hWnd=%.x8)\n", szSendKeys, hWnd);
#endif
	char	ch;
	int		nPos = 0;
	bool	bCapsWasOn = false;

	// Store window (or NULL if not specified)
	m_hWnd = hWnd;

	// Attach to the window so that we can add things to the input queues
	WinAttach(m_hWnd, true);

	// First, store the state of capslock, then turn off
	if ( m_bStoreCapslockMode == true )
		bCapsWasOn = SetToggleState(VK_CAPITAL, false);

	// Send the keys, but watch out for the main script pausing/closing
	while ( (ch = szSendKeys[nPos]) != '\0' )
	{
		nPos++;									// Next key

		SendCh(ch, 1);
		m_nKeyMod &= RESETMOD;			// reset modifiers
	}


	// If required, restore caps lock state
		if ( m_bStoreCapslockMode == true )
			SetToggleState(VK_CAPITAL, bCapsWasOn);

	// Detach
	WinAttach(m_hWnd, false);

} // SendRaw()


///////////////////////////////////////////////////////////////////////////////
// WinAttach()
//
// Associates us with lots of input queues so that IsPhysicallyDown() works
// better and so we can send directly to input queues and SetKeyBoardState.
//
///////////////////////////////////////////////////////////////////////////////

void HS_SendKeys::WinAttach(HWND hWnd, bool bAttach)
{
	if (hWnd == NULL && m_bAttachMode == false)
		return;

	DWORD			myThread;
	static DWORD	newThread;
	static DWORD	curThread;

	myThread  = GetCurrentThreadId();

	// Don't attach if we are running in Send() mode - seems to mess up games with the {... down} syntax
	// I can only assume that detaching the input queue is breaking something (verified by the docs, AttachThread
	// resets the keyboard state! but without it we can't detect keystates reliably)

	if (bAttach)
	{
		curThread = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
		AttachThreadInput(myThread, curThread, TRUE);

		if (hWnd != NULL)
		{
			newThread = GetWindowThreadProcessId(hWnd, NULL);
			AttachThreadInput(curThread, newThread, TRUE);
			AttachThreadInput(myThread, newThread, TRUE);
		}
	}
	else
	{
		if (hWnd != NULL)
		{
			AttachThreadInput(myThread, newThread, FALSE);
			AttachThreadInput(curThread, newThread, FALSE);
		}

		AttachThreadInput(myThread, curThread, FALSE);
	}

} // WinAttach()


///////////////////////////////////////////////////////////////////////////////
// SetToggleState()
//
// Sets the state of a toggle key such as CAPS, SCROLL, NUMLOCK
// These keys only work properly if sent with keybd_event()
///////////////////////////////////////////////////////////////////////////////

bool HS_SendKeys::SetToggleState(UINT vk, bool bState)
{
	bool	bInitial;

	if (GetKeyState(vk) & 0x01)
		bInitial = true;						// Was on
	else
		bInitial = false;						// Was off

#ifdef _DEBUG
	Util_DebugMsg("==> HS_SendKeys::SetToggleState : vk=%d, bState=%d, bInitial=%d)\n", vk, bState, bInitial);
#endif

	// Do we need to do anything?
	if (bState == bInitial)
		return bInitial;						// Nope

//	MessageBox(NULL, "Doing Capslock", "", MB_OK);

	keybd_event((BYTE)vk, MapVirtualKey(vk, 0), 0, 0);
	DoKeyDownDelay();
	keybd_event((BYTE)vk, MapVirtualKey(vk, 0), KEYEVENTF_KEYUP, 0);
	DoKeyDelay();

	return bInitial;

} // SetToggleState()


///////////////////////////////////////////////////////////////////////////////
// SendCh()
//
// Sends the character "ch" as a keystroke
//
///////////////////////////////////////////////////////////////////////////////

void HS_SendKeys::SendCh(char ch, int nRep)
{
#ifdef _DEBUG
	Util_DebugMsg("==> HS_SendKeys::SendCh : ch=%d, nRep=%d)\n", ch, nRep);
#endif
	UINT	vkres;

	vkres = VkKeyScan(ch);						// get VK code and modifier info

	// Was a valid code obtained?  If not try and send it using special means
	// In these cases we IGNORE any modifiers as they won't work anyway with
	// the methods we use to send special chars
	if ( (int)vkres == -1)
	{
		SendSpecialCh(ch);
		return;
	}

	// Resolve any key modifiers with perm key downs and merges into m_nKeyMods
	ResolveKeyModifiers(vkres);

	// press the modifier keys required
	SimModsDown();

	while (nRep)
	{
		SimKeystroke((BYTE)(vkres & 0xff));		// Do the key (with automatic key delay)
		nRep--;
	}

	SimModsUp();

} // SendCh


///////////////////////////////////////////////////////////////////////////////
// SendVk()
//
// Used for sending keys like {ENTER} rather than normal chars.
//
///////////////////////////////////////////////////////////////////////////////

void HS_SendKeys::SendVk(UINT vk, int nRep, bool bExtended)
{
#ifdef _DEBUG
	Util_DebugMsg("==> HS_SendKeys::SendVk : vk=%d, nRep=%d, bExtended=%d)\n", vk, nRep, bExtended);
#endif
	// NOTE ch is a VK_CODE already

	// Resolve any key modifiers
	ResolveKeyModifiers(NONEMOD);

	SimModsDown();

	while (nRep)
	{
		SimKeystroke(vk, bExtended );			// Do the key(s)
		nRep --;
	}

	SimModsUp();

} // SendVk()


///////////////////////////////////////////////////////////////////////////////
// SendSpecialCh()
//
// When we try and get the VK code for a character and it fails we end up here.
//
// We will try sending the characters either by diadics or by using the ALT+0nnn
// syntax.  Note previous versions of this code had a bug where we used ALT+nnn
// (without the leading 0 which ends up using the obsolete 437 code page!)
//
///////////////////////////////////////////////////////////////////////////////

void  HS_SendKeys::SendSpecialCh(char ch)
{
#ifdef _DEBUG
	Util_DebugMsg("==> HS_SendKeys::SendSpecialCh : ch=%d)\n", ch);
#endif
// Table to convert char >=128 to {Asc nnn} or to a diadic + letter
// the comment above the value show as € due to Courier Font but they are real Ansi chars
// 0 means no translation = try using the ALT+0nnn syntax
// value >127 (x7f) will be the translated value sent by ALT+nnn (not ALT+0nnn)
// value <128 define the diadic 0-3 bits index in g_cDiadic[] : if ' '  will not be sent
//								4-7 bits index in g_cDiadicLetter[]
	static char cAnsiToAscii [128] =
	{
// 80   €            ‚      ƒ      „      …      †      ‡      ˆ      ‰      Š      ‹      Œ            Ž      
     '\x80','\x81','\x82','\x83','\x84','\x85','\x86','\x87','\x88','\x89','\x8a','\x8b','\x8c','\x8d','\x8e','\x8f',

// 90         ‘      ’      “      ”     •:f9    –      —      ˜      ™      š      ›      œ            ž      Ÿ
     '\x90','\x91','\x92','\x93','\x94','\x95','\x96','\x97','\x98','\x99','\x9a','\x9b','\x9c','\x9d','\x9e','\x9f',

// A0          ¡      ¢      £      ¤      ¥      ¦      §      ¨      ©      ª      «      ¬      ­      ®      ¯
     '\xa0','\xa1','\xa2','\xa3','\xa4','\xa5','\xa6','\x15','\xa8','\xa9','\xaa','\xab','\xac','\xad','\xae','\xaf',

// B0   °      ±      ²      ³      ´      µ      ¶      ·      ¸      ¹      º      »      ¼      ½      ¾      ¿
     '\xb0','\xb1','\xb2','\xb3','\xb4','\xb5','\x14','\xb7','\xb8','\xb9','\xba','\xbb','\xbc','\xbd','\xbe','\xbf',

// C0   À      Á      Â      Ã      Ä      Å      Æ      Ç      È      É      Ê      Ë      Ì      Í      Î      Ï
     '\x62','\x22','\x32','\x42','\xc4','\xc5','\xc6','\xc7','\x64','\xc9','\x34','\x54','\x66','\x26','\x36','\x56',

// D0   Ð      Ñ      Ò      Ó      Ô      Õ      Ö      ×      Ø      Ù      Ú      Û      Ü      Ý      Þ      ß
     '\xd0','\xd1','\x68','\x28','\x38','\x48','\xd6','\xd7','\xd8','\x6a','\x2a','\x3a','\xdc','\x2c','\xde','\xdf',

// E0   à      á      â      ã      ä      å      æ      ç      è      é      ê      ë      ì      í      î      ï
     '\xe0','\xe1','\xe2','\x41','\xe4','\xe5','\xe6','\xe7','\xe8','\xe9','\xea','\xeb','\xec','\xed','\xee','\xef',

// F0   ð      ñ      ò      ó      ô      õ      ö      ÷      ø      ù      ú      û      ü      ý      þ      ÿ
     '\xf0','\xf1','\xf2','\xf3','\xf4','\x47','\xf6','\xf7','\xf8','\xf9','\xfa','\xfb','\xfc','\x2b','\xfe','\xff'
	};

//                                      0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
	static char g_cDiadicLetter[16] = {' ','a','A','e','E','i','I','o','O','u','U','y','Y','n','N',' '};
	char szAsc[10];

	int i = (ch - 128) & 0xff;
	i = cAnsiToAscii[i] & 0xff;

	// Do the key simulation using {ASC nnn} - not {ASC 0nnn}
	// Only the char code between whose corresponding value
	// in cAnsiToAscii[] >=128 can be sent directly
	if ( i>=128 || i<32 )
	{
		sprintf(szAsc, "ASC 0%d", i);
		SendSpecial(szAsc);
		m_nKeyMod &= RESETMOD;		// reset modifiers
		return;
	}


	// Try the key simulation using diadic keystrokes
	char ch1,ch2;

	// pick up the diadic char according to cTradAnsiLetter
	// 0-3 part is the index in szDiadic defining the first character to be sent
	//        this implementation will allow to change the szDiadic value
	//			and if set to blank not to send the diadic char
	//			This will allow to treat without extra char when language keyboard
	//			does not support this diadic char Ex; ~ in german

	ch1 = g_cDiadic[i>>4];

	// If our keyboard doesn't support diadics then we have no choice but to send the
	// ALT+0nnn code instead!
	if (ch1 == ' ')
	{
		// Doesn't support required diadic - i.e. english keyboards
		sprintf(szAsc, "ASC 0%d", (uchar)ch);
		SendSpecial(szAsc);
		m_nKeyMod &= RESETMOD;		// reset modifiers
	}
	else
	{
		// Does support the diadic key
		SendCh(ch1, 1);						// send diadic character
		m_nKeyMod &= RESETMOD;				// reset modifiers

		ch2 = g_cDiadicLetter[i & 0x0f];
		SendCh(ch2,1 );						// send non accent character
		m_nKeyMod &= RESETMOD;				// reset modifiers
	}

} // SendSpecialCh()


///////////////////////////////////////////////////////////////////////////////
// ResolveKeyModifiers()
//
// Merges the requested key's modifiers with any permanent {ALTDOWN} type
// modifiers and also TRIES to take into account the physical state of the modifier
// keys in case the user is messing around
//
///////////////////////////////////////////////////////////////////////////////

void HS_SendKeys::ResolveKeyModifiers(UINT vkres)
{
	// Examine the vkres code and see what state modifiers it is requesting
	if ( (vkres & 0x0200)  ) 					// CTRL required?
		m_nKeyMod |= CTRLMOD;

	if ( (vkres & 0x0400) )						// ALT required?
		m_nKeyMod |= ALTMOD;

	if ( (vkres & 0x0100) )						// SHIFT required?
		m_nKeyMod |= SHIFTMOD;

	// if we have permanent key down situation, we don't want to press those
	// keys again so we remove that key!

	if ( (m_nKeyMod & CTRLMOD) && (m_nKeyMod & CTRLPERMMOD) )
		m_nKeyMod ^= CTRLMOD;					// remove ctrl flag

	if ( (m_nKeyMod & ALTMOD) && (m_nKeyMod & ALTPERMMOD) )
		m_nKeyMod ^= ALTMOD;					// remove alt flag

	if ( (m_nKeyMod & SHIFTMOD) && (m_nKeyMod & SHIFTPERMMOD) )
		m_nKeyMod ^= SHIFTMOD;					// remove shift flag

	if ( (m_nKeyMod & LWINMOD) && (m_nKeyMod & LWINPERMMOD) )
		m_nKeyMod ^= LWINMOD;					// remove left win flag

#ifdef _DEBUG
	Util_DebugMsg("==> HS_SendKeys::ResolveKeyModifiers : vkres=%d, m_nKeyMod=%d)\n", vkres, m_nKeyMod);
#endif

	// Now check the physical state (as best as the buggy API lets us) to see if any
	// of the modifier keys are held down.  If they are - and none of the CTRLPER type
	// flags are used then force them to be released - even if our next keypress is
	// going to use the same modifier - it's just not close to reliable that way
	// This is not perfect, and if the user continues to twat around it will fail
	if (m_hWnd)
	{
		BYTE	KeybdState[256];

		GetKeyboardState((LPBYTE)&KeybdState);

		// In sendTo mode just make sure that the new keyboard state has all the mods
		// reset unless they are perm on
		if (!(m_nKeyMod & CTRLPERMMOD))
			KeybdState[VK_CONTROL] = 0x00;
		if (!(m_nKeyMod & ALTPERMMOD))
			KeybdState[VK_MENU] = 0x00;
		if (!(m_nKeyMod & SHIFTPERMMOD))
			KeybdState[VK_SHIFT] = 0x00;
		if (!(m_nKeyMod & LWINPERMMOD))
			KeybdState[VK_LWIN] = 0x00;

		SetKeyboardState((LPBYTE)&KeybdState);
	}
	else
	{
		// keybd_event() mode
		if ( IsPhysicallyDown(VK_CONTROL) && (!(m_nKeyMod & CTRLPERMMOD)) )
		{
			SimKeyUp(VK_CONTROL);
			DoKeyDelay();
		}

		if ( IsPhysicallyDown(VK_MENU) && (!(m_nKeyMod & ALTPERMMOD)) )
		{
			SimKeyUp(VK_MENU);
			DoKeyDelay();
		}

		if ( IsPhysicallyDown(VK_SHIFT) && (!(m_nKeyMod & SHIFTPERMMOD)) )
		{
			SimKeyUp(VK_SHIFT);
			DoKeyDelay();
		}

		if ( IsPhysicallyDown(VK_LWIN) && (!(m_nKeyMod & LWINPERMMOD)) )
		{
			SimKeyUp(VK_LWIN);
			DoKeyDelay();
		}
	}

} // ResolveKeyModifiers()


///////////////////////////////////////////////////////////////////////////////
// IsVKExtended()
//
// Checks if a given Virtual Key code is an "extended" key
//
///////////////////////////////////////////////////////////////////////////////

bool HS_SendKeys::IsVKExtended(UINT key)
{
	if (key == VK_INSERT || key == VK_DELETE || key == VK_END || key == VK_DOWN ||
		key == VK_NEXT || key == VK_LEFT || key == VK_RIGHT || key == VK_HOME || key == VK_UP ||
		key == VK_PRIOR || key == VK_DIVIDE || key == VK_APPS || key == VK_LWIN || key == VK_RWIN ||
		key == VK_RMENU || key == VK_RCONTROL || key == VK_SLEEP || key == VK_BROWSER_BACK ||
		key == VK_BROWSER_FORWARD || key == VK_BROWSER_REFRESH || key == VK_BROWSER_STOP ||
		key == VK_BROWSER_SEARCH || key == VK_BROWSER_FAVORTIES || key == VK_BROWSER_HOME ||
		key == VK_VOLUME_MUTE || key == VK_VOLUME_DOWN || key == VK_VOLUME_UP || key == VK_MEDIA_NEXT_TRACK ||
		key == VK_MEDIA_PREV_TRACK || key == VK_MEDIA_STOP || key == VK_MEDIA_PLAY_PAUSE ||
		key == VK_LAUNCH_MAIL || key == VK_LAUNCH_MEDIA_SELECT || key == VK_LAUNCH_APP1 || key == VK_LAUNCH_APP2)
	{
		return true;
	}
	else
		return false;

} // IsVKExtended()


///////////////////////////////////////////////////////////////////////////////
// SimModsDown()
///////////////////////////////////////////////////////////////////////////////

void HS_SendKeys::SimModsDown(void)
{
#ifdef _DEBUG
	Util_DebugMsg("==> HS_SendKeys::SimModsDown : m_nKeyMod=%d)\n", m_nKeyMod);
#endif
	// If the window is NULL use keybd_event
	if (m_hWnd == NULL)
	{
		if ( m_nKeyMod & LWINMOD ) 				// WIN required?
			keybd_event(VK_LWIN, m_scanLWin, 0, 0);
		if ( m_nKeyMod & SHIFTMOD ) 			// SHIFT required?
			keybd_event(VK_SHIFT, m_scanShift, 0, 0);
		if ( m_nKeyMod & CTRLMOD ) 				// CTRL required?
			keybd_event(VK_CONTROL, m_scanCtrl, 0, 0);
		if ( m_nKeyMod & ALTMOD ) 				// ALT required?
			keybd_event(VK_MENU, m_scanAlt, 0, 0);
	}
	else
	{
		LPARAM	lparam;
		BYTE	KeybdState[256];

		GetKeyboardState((LPBYTE)&KeybdState);

		// First alter the keyboard state to match the mods we are about to send
		if ( m_nKeyMod & LWINMOD ) 				// WIN required?
			KeybdState[VK_LWIN] |= 0x80;
		if ( m_nKeyMod & SHIFTMOD ) 			// SHIFT required?
			KeybdState[VK_SHIFT] |= 0x80;
		if ( m_nKeyMod & CTRLMOD ) 				// CTRL required?
			KeybdState[VK_CONTROL] |= 0x80;
		if (m_nKeyMod & ALTMOD)					// ALT required?
			KeybdState[VK_MENU] |= 0x80;

		SetKeyboardState((LPBYTE)&KeybdState);

		// Now send the individual WM_KEY messages
		if ( m_nKeyMod & LWINMOD ) 				// WIN required?
		{
			lparam = 0x00000001 | (LPARAM)(m_scanLWin << 16);
			PostMessage(m_hWnd, WM_KEYDOWN, VK_LWIN, lparam);
		}

		if ( m_nKeyMod & SHIFTMOD ) 			// SHIFT required?
		{
			lparam = 0x00000001 | (LPARAM)(m_scanShift << 16);
			PostMessage(m_hWnd, WM_KEYDOWN, VK_SHIFT, lparam);
		}

		if ( m_nKeyMod & CTRLMOD ) 				// CTRL required?
		{
			lparam = 0x00000001 | (LPARAM)(m_scanCtrl << 16);
			PostMessage(m_hWnd, WM_KEYDOWN, VK_CONTROL, lparam);
		}

		if ( (m_nKeyMod & ALTMOD) && !(m_nKeyMod & CTRLMOD) )	// Alt without Ctrl
		{
			lparam = 0x20000001 | (LPARAM)(m_scanAlt << 16);		// AltDown=1, repeat=1
			PostMessage(m_hWnd, WM_SYSKEYDOWN, VK_MENU, lparam);
		}
		else if (m_nKeyMod & ALTMOD)			// Alt with Ctrl
		{
			lparam = 0x00000001 | (LPARAM)(m_scanAlt << 16);
			PostMessage(m_hWnd, WM_KEYDOWN, VK_MENU, lparam);
		}
	}

	// Key down key delay
	DoKeyDownDelay();

} // SimModsDown()


///////////////////////////////////////////////////////////////////////////////
// SimModsUp()
///////////////////////////////////////////////////////////////////////////////

void HS_SendKeys::SimModsUp(void)
{
#ifdef _DEBUG
	Util_DebugMsg("==> HS_SendKeys::SimModsUp : m_nKeyMod=%d)\n", m_nKeyMod);
#endif
	// If the window is NULL use keybd_event
	if (m_hWnd == NULL)
	{
		if ( m_nKeyMod & ALTMOD ) 				// ALT required?
			keybd_event(VK_MENU, m_scanAlt, KEYEVENTF_KEYUP, 0);
		if ( m_nKeyMod & CTRLMOD ) 				// CTRL required?
			keybd_event(VK_CONTROL, m_scanCtrl, KEYEVENTF_KEYUP, 0);
		if ( m_nKeyMod & SHIFTMOD ) 			// SHIFT required?
			keybd_event(VK_SHIFT, m_scanShift, KEYEVENTF_KEYUP, 0);
		if ( m_nKeyMod & LWINMOD ) 				// WIN required?
			keybd_event(VK_LWIN, m_scanLWin, KEYEVENTF_KEYUP, 0);
	}
	else
	{
		LPARAM	lparam;
		BYTE	KeybdState[256];

		GetKeyboardState((LPBYTE)&KeybdState);

		if ( (m_nKeyMod & ALTMOD) && !(m_nKeyMod & CTRLMOD) )	// Alt without Ctrl
		{
			lparam = 0xE0000001 | (LPARAM)(m_scanAlt << 16);	// AltDown=1, Repeat=1, Key = up
			PostMessage(m_hWnd, WM_SYSKEYUP, VK_MENU, lparam);
		}
		else if (m_nKeyMod & ALTMOD)			// Alt with Ctrl
		{
			lparam = 0xC0000001 | (LPARAM)(m_scanAlt << 16);
			PostMessage(m_hWnd, WM_KEYUP, VK_MENU, lparam);
		}

		if ( m_nKeyMod & CTRLMOD ) 				// CTRL required?
		{
			lparam = 0xC0000001 | (LPARAM)(m_scanCtrl << 16);
			PostMessage(m_hWnd, WM_KEYUP, VK_CONTROL, lparam);
		}

		if ( m_nKeyMod & SHIFTMOD ) 			// SHIFT required?
		{
			lparam = 0xC0000001 | (LPARAM)(m_scanShift << 16);
			PostMessage(m_hWnd, WM_KEYUP, VK_SHIFT, lparam);
		}

		if ( m_nKeyMod & LWINMOD ) 				// WIN required?
		{
			lparam = 0xC0000001 | (LPARAM)(m_scanLWin << 16);
			PostMessage(m_hWnd, WM_KEYUP, VK_LWIN, lparam);
		}

		// Now alter the keyboard state to match the mods we just sent
		if ( m_nKeyMod & LWINMOD ) 				// WIN required?
			KeybdState[VK_LWIN] ^= 0x80;
		if ( m_nKeyMod & SHIFTMOD ) 			// SHIFT required?
			KeybdState[VK_SHIFT] ^= 0x80;
		if ( m_nKeyMod & CTRLMOD ) 				// CTRL required?
			KeybdState[VK_CONTROL] ^= 0x80;
		if (m_nKeyMod & ALTMOD)					// ALT required?
			KeybdState[VK_MENU] ^= 0x80;

		SetKeyboardState((LPBYTE)&KeybdState);
	}

	// Key up keydelay
	DoKeyDelay();

} // SimModsUp()


///////////////////////////////////////////////////////////////////////////////
// SimKeystroke()
///////////////////////////////////////////////////////////////////////////////

void HS_SendKeys::SimKeystroke(UINT vk, bool bForceExtended)
{
#ifdef _DEBUG
	Util_DebugMsg("==> HS_SendKeys::SimKeystroke : vk=%d, bForceExtended=%d)\n", vk, bForceExtended);
#endif

	SimKeyDown(vk, bForceExtended);
	DoKeyDownDelay();							// Hold key down

	SimKeyUp(vk, bForceExtended);
	DoKeyDelay();								// Delay before next key stroke

} // SimKeystroke()


///////////////////////////////////////////////////////////////////////////////
// SimKeyDown()
///////////////////////////////////////////////////////////////////////////////

void HS_SendKeys::SimKeyDown(UINT vk, bool bForceExtended)
{
#ifdef _DEBUG
	Util_DebugMsg("==> HS_SendKeys::SimKeyDown : vk=%d, bForceExtended=%d)\n", vk, bForceExtended);
#endif
	UINT	scan;
	LPARAM	lparam;

	scan = MapVirtualKey(vk, 0);

	// If the window is NULL
	if (m_hWnd == NULL || vk == VK_CAPITAL || vk == VK_NUMLOCK || vk == VK_SCROLL)
	{
		if (bForceExtended == true || IsVKExtended(vk) == true)
			keybd_event((BYTE)vk, (BYTE)scan, KEYEVENTF_EXTENDEDKEY, 0);
		else
			keybd_event((BYTE)vk, (BYTE)scan, 0, 0);
	}
	else
	{
		// Build the generic lparam to be used for WM_KEYDOWN/WM_KEYUP/WM_CHAR
		lparam = 0x00000001 | (LPARAM)(scan << 16);			// Scan code, repeat=1
		if (bForceExtended == true || IsVKExtended(vk) == true)
			lparam = lparam | 0x01000000;		// Extended code if required

		if ( (m_nKeyMod & ALTMOD) && !(m_nKeyMod & CTRLMOD) )	// Alt without Ctrl
			PostMessage(m_hWnd, WM_SYSKEYDOWN, vk, lparam | 0x20000000);	// Key down, AltDown=1
		else
			PostMessage(m_hWnd, WM_KEYDOWN, vk, lparam);	// Key down
	}

} // SimKeyDown()


///////////////////////////////////////////////////////////////////////////////
// SimKeyUp()
///////////////////////////////////////////////////////////////////////////////

void HS_SendKeys::SimKeyUp(UINT vk, bool bForceExtended)
{
#ifdef _DEBUG
	Util_DebugMsg("==> HS_SendKeys::SimKeyUp : vk=%d, bForceExtended=%d)\n", vk, bForceExtended);
#endif
	UINT	scan;
	LPARAM	lparam;

	scan = MapVirtualKey(vk, 0);

	// If the window is NULL or a modifer keypress (shift, alt, etc) is requested
	// then use the keybd_event routines
	if (m_hWnd == NULL || vk == VK_CAPITAL || vk == VK_NUMLOCK || vk == VK_SCROLL)
	{
		if (bForceExtended == true || IsVKExtended(vk) == true)
			keybd_event((BYTE)vk, (BYTE)scan, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
		else
			keybd_event((BYTE)vk, (BYTE)scan, KEYEVENTF_KEYUP, 0);
	}
	else
	{
		// Build the generic lparam to be used for WM_KEYDOWN/WM_KEYUP/WM_CHAR
		lparam = 0xC0000001 | (LPARAM)(scan << 16);			// Scan code, Repeat=1, Keyup
		if (bForceExtended == true || IsVKExtended(vk) == true)
			lparam = lparam | 0x01000000;		// Extended code if required

		if ( (m_nKeyMod & ALTMOD) && !(m_nKeyMod & CTRLMOD) )	// Alt without Ctrl
			PostMessage(m_hWnd, WM_SYSKEYUP, vk, lparam | 0x20000000);	// Key down, AltDown=1
		else
			PostMessage(m_hWnd, WM_KEYUP, vk, lparam);
	}

} // SimKeyUp()


///////////////////////////////////////////////////////////////////////////////
// SendSpecial()
///////////////////////////////////////////////////////////////////////////////

void HS_SendKeys::SendSpecial(char *szTemp)
{
#ifdef _DEBUG
	Util_DebugMsg("==> HS_SendKeys::SendSpecial : szTemp=%s)\n", szTemp);
#endif
	int		nRep;
	int		nPos	= 0;
	int		n		= 0;
	bool	bUp		= false;
	bool	bDown	= false;
	bool	bOn		= false;
	bool	bOff	= false;

	char	*szCmd;
	char	*szRep;

	// Allocate temp memory
	szCmd = new char[strlen(szTemp)+1];
	szRep = new char[strlen(szTemp)+1];

	nRep = 1;									// Default number of repeats

	if ( !ReadToChar(' ', szTemp, szCmd, nPos) )
	{
		// Skip spaces to get to the number of repeats or "up", "down"
		while (szTemp[nPos] == ' ' || szTemp[nPos] == '\t')
			nPos++;

		ReadToChar('\0', szTemp, szRep, nPos);

		if (!stricmp(szRep, "up"))
			bUp = true;
		else if (!stricmp(szRep, "down"))
			bDown = true;
		else if (!stricmp(szRep, "on"))
			bOn = true;
		else if (!stricmp(szRep, "off"))
			bOff = true;
		else
		{
			nRep = atoi(szRep);
			if (nRep <= 0)
				nRep = 1;
		}
	}

	// Look up the index of the key
	while ( (n < NUMKEYS) && (stricmp(g_szKeyTable[n], szCmd)) )
		n++;

	// if unknown command, send the first letter
	if (n == NUMKEYS)
	{
		if (bDown)
			SimKeyDown(VkKeyScan(szCmd[0]) & 0xff);
		else if (bUp)
			SimKeyUp(VkKeyScan(szCmd[0]) & 0xff);
		else
			SendCh(szCmd[0], nRep);				// unknown command, send first char
	}
	else
	{
		// the command HAS been found if we are here...
		// n is the index
		// check if this is a simple lookup or a special function
		if (g_cKeyLookupType[n] == SK_LOOKUP)
		{
			if (bDown)
				SimKeyDown(g_nKeyCodes[n]);
			else if (bUp)
				SimKeyUp(g_nKeyCodes[n]);
			else if (bOn)
				SetToggleState(g_nKeyCodes[n], true);
			else if (bOff)
				SetToggleState(g_nKeyCodes[n], false);
			else
				SendVk(g_nKeyCodes[n], nRep);
		}
		else
		{
			// Special function
			switch ( g_nKeyCodes[n] )
			{
				case ALT:
					if ( !(m_nKeyMod & ALTPERMMOD) )
					{
						m_nKeyMod |= ALTMOD;// We have to use Mods to get the ALT SYSKEY in SendTo mode

						if (bDown)
							SimKeyDown(VK_MENU);
						else if (bUp)
							SimKeyUp(VK_MENU);
						else
							SimKeystroke(VK_MENU);
					}

					break;

				case NUMPADENTER:
					// This is the same as VK_RETURN but we set the extended key code
					// to indicate that it is on the numpad and not main keyboard
					if (bDown)
						SimKeyDown(VK_RETURN);
					else if (bUp)
						SimKeyUp(VK_RETURN);
					else
						SimKeystroke(VK_RETURN, true);
					break;

				case ALTDOWN:
					if ( !(m_nKeyMod & ALTPERMMOD) )
					{
						m_nKeyMod |= ALTMOD;// We have to use Mods to get the ALT SYSKEY in SendTo mode

						m_nKeyMod |= ALTPERMMOD;
						SimKeyDown(VK_MENU);
					}
					break;

				case ALTUP:
					if ( m_nKeyMod & ALTPERMMOD)
					{
						m_nKeyMod |= ALTMOD;// We have to use Mods to get the ALT SYSKEY in SendTo mode

						m_nKeyMod ^= ALTPERMMOD;
						SimKeyUp(VK_MENU);
					}
					break;

				case SHIFTDOWN:
					if ( !(m_nKeyMod & SHIFTPERMMOD) )
					{
						m_nKeyMod |= SHIFTPERMMOD;
						SimKeyDown(VK_SHIFT);
					}
					break;

				case SHIFTUP:
					if ( m_nKeyMod & SHIFTPERMMOD)
					{
						m_nKeyMod ^= SHIFTPERMMOD;
						SimKeyUp(VK_SHIFT);
					}
					break;

				case CTRLDOWN:
					if ( !(m_nKeyMod & CTRLPERMMOD) )
					{
						m_nKeyMod |= CTRLPERMMOD;
						SimKeyDown(VK_CONTROL);
					}
					break;

				case CTRLUP:
					if ( m_nKeyMod & CTRLPERMMOD)
					{
						m_nKeyMod ^= CTRLPERMMOD;
						SimKeyUp(VK_CONTROL);
					}
					break;

				case LWINDOWN:
					if ( !(m_nKeyMod & LWINPERMMOD) )
					{
						m_nKeyMod |= LWINPERMMOD;
						SimKeyDown(VK_LWIN);
					}
					break;

				case LWINUP:
					if ( m_nKeyMod & LWINPERMMOD)
					{
						m_nKeyMod ^= LWINPERMMOD;
						SimKeyUp(VK_LWIN);
					}
					break;

				case RWINDOWN:
					if ( !(m_nKeyMod & RWINPERMMOD) )
					{
						m_nKeyMod |= RWINPERMMOD;
						SimKeyDown(VK_RWIN);
					}
					break;

				case RWINUP:
					if ( m_nKeyMod & RWINPERMMOD)
					{
						m_nKeyMod ^= RWINPERMMOD;
						SimKeyUp(VK_RWIN);
					}
					break;


				case ASC:
					// convert nRep to text and send the chars with ALT held down
					// For this to work we need to use a keybd_event
					// keystroke for the ALT key

					// If in SendTo mode this will fail unless we have the controls parent window
					// active :( - Removed 3.0.103 - Probably not a useful thing to force
					//if (m_hWnd && GetForegroundWindow() != GetParent(m_hWnd) && GetParent(m_hWnd) != NULL)
					//{
						//MessageBox(NULL, "", "", MB_OK);
						//g_oSetForeWinEx.Activate(GetParent(m_hWnd));
						//::Sleep(250);
					//	break;
					//}

					if ( !(m_nKeyMod & ALTPERMMOD) )
					{
						keybd_event(VK_MENU, m_scanAlt, 0, 0);
						DoKeyDownDelay();
					}

					// ASCII 0 is 48, NUMPAD0 is 96, add on 48 to the ASCII
					n = 0;
					while (szRep[n] != '\0')
					{
						SimKeystroke(szRep[n]+48);	// Auto key delays
						n++;
					}

					if ( !(m_nKeyMod & ALTPERMMOD) )
					{
						keybd_event(VK_MENU, m_scanAlt, KEYEVENTF_KEYUP, 0);
						DoKeyDelay();
					}

					break;

				} // End Switch

			} // End If

	} // End If

	// Clean up temp memory
	delete []szCmd;
	delete []szRep;

} // SendSpecial()


///////////////////////////////////////////////////////////////////////////////
// ReadToChar()
///////////////////////////////////////////////////////////////////////////////

int HS_SendKeys::ReadToChar(char ch, const char *szLine, char *szResult, int &nPos)
{
	int		nFlag	= 1;
	int		nResPos	= 0;
	char	cTemp;

	// Find the selected char, and seperate
	while ( ( (cTemp = szLine[nPos]) != '\0') && (nFlag == 1) )
	{
		if (cTemp == ch)
			nFlag = 0;
		else
			szResult[nResPos++] = cTemp;

		nPos++;									// skip to next char

	} // End While


	// End the temp string, whether ch was found or not
	szResult[nResPos] = '\0';

	// if the char to search for was '\0' then it WILL have been found
	if (ch == '\0')
		return 0;
	else
		return nFlag;							// 1 is bad, 0 is good

} // ReadToChar()


///////////////////////////////////////////////////////////////////////////////
// GetSingleVKandMods()
// Get a single VK code and modifiers for a given string - used for getting
// single hotkey/shortcut key definitions
///////////////////////////////////////////////////////////////////////////////

bool HS_SendKeys::GetSingleVKandMods(const char *szString, UINT &vk, bool &bShift, bool &bControl, bool &bAlt, bool &bWin)
{
	int		nPos = 0;
	char	ch;
	char	*szTemp;
	bool	bRes = true;						// Success by default
	int		n = 0;

	// Reset mods
	bShift = bControl = bAlt = bWin = false;

	// Allocate some temporary memory for szTemp
	szTemp =  new char[strlen(szString)+1];


	while ( szString[nPos] == '+' || szString[nPos] == '^' || szString[nPos] == '!' || szString[nPos] == '#' )
	{
		// Is there a modifier requested?
		if (szString[nPos] == '+')
			bShift = true;
		else if (szString[nPos] == '^')
			bControl = true;
		else if (szString[nPos] == '!')
			bAlt = true;
		else if (szString[nPos] == '#')
			bWin = true;

		++nPos;									// Next char
	}


	ch = szString[nPos++];						// Get next char


	// Is the next char a { or a simple key?
	if (ch == '{')
	{
		// Special key
		if ( ReadToChar('}', szString, szTemp, nPos) )
			bRes = false;						// Failed - no close bracket
		else
		{
			// Lookup special codes-  Look up the index of the key
			while ( (n < NUMKEYS) && (stricmp(g_szKeyTable[n], szTemp)) )
				n++;

			// Is it a known or valid key
			if (n == NUMKEYS)
			{
				bRes = false;							// Unknown
			}
			else
			{
				if (g_cKeyLookupType[n] != SK_LOOKUP)
					bRes = false;						// Invalid
				else
					vk = g_nKeyCodes[n];
			}
		}
	}
	else
	{
		// Simple char
		vk = VkKeyScan(ch);

		if ( (vk & 0x0200)  ) 					// CTRL required?
			bControl = true;

		if ( (vk & 0x0400) )					// ALT required?
			bAlt = true;

		if ( (vk & 0x0100) )					// SHIFT required?
			bShift = true;
	}

	// Make sure only the VK code (lower byte) is passed back (sans shift states)
	vk = vk & 0xff;

	// Free temp string memory
	delete [] szTemp;

	return bRes;

} // GetSingleVKandMods()

