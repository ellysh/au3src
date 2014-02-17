#ifndef __SCRIPT_H
#define __SCRIPT_H

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
// script.h
//
// The main script object.
//
///////////////////////////////////////////////////////////////////////////////


// Includes
#ifndef _MSC_VER								// Includes for non-MS compilers
	#include <stdio.h>
	#include <windows.h>
	#include <wininet.h>
#endif

#include "AutoIt.h"
#include "variant_datatype.h"
#include "token_datatype.h"
#include "vector_variant_datatype.h"
#include "vector_token_datatype.h"
#include "stack_statement_datatype.h"
#include "stack_int_datatype.h"
#include "stack_variant_datatype.h"
#include "variabletable.h"
#include "os_version.h"
#include "sendkeys.h"
#include "userfunction_list.h"
#include "regexp.h"


// Possible states of the script
enum
{
	AUT_RUN, AUT_QUIT,
	AUT_SLEEP,
	AUT_WINWAIT, AUT_WINWAITCLOSE,
	AUT_WINWAITACTIVE, AUT_WINWAITNOTACTIVE,
	AUT_RUNWAIT,
	AUT_PROCESSWAIT, AUT_PROCESSWAITCLOSE,
	AUT_INETGET,
	AUT_PAUSED									// Not actually used except for a ProcessMessages() return value
};


// Possible exit methods
enum { AUT_EXITBY_NATURAL, AUT_EXITBY_EXITKEYWORD, AUT_EXITBY_TRAY };

// ControlSearch methods
enum { AUT_CONTROLSEARCH_CLASS, AUT_CONTROLSEARCH_TEXT, AUT_CONTROLSEARCH_ID };


#ifdef AUT_CONFIG_GUI							// Is GUI enabled?
// Possible Gui controls
enum
{
	AUT_GUI_COMBO=0,
	AUT_GUI_LIST=1,

	AUT_GUI_EDIT=2,
	AUT_GUI_INPUT=3,
	AUT_GUI_GROUP=4,
	AUT_GUI_DATE=5,
	AUT_GUI_PIC=6,
	AUT_GUI_ICON=7,
	AUT_GUI_PROGRESS=8,
	AUT_GUI_AVI=9,
	AUT_GUI_TAB=10,
	AUT_GUI_TABITEM=11,
	AUT_GUI_CONTEXTMENU=12,
	AUT_GUI_TRAYMENU=13,
	AUT_GUI_MENU=14,
	AUT_GUI_MENUITEM=15,
	AUT_GUI_TREEVIEW=16,
	AUT_GUI_TREEVIEWITEM=17,
	AUT_GUI_SLIDER=18,
	AUT_GUI_LISTVIEW=19,
	AUT_GUI_LISTVIEWITEM=20,
	AUT_GUI_DUMMY=21,
	AUT_GUI_UPDOWN=22,
	AUT_GUI_LABEL=23,
	AUT_GUI_BUTTON=24,
	AUT_GUI_CHECKBOX=25,
	AUT_GUI_RADIO=26,

	// subtype to suppress action during the creation by CtrlCreate
	AUT_GUI_NOFONT = 1,
	AUT_GUI_NORESIZE = 2,
	AUT_GUI_NOTEXTSIZE = 4
};
#endif


// Keyword values (must match the order as in script.cpp)
enum
{
	K_AND = 0, K_OR, K_NOT,
	K_IF, K_THEN, K_ELSE, K_ELSEIF, K_ENDIF,
	K_WHILE, K_WEND,
	K_DO, K_UNTIL,
	K_FOR, K_NEXT, K_TO, K_STEP,
	K_EXITLOOP, K_CONTINUELOOP,
	K_SELECT, K_CASE, K_ENDSELECT,
	K_DIM, K_REDIM, K_LOCAL, K_GLOBAL, K_CONST,
	K_FUNC, K_ENDFUNC, K_RETURN,
	K_EXIT,
	K_BYREF,
	K_MAX
};


