
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
// script.cpp
//
// The main script object.
//
///////////////////////////////////////////////////////////////////////////////


// Includes
#include "StdAfx.h"								// Pre-compiled headers

#ifndef _MSC_VER								// Includes for non-MS compilers
	#include <stdio.h>
	#include <windows.h>
	#include <limits.h>
#else
	#include "qmath.h"							// MinGW doesn't like our asm maths functions
#endif

#include "AutoIt.h"								// Autoit values, macros and config options

#include "globaldata.h"
#include "script.h"
#include "resources\resource.h"
#include "utility.h"
#include "inputbox.h"


///////////////////////////////////////////////////////////////////////////////
// Initialize static data variables
//
// Note: the order of these keywords is critical and needs to match the order
// in script.h
//
///////////////////////////////////////////////////////////////////////////////

// Keyword values (must match the order as in script.cpp)
// Must be in UPPERCASE
char * AutoIt_Script::m_szKeywords[K_MAX] =	{
	"AND", "OR", "NOT",
	"IF", "THEN", "ELSE", "ELSEIF", "ENDIF",
	"WHILE", "WEND",
	"DO", "UNTIL",
	"FOR", "NEXT", "TO", "STEP",
	"EXITLOOP", "CONTINUELOOP",
	"SELECT", "CASE", "ENDSELECT",
	"DIM", "REDIM", "LOCAL", "GLOBAL", "CONST",
	"FUNC", "ENDFUNC", "RETURN",
	"EXIT",
	"BYREF"
};


///////////////////////////////////////////////////////////////////////////////
// Constructor()
///////////////////////////////////////////////////////////////////////////////