enum
{
	OPR_LESS,									// <
	OPR_GTR,									// >
	OPR_LESSEQUAL,								// <=
	OPR_GTREQUAL,								// >=
	OPR_NOTEQUAL,								// NotEqual comparision
	OPR_EQUAL,									// = Equals comparison (case insensitive with strings)
	OPR_STRINGEQUAL,							// == Equals comparison (case sensitive with strings)
	OPR_LOGAND,									// Logcial AND (keyword for conditions)
	OPR_LOGOR,									// Logical OR (keyword for conditions)
	OPR_CONCAT,									// &
	OPR_NOT,									// Unary NOT
	OPR_ADD,									// +
	OPR_SUB,									// -
	OPR_MUL,									// *
	OPR_DIV,									// /
	OPR_POW,									// ^
	OPR_UMI,									// Unary minus ( 4--3, etc)
	OPR_UPL,									// Unary plus
	OPR_LPR,									// (
	OPR_RPR,									// )
	OPR_END,									// $ (End)
	OPR_MAXOPR,									// Number of operators (used for array sizing)
	OPR_VAL,									// Value
	OPR_NULL									// Dummy value, used in parsing expression routine
};


// Coordinate modes (pixel, caret, mouse functions)
#define AUT_COORDMODE_WINDOW	0				// Relative to active window
#define AUT_COORDMODE_SCREEN	1				// Relative to the screen
#define AUT_COORDMODE_CLIENT	2				// Relative to active window's CLIENT area


// Structure for storing file handles for the File functions
#define AUT_MAXOPENFILES	64

// FileOpen/FileFind handles
enum { AUT_FILEOPEN, AUT_FILEFIND};
typedef struct
{
	int			nType;							// Type of entry stored (fileopen, or filefind)

	// File Open
	FILE		*fptr;							// File handle
	int			nMode;							// Mode the file was opened in (0=read, 1=write)

	// File Find
	HANDLE		hFind;							// Find handle
	char		*szFind;						// First search result (or NULL on 2nd, 3rd, etc)

} FileHandleDetails;

// Structure for storing hotkeys
#define AUT_MAXHOTKEYS		64					// Maximum number of hotkeys
typedef struct
{
	WPARAM	wParam;								// Hotkey ID
	LPARAM	lParam;								// Key and modifiers (control, alt, etc)
	AString	sFunction;							// Function to call

} HotKeyDetails;


// Regular Expressions info
#define AUT_MAXREGEXPS		8					// Size of regular expression cache


// Function lookup structures
class AutoIt_Script;							// Forward declaration of AutoIt_Script

typedef AUT_RESULT (AutoIt_Script::*AU3_FUNCTION)(VectorVariant &vParams, Variant &vResult);

typedef struct
{
	char			*szName;					// Function name
	AU3_FUNCTION	lpFunc;						// Pointer to function
	int				nMin;						// Min params
	int				nMax;						// Max params
} AU3_FuncInfo;


/*
// Plugin lookup structures
typedef struct _PluginFuncs
{
	HINSTANCE			hDll;					// Handle of the loaded DLL
	char				*szFuncName;			// Name of the function
	struct _PluginFuncs	*lpNext;				// Next entry, or NULL for last entry
} PluginFuncs;
*/


// Proxy stuff
#define AUT_PROXY_REGISTRY	0
#define AUT_PROXY_DIRECT	1
#define AUT_PROXY_PROXY		2


// Lexer caching
// Assuming an average of 7 tokens per line at 256 bytes per line
// 256 lines of buffer = 64KB.
#define AUT_LEXER_CACHESIZE	256					// Must be power of 2
#define AUT_LEXER_CACHEMASK	255					// size - 1
typedef struct
{
	int			nLineNum;						// Line cached here (or -1)
	VectorToken	vLine;							// Cached line of tokens
} LexerCache;


// InetGet handles
typedef struct
{
//	HANDLE		hThread;						// Handle to the internet worker thread
	bool		bInProgress;
	int			nBytesRead;
	HINTERNET	hInet, hConnect, hFile;
	FILE		*fptr;
	DWORD		dwService;
	bool		bRequestAbort;
} InetGetDetails;


// List structure for the WinGetList function
typedef struct _WinListNode
{
	HWND	hWnd;
	struct _WinListNode	*lpNext;				// next node (or NULL)
} WinListNode;


///////////////////////////////////////////////////////////////////////////////


// The AutoIt Script object
class AutoIt_Script
{
public:
	// Variables

	// Functions
	AutoIt_Script();							// Constructor
	~AutoIt_Script();							// Destrucutor

	AUT_RESULT		InitScript(char *szFile);	// Perform setup of a loaded script
	int				ProcessMessages();
	AUT_RESULT		Execute(int nScriptLine=0);	// Run script at this line number
	int             GetCurLineNumber (void) const { return m_nErrorLine; }  // Return current line number for TrayTip debugging

private:

	// Variables
	HS_SendKeys		m_oSendKeys;				// SendKeys object
	AString			m_sScriptName;				// Filename of script
	AString			m_sScriptFullPath;			// Full pathname of script
	AString			m_sScriptDir;				// Directory the script is in

	HWND			m_hWndTip;					// ToolTip window

	int				m_nExecuteRecursionLevel;	// Keeps track of the recursive calls of Execute()
	int				m_nErrorLine;				// Line number used to generate error messages
	int				m_nCurrentOperation;		// The current state of the script (RUN, WAIT, SLEEP, etc)
	bool			m_bWinQuitProcessed;		// True when windows WM_QUIT message has been processed

	// Options (AutoItSetOption)
	int				m_nCoordMouseMode;			// Mouse position mode (screen or relative to active window)
	int				m_nCoordPixelMode;			// Pixel position mode (screen or relative to active window)
	int				m_nCoordCaretMode;
	bool			m_bRunErrorsFatal;			// Determines if "Run" function errors are fatal
	bool			m_bExpandEnvStrings;		// Determines if %Env% are expanded in strings
	bool			m_bExpandVarStrings;		// Determines if $Env$ @Env@ are expanded in strings
	bool			m_bMustDeclareVars;			// Must declare variables efore use
	int				m_nMouseClickDelay;			// Time between mouse clicks
	int				m_nMouseClickDownDelay;		// Time the click is held down
	int				m_nMouseClickDragDelay;		// The delay at the start and end of a drag operation
	bool			m_bColorModeBGR;			// True if using the old BGR (rather than RGB) colour mode
	AString			m_sOnExitFunc;				// Name of our OnExit function
	bool			m_bFtpBinaryMode;			// True if binary mode ftp is required

	// Proxy Settings
	int				m_nHttpProxyMode;
	AString			m_sHttpProxy;
	AString			m_sHttpProxyUser;
	AString			m_sHttpProxyPwd;
	int				m_nFtpProxyMode;
	AString			m_sFtpProxy;
	AString			m_sFtpProxyUser;
	AString			m_sFtpProxyPwd;

	// Net download details
	InetGetDetails	m_InetGetDetails;


	// User functions variables
	UserFuncList	m_oUserFuncList;			// Details (line numbers, num params) for user defined functions
	Variant			m_vUserRetVal;				// Temp storage for return value of a user function (or winwait result)
	bool			m_bUserFuncReturned;		// Becomes true when userfunctions end (return or endfunc)
	int				m_nFuncErrorCode;			// Extended error code
	int				m_nFuncExtCode;				// Extended code
	int				m_nNumParams;				// Number of parameters when calling a user function


	// Plugin variables
	//PluginFuncs		*m_PluginFuncs;				// Linked list of plugin functions

	// Statement stacks
	StackStatement	m_StatementStack;			// Stack for tracking If/Func/Select/Loop statements


	// File variables
	int					m_nNumFileHandles;						// Number of file handles in use
	FileHandleDetails	*m_FileHandleDetails[AUT_MAXOPENFILES];	// Array contains file handles for File functions

	// DLL variables
	HINSTANCE			m_DLLHandleDetails[AUT_MAXOPENFILES];

	// HotKey stuff
	HotKeyDetails	*m_HotKeyDetails[AUT_MAXHOTKEYS];	// Array for tracking hotkey details
	int				m_nHotKeyQueuePos;					// Position in the global hotkey queue


	// Lexing and parsing vars
#ifdef AUT_CONFIG_LEXERCACHE
	LexerCache		m_LexerCache[AUT_LEXER_CACHESIZE];
#endif
	static char		m_PrecOpRules[OPR_MAXOPR][OPR_MAXOPR];	// Table for precedence rules
	static char		*m_szKeywords[];			// Valid keywords
	static char		*m_szMacros[];				// Valid functions
	AU3_FuncInfo	*m_FuncList;				// List of functions and details for each
	int				m_nFuncListSize;			// Number of functions

	// Window related vars
	Variant			m_vWindowSearchTitle;		// Title/text used for win searches
	Variant			m_vWindowSearchText;		// Title/text used for win searches
//	bool			m_bWinSearchFoundFlag;		// Temp var used in Window searches
	HWND			m_WindowSearchHWND;			// Temp var used in Window searches
	int				m_nWindowSearchMatchMode;	// Window title substring match mode
	int				m_nWindowSearchTextMode;	// Window title substring match mode
	WinListNode		*m_lpWinListFirst;			// First entry in window list
	WinListNode		*m_lpWinListLast;			// First entry in window list
	int				m_nWinListCount;			// Number of entries
	bool			m_bDetectHiddenText;		// Detect hidden window text in window searches?
	bool			m_bWinSearchChildren;		// Search just top level windows or children too?
	bool			m_bWindowSearchFirstOnly;	// When true will only find the first matching window (otherwise all)