AutoIt_Script::AutoIt_Script()
{
	int i;

	m_bWinQuitProcessed			= false;

	m_nCurrentOperation			= AUT_RUN;		// Current operation is to run the script
	m_nExecuteRecursionLevel	= 0;			// Reset our recursion tracker for the Execute() function

	m_hWndTip					= NULL;			// ToolTip window

	m_nErrorLine				= 0;			// Set last line read as 0 by default (application.cpp may try reading it)
	m_nNumParams				= 0;			// Number of UDF parameters initially 0
	
	m_nCoordMouseMode			= AUT_COORDMODE_SCREEN;		// Mouse functions use screen coords by default
	m_nCoordPixelMode			= AUT_COORDMODE_SCREEN;		// Pixel functions use screen coords by default
	m_nCoordCaretMode			= AUT_COORDMODE_SCREEN;
	m_bRunErrorsFatal			= true;			// Run errors are fatal by default
	m_bExpandEnvStrings			= false;		// ENV expansion in strings is off by default
	m_bExpandVarStrings			= false;		// Var expansion in strings is off by default
	m_bMustDeclareVars			= false;		// Variables must be pre-declared
	m_bColorModeBGR				= false;		// Use RGB colours by default
	m_sOnExitFunc				= "OnAutoItExit";
	m_bFtpBinaryMode			= true;			// Use binary ftp transfers by default

	m_WindowSearchHWND			= NULL;			// Last window found set to NULL
	m_bDetectHiddenText			= false;		// Don't detect hidden text by default
	m_bWinSearchChildren		= false;		// Only search top level windows by default
	m_nWindowSearchMatchMode	= 1;			// Title match mode default is 1
	m_nWindowSearchTextMode		= 1;			// Text match mode default is 1
	m_nWinWaitDelay				= 250;			// Wait 250 ms after a window wait operation
	m_lpWinListFirst			= NULL;			// First entry in window list
	m_lpWinListLast				= NULL;			// First entry in window list
	m_nWinListCount				= 0;			// Number of entries

	m_bAdlibEnabled				= false;		// True if an adlib function is specified
	m_bAdlibInProgress			= false;		// True if we are currently running adlib function

	m_bGuiEventInProgress		= false;		// True if we are currently running event function

	m_oSendKeys.Init();							// Init sendkeys to defaults

	m_nMouseClickDelay			= 10;			// Time between mouse clicks
	m_nMouseClickDownDelay		= 10;			// Time the click is held down
	m_nMouseClickDragDelay		= 250;			// The delay at the start and end of a drag operation

	// RunAsSet stuff
	m_bRunAsSet					= false;		// Don't use RunAs by default
	m_wszRunUser = m_wszRunDom = m_wszRunPwd = NULL;	// Strings are empty


	// Make sure our lexer cache is set to empty defaults
	for (i=0; i<AUT_LEXER_CACHESIZE; ++i)
		m_LexerCache[i].nLineNum = -1;
	
	// Initialise file handles to NULL
	m_nNumFileHandles = 0;						// Initalise file handle count
	for (i=0; i<AUT_MAXOPENFILES; ++i)
		m_FileHandleDetails[i] = NULL;

	// Initialise DLL handles to NULL
	for (i=0; i<AUT_MAXOPENFILES; ++i)
		m_DLLHandleDetails[i] = NULL;

	// Initialise hotkeys to NULL 
	for (i=0; i<AUT_MAXHOTKEYS; ++i)
		m_HotKeyDetails[i] = NULL;
	m_nHotKeyQueuePos	= 0;

	
	// Proxy stuff
	m_nHttpProxyMode = AUT_PROXY_REGISTRY;		// Use whatever IE defaults have been set to
	m_nFtpProxyMode = AUT_PROXY_REGISTRY;		// Use whatever IE defaults have been set to

	// Internet download/upload defaults
	m_InetGetDetails.bInProgress	= false;
	m_InetGetDetails.nBytesRead		= -1;


// Initialize the function list (very long - thank VC6 for being buggy)
// Functon names to be in UPPERCASE and in ALPHABETICAL ORDER (Use the TextPad sort function or similar)
// Failure to observe these instructions will be very bad...
AU3_FuncInfo funcList[] = 
{
	{"ABS", &AutoIt_Script::F_Abs, 1, 1},
	{"ACOS", &AutoIt_Script::F_ACos, 1, 1},
	{"ADLIBDISABLE", &AutoIt_Script::F_AdlibDisable, 0, 0},
	{"ADLIBENABLE", &AutoIt_Script::F_AdlibEnable, 1, 2},
	{"ASC", &AutoIt_Script::F_Asc, 1, 1},
	{"ASIN", &AutoIt_Script::F_ASin, 1, 1},
	{"ASSIGN", &AutoIt_Script::F_Assign, 2, 3},
	{"ATAN", &AutoIt_Script::F_ATan, 1, 1},
	{"AUTOITSETOPTION", &AutoIt_Script::F_AutoItSetOption, 2, 2},
	{"AUTOITWINGETTITLE", &AutoIt_Script::F_AutoItWinGetTitle, 0, 0},
	{"AUTOITWINSETTITLE", &AutoIt_Script::F_AutoItWinSetTitle, 1, 1},
	{"BITAND", &AutoIt_Script::F_BitAND, 2, 255},
	{"BITNOT", &AutoIt_Script::F_BitNOT, 1, 1},
	{"BITOR", &AutoIt_Script::F_BitOR, 2, 255},
	{"BITSHIFT", &AutoIt_Script::F_BitShift, 2, 2},
	{"BITXOR", &AutoIt_Script::F_BitXOR, 2, 255},
	{"BLOCKINPUT", &AutoIt_Script::F_BlockInput, 1, 1},
	{"BREAK", &AutoIt_Script::F_Break, 1, 1},
	{"CALL", &AutoIt_Script::F_Call, 1, 1},
	{"CDTRAY", &AutoIt_Script::F_CDTray, 2, 2},
	{"CHR", &AutoIt_Script::F_Chr, 1, 1},
	{"CLIPGET", &AutoIt_Script::F_ClipGet, 0, 0},
	{"CLIPPUT", &AutoIt_Script::F_ClipPut, 1, 1},
	{"CONSOLEWRITE", &AutoIt_Script::F_ConsoleWrite, 1, 1},
	{"CONTROLCLICK", &AutoIt_Script::F_ControlClick, 3, 5},
	{"CONTROLCOMMAND", &AutoIt_Script::F_ControlCommand, 4, 5},
	{"CONTROLDISABLE", &AutoIt_Script::F_ControlDisable, 3, 3},
	{"CONTROLENABLE", &AutoIt_Script::F_ControlEnable, 3, 3},
	{"CONTROLFOCUS", &AutoIt_Script::F_ControlFocus, 3, 3},
	{"CONTROLGETFOCUS", &AutoIt_Script::F_ControlGetFocus, 1, 2},
	{"CONTROLGETHANDLE", &AutoIt_Script::F_ControlGetHandle, 3, 3},
	{"CONTROLGETPOS", &AutoIt_Script::F_ControlGetPos, 3, 3},
	{"CONTROLGETTEXT", &AutoIt_Script::F_ControlGetText, 3, 3},
	{"CONTROLHIDE", &AutoIt_Script::F_ControlHide, 3, 3},
//	{"CONTROLLISTVIEW", &AutoIt_Script::F_ControlListView, 4, 6},
	{"CONTROLMOVE", &AutoIt_Script::F_ControlMove, 5, 7},
	{"CONTROLSEND", &AutoIt_Script::F_ControlSend, 4, 5},
	{"CONTROLSETTEXT", &AutoIt_Script::F_ControlSetText, 4, 4},
	{"CONTROLSHOW", &AutoIt_Script::F_ControlShow, 3, 3},
	{"COS", &AutoIt_Script::F_Cos, 1, 1},
	{"DEC", &AutoIt_Script::F_Dec, 1, 1},
	{"DIRCOPY", &AutoIt_Script::F_DirCopy, 2, 3},
	{"DIRCREATE", &AutoIt_Script::F_DirCreate, 1, 1},
	{"DIRGETSIZE", &AutoIt_Script::F_DirGetSize, 1, 2},
	{"DIRMOVE", &AutoIt_Script::F_DirMove, 2, 3},
	{"DIRREMOVE", &AutoIt_Script::F_DirRemove, 1, 2},
//	{"DLLCALL", &AutoIt_Script::F_DllCall, 3, 255},
//	{"DLLCLOSE", &AutoIt_Script::F_DllClose, 1, 1},
//	{"DLLOPEN", &AutoIt_Script::F_DllOpen, 1, 1},
	{"DRIVEGETDRIVE", &AutoIt_Script::F_DriveGetDrive, 1, 1},
	{"DRIVEGETFILESYSTEM", &AutoIt_Script::F_DriveGetFileSystem, 1, 1},
	{"DRIVEGETLABEL", &AutoIt_Script::F_DriveGetLabel, 1, 1},
	{"DRIVEGETSERIAL", &AutoIt_Script::F_DriveGetSerial, 1, 1},
	{"DRIVEGETTYPE", &AutoIt_Script::F_DriveGetType, 1, 1},
	{"DRIVEMAPADD", &AutoIt_Script::F_DriveMapAdd, 2, 5},
	{"DRIVEMAPDEL", &AutoIt_Script::F_DriveMapDel, 1, 1},
	{"DRIVEMAPGET", &AutoIt_Script::F_DriveMapGet, 1, 1},
	{"DRIVESETLABEL", &AutoIt_Script::F_DriveSetLabel, 2, 2},
	{"DRIVESPACEFREE", &AutoIt_Script::F_DriveSpaceFree, 1, 1},
	{"DRIVESPACETOTAL", &AutoIt_Script::F_DriveSpaceTotal, 1, 1},
	{"DRIVESTATUS", &AutoIt_Script::F_DriveStatus, 1, 1},
	{"ENVGET", &AutoIt_Script::F_EnvGet, 1, 1},
	{"ENVSET", &AutoIt_Script::F_EnvSet, 1, 2},
	{"ENVUPDATE", &AutoIt_Script::F_EnvUpdate, 0, 0},
	{"EVAL", &AutoIt_Script::F_Eval, 1, 1},
	{"EXP", &AutoIt_Script::F_Exp, 1, 1},
	{"FILECHANGEDIR", &AutoIt_Script::F_FileChangeDir, 1, 1},
	{"FILECLOSE", &AutoIt_Script::F_FileClose, 1, 1},
	{"FILECOPY", &AutoIt_Script::F_FileCopy, 2, 3},
	{"FILECREATESHORTCUT", &AutoIt_Script::F_FileCreateShortcut, 2, 9},
	{"FILEDELETE", &AutoIt_Script::F_FileDelete, 1, 1},
	{"FILEEXISTS", &AutoIt_Script::F_FileExists, 1, 1},
	{"FILEFINDFIRSTFILE", &AutoIt_Script::F_FileFindFirstFile, 1, 1},
	{"FILEFINDNEXTFILE", &AutoIt_Script::F_FileFindNextFile, 1, 1},
	{"FILEGETATTRIB", &AutoIt_Script::F_FileGetAttrib, 1, 1},
	{"FILEGETLONGNAME", &AutoIt_Script::F_FileGetLongName, 1, 1},
	{"FILEGETSHORTCUT", &AutoIt_Script::F_FileGetShortcut, 1, 1},
	{"FILEGETSHORTNAME", &AutoIt_Script::F_FileGetShortName, 1, 1},
	{"FILEGETSIZE", &AutoIt_Script::F_FileGetSize, 1, 1},
	{"FILEGETTIME", &AutoIt_Script::F_FileGetTime, 1, 3},
	{"FILEGETVERSION", &AutoIt_Script::F_FileGetVersion, 1, 1},
	{"FILEINSTALL", &AutoIt_Script::F_FileInstall, 2, 3},
	{"FILEMOVE", &AutoIt_Script::F_FileMove, 2, 3},
	{"FILEOPEN", &AutoIt_Script::F_FileOpen, 2, 2},
	{"FILEOPENDIALOG", &AutoIt_Script::F_FileOpenDialog, 3, 5},
	{"FILEREAD", &AutoIt_Script::F_FileRead, 2, 2},
	{"FILEREADLINE", &AutoIt_Script::F_FileReadLine, 1, 2},
	{"FILERECYCLE", &AutoIt_Script::F_FileRecycle, 1, 1},
	{"FILERECYCLEEMPTY", &AutoIt_Script::F_FileRecycleEmpty, 0, 1},
	{"FILESAVEDIALOG", &AutoIt_Script::F_FileSaveDialog, 3, 5},
	{"FILESELECTFOLDER", &AutoIt_Script::F_FileSelectFolder, 2, 4},
	{"FILESETATTRIB", &AutoIt_Script::F_FileSetAttrib, 2, 3},
	{"FILESETTIME", &AutoIt_Script::F_FileSetTime, 2, 4},
	{"FILEWRITE", &AutoIt_Script::F_FileWrite, 2, 2},
	{"FILEWRITELINE", &AutoIt_Script::F_FileWriteLine, 2, 2},
	{"FTPSETPROXY", &AutoIt_Script::F_FtpSetProxy, 1, 4},

#ifdef AUT_CONFIG_GUI							// Is GUI enabled?
	{"GUICREATE", &AutoIt_Script::F_GUICreate, 1, 8},
	{"GUICTRLCREATEAVI", &AutoIt_Script::F_GUICtrlCreateAvi, 4, 8},
	{"GUICTRLCREATEBUTTON", &AutoIt_Script::F_GUICtrlCreateButton, 3, 7},
	{"GUICTRLCREATECHECKBOX", &AutoIt_Script::F_GUICtrlCreateCheckbox, 3, 7},
	{"GUICTRLCREATECOMBO", &AutoIt_Script::F_GUICtrlCreateCombo, 3, 7},
	{"GUICTRLCREATECONTEXTMENU", &AutoIt_Script::F_GUICtrlCreateContextMenu, 0, 1},
	{"GUICTRLCREATEDATE", &AutoIt_Script::F_GUICtrlCreateDate, 3, 7},
	{"GUICTRLCREATEDUMMY", &AutoIt_Script::F_GUICtrlCreateDummy, 0, 0},
	{"GUICTRLCREATEEDIT", &AutoIt_Script::F_GUICtrlCreateEdit, 3, 7},
	{"GUICTRLCREATEGROUP", &AutoIt_Script::F_GUICtrlCreateGroup, 3, 7},
	{"GUICTRLCREATEICON", &AutoIt_Script::F_GUICtrlCreateIcon, 4, 8},
	{"GUICTRLCREATEINPUT", &AutoIt_Script::F_GUICtrlCreateInput, 3, 7},
	{"GUICTRLCREATELABEL", &AutoIt_Script::F_GUICtrlCreateLabel, 3, 7},
	{"GUICTRLCREATELIST", &AutoIt_Script::F_GUICtrlCreateList, 3, 7},
	{"GUICTRLCREATELISTVIEW", &AutoIt_Script::F_GUICtrlCreateListView, 3, 7},
	{"GUICTRLCREATELISTVIEWITEM", &AutoIt_Script::F_GUICtrlCreateListViewItem, 2, 2},
	{"GUICTRLCREATEMENU", &AutoIt_Script::F_GUICtrlCreateMenu, 1, 3},
	{"GUICTRLCREATEMENUITEM", &AutoIt_Script::F_GUICtrlCreateMenuItem, 2, 4},
	{"GUICTRLCREATEPIC", &AutoIt_Script::F_GUICtrlCreatePic, 3, 7},
	{"GUICTRLCREATEPROGRESS", &AutoIt_Script::F_GUICtrlCreateProgress, 2, 6},
	{"GUICTRLCREATERADIO", &AutoIt_Script::F_GUICtrlCreateRadio, 3, 7},
	{"GUICTRLCREATESLIDER", &AutoIt_Script::F_GUICtrlCreateSlider, 2, 6},
	{"GUICTRLCREATETAB", &AutoIt_Script::F_GUICtrlCreateTab, 2, 6},
	{"GUICTRLCREATETABITEM", &AutoIt_Script::F_GUICtrlCreateTabitem, 1, 1},
	{"GUICTRLCREATETREEVIEW", &AutoIt_Script::F_GUICtrlCreateTreeView, 2, 6},
	{"GUICTRLCREATETREEVIEWITEM", &AutoIt_Script::F_GUICtrlCreateTreeViewItem, 2, 2},
	{"GUICTRLCREATEUPDOWN", &AutoIt_Script::F_GUICtrlCreateUpdown, 1, 2},
	{"GUICTRLDELETE", &AutoIt_Script::F_GUICtrlDelete, 1, 1},
	{"GUICTRLGETSTATE", &AutoIt_Script::F_GUICtrlGetState, 0, 1},
	{"GUICTRLREAD", &AutoIt_Script::F_GUIRead, 1, 1},
	{"GUICTRLRECVMSG", &AutoIt_Script::F_GUIRecvMsg, 2, 4},
	{"GUICTRLSENDMSG", &AutoIt_Script::F_GUISendMsg, 4, 4},
	{"GUICTRLSENDTODUMMY", &AutoIt_Script::F_GUISendToDummy, 1, 2},
	{"GUICTRLSETBKCOLOR", &AutoIt_Script::F_GUICtrlSetBkColor, 2, 2},
	{"GUICTRLSETCOLOR", &AutoIt_Script::F_GUICtrlSetColor, 2, 2},
	{"GUICTRLSETCURSOR", &AutoIt_Script::F_GUICtrlSetCursor, 2, 2},
	{"GUICTRLSETDATA", &AutoIt_Script::F_GUICtrlSetData, 2, 3},
	{"GUICTRLSETFONT", &AutoIt_Script::F_GUICtrlSetFont, 2, 5},
	{"GUICTRLSETIMAGE", &AutoIt_Script::F_GUICtrlSetImage, 2, 4},
	{"GUICTRLSETLIMIT", &AutoIt_Script::F_GUICtrlSetLimit, 2, 3},
	{"GUICTRLSETONEVENT", &AutoIt_Script::F_GUICtrlSetOnEvent, 2, 2},
	{"GUICTRLSETPOS", &AutoIt_Script::F_GUICtrlSetPos, 3, 5},
	{"GUICTRLSETRESIZING", &AutoIt_Script::F_GUICtrlSetResizing, 2, 2},
	{"GUICTRLSETSTATE", &AutoIt_Script::F_GUICtrlSetState, 2, 2},
	{"GUICTRLSETSTYLE", &AutoIt_Script::F_GUICtrlSetStyle, 2, 3},
	{"GUICTRLSETTIP", &AutoIt_Script::F_GUICtrlSetTip, 2, 2},
	{"GUIDELETE", &AutoIt_Script::F_GUIDelete, 0, 1},
	{"GUIGETCURSORINFO", &AutoIt_Script::F_GUIGetCursorInfo, 0, 1},
	{"GUIGETMSG", &AutoIt_Script::F_GUIGetMsg, 0, 1},
	{"GUISETBKCOLOR", &AutoIt_Script::F_GUISetBkColor, 1, 2},
	{"GUISETCOORD", &AutoIt_Script::F_GUISetCoord, 2, 5},
	{"GUISETCURSOR", &AutoIt_Script::F_GUISetCursor, 0, 3},
	{"GUISETFONT", &AutoIt_Script::F_GUISetFont, 1, 5},
	{"GUISETHELP", &AutoIt_Script::F_GUISetHelp, 1, 2},
	{"GUISETICON", &AutoIt_Script::F_GUISetIcon, 1, 3},
	{"GUISETONEVENT", &AutoIt_Script::F_GUISetOnEvent, 2, 3},
	{"GUISETSTATE", &AutoIt_Script::F_GUISetState, 0, 2},
	{"GUISTARTGROUP", &AutoIt_Script::F_GUIStartGroup, 0, 1},
	{"GUISWITCH", &AutoIt_Script::F_GUISwitch, 1, 1},
#endif

	{"HEX", &AutoIt_Script::F_Hex, 2, 2},
	{"HOTKEYSET", &AutoIt_Script::F_HotKeySet, 1, 2},
	{"HTTPSETPROXY", &AutoIt_Script::F_HttpSetProxy, 1, 4},
//	{"INETGET", &AutoIt_Script::F_InetGet, 1, 4},
//	{"INETGETSIZE", &AutoIt_Script::F_InetGetSize, 1, 1},
	{"INIDELETE", &AutoIt_Script::F_IniDelete, 2, 3},
	{"INIREAD", &AutoIt_Script::F_IniRead, 4, 4},
	{"INIREADSECTION", &AutoIt_Script::F_IniReadSection, 2, 2},
	{"INIREADSECTIONNAMES", &AutoIt_Script::F_IniReadSectionNames, 1, 1},
	{"INIWRITE", &AutoIt_Script::F_IniWrite, 4, 4},
	{"INPUTBOX", &AutoIt_Script::F_InputBox, 2, 9},
	{"INT", &AutoIt_Script::F_Int, 1, 1},
	{"ISADMIN", &AutoIt_Script::F_IsAdmin, 0, 0},
	{"ISARRAY", &AutoIt_Script::F_IsArray, 1, 1},
	{"ISDECLARED", &AutoIt_Script::F_IsDeclared, 1, 1},
	{"ISFLOAT", &AutoIt_Script::F_IsFloat, 1, 1},
	{"ISINT", &AutoIt_Script::F_IsInt, 1, 1},
	{"ISNUMBER", &AutoIt_Script::F_IsNumber, 1, 1},
	{"ISSTRING", &AutoIt_Script::F_IsString, 1, 1},
	{"LOG", &AutoIt_Script::F_Log, 1, 1},
	{"MEMGETSTATS", &AutoIt_Script::F_MemGetStats, 0, 0},
	{"MOD", &AutoIt_Script::F_Mod, 2, 2},
	{"MOUSECLICK", &AutoIt_Script::F_MouseClick, 1, 5},
	{"MOUSECLICKDRAG", &AutoIt_Script::F_MouseClickDrag, 5, 6},
	{"MOUSEDOWN", &AutoIt_Script::F_MouseDown, 1, 1},
	{"MOUSEGETCURSOR", &AutoIt_Script::F_MouseGetCursor, 0, 0},
	{"MOUSEGETPOS", &AutoIt_Script::F_MouseGetPos, 0, 0},
	{"MOUSEMOVE", &AutoIt_Script::F_MouseMove, 2, 3},
	{"MOUSEUP", &AutoIt_Script::F_MouseUp, 1, 1},
	{"MOUSEWHEEL", &AutoIt_Script::F_MouseWheel, 1, 2},
	{"MSGBOX", &AutoIt_Script::F_MsgBox, 3, 4},
	{"NUMBER", &AutoIt_Script::F_Number, 1, 1},
	{"OPT", &AutoIt_Script::F_AutoItSetOption, 2, 2},
//	{"PING", &AutoIt_Script::F_Ping, 1, 2},
	{"PIXELCHECKSUM", &AutoIt_Script::F_PixelChecksum, 4, 5},
	{"PIXELGETCOLOR", &AutoIt_Script::F_PixelGetColor, 2, 2},
	{"PIXELSEARCH", &AutoIt_Script::F_PixelSearch, 5, 7},
	{"PROCESSCLOSE", &AutoIt_Script::F_ProcessClose, 1, 1},
	{"PROCESSEXISTS", &AutoIt_Script::F_ProcessExists, 1, 1},
	{"PROCESSLIST", &AutoIt_Script::F_ProcessList, 0, 1},
	{"PROCESSSETPRIORITY", &AutoIt_Script::F_ProcessSetPriority, 2, 2},
	{"PROCESSWAIT", &AutoIt_Script::F_ProcessWait, 1, 2},
	{"PROCESSWAITCLOSE", &AutoIt_Script::F_ProcessWaitClose, 1, 2},
	{"PROGRESSOFF", &AutoIt_Script::F_ProgressOff, 0, 0},
	{"PROGRESSON", &AutoIt_Script::F_ProgressOn, 2, 6},
	{"PROGRESSSET", &AutoIt_Script::F_ProgressSet, 1, 3},
	{"RANDOM", &AutoIt_Script::F_Random, 0, 3},
	{"REGDELETE", &AutoIt_Script::F_RegDelete, 1, 2},
	{"REGENUMKEY", &AutoIt_Script::F_RegEnumKey, 2, 2},
	{"REGENUMVAL", &AutoIt_Script::F_RegEnumVal, 2, 2},
	{"REGREAD", &AutoIt_Script::F_RegRead, 2, 2},
	{"REGWRITE", &AutoIt_Script::F_RegWrite, 1, 4},
	{"ROUND", &AutoIt_Script::F_Round, 1, 2},
	{"RUN", &AutoIt_Script::F_Run, 1, 3},
	{"RUNASSET", &AutoIt_Script::F_RunAsSet, 0, 4},
	{"RUNWAIT", &AutoIt_Script::F_RunWait, 1, 3},
	{"SEND", &AutoIt_Script::F_Send, 1, 2},
	{"SETERROR", &AutoIt_Script::F_SetError, 1, 1},
	{"SETEXTENDED", &AutoIt_Script::F_SetExtended, 1, 1},
	{"SHUTDOWN", &AutoIt_Script::F_Shutdown, 1, 1},
	{"SIN", &AutoIt_Script::F_Sin, 1, 1},
	{"SLEEP", &AutoIt_Script::F_Sleep, 1, 1},
	{"SOUNDPLAY", &AutoIt_Script::F_SoundPlay, 1, 2},
	{"SOUNDSETWAVEVOLUME", &AutoIt_Script::F_SoundSetWaveVolume, 1, 1},
	{"SPLASHIMAGEON", &AutoIt_Script::F_SplashImageOn, 2, 7},
	{"SPLASHOFF", &AutoIt_Script::F_SplashOff, 0, 0},
	{"SPLASHTEXTON", &AutoIt_Script::F_SplashTextOn, 2, 10},
	{"SQRT", &AutoIt_Script::F_Sqrt, 1, 1},
//	{"STATUSBARGETTEXT", &AutoIt_Script::F_StatusbarGetText, 1, 3},
	{"STRING", &AutoIt_Script::F_String, 1, 1},
	{"STRINGADDCR", &AutoIt_Script::F_StringAddCR, 1, 1},
	{"STRINGFORMAT", &AutoIt_Script::F_StringFormat, 1, 33},
	{"STRINGINSTR", &AutoIt_Script::F_StringInStr, 2, 4},
	{"STRINGISALNUM", &AutoIt_Script::F_StringIsAlnum, 1, 1},
	{"STRINGISALPHA", &AutoIt_Script::F_StringIsAlpha, 1, 1},
	{"STRINGISASCII", &AutoIt_Script::F_StringIsASCII, 1, 1},
	{"STRINGISDIGIT", &AutoIt_Script::F_StringIsDigit, 1, 1},
	{"STRINGISFLOAT", &AutoIt_Script::F_StringIsFloat, 1, 1},
	{"STRINGISINT", &AutoIt_Script::F_StringIsInt, 1, 1},
	{"STRINGISLOWER", &AutoIt_Script::F_StringIsLower, 1, 1},
	{"STRINGISSPACE", &AutoIt_Script::F_StringIsSpace, 1, 1},
	{"STRINGISUPPER", &AutoIt_Script::F_StringIsUpper, 1, 1},
	{"STRINGISXDIGIT", &AutoIt_Script::F_StringIsXDigit, 1, 1},
	{"STRINGLEFT", &AutoIt_Script::F_StringLeft, 2, 2},
	{"STRINGLEN", &AutoIt_Script::F_StringLen, 1, 1},
	{"STRINGLOWER", &AutoIt_Script::F_StringLower, 1, 1},
	{"STRINGMID", &AutoIt_Script::F_StringMid, 2, 3},
//	{"STRINGREGEXP", &AutoIt_Script::F_StringRegExp, 2, 3},
//	{"STRINGREGEXPREPLACE", &AutoIt_Script::F_StringRegExpReplace, 3, 4},
	{"STRINGREPLACE", &AutoIt_Script::F_StringReplace, 3, 5},
	{"STRINGRIGHT", &AutoIt_Script::F_StringRight, 2, 2},
	{"STRINGSPLIT", &AutoIt_Script::F_StringSplit, 2, 3},
	{"STRINGSTRIPCR", &AutoIt_Script::F_StringStripCR, 1, 1},
	{"STRINGSTRIPWS", &AutoIt_Script::F_StringStripWS, 2, 2},
	{"STRINGTRIMLEFT", &AutoIt_Script::F_StringTrimLeft, 2, 2},
	{"STRINGTRIMRIGHT", &AutoIt_Script::F_StringTrimRight, 2, 2},
	{"STRINGUPPER", &AutoIt_Script::F_StringUpper, 1, 1},
	{"TAN", &AutoIt_Script::F_Tan, 1, 1},
	{"TIMERDIFF", &AutoIt_Script::F_TimerDiff, 1, 1},
	{"TIMERINIT", &AutoIt_Script::F_TimerInit, 0, 0},
	{"TIMERSTART", &AutoIt_Script::F_TimerInit, 0, 0},
	{"TIMERSTOP", &AutoIt_Script::F_TimerDiff, 1, 1},
	{"TOOLTIP", &AutoIt_Script::F_ToolTip, 1, 3},
	{"TRAYTIP", &AutoIt_Script::F_TrayTip, 3, 4},
	{"UBOUND", &AutoIt_Script::F_UBound, 1, 2},
//	{"URLDOWNLOADTOFILE", &AutoIt_Script::F_InetGet, 2, 2},
	{"VARTYPE", &AutoIt_Script::F_VarType, 1, 1},
	{"WINACTIVATE", &AutoIt_Script::F_WinActivate, 1, 2},
	{"WINACTIVE", &AutoIt_Script::F_WinActive, 1, 2},
	{"WINCLOSE", &AutoIt_Script::F_WinClose, 1, 2},
	{"WINEXISTS", &AutoIt_Script::F_WinExists, 1, 2},
	{"WINGETCARETPOS", &AutoIt_Script::F_WinGetCaretPos, 0, 0},
	{"WINGETCLASSLIST", &AutoIt_Script::F_WinGetClassList, 1, 2},
	{"WINGETCLIENTSIZE", &AutoIt_Script::F_WinGetClientSize, 1, 2},
	{"WINGETHANDLE", &AutoIt_Script::F_WinGetHandle, 1, 2},
	{"WINGETPOS", &AutoIt_Script::F_WinGetPos, 1, 2},
	{"WINGETPROCESS", &AutoIt_Script::F_WinGetProcess, 1, 2},
	{"WINGETSTATE", &AutoIt_Script::F_WinGetState, 1, 2},
	{"WINGETTEXT", &AutoIt_Script::F_WinGetText, 1, 2},
	{"WINGETTITLE", &AutoIt_Script::F_WinGetTitle, 1, 2},
	{"WINKILL", &AutoIt_Script::F_WinKill, 1, 2},
	{"WINLIST", &AutoIt_Script::F_WinList, 0, 2},
	{"WINMENUSELECTITEM", &AutoIt_Script::F_WinMenuSelectItem, 3, 9},
	{"WINMINIMIZEALL", &AutoIt_Script::F_WinMinimizeAll, 0, 0},
	{"WINMINIMIZEALLUNDO", &AutoIt_Script::F_WinMinimizeAllUndo, 0, 0},
	{"WINMOVE", &AutoIt_Script::F_WinMove, 4, 6},
	{"WINSETONTOP", &AutoIt_Script::F_WinSetOnTop, 3, 3},
	{"WINSETSTATE", &AutoIt_Script::F_WinShow, 3, 3},
	{"WINSETTITLE", &AutoIt_Script::F_WinSetTitle, 3, 3},
	{"WINSETTRANS", &AutoIt_Script::F_WinSetTrans, 3, 3},
	{"WINSHOW", &AutoIt_Script::F_WinShow, 3, 3},
	{"WINWAIT", &AutoIt_Script::F_WinWait, 1, 3},
	{"WINWAITACTIVE", &AutoIt_Script::F_WinWaitActive, 1, 3},
	{"WINWAITCLOSE", &AutoIt_Script::F_WinWaitClose, 1, 3},
	{"WINWAITNOTACTIVE", &AutoIt_Script::F_WinWaitNotActive, 1, 3}
};

	m_nFuncListSize = sizeof(funcList) / sizeof(AU3_FuncInfo);

	// Copy the function list into a member variable
	m_FuncList = new AU3_FuncInfo[m_nFuncListSize];
	for (i=0; i<m_nFuncListSize; ++i) 
	{
		m_FuncList[i].szName = Util_StrCpyAlloc(funcList[i].szName);
		m_FuncList[i].lpFunc = funcList[i].lpFunc;
		m_FuncList[i].nMin = funcList[i].nMin;
		m_FuncList[i].nMax = funcList[i].nMax;

#ifdef _DEBUG
		// test to make sure that the list is in order, but only during development
		if (i>0 && strcmp(m_FuncList[i-1].szName, m_FuncList[i].szName) > 0)	// out of sequence
		{
			// display out of order name before aborting.
			AUT_DEBUGMESSAGEBOX(m_FuncList[i].szName);
			AUT_ASSERT(strcmp(m_FuncList[i-1].szName, m_FuncList[i].szName) < 0);
		}
#endif

	}

} // AutoIt_Script()


///////////////////////////////////////////////////////////////////////////////
// Destructor()
///////////////////////////////////////////////////////////////////////////////

AutoIt_Script::~AutoIt_Script()
{
	int i;

	// Free up any thing

	// RunAsSet stuff
	delete [] m_wszRunUser;						// NULL if not used, which is OK
	delete [] m_wszRunDom;
	delete [] m_wszRunPwd;

	// Clear any WindowSearch() lists
	Win_WindowSearchDeleteList();

	// Destroy the ToolTip window if required
	if (m_hWndTip)
		DestroyWindow(m_hWndTip);

	// close SoundPlay handle
//	char	szBuffer[256] = "";
//	mciSendString("status PlayMe mode",szBuffer,sizeof(szBuffer),NULL);
//	if ( !szBuffer[0]=='\0' )
//		mciSendString("close PlayMe",NULL,0,NULL);
// NOTE: The above code was causing hanging in rare cases when SoundPlay wasn't even used
    mciSendString("close all", NULL, 0, NULL);


	// Free memory for hotkeys and unregister hotkeys if required
	for (i=0; i<AUT_MAXHOTKEYS; ++i)
	{
		if (m_HotKeyDetails[i] != NULL)
		{
			UnregisterHotKey(g_hWnd, (int)m_HotKeyDetails[i]->wParam);
			delete m_HotKeyDetails[i];			// Delete memory
		}
	}


	// Close any file handles that script writer has not closed (naughty!)
	for (i=0; i<AUT_MAXOPENFILES; ++i)
	{
		if (m_FileHandleDetails[i] != NULL)
		{
			if (m_FileHandleDetails[i]->nType == AUT_FILEOPEN)
			{
				fclose(m_FileHandleDetails[i]->fptr);	// Close file
			}
			else
			{
				FindClose(m_FileHandleDetails[i]->hFind);
				delete [] m_FileHandleDetails[i]->szFind;
			}

			delete m_FileHandleDetails[i];			// Delete memory
		}
	}

	// Close any dll handles that script writer has not closed (naughty!)
	for (i=0; i<AUT_MAXOPENFILES; ++i)
	{
		if (m_DLLHandleDetails[i] != NULL)
			FreeLibrary(m_DLLHandleDetails[i]);
	}

	// Free up memory used in our function list
	for (i=0; i<m_nFuncListSize; ++i) 
		delete [] m_FuncList[i].szName;				// Free allocated string

	delete [] m_FuncList;							// Delete the entire list

}