	DWORD			m_nWinWaitTimeout;			// Time (ms) left before timeout (0=no timeout)
	int				m_nWinWaitDelay;			// 500 = default (wait this long after a window is matched)
	DWORD			m_tWinTimerStarted;			// Time in millis that timer was started

	int				m_nControlSearchMethod;		// Temp variable used in the search functions
	Variant			m_vControlSearchValue;		// The ID, classname or text to search for
	uint			m_iControlSearchInstance;	// Variable to keep track of class instance during search
	bool			m_bControlSearchFoundFlag;	// Temp variable used in the search functions
	HWND			m_ControlSearchHWND;		// Contains HWND of a successful control search


	// Process related vars
	AString			m_sProcessSearchTitle;		// Name of process to wait for
	DWORD			m_nProcessWaitTimeout;		// Time (ms) left before timeout (0=no timeout)
	DWORD			m_tProcessTimerStarted;		// Time in millis that timer was started
	HANDLE			m_piRunProcess;				// Used in RunWait command

	bool			m_bRunAsSet;				// Flag if we want to use RunAs user/password in the Run function
	DWORD			m_dwRunAsLogonFlags;		// RunAs logon flags
	wchar_t			*m_wszRunUser;				// User name for RunAs (wide char)
	wchar_t			*m_wszRunDom;				// Domain name for RunAs (wide char)
	wchar_t			*m_wszRunPwd;				// Password for RunAs (wide char)


	// ADLIB related vars
	AString			m_sAdlibFuncName;			// The name of the current adlib function
	bool			m_bAdlibEnabled;			// True if an adlib function is specified
	bool			m_bAdlibInProgress;			// True if we are currently running adlib function
	DWORD			m_tAdlibTimerStarted;		// Time in millis that timer was started
	DWORD			m_nAdlibTimeout;			// Time (ms) left before timeout (0=no timeout)


	// GUI related vars
	bool			m_bGuiEventInProgress;		// True if we are currently GUI event function


	// WinList() structs and vars


	// Functions
	void		SaveExecute(int nScriptLine, bool bRaiseScope, bool bRestoreErrorCode);		// Save state and then Execute()
	void		FatalError(int iErr, int nCol = -1);				// Output an error and signal quit (String resource errors)
	void		FatalError(int iErr, const char *szText2);			// Output an error and signal quit (passed text errors)
	const char * FormatWinError(DWORD dwCode = 0xffffffff);			// Gets the string output for a Windows error code

	void		SetFuncErrorCode(int nCode)
					{m_nFuncErrorCode = nCode;};					// Set script error info (@error code)
	void		SetFuncExtCode(int nCode)
					{m_nFuncExtCode = nCode;};						// Set script extended info (@extended code)

	AUT_RESULT	StoreUserFuncs(void);								// Get all user function details
	AUT_RESULT	StoreUserFuncs2(VectorToken &LineTokens, uint &ivPos, const AString &sFuncName, int &nScriptLine);
	AUT_RESULT	StoreUserFuncsFindEnd(int &nScriptLine);			// Finds a matching endfunc during the StoreUserFuncs functions
	AUT_RESULT	VerifyUserFuncCalls(void);							// Ensures user function calls are defined

	AUT_RESULT	StorePluginFuncs(void);								// Get all plugin function details

	bool		HandleDelayedFunctions(void);						// Handle delayed commands
	bool		HandleAdlib(void);
	bool		HandleHotKey(void);
	bool		HandleGuiEvent(void);

	// Parser functions (script_parser.cpp)
	AUT_RESULT	Parser_VerifyBlockStructure(void);
	AUT_RESULT	Parser_VerifyBlockStructure2(int nDo, int nWhile, int nFor, int nSelect, int nIf);
	void		Parser(VectorToken &vLineToks, int &nScriptLine);
	void		Parser_StartWithVariable(VectorToken &vLineToks, uint &ivPos);
	AUT_RESULT	Parser_GetArrayElement(VectorToken &vLineToks, uint &ivPos, Variant **ppvTemp);
	void		Parser_StartWithKeyword(VectorToken &vLineToks, uint &ivPos, int &nScriptLine);
	AUT_RESULT	Parser_FunctionCall(VectorToken &vLineToks, uint &ivPos, Variant &vResult);
	AUT_RESULT	Parser_GetFunctionCallParams(VectorVariant &vParams, VectorToken &vLineToks, uint &ivPos, int &nNumParams);
	AUT_RESULT	FunctionExecute(int nFunction, VectorVariant &vParams, Variant &vResult);
	bool		Parser_FindUserFunction(const char *szName, int &nLineNum, int &nNumParams,int &nNumParamsMin, int &nEndLineNum);
	AUT_RESULT	Parser_UserFunctionCall(VectorToken &vLineToks, uint &ivPos, Variant &vResult);
	bool		Parser_PluginFunctionCall(VectorToken &vLineToks, uint &ivPos, Variant &vResult);
	void		Parser_Keyword_IF(VectorToken &vLineToks, uint &ivPos, int &nScriptLine);
	void		Parser_Keyword_ELSE(int &nScriptLine);
	void		Parser_Keyword_ENDIF(VectorToken &vLineToks, uint &ivPos);
	void		Parser_Keyword_WHILE(VectorToken &vLineToks,uint &ivPos, int &nScriptLine);
	void		Parser_Keyword_WEND(VectorToken &vLineToks, uint &ivPos, int &nScriptLine);
	void		Parser_Keyword_EXITLOOP(VectorToken &vLineToks, uint &ivPos, int &nScriptLine);
	void		Parser_Keyword_CONTINUELOOP(VectorToken &vLineToks, uint &ivPos, int &nScriptLine);
	void		Parser_Keyword_DO(VectorToken &vLineToks,uint &ivPos, int &nScriptLine);
	void		Parser_Keyword_UNTIL(VectorToken &vLineToks,uint &ivPos, int &nScriptLine);
	void		Parser_Keyword_FOR(VectorToken &vLineToks,uint &ivPos, int &nScriptLine);
	void		Parser_Keyword_NEXT(VectorToken &vLineToks, uint &ivPos, int &nScriptLine);
	void		Parser_Keyword_SELECT(VectorToken &vLineToks, uint &ivPos, int &nScriptLine);
	void		Parser_Keyword_CASE(int &nScriptLine);
	void		Parser_Keyword_ENDSELECT(VectorToken &vLineToks, uint &ivPos);
	void		Parser_Keyword_DIM(VectorToken &vLineToks, uint &ivPos, int nReqScope);
	void		Parser_Keyword_CONST(VectorToken &vLineToks, uint &ivPos, int nReqScope);
	void		Parser_Keyword_RETURN(VectorToken &vLineToks, uint &ivPos);
	void		Parser_Keyword_EXIT(VectorToken &vLineToks, uint &ivPos);


	// Lexer functions (script_lexer.cpp)
	AUT_RESULT	Lexer(int nLineNum, const char *szLine, VectorToken &vLineToks);	// Convert a string into tokens
	AUT_RESULT	Lexer_String(const char *szLine, uint &iPos, char *szTemp);
	bool		Lexer_Number(const char *szLine, uint &iPos, Token &rtok, char *szTemp);
	void		Lexer_KeywordOrFunc(const char *szLine, uint &iPos, Token &rtok, char *szTemp);


	// Parsing expression/conditions (script_parse_exp.cpp)
	void		Parser_ExpandEnvString(Variant &vString);
	void		Parser_ExpandVarString(Variant &vString);
	AUT_RESULT	Parser_EvaluateVariable(VectorToken &vLineToks, uint &ivPos, Variant &vResult);
	AUT_RESULT	Parser_EvaluateMacro(const char *szName, Variant &vResult);
	AUT_RESULT	Parser_EvaluateCondition(VectorToken &vLineToks, uint &ivPos, bool &bResult);
	AUT_RESULT	Parser_EvaluateExpression(VectorToken &vLineToks, uint &ivPos, Variant &vResult);
	AUT_RESULT	Parser_OprReduce(StackInt &opStack, StackVariant &valStack);
	AUT_RESULT	Parser_SkipBoolean(VectorToken &vLineToks, unsigned int &ivPos);