///////////////////////////////////////////////////////////////////////////////
// FatalError()
//
// This function will print a message box giving the line and line number
// of any script stopping errors, it will also set the flag that makes the
// script quit.
//
// The line used to print the error is the last one stored in m_nErrorLine,
// if the column number is < zero, this is also used in the error message
///////////////////////////////////////////////////////////////////////////////

void AutoIt_Script::FatalError(int iErr, int nCol)
{
	char		szTitle[AUT_MAX_LINESIZE];
	char		szText[AUT_MAX_LINESIZE];
	char		szOutput[AUT_MAX_LINESIZE*3];
	char		szOutput2[AUT_MAX_LINESIZE*3];

	LoadString(g_hInstance, IDS_AUT_E_TITLE, szTitle, AUT_MAX_LINESIZE);
	LoadString(g_hInstance, iErr, szText, AUT_MAX_LINESIZE);

	// Get the line and include file
	const char *szScriptLine = g_oScriptFile.GetLine(m_nErrorLine);
	int nAutScriptLine = g_oScriptFile.GetAutLineNumber(m_nErrorLine);
	int nIncludeID = g_oScriptFile.GetIncludeID(m_nErrorLine);
	const char *szInclude = g_oScriptFile.GetIncludeName(nIncludeID);

	if (szInclude == NULL)
		sprintf(szOutput, "Line %d:\n\n", nAutScriptLine);
	else
		sprintf(szOutput, "Line %d  (File \"%s\"):\n\n", nAutScriptLine, szInclude);

	strcat(szOutput, szScriptLine);
	strcat(szOutput, "\n");

	if (nCol >= 0)
	{
		strcpy(szOutput2, szScriptLine);
		szOutput2[nCol] = '\0';
		strcat(szOutput2, "^ ERROR");
		
		strcat(szOutput, szOutput2);
		strcat(szOutput, "\n");
	}

	strcat(szOutput, "\nError: ");
	strcat(szOutput, szText);

	if (g_bStdOut)
		printf("%s (%d) : ==> %s: \n%s \n%s\n",szInclude, nAutScriptLine, szText, szScriptLine, szOutput2 );
	else
		MessageBox(g_hWnd, szOutput, szTitle, MB_ICONSTOP | MB_OK | MB_SYSTEMMODAL | MB_SETFOREGROUND);

	// Signal that we want to quit as soon as possible
	m_nCurrentOperation = AUT_QUIT;

} // FatalError()


///////////////////////////////////////////////////////////////////////////////
// FatalError()
//
// This function will print a message box giving the line and line number
// of any script stopping errors, it will also set the flag that makes the
// script quit.
//
// The line used to print the error is the last one stored in m_nErrorLine
//
///////////////////////////////////////////////////////////////////////////////

void AutoIt_Script::FatalError(int iErr, const char *szText2)
{
	char	szTitle[AUT_MAX_LINESIZE];
	char	szText[AUT_MAX_LINESIZE];
	char	szOutput[AUT_MAX_LINESIZE*3];

	LoadString(g_hInstance, IDS_AUT_E_TITLE, szTitle, AUT_MAX_LINESIZE);
	LoadString(g_hInstance, iErr, szText, AUT_MAX_LINESIZE);

	// Get the line
	const char *szScriptLine = g_oScriptFile.GetLine(m_nErrorLine);
	int nAutScriptLine = g_oScriptFile.GetAutLineNumber(m_nErrorLine);
	int nIncludeID = g_oScriptFile.GetIncludeID(m_nErrorLine);
	const char *szInclude = g_oScriptFile.GetIncludeName(nIncludeID);

	if (szInclude == NULL)
		sprintf(szOutput, "Line %d:\n\n", nAutScriptLine);
	else
		sprintf(szOutput, "Line %d  (File \"%s\"):\n\n", nAutScriptLine, szInclude);

	strcat(szOutput, szScriptLine);
	strcat(szOutput, "\n\nError: ");
	strcat(szOutput, szText);
	strcat(szOutput, "\n\n");
	strcat(szOutput, szText2);

	if (g_bStdOut)
		printf("%s (%d) : ==> %s: \n%s \n%s\n",szInclude, nAutScriptLine, szText, szScriptLine, szText2);
	else
		MessageBox(g_hWnd, szOutput, szTitle, MB_ICONSTOP | MB_OK | MB_SYSTEMMODAL | MB_SETFOREGROUND);

	// Signal that we want to quit as soon as possible
	m_nCurrentOperation = AUT_QUIT;

} // FatalError()


///////////////////////////////////////////////////////////////////////////////
// FormatWinError()
//
// This function retrieves the error text for the given windows error.
///////////////////////////////////////////////////////////////////////////////

const char * AutoIt_Script::FormatWinError(DWORD dwCode)
{
	static char szBuffer[AUT_STRBUFFER+1];

	if (dwCode == 0xffffffff)
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0, szBuffer, AUT_STRBUFFER, NULL);
	else
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwCode, 0, szBuffer, AUT_STRBUFFER, NULL);

	return szBuffer;

} // FormatWinError()


///////////////////////////////////////////////////////////////////////////////
// InitScript()
//
// Take the loaded script and then make a note of the location
// and details of user functions (line number, parameters)
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::InitScript(char *szFile)
{
	// Check that the block structures are correct
	if ( AUT_FAILED(Parser_VerifyBlockStructure()) )
		return AUT_ERR;

	// Scan for user functions
	if ( AUT_FAILED(StoreUserFuncs()) )
		return AUT_ERR;

	// Scan for plugin functions and load required DLLs
//	if ( AUT_FAILED(StorePluginFuncs()) )
//		return AUT_ERR;

	// Check user function calls refer to existing functions (or plugin functions)
	if ( AUT_FAILED(VerifyUserFuncCalls()) )
		return AUT_ERR;

	// Make a note of the script filename (for @ScriptDir, etc)
	char	szFileTemp[_MAX_PATH+1];
	char	*szFilePart;

	GetFullPathName(szFile, _MAX_PATH, szFileTemp, &szFilePart);
	m_sScriptFullPath = szFileTemp;
	m_sScriptName = szFilePart;
	szFilePart[-1] = '\0';
	m_sScriptDir = szFileTemp;

	// Initialise the random number routine (must be done after the script has
	// loaded as the compiled script loader also uses random numbers and must
	// be reseeded now to get random numbers again!
	Util_RandInit();

	return AUT_OK;

} // InitScript()


///////////////////////////////////////////////////////////////////////////////
// ProcessMessages()
//
///////////////////////////////////////////////////////////////////////////////