	// Window-related functions (script_win.cpp)
	AUT_RESULT	F_WinExists(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_WinActive(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_WinWait(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_WinWaitActive(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_WinWaitNotActive(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_WinWaitClose(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_WinActivate(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_WinShow(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_WinClose(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_WinKill(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_WinMinimizeAll(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_WinMinimizeAllUndo(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_WinSetTitle(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_WinMove(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_WinGetTitle(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_WinGetText(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_ControlClick(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_ControlFocus(VectorVariant &vParams, Variant &vResult);

	AUT_RESULT	F_ControlGetFocus(VectorVariant &vParams, Variant &vResult);
	void		ControlWithFocus(HWND hWnd, Variant &vResult);
	static BOOL CALLBACK ControlWithFocusProc(HWND hWnd, LPARAM lParam);
	BOOL		ControlWithFocusProcHandler(HWND hWnd, LPARAM lParam);

	AUT_RESULT	F_WinSetOnTop(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_WinGetPos(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_ControlGetText(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_ControlSetText(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_ControlGetPos(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_ControlCommand(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_ControlListView(VectorVariant &vParams,  Variant &vResult);
	void		ControlLVSelect(bool bSelect, int nFromIndex, int nToIndex);
	AUT_RESULT	F_ControlTreeView(VectorVariant &vParams,  Variant &vResult);
	AUT_RESULT	F_ControlEnable(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_ControlDisable(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_ControlHide(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_ControlMove(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_ControlShow(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_WinGetClassList(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_StatusbarGetText(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_ControlSend(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_WinMenuSelectItem(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_WinGetClientSize(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_WinGetHandle(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_ControlGetHandle(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_WinGetCaretPos(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_WinGetState(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_WinSetTrans(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_ToolTip(VectorVariant &vParams, Variant &vResult);

	bool		Win_HandleWinWait(void);
	void		Win_WindowSearchInit(VectorVariant &vParams);
	void		Win_WindowWaitInit(VectorVariant &vParams);

	void		Win_WindowSearchDeleteList(void);
	void		Win_WindowSearchAddToList(HWND hWnd);
	bool		Win_WindowSearch(bool bFirstOnly = true);
	static BOOL	CALLBACK Win_WindowSearchProc(HWND hWnd, LPARAM lParam);
	BOOL		Win_WindowSearchProcHandler(HWND hWnd, LPARAM lParam);
	bool		Win_WindowSearchText(void);
	static BOOL	CALLBACK Win_WindowSearchTextProc(HWND hWnd, LPARAM lParam);
	BOOL		Win_WindowSearchTextProcHandler(HWND hWnd, LPARAM lParam);

	AUT_RESULT	F_WinList(VectorVariant &vParams, Variant &vResult);

	bool		Win_WinActive(void);
	bool		Win_WinExists(void);

	bool		ControlSearch(VectorVariant &vParams);
	static BOOL CALLBACK ControlSearchProc(HWND hWnd, LPARAM lParam);
	BOOL		ControlSearchProcHandler(HWND hWnd, LPARAM lParam);

	AUT_RESULT	F_WinGetProcess(VectorVariant &vParams, Variant &vResult);



	// Process related functions (script_process.cpp)
	AUT_RESULT	F_Run(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_RunWait(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_RunAsSet(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	Run(int nFunction, VectorVariant &vParams, uint iNumParams, Variant &vResult);
	AUT_RESULT	F_ProcessClose(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_ProcessExists(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_ProcessWait(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_ProcessWaitClose(VectorVariant &vParams, Variant &vResult);
	bool		HandleProcessWait(void);
	void		ProcessWaitInit(VectorVariant &vParams, uint iNumParams);
	AUT_RESULT	F_Shutdown(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_ProcessSetPriority(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_ProcessList(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_DllCall(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_DllOpen(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_DllClose(VectorVariant &vParams, Variant &vResult);


	// Misc functions (script_misc.cpp)
	AUT_RESULT	F_MsgBox (VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_Send(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_MouseDown(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_MouseUp(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_TrayTip(VectorVariant &vString, Variant &vResult);
	AUT_RESULT	F_Sleep(VectorVariant &vString, Variant &vResult);
	AUT_RESULT	F_EnvGet(VectorVariant &vString, Variant &vResult);
	AUT_RESULT	F_EnvSet(VectorVariant &vString, Variant &vResult);
	AUT_RESULT	F_BlockInput(VectorVariant &vString, Variant &vResult);
	AUT_RESULT	F_AdlibDisable(VectorVariant &vString, Variant &vResult);
	AUT_RESULT	F_AdlibEnable(VectorVariant &vString, Variant &vResult);
	AUT_RESULT	F_AutoItWinSetTitle(VectorVariant &vString, Variant &vResult);
	AUT_RESULT	F_AutoItWinGetTitle(VectorVariant &vString, Variant &vResult);
	AUT_RESULT	F_ClipGet(VectorVariant &vString, Variant &vResult);
	AUT_RESULT	F_ClipPut(VectorVariant &vString, Variant &vResult);
	AUT_RESULT	F_HttpSetProxy(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_FtpSetProxy(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_InetGet(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_InetGetSize(VectorVariant &vParams, Variant &vResult);
	bool		MySplitURL(const char *szUrlFull, DWORD &dwService, int &nPort, char *szHost, char *szUrl, char *szUser, char *szPwd);
	bool		MyInternetOpen(DWORD dwService, HINTERNET &hInet);
	static void __cdecl InetGetThreadHandler(void *vp);
	void		InetGetThread(void);
	AUT_RESULT	F_PixelSearch(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_PixelGetColor(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_UBound(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_SetError(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_SetExtended(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_SoundPlay(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_IsAdmin(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_MouseClick(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_MouseMove(VectorVariant &vParams, Variant &vResult);
	void		MouseMoveExecute(int x, int y, int nSpeed);
	AUT_RESULT	F_MouseGetPos(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_SplashImageOn(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_SplashTextOn(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_SplashOff(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	Splash(VectorVariant &vParams, uint iNumParams, int nFlag);
	AUT_RESULT	F_ProgressOn(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_ProgressOff(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_ProgressSet(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	Progress(VectorVariant &vParams, uint iNumParams, int nFlag);
	AUT_RESULT	F_MouseGetCursor(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_Break(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_MouseClickDrag(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_InputBox(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_EnvUpdate(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_SoundSetWaveVolume(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_TimerInit(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_TimerDiff(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_Call(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_AutoItSetOption(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_HotKeySet(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_Eval(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_Assign(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_IsDeclared(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_MemGetStats(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_MouseWheel(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_PixelChecksum (VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_Ping(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_ConsoleWrite (VectorVariant &vParams, Variant &vResult);
	void		ConvertCoords(int nCoordMode, POINT &pt);


	// Gui related functions
#ifdef AUT_CONFIG_GUI							// Is GUI enabled?
	AUT_RESULT	F_GUICreate(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUISwitch(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUICtrlDelete(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUICtrlCreateAvi(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUICtrlCreateButton(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUICtrlCreateCheckbox (VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUICtrlCreateCombo(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUICtrlCreateContextMenu(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUICtrlCreateDate(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUICtrlCreateEdit(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUICtrlCreateGroup(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUICtrlCreateIcon(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUICtrlCreateInput(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUICtrlCreateLabel(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUICtrlCreateList(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUICtrlCreateMenu(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUICtrlCreateMenuItem(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUICtrlCreatePic(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUICtrlCreateProgress(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUICtrlCreateRadio(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUICtrlCreateSlider(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUICtrlCreateTab(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUICtrlCreateTabitem(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUICtrlCreateTrayMenu(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUICtrlCreateTreeView(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUICtrlCreateTreeViewItem(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUICtrlCreateListView(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUICtrlCreateListViewItem(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUICtrlCreateUpdown(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUICtrlCreateDummy(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	GUICtrlCreate(int nType, VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUICtrlGetState(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUICtrlSetBkColor(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUICtrlSetOnEvent(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUICtrlSetColor(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUICtrlSetCursor(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUICtrlSetData(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUICtrlSetImage(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUICtrlSetFont(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUICtrlSetLimit(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUICtrlSetPos(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUICtrlSetResizing(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUICtrlSetState(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUICtrlSetStyle(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUICtrlSetTip(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUIDelete(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUIGetMsg(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUIRead(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUISendToDummy(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUISendMsg(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUIRecvMsg(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUISetBkColor(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUISetCoord(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUIGetCursorInfo(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUISetOnEvent(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUISetCursor(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUISetFont(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUISetHelp(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUISetIcon(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUISetState(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUISetTrayTip(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUISetTrayBalloon(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUISetTrayIcon(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_GUIStartGroup(VectorVariant &vParams, Variant &vResult);
#endif

	// Registry related functions (script_registry.cpp)
	AUT_RESULT	F_RegRead(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_RegWrite(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_RegDelete(VectorVariant &vParams, Variant &vResult);
	void		RegSplitKey(AString sFull, AString &sCname, AString &sKey, AString &sSubKey);
	bool		RegGetMainKey(AString sKey, HKEY &hKey);
	AUT_RESULT	F_RegEnumKey(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_RegEnumVal(VectorVariant &vParams, Variant &vResult);



	// File related functions (script_file.cpp)
	AUT_RESULT	F_DirCopy(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_IniRead(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_IniWrite(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_IniDelete(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_IniReadSectionNames(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_IniReadSection(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_FileOpen(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_FileClose(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_FileReadLine(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_FileGetTime(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_FileGetSize(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_FileOpenDialog(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_FileSaveDialog(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	FileDialog(VectorVariant &vParams, Variant &vResult, uint iNumParams, int nFlag);
	AUT_RESULT	F_FileWriteLine(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_FileWrite(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	FileWriteLine(VectorVariant &vParams, Variant &vResult, bool bWriteLine);
	AUT_RESULT	F_FileSelectFolder(VectorVariant &vParams, Variant &vResult);
	static int	CALLBACK BrowseForFolderProc(HWND hWnd,UINT iMsg,LPARAM lParam,LPARAM lpData);
	AUT_RESULT	F_DriveMapAdd(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_DriveMapDel(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_DriveMapGet(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_DriveSpaceTotal(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_DriveSpaceFree(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_FileCreateShortcut(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_FileGetShortcut(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_DriveStatus(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_FileExists(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_FileFindFirstFile(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_FileFindNextFile(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_DirCreate(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_DirRemove(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_CDTray(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_DriveGetDrive(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_FileCopy(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_FileDelete(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_FileInstall(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_FileRecycle(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_DriveGetLabel(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_DriveGetSerial(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_DriveGetFileSystem(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_DriveSetLabel(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_DriveGetType(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_FileMove(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_FileGetAttrib(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_FileGetVersion(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_FileGetLongName(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_FileGetShortName(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_FileChangeDir(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_FileSetAttrib(VectorVariant &vParams, Variant &vResult);
	bool		FileSetAttrib_recurse (const char *szFile, DWORD dwAdd, DWORD dwRemove, bool bRecurse);
	AUT_RESULT	F_FileSetTime(VectorVariant &vParams, Variant &vResult);
	bool		FileSetTime_recurse (const char *szIn, FILETIME *ft, int nWhichTime, bool bRecurse);
	AUT_RESULT	F_DirMove(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_FileRead(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_FileRecycleEmpty(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_DirGetSize(VectorVariant &vParams, Variant &vResult);
	bool		GetDirSize(const char *szInputPath, __int64 &nSize, __int64 &nFiles, __int64 &nDirs, bool bExt, bool bRec);


	// String related functions (script_string.cpp)
	AUT_RESULT	F_StringInStr(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_StringLen(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_StringLeft(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_StringRight(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_StringMid(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_StringTrimLeft(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_StringTrimRight(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_StringLower(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_StringUpper(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_StringReplace(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_StringStripCR(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_StringAddCR(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_StringIsAlpha(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_StringIsAlnum(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_StringIsDigit(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_StringIsXDigit(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_StringIsLower(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_StringIsUpper(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_StringIsSpace(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_StringIsASCII(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_StringStripWS(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_StringIsFloat(VectorVariant &vParams, Variant &vResult);
	bool		StringIsFloat(Variant &vParams);
	AUT_RESULT	F_StringIsInt(VectorVariant &vParams, Variant &vResult);
	bool		StringIsInt(Variant &vParams);
	AUT_RESULT	F_StringSplit(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_StringFormat(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_StringRegExp(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_StringRegExpReplace(VectorVariant &vParams, Variant &vResult);

	// Math / Conversion functions (script_math.cpp)
	AUT_RESULT	F_BitAND(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_BitOR(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_BitNOT(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_BitXOR(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_BitShift(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_Chr(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_Asc(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_Dec(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_VarType(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_Int(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_IsArray(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_IsString(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_IsInt(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_IsFloat(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_IsNumber(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_Number(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_String(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_Random(VectorVariant &vParams, Variant &vResult);

	AUT_RESULT	F_Sin(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_ASin(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_Cos(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_ACos(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_Tan(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_ATan(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_Log(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_Exp(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_Abs(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_Mod(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_Sqrt(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_Round(VectorVariant &vParams, Variant &vResult);
	AUT_RESULT	F_Hex(VectorVariant &vParams, Variant &vResult);

};

///////////////////////////////////////////////////////////////////////////////

#endif