int	AutoIt_Script::ProcessMessages()
{
	MSG		msg;

	// Check if there is a message waiting
	while (PeekMessage( &msg, NULL, 0U, 0U, PM_REMOVE ))
	{
		// Is it a quit message?
		if (msg.message == WM_QUIT)
		{
			m_bWinQuitProcessed = true;
			m_nCurrentOperation = AUT_QUIT;	// Script to end (if not already)
			break;							// Exit this while loop
		}

		// Dispatch the message - check for special dialog/GUI related messages first!
#ifdef AUT_CONFIG_GUI							// Is GUI enabled?
		if (!g_oGUI.IsDialogMessage(&msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
#else
			TranslateMessage(&msg);
			DispatchMessage(&msg);
#endif
	}

	// Check if the user has clicked EXIT in the tray or someone tried to WM_CLOSE the main window - tut
	if (g_bTrayExitClicked == true)
	{
		g_bScriptPaused = false;			// Force unpause (clicking the tray pauses the script)	
		g_nExitMethod = AUT_EXITBY_TRAY;
		g_bTrayExitClicked = false;			// To stop re-triggering which would disrupte any OnExit() func
		m_nCurrentOperation = AUT_QUIT;
	}

	// This return value is not used in the Execute() function - just from functions like
	// DirGetSize() that need to know when to quit/pause
	if (m_nCurrentOperation == AUT_QUIT)
		return AUT_QUIT;
	else if (g_bScriptPaused)
		return AUT_PAUSED;
	else
		return AUT_RUN;
}


///////////////////////////////////////////////////////////////////////////////
// Execute()
//
// This is the main function - it reads in a line of script, and then executes
// it :)  It is also called recursively to process winwait, processwait and
// user function calls.
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::Execute(int nScriptLine)
{
	MSG			msg;
	VectorToken	LineTokens;						// Vector (array) of tokens for a line of script
	const char	*szScriptLine;

	// Increase the recursion level of this function and check that we've not gone too far
	if (m_nExecuteRecursionLevel >= AUT_MAXEXECUTERECURSE)
	{
		FatalError(IDS_AUT_E_MAXRECURSE);
		return AUT_ERR;
	}
	else
		++m_nExecuteRecursionLevel;

	// Is this recursion level 1 (starts as 0 and +1 above)?  If so, we have just started the script so run
	// our OnAutoItStart() function if defined
	if (m_nExecuteRecursionLevel == 1)
	{
		int	nTemp1, nTemp2, nTemp3, nTemp4;

		if (Parser_FindUserFunction("OnAutoItStart", nTemp1, nTemp2, nTemp3, nTemp4) == true)
			SaveExecute(nTemp1+1, true, false);	// Run the user function (line after the Func declaration)
	}


	m_bUserFuncReturned = false;				// When this becomes true, a user function (or winwait
												// operation has requested to return (or EndFunc encountered)

	// Run our Execute() loop
	while(m_bWinQuitProcessed == false && m_bUserFuncReturned == false)
	{
		// Run the windows message loop and handle quit conditions
		ProcessMessages();

		// If script is in a quit state then don't execute any more code
		if (m_nCurrentOperation == AUT_QUIT)
			break;								// Exit while loop

		// If we are waiting for something (winwait, sleep, paused, etc) then loop again
		if (HandleDelayedFunctions() == true)
			continue;

		// Get the next line, or quit if none left
		m_nErrorLine = nScriptLine;				// Keep track for errors
		szScriptLine = g_oScriptFile.GetLine(nScriptLine++);
		if ( szScriptLine == NULL )
		{
			m_nCurrentOperation = AUT_QUIT;
			continue;							// Back to the start of the while loop
		}

		// Do the lexing (convert the line into tokens) - stored in LineTokens
		Lexer(nScriptLine-1, szScriptLine, LineTokens);				// No need to check for errors, already lexed in InitScript()

		// Parse and execute the line
		Parser(LineTokens, nScriptLine);

	} // End While


	// Reset the UserFunction flag before returning (in case Execute() was called recursively)
	m_bUserFuncReturned = false;

	// Is this the final call of Execute() - Level = 1 when finishing completely
	if (m_nExecuteRecursionLevel != 1)
	{
		--m_nExecuteRecursionLevel;
		return AUT_OK;							// Not completely finishing, just return normally
	}

	//
	// If we are here then we are completely finishing
	//

	// Ask worker threads to close
	g_bKillWorkerThreads = true;

	while (m_InetGetDetails.bInProgress)		// Wait for the thread to finish nicely
		Sleep(AUT_IDLE);


	// Destroy GUI (no longer always a child of the main window so we clean up manually...)
#ifdef AUT_CONFIG_GUI							// Is GUI enabled?
	g_oGUI.DeleteAll();
#endif

	// Did we recieve a WM_QUIT message above?  If so the message loop and windows are dead
	// so we might as well quit now (any attempt to call an exit function would fail with no windows...)
	// NOTE: WM_QUIT is only EVER posted by the main window's WM_DESTROY code so it is safe to assume
	// that if WM_QUIT then the main window has already run WM_DESTROY
	if (m_bWinQuitProcessed == true)
		return AUT_OK;


	// Run our OnExit function if it exists
	int	nTemp1, nTemp2, nTemp3, nTemp4;

	if (Parser_FindUserFunction(m_sOnExitFunc.c_str(), nTemp1, nTemp2, nTemp3, nTemp4) == true)
	{
		// Set global vars for @ExitCode and @ExitMethod - Note these names CANNOT usually
		// exist because they start with @ :)
		Variant		vTemp;

		vTemp = g_nExitCode;
		g_oVarTable.Assign("@ExitCode", vTemp, true, VARTABLE_FORCEGLOBAL);
		vTemp = g_nExitMethod;
		g_oVarTable.Assign("@ExitMethod", vTemp, true, VARTABLE_FORCEGLOBAL);

		m_nCurrentOperation = AUT_RUN;		// Get back into a run state for the final push :)
		SaveExecute(nTemp1+1, true, false);	// Run the user function (line after the Func declaration)
		m_nCurrentOperation = AUT_QUIT;
	}


	// Destroy our main window (Calls WM_DESTROY on our and any child windows)
	DestroyWindow(g_hWnd);

	// Get all remaining window messages until the quit WM_QUIT message is received as a result
	// of the DestroyWindow and WM_DESTROY processing
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return AUT_OK;

} // Execute()


///////////////////////////////////////////////////////////////////////////////
// HandleDelayedFunctions()
//
// Handles the processing of delayed commands, Sleep, WinWait, RunWait, etc.
///////////////////////////////////////////////////////////////////////////////

bool AutoIt_Script::HandleDelayedFunctions(void)
{
	// Handle hotkeys first (even if paused - eventually we will and an unpause function to make this useful)
	if (HandleHotKey() == true)
		return true;

	// If the script is paused, sleep then loop again
	if (g_bScriptPaused == true)
	{
		Sleep(AUT_IDLE);					// Quick sleep to reduce CPU load
		return true;
	}

	// If we need to, run an adlib section
	if (HandleAdlib() == true)
		return true;

	// If we need to, run a GUI event function
	if (HandleGuiEvent() == true)
		return true;

	// Check for RunWait commands to finish
	if (m_nCurrentOperation == AUT_RUNWAIT)
	{
		DWORD dwexitcode;
		GetExitCodeProcess(m_piRunProcess, &dwexitcode);

		if (dwexitcode != STILL_ACTIVE)
		{
			CloseHandle(m_piRunProcess);	// Close handle
			m_vUserRetVal = (int)dwexitcode;// Set exit code
			m_bUserFuncReturned = true;		// Request exit from Execute()
			m_nCurrentOperation = AUT_RUN;	// Continue script
		}
		else
            Sleep(AUT_IDLE);				// Quick sleep to reduce CPU load

		return true;						// Next loop
	}

	// If required, process any processwait style commands
	if (HandleProcessWait())
		return true;


	// If required, process any winwait style commands
	return Win_HandleWinWait();

} // HandleDelayedFunctions()


///////////////////////////////////////////////////////////////////////////////
// FunctionExecute()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::FunctionExecute(int nFunction, VectorVariant &vParams, Variant &vResult)
{
	vResult		= 1;							// Default return value is 1
	SetFuncErrorCode(0);						// Default extended error code is zero
	SetFuncExtCode(0);							// Default extended code is zero

	// Lookup the function and execute
	return (this->*m_FuncList[nFunction].lpFunc)(vParams, vResult);	

} // FunctionExecute()


///////////////////////////////////////////////////////////////////////////////
// StoreUserFuncs()
//
// Make a note of the location and details of the user functions
// (name, line number, parameters, end line number)
//
// This function also "sanity checks" the function declaration so that we
// can make assumptions during user function calls
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::StoreUserFuncs(void)
{
	uint			ivPos;						// Position in the vector
	AString			sFuncName;					// Temp function name
	VectorToken		LineTokens;					// Vector (array) of tokens for a line of script
	int				nScriptLine = 1;			// 1 = first line
	const char		*szScriptLine;

	// Check each line of the script for the FUNC keyword
	while ( (szScriptLine = g_oScriptFile.GetLine(nScriptLine++)) != NULL )
	{
		m_nErrorLine = nScriptLine - 1;			// Keep track for errors

		if ( AUT_FAILED( Lexer(nScriptLine-1, szScriptLine, LineTokens) ) )
			return AUT_ERR;						// Bad line

		ivPos = 0;

		if ( !(LineTokens[ivPos].m_nType == TOK_KEYWORD && LineTokens[ivPos].nValue == K_FUNC) )
			continue;							// Next line

		++ivPos;								// Skip "func" keyword

		// Tokens should be: userfunctionname ( $variable , [ByRef] $variable , ... )

		// Get user function name
		if ( LineTokens[ivPos].m_nType != TOK_USERFUNCTION )
		{
			FatalError(IDS_AUT_E_BADFUNCSTATEMENT);
			return AUT_ERR;
		}

		// Check that this function isn't a duplicate
		sFuncName = LineTokens[ivPos].szValue;	// Get function name
		if (m_oUserFuncList.find(sFuncName.c_str()) != NULL)
		{
			FatalError(IDS_AUT_E_DUPLICATEFUNC);
			return AUT_ERR;
		}

		++ivPos;								// Skip function name

		if ( LineTokens[ivPos].m_nType != TOK_LEFTPAREN )
		{
			FatalError(IDS_AUT_E_BADFUNCSTATEMENT);
			return AUT_ERR;
		}

		++ivPos;								// Skip (

		// Parse the parameter declarations
		if ( AUT_FAILED(StoreUserFuncs2(LineTokens, ivPos, sFuncName, nScriptLine)) )
			return AUT_ERR;

	} // End While()

	// Finalize the list of user functions which sorts the list for speed searching. 
	// NO USER MORE FUNCTIONS CAN BE ADDED AFTER THIS
	m_oUserFuncList.createindex();

	return AUT_OK;

} // StoreUserFuncs()


///////////////////////////////////////////////////////////////////////////////
// StoreUserFuncs2()
//
// More StoreUserFuncs - split to aid readabilty
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::StoreUserFuncs2(VectorToken &LineTokens, uint &ivPos, const AString &sFuncName, int &nScriptLine)
{
	UserFuncDetails	tFuncDetails;

	// Tokens should be: [ByRef] $variable , ... [ByRef] $variable , ... )

	int	nNumParamsMin = -1;

	int nNumParams = 0;
	int nFuncStart = nScriptLine - 1;				// Make a note of the Func line

	for (;;)
	{
		// Is it )
		if ( LineTokens[ivPos].m_nType == TOK_RIGHTPAREN )
		{
			// Finishing parsing function, next token must be end
			++ivPos;							// Skip )

			if (LineTokens[ivPos].m_nType != TOK_END)
			{
				FatalError(IDS_AUT_E_EXTRAONLINE);
				return AUT_ERR;
			}
			else
				break;							// Exit while loop
		}

		// Is it the ByRef keyword 
		if ( LineTokens[ivPos].m_nType == TOK_KEYWORD && LineTokens[ivPos].nValue == K_BYREF )
		{
			// We don't need to do anything except skip the ByRef keyword, but check that the next 
			// token is a variable
			++ivPos;							// Skip ByRef keyword

			if ((LineTokens[ivPos].m_nType != TOK_VARIABLE) || (nNumParamsMin != -1) )
			{
				FatalError(IDS_AUT_E_BADFUNCSTATEMENT);
				return AUT_ERR;
			}
		}

		// Is is a variable?
		if ( LineTokens[ivPos].m_nType == TOK_VARIABLE )
		{
			++ivPos;							// Skip variable name

			// Is it the Optional parameter
			if ( LineTokens[ivPos].m_nType == TOK_EQUAL )
			{
				++ivPos;						// Skip TOK_EQUAL

				// May be an optional sign (TOK_PLUS/TOK_MINUS next)
				if (LineTokens[ivPos].m_nType == TOK_PLUS || LineTokens[ivPos].m_nType == TOK_MINUS)
					++ivPos;					// Skip sign

				// Then a literal or macro
				if (LineTokens[ivPos].isliteral())
				{
					++ivPos;					// Skip literal
					if (nNumParamsMin == -1)
						nNumParamsMin = nNumParams;			// Memorize min number of Param
				}
				else
				{
					FatalError(IDS_AUT_E_BADFUNCSTATEMENT);
					return AUT_ERR;
				}
			}
			else
			{
				// Not a default value, so check if any previous were (once a default param
				// is used all following params must also have a default param)
				if (nNumParamsMin != -1) // optional par without default value
				{
					FatalError(IDS_AUT_E_BADFUNCSTATEMENT);
					return AUT_ERR;
				}
			}
		
			++nNumParams;						// Increase the number of parameters

			// Must be a , or ) next
			if ( LineTokens[ivPos].m_nType != TOK_COMMA && LineTokens[ivPos].m_nType != TOK_RIGHTPAREN )
			{
				FatalError(IDS_AUT_E_BADFUNCSTATEMENT);
				return AUT_ERR;
			}
			else
				continue;						// Next loop
		}

		// Is it a , ?
		if ( LineTokens[ivPos].m_nType == TOK_COMMA )
		{
			++ivPos;							// Skip ,

			// Must be a variable or ByRef keyword next
			if ( LineTokens[ivPos].m_nType != TOK_VARIABLE && 
				!(LineTokens[ivPos].m_nType == TOK_KEYWORD && LineTokens[ivPos].nValue == K_BYREF) )
			{
				FatalError(IDS_AUT_E_BADFUNCSTATEMENT);
				return AUT_ERR;
			}
			else
				continue;						// Next loop
		}

		// no match
		FatalError(IDS_AUT_E_BADFUNCSTATEMENT, LineTokens[ivPos].m_nCol);
		return AUT_ERR;


	} // End While (getting parameters)



	// Now we have the funcline,  and number of parameters (the function would have
	// already returned if there were errors

	// Search for EndFunc keyword - fatal error if not found
	if ( AUT_FAILED(StoreUserFuncsFindEnd(nScriptLine)) )
		return AUT_ERR;
	
	// Complete user function found, store
	tFuncDetails.sName = sFuncName;
	tFuncDetails.nFuncLineNum = nFuncStart;
	tFuncDetails.nEndFuncLineNum = nScriptLine - 1;		// Line contained EndFunc
	tFuncDetails.nNumParams = nNumParams;
	if (nNumParamsMin == -1)
		nNumParamsMin = nNumParams;		// no optional parameters
	tFuncDetails.nNumParamsMin = nNumParamsMin;
	m_oUserFuncList.add(tFuncDetails);

	return AUT_OK;

} // StoreUserFuncs2()


///////////////////////////////////////////////////////////////////////////////
// StoreUserFuncsFindEnd()
//
// Looks for a valid matching EndFunc, returns the line number of the endfunc (+1)
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::StoreUserFuncsFindEnd(int &nScriptLine)
{
	VectorToken	LineTokens;						// Vector (array) of tokens for a line of script
	uint		ivPos;
	const char	*szScriptLine;

	bool bEndFuncFound = false;
	while ( bEndFuncFound == false && (szScriptLine = g_oScriptFile.GetLine(nScriptLine)) != NULL )
	{
		++nScriptLine;
		m_nErrorLine = nScriptLine - 1;			// Keep track for errors

		if ( AUT_FAILED( Lexer(nScriptLine-1, szScriptLine, LineTokens) ) )
			return AUT_ERR;						// Bad line

		ivPos = 0;

		if (LineTokens[ivPos].m_nType == TOK_KEYWORD && LineTokens[ivPos].nValue == K_FUNC)
		{
			FatalError(IDS_AUT_E_MISSINGENDFUNC);
			return AUT_ERR;
		}

		if (LineTokens[ivPos].m_nType == TOK_KEYWORD && LineTokens[ivPos].nValue == K_ENDFUNC)
			bEndFuncFound = true;			// Break out of endfunc while loop

	} // End While

	return AUT_OK;

} // StoreUserFuncsFindEnd


///////////////////////////////////////////////////////////////////////////////
// VerifyUserFuncCalls()
//
// Checks that each user function call refers to a defined user function
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::VerifyUserFuncCalls(void)
{
	uint			ivPos;						// Position in the vector
	VectorToken		LineTokens;					// Vector (array) of tokens for a line of script
	int				nScriptLine = 1;			// 1 = first line
	const char		*szScriptLine;
	int				nLineNum, nNumParams, nNumParamsMin, nEndLineNum;

	// Check each line for user function calls
	while ( (szScriptLine = g_oScriptFile.GetLine(nScriptLine++)) != NULL )
	{
		m_nErrorLine = nScriptLine - 1;			// Keep track for errors
		
		Lexer(nScriptLine-1, szScriptLine, LineTokens);		// No need to test for errors, already done in StoreUserFuncs()

		ivPos = 0;

		while (LineTokens[ivPos].m_nType != TOK_END)
		{
			if (LineTokens[ivPos].m_nType == TOK_USERFUNCTION)
			{
				if (Parser_FindUserFunction(LineTokens[ivPos].szValue, nLineNum, nNumParams, nNumParamsMin, nEndLineNum) == false)
				{
					FatalError(IDS_AUT_E_UNKNOWNUSERFUNC, LineTokens[ivPos].m_nCol);
					return AUT_ERR;
				}
			}

			++ivPos;
		}

	} // End While()

	return AUT_OK;

} // VerifyUserFuncCalls()


///////////////////////////////////////////////////////////////////////////////
// StorePluginFuncs()
//
// Query the scriptfile object for all the plugin dlls specified, load each one
// and make a note of all the function names it supplies.
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::StorePluginFuncs(void)
{
/*	HINSTANCE hDll = LoadLibrary("example.dll");
	
	if (hDll == NULL)
		return AUT_ERR;

	PluginFuncs *lpEntry = new PluginFuncs;
	
	lpEntry->hDll = hDll;
	lpEntry->sFuncName = "PluginTest";
	lpEntry->lpNext = NULL;

	// Add to the list

*/
	return AUT_OK;

} // StorePluginFuncs()


///////////////////////////////////////////////////////////////////////////////
// HandleAdlib()
//
// Handles the processing of Adlid
///////////////////////////////////////////////////////////////////////////////

bool AutoIt_Script::HandleAdlib(void)
{
	// Only continue if adlibbing is enabled AND we are not currently adlibing already!
	if (m_bAdlibEnabled == false || m_bAdlibInProgress == true)
		return false;

	// Check the timer - we only run the adlib every so often as it is a strain on the
	// CPU otherwise
	DWORD	dwDiff;
	DWORD	dwCur = timeGetTime();

	if (dwCur < m_tAdlibTimerStarted)
		dwDiff = (UINT_MAX - m_tAdlibTimerStarted) + dwCur; // timer wraps at 2^32
	else
		dwDiff = dwCur - m_tAdlibTimerStarted;

	// Timer elapsed?
	if (dwDiff < m_nAdlibTimeout)
		return false;							// Not time yet

	// Reset the timer
	m_tAdlibTimerStarted = dwCur;


	// Get the details of the function (we should have previously checked that it
	// exists)
	int		nLineNum, nNumParams, nNumParamsMin, nEndLineNum;

	Parser_FindUserFunction(m_sAdlibFuncName.c_str(), nLineNum, nNumParams, nNumParamsMin, nEndLineNum);
	++nLineNum;									// Skip function declaration

	// Save current operation (may be waiting)
	int nCurrentOperation	= m_nCurrentOperation;

	// Continue execution with the user function (using recursive call of Execute() )
	m_nCurrentOperation = AUT_RUN;				// Change mode to run script (may have been waiting)
	m_bAdlibInProgress	= true;					// True if we are currently running adlib function

	SaveExecute(nLineNum, true, true);			// Save state and recursively call Execute()

	m_bAdlibInProgress	= false;

	// If the function caused the script to end, we must honour the request rather than restoring
	if (m_nCurrentOperation != AUT_QUIT)
	{
		m_nCurrentOperation	= nCurrentOperation;
		return false;							// Continue Execute() (to give script a chance to run!)
	}

	return true;								// Returning true will stop further
												// progress in Execute() and restart the loop
} // HandleAdlib()


///////////////////////////////////////////////////////////////////////////////
// HandleHotKey()
//
// Handles the processing of a hotkey and calls a function
///////////////////////////////////////////////////////////////////////////////

bool AutoIt_Script::HandleHotKey(void)
{
	// See if there is a hotkey request in the queue - only process one per loop!
	if (m_nHotKeyQueuePos == g_HotKeyNext)
		return false;

	// What ID is the hotkey pressed?  Also increment the position and reset if needed
	WPARAM wParam = g_HotKeyQueue[m_nHotKeyQueuePos++];
	if (m_nHotKeyQueuePos >= AUT_HOTKEYQUEUESIZE)
		m_nHotKeyQueuePos = 0;					// reset

	// Find the corresponding hotkey definition and function
	int		n;

	for (n = 0; n < AUT_MAXHOTKEYS; ++n)
	{
		if (m_HotKeyDetails[n] != NULL && m_HotKeyDetails[n]->wParam == wParam)
			break;
	}
	if (n == AUT_MAXHOTKEYS)
		return false;							// Not found the ID! Ignore


	// Get the details of the function (we should have previously checked that it
	// exists!)
	int		nLineNum, nNumParams, nNumParamsMin, nEndLineNum;

	Parser_FindUserFunction(m_HotKeyDetails[n]->sFunction.c_str(), nLineNum, nNumParams, nNumParamsMin, nEndLineNum);
	++nLineNum;									// Skip function declaration

	// Save current operation (may be waiting)
	int nCurrentOperation	= m_nCurrentOperation;

	// Continue execution with the user function (using recursive call of Execute() )
	m_nCurrentOperation = AUT_RUN;				// Change mode to run script (may have been waiting)

	SaveExecute(nLineNum, true, true);			// Save state and recursively call Execute()

	// If the function caused the script to end, we must honour the request rather than restoring
	if (m_nCurrentOperation != AUT_QUIT)
		m_nCurrentOperation	= nCurrentOperation;

	return true;								// Returning true will stop further
												// progress in Execute() and start the loop again
} // HandleHotKey()


///////////////////////////////////////////////////////////////////////////////
// HandleGuiEvent()
//
// Handles the processing of GUI events
///////////////////////////////////////////////////////////////////////////////

bool AutoIt_Script::HandleGuiEvent(void)
{
#ifdef AUT_CONFIG_GUI

	// Only continue if GUI event is enabled AND we are not currently running a GUI function
	if (g_oGUI.m_bGuiEventEnabled == false || m_bGuiEventInProgress == true)
		return false;

	GUIEVENT	Event;
	int			nLineNum, nNumParams, nNumParamsMin, nEndLineNum;

	// Get the messages until there are no more
	while (g_oGUI.GetMsg(Event))
	{
		// Is the event a callback?
		if (Event.sCallback.empty())
			continue;

		// Callback

		// Check that this user function exists
		if (Parser_FindUserFunction(Event.sCallback.c_str(), nLineNum, nNumParams, nNumParamsMin, nEndLineNum) == false)
			continue;
		else
		{
			Variant		vTemp;

			// Set global vars for - Note these names CANNOT usually
			// exist because they start with @ :)
			vTemp = Event.nGlobalID;
			g_oVarTable.Assign("@GUI_CTRLID", vTemp, true, VARTABLE_FORCEGLOBAL);

			vTemp = Event.hWnd;
			g_oVarTable.Assign("@GUI_WINHANDLE", vTemp, true, VARTABLE_FORCEGLOBAL);

			vTemp = Event.hCtrl;
			g_oVarTable.Assign("@GUI_CTRLHANDLE", vTemp, true, VARTABLE_FORCEGLOBAL);

			// Save current operation (may be waiting)
			int nCurrentOperation	= m_nCurrentOperation;

			// Continue execution with the user function (using recursive call of Execute() )
			m_nCurrentOperation = AUT_RUN;				// Change mode to run script (may have been waiting)
			m_bGuiEventInProgress	= true;				// True if we are currently running adlib function

			SaveExecute(nLineNum+1, true, true);		// Save state and recursively call Execute()

			m_bGuiEventInProgress	= false;

			// If the function caused the script to end, we must honour the request rather than restoring
			if (m_nCurrentOperation != AUT_QUIT)
				m_nCurrentOperation	= nCurrentOperation;
			else
				return true;							// Restart the messageloop in order to check the quit condition
		}
	}

#endif
	
	return false;

} // HandleGuiEvent()


///////////////////////////////////////////////////////////////////////////////
// SaveExecute()
//
// Saves the current state of the script and then performs a recursive
// call to Execute() and then restores state. Not all the items saved in state
// are required for every usage but the function is used from many parts of the
// code and needs to be safe for all of them.
// The current scope can optionally be raised/lowered too.
// This function is used by UserFunction calling, Adlib, Hotkey, Call
///////////////////////////////////////////////////////////////////////////////

void AutoIt_Script::SaveExecute(int nScriptLine, bool bRaiseScope, bool bRestoreErrorCode)
{
	// Saved state vars - Win related
	Variant			vWindowSearchTitle;			// Title/text used for winwait commands
	vWindowSearchTitle		= m_vWindowSearchTitle;

	Variant			vWindowSearchText;			// Title/text used for winwait commands
	vWindowSearchText		= m_vWindowSearchText;

	DWORD			nWinWaitTimeout		= m_nWinWaitTimeout;	// Time (ms) left before timeout (0=no timeout)
	HWND			WindowSearchHWND	= m_WindowSearchHWND;	// Last window searched for
	DWORD			tWinTimerStarted	= m_tWinTimerStarted;	// Time in millis that timer was started

	// Saved state vars - Control related
	Variant			vControlSearchValue;						// The ID, classname or text to search for 
	vControlSearchValue		= m_vControlSearchValue;
	HWND			ControlSearchHWND = m_ControlSearchHWND;	// Contains HWND of a successful control search

	// Process related vars
	AString			sProcessSearchTitle		= m_sProcessSearchTitle;	// Name of process to wait for
	DWORD			nProcessWaitTimeout		= m_nProcessWaitTimeout;	// Time (ms) left before timeout (0=no timeout)
	DWORD			tProcessTimerStarted	= m_tProcessTimerStarted;	// Time in millis that timer was started

	// Function call related
	int				nNumParams = m_nNumParams;	// Number of parameters passed to a UDF

	// Main vars
	int				nErrorLine		= m_nErrorLine;			// Line number used to generate error messages
	int				nFuncErrorCode	= m_nFuncErrorCode;		// Extended error code
	int				nFuncExtCode	= m_nFuncExtCode;		// Extended code

	// We also need to keep track of how many statements (if, while, etc)
	// we have on the go just in case the user function is naughty (return function in the
	// middle of a loop) and leaves the statement stacks in an incorrect state - we will
	// have to correct this after Execute() returns
	uint	nStackSize		= m_StatementStack.size();


	// Continue execution with the user function (using recursive call of Execute() )

	if (bRaiseScope)
		g_oVarTable.ScopeIncrease();				// Increase scope

	Execute(nScriptLine);

	// Descrease the variable scope - deletes function local variables
	if (bRaiseScope)
		g_oVarTable.ScopeDecrease();

	// Make sure the statement stacks are the same as before the user function was called, 
	// if not we fix them
	while (nStackSize < m_StatementStack.size())
		m_StatementStack.pop();


	// Restore "state"
	m_vWindowSearchTitle	= vWindowSearchTitle;
	m_vWindowSearchText		= vWindowSearchText;
	m_nWinWaitTimeout		= nWinWaitTimeout;
	m_WindowSearchHWND		= WindowSearchHWND;
	m_tWinTimerStarted		= tWinTimerStarted;

	m_vControlSearchValue	= vControlSearchValue;
	m_ControlSearchHWND		= ControlSearchHWND;

	m_sProcessSearchTitle	= sProcessSearchTitle;
	m_nProcessWaitTimeout	= nProcessWaitTimeout;
	m_tProcessTimerStarted	= tProcessTimerStarted;
	m_nNumParams			= nNumParams;
	m_nErrorLine			= nErrorLine;

	// Only restore the @error code in certain situations (adlib/hotkey)
	if (bRestoreErrorCode)
	{
		m_nFuncErrorCode	= nFuncErrorCode;
		m_nFuncExtCode		= nFuncExtCode;
	}

} // SaveExecute()

