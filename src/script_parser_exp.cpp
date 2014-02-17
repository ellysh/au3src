
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
// script_parser_exp.cpp
//
// Contains expression parsing code.  Part of script.cpp
//
///////////////////////////////////////////////////////////////////////////////


// Includes
#include "StdAfx.h"								// Pre-compiled headers

#ifndef _MSC_VER								// Includes for non-MS compilers
	#include <stdio.h>
	#include <windows.h>
	#include <time.h>
	#include <math.h>
#else
	#include "qmath.h"							// MinGW doesn't like our asm maths functions
#endif

#include "AutoIt.h"								// Autoit values, macros and config options

#include "script.h"
#include "utility.h"
#include "resources\resource.h"
#include "globaldata.h"


// Inbuilt macro values (must match the order as in script.cpp)
enum
{
	M_ERROR = 0, M_EXTENDED,
	M_SEC, M_MIN, M_HOUR, M_MDAY, M_MON, M_YEAR, M_WDAY, M_YDAY,
	M_PROGRAMFILESDIR, M_COMMONFILESDIR,
	M_MYDOCUMENTSDIR, M_APPDATACOMMONDIR, M_DESKTOPCOMMONDIR, M_DOCUMENTSCOMMONDIR, M_FAVORITESCOMMONDIR,
	M_PROGRAMSCOMMONDIR, M_STARTMENUCOMMONDIR, M_STARTUPCOMMONDIR,
	M_APPDATADIR, M_DESKTOPDIR, M_FAVORITESDIR, M_PROGRAMSDIR, M_STARTMENUDIR, M_STARTUPDIR,
	M_COMPUTERNAME, M_WINDOWSDIR, M_SYSTEMDIR,
	M_SW_HIDE, M_SW_MINIMIZE, M_SW_MAXIMIZE, M_SW_RESTORE, M_SW_SHOW, M_SW_SHOWDEFAULT, M_SW_ENABLE, M_SW_DISABLE,
	M_SW_SHOWMAXIMIZED, M_SW_SHOWMINIMIZED, M_SW_SHOWMINNOACTIVE, M_SW_SHOWNA, M_SW_SHOWNOACTIVATE, M_SW_SHOWNORMAL,
	M_SCRIPTFULLPATH, M_SCRIPTNAME, M_SCRIPTDIR, M_WORKINGDIR,
	M_OSTYPE, M_OSVERSION, M_OSBUILD, M_OSSERVICEPACK, M_OSLANG,
	M_AUTOITVERSION, M_AUTOITEXE, M_IPADDRESS1, M_IPADDRESS2, M_IPADDRESS3,
	M_IPADDRESS4, M_CR, M_LF, M_CRLF, M_DESKTOPWIDTH, M_DESKTOPHEIGHT, M_DESKTOPDEPTH, M_DESKTOPREFRESH,
	M_COMPILED, M_COMSPEC, M_TAB,
	M_USERNAME, M_TEMPDIR,
	M_USERPROFILEDIR, M_HOMEDRIVE,
	M_HOMEPATH, M_HOMESHARE, M_LOGONSERVER, M_LOGONDOMAIN,
	M_LOGONDNSDOMAIN, M_INETGETBYTESREAD, M_INETGETACTIVE,
	M_NUMPARAMS,
	M_MAX
};


// Macro variables - order must match above
// Must be in UPPERCASE
char * AutoIt_Script::m_szMacros[M_MAX] =	{
	"ERROR", "EXTENDED",
	"SEC", "MIN", "HOUR", "MDAY", "MON", "YEAR", "WDAY", "YDAY",
	"PROGRAMFILESDIR", "COMMONFILESDIR",
	"MYDOCUMENTSDIR", "APPDATACOMMONDIR", "DESKTOPCOMMONDIR", "DOCUMENTSCOMMONDIR", "FAVORITESCOMMONDIR",
	"PROGRAMSCOMMONDIR", "STARTMENUCOMMONDIR", "STARTUPCOMMONDIR",
	"APPDATADIR", "DESKTOPDIR", "FAVORITESDIR", "PROGRAMSDIR", "STARTMENUDIR", "STARTUPDIR",
	"COMPUTERNAME", "WINDOWSDIR", "SYSTEMDIR",
	"SW_HIDE", "SW_MINIMIZE", "SW_MAXIMIZE", "SW_RESTORE", "SW_SHOW", "SW_SHOWDEFAULT", "SW_ENABLE", "SW_DISABLE",
	"SW_SHOWMAXIMIZED", "SW_SHOWMINIMIZED", "SW_SHOWMINNOACTIVE", "SW_SHOWNA", "SW_SHOWNOACTIVATE", "SW_SHOWNORMAL",
	"SCRIPTFULLPATH", "SCRIPTNAME", "SCRIPTDIR", "WORKINGDIR",
	"OSTYPE", "OSVERSION", "OSBUILD", "OSSERVICEPACK", "OSLANG",
	"AUTOITVERSION", "AUTOITEXE", "IPADDRESS1", "IPADDRESS2", "IPADDRESS3",
	"IPADDRESS4", "CR", "LF", "CRLF", "DESKTOPWIDTH", "DESKTOPHEIGHT", "DESKTOPDEPTH", "DESKTOPREFRESH",
	"COMPILED", "COMSPEC", "TAB",
	"USERNAME", "TEMPDIR",
	"USERPROFILEDIR", "HOMEDRIVE",
	"HOMEPATH", "HOMESHARE", "LOGONSERVER", "LOGONDOMAIN",
	"LOGONDNSDOMAIN", "INETGETBYTESREAD", "INETGETACTIVE",
	"NUMPARAMS"
};


// Operator precedence parsing stuff - ordering is important (see script_parser_exp.cpp)
enum
{
	S,											// Shift
	R,											// Reduce
	RP,											// Reduce parenthesis
	A,											// Accept
	E1,											// Error: missing )
	E2,											// Error: mising operator
	E3											// Error: unbalanced )
};

// Table for operator precedence rules (order must match script.h)
// Also not that the Boolean "skipping" function relies on the fact than AND/OR are
// lowest precedence and the value stack is reduced when they are encountered
//
// Precedence is Aritmentic -> comparison -> logical
// ( )
// M NOT
// ^
// * /
// + -
// &
// < > <= >= = <> ==
// AND OR
//
// Input is higher precedence than stack = Shift
// Input is lower/same prec. than stack  = Reduce

char AutoIt_Script::m_PrecOpRules[OPR_MAXOPR][OPR_MAXOPR] = {
/* Stack    ------------------ Input -------------------- */
/*        <   >   <=  >=  <>  =   == AND  OR  &  NOT  +   -   *   /   ^   M   P,  (   )   END */
/* < */ { R,  R,  R,  R,  R,  R,  R,  R,  R,  S,  S,  S,  S,  S,  S,  S,  S,  S,  S,  RP, R  },
/* > */ { R,  R,  R,  R,  R,  R,  R,  R,  R,  S,  S,  S,  S,  S,  S,  S,  S,  S,  S,  RP, R  },
/*<= */ { R,  R,  R,  R,  R,  R,  R,  R,  R,  S,  S,  S,  S,  S,  S,  S,  S,  S,  S,  RP, R  },
/*>= */ { R,  R,  R,  R,  R,  R,  R,  R,  R,  S,  S,  S,  S,  S,  S,  S,  S,  S,  S,  RP, R  },
/*<> */ { R,  R,  R,  R,  R,  R,  R,  R,  R,  S,  S,  S,  S,  S,  S,  S,  S,  S,  S,  RP, R  },
/* = */ { R,  R,  R,  R,  R,  R,  R,  R,  R,  S,  S,  S,  S,  S,  S,  S,  S,  S,  S,  RP, R  },
/*== */ { R,  R,  R,  R,  R,  R,  R,  R,  R,  S,  S,  S,  S,  S,  S,  S,  S,  S,  S,  RP, R  },
/*AND*/ { S,  S,  S,  S,  S,  S,  S,  R,  R,  S,  S,  S,  S,  S,  S,  S,  S,  S,  S,  RP, R  },
/*OR */ { S,  S,  S,  S,  S,  S,  S,  R,  R,  S,  S,  S,  S,  S,  S,  S,  S,  S,  S,  RP, R  },
/* & */ { R,  R,  R,  R,  R,  R,  R,  R,  R,  R,  S,  S,  S,  S,  S,  S,  S,  S,  S,  RP, R  },
/*NOT*/ { R,  R,  R,  R,  R,  R,  R,  R,  R,  R,  S,  R,  R,  R,  R,  R,  S,  S,  S,  RP, R  },
/* + */ { R,  R,  R,  R,  R,  R,  R,  R,  R,  R,  S,  R,  R,  S,  S,  S,  S,  S,  S,  RP, R  },
/* - */ { R,  R,  R,  R,  R,  R,  R,  R,  R,  R,  S,  R,  R,  S,  S,  S,  S,  S,  S,  RP, R  },
/* * */ { R,  R,  R,  R,  R,  R,  R,  R,  R,  R,  S,  R,  R,  R,  R,  S,  S,  S,  S,  RP, R  },
/* / */ { R,  R,  R,  R,  R,  R,  R,  R,  R,  R,  S,  R,  R,  R,  R,  S,  S,  S,  S,  RP, R  },
/* ^ */ { R,  R,  R,  R,  R,  R,  R,  R,  R,  R,  S,  R,  R,  R,  R,  R,  S,  S,  S,  RP, R  },
/* M */ { R,  R,  R,  R,  R,  R,  R,  R,  R,  R,  S,  R,  R,  R,  R,  R,  S,  S,  S,  RP, R  },
/* P */ { R,  R,  R,  R,  R,  R,  R,  R,  R,  R,  S,  R,  R,  R,  R,  R,  S,  S,  S,  RP, R  },
/* ( */ { S,  S,  S,  S,  S,  S,  S,  S,  S,  S,  S,  S,  S,  S,  S,  S,  S,  S,  S,  RP, E1 },
/* ) */ { R,  R,  R,  R,  R,  R,  R,  R,  R,  R,  R,  R,  R,  R,  R,  R,  R,  R,  E2, RP, R  },
/*END*/ { S,  S,  S,  S,  S,  S,  S,  S,  S,  S,  S,  S,  S,  S,  S,  S,  S,  S,  S,  E3, A  },
};



///////////////////////////////////////////////////////////////////////////////
// Parser_ExpandEnvString()
//
// This function takes a string variant and expands any environment variables
// that it contains.
//
// This will expand ANY environment variables in a given line
// An env variable is of the form %variable%
// The sequence %% will be changed to a single %
//
///////////////////////////////////////////////////////////////////////////////

void AutoIt_Script::Parser_ExpandEnvString(Variant &vString)
{
	AString	sResult;
	AString sName;
	char	szValue[AUT_MAX_LINESIZE+1];		// Temp variable value
	int		nLine = 0;					// In and out string positions
	char	ch = '\0';							// Current character
	const	char *szLine = vString.szValue();
	bool	bModified = false;

	// Perform loop until we hit the end of the input string
	while ( (ch = szLine[nLine++]) != '\0')
	{
		if ( ch == '%' )
		{
			bModified = true;

			// if is %% then just store a single %
			if (szLine[nLine] == '%')
			{
				sResult += '%';
				++nLine;
			}
			else
			{
				// This is the real start of an ENV variable, search for the matching % or \0 (error ish)
				sName = "";

				while ( (szLine[nLine] != '%') && (szLine[nLine] != '\0') )
					sName += szLine[nLine++];

				if (szLine[nLine] == '%')
					++nLine;						// Skip the closing %

				// Get the value of this variable and copy it to our result string
				szValue[0] = '\0';				// Term just in case the GetEnv function fails
				GetEnvironmentVariable(sName.c_str(), szValue, AUT_MAX_LINESIZE);

				sResult += szValue;
			}
		}
		else
			sResult += ch;			// Store the char

	} // End While

	// Only copy the string if any expansion happened - waste of performance otherwise
	if (bModified)
		vString = sResult.c_str();

} // Parser_ExpandEnvString()


///////////////////////////////////////////////////////////////////////////////
// Parser_ExpandVarString()
//
// This function takes a string variant and expands any variabls and macros
//
///////////////////////////////////////////////////////////////////////////////

void AutoIt_Script::Parser_ExpandVarString(Variant &vString)
{
	AString	sResult;
	AString sName;
	int		nLine = 0;					// In and out string positions
	char	ch = '\0';							// Current character
	const	char *szLine = vString.szValue();
	bool	bModified = false;
	Variant	*pvTemp;
	bool	bConst = false;
	Variant	vTemp;

	// Perform loop until we hit the end of the input string
	while ( (ch = szLine[nLine++]) != '\0')
	{
		if ( ch == '$' )
		{
			bModified = true;

			// if is %% then just store a single %
			if (szLine[nLine] == '$')
			{
				sResult += '$';
				nLine++;
			}
			else
			{
				// This is the real start of an ENV variable, search for the matching % or \0 (error ish)
				sName = "";

				while ( (szLine[nLine] != '$') && (szLine[nLine] != '\0') )
					sName += szLine[nLine++];

				if (szLine[nLine] == '\0')
					break;

				++nLine;						// Skip the closing %

				// Get the value of this variable and copy it to our result string
				g_oVarTable.GetRef(sName.c_str(), &pvTemp, bConst);
				if (pvTemp != NULL)
					sResult += pvTemp->szValue();
			}
		}
		else if ( ch == '@' )
		{
			// if is %% then just store a single %
			if (szLine[nLine] == '@')
			{
				sResult += '@';
				nLine++;
			}
			else
			{
				// This is the real start of an ENV variable, search for the matching % or \0 (error ish)
				bModified = true;
				sName = "";

				while ( (szLine[nLine] != '@') && (szLine[nLine] != '\0') )
					sName += szLine[nLine++];

				if (szLine[nLine] == '\0')
					break;

				++nLine;						// Skip the closing %

				// Get the value of this variable and copy it to our result string
				if ( AUT_SUCCEEDED(Parser_EvaluateMacro(sName.c_str(), vTemp)) )
					sResult += vTemp.szValue();
			}
		}

		else
			sResult += ch;			// Store the char

	} // End While

	// Only copy the string if any expansion happened - waste of performance otherwise
	if (bModified)
		vString = sResult.c_str();

} // Parser_ExpandVarString()


///////////////////////////////////////////////////////////////////////////////
// Parser_EvaluateVariable()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::Parser_EvaluateVariable(VectorToken &vLineToks, unsigned int &ivPos, Variant &vResult)
{
	Variant		*pvTemp;
	bool		bConst = false;
	Variant		vTemp;

	// Get a reference to the variant
	g_oVarTable.GetRef(vLineToks[ivPos].szValue, &pvTemp, bConst);
	if (pvTemp == NULL)
	{
		FatalError(IDS_AUT_E_VARNOTFOUND, vLineToks[ivPos].m_nCol);
		return AUT_ERR;
	}

	// Is the variable a single variant or part of any array?
	// Treat arrays with no subscripts as single objects
	if (pvTemp->type() != VAR_ARRAY)
	{
		// Normal/single  variant
		vResult = *pvTemp;
		ivPos++;								// Next token

		// If next token is [ then trying to use a non array as an array
		if (pvTemp->type() != VAR_ARRAY && vLineToks[ivPos].m_nType == TOK_LEFTSUBSCRIPT)
		{
			FatalError(IDS_AUT_E_NONARRAYWITHSUBSCRIPT, vLineToks[ivPos].m_nCol);
			return AUT_ERR;
		}
		else
			return AUT_OK;
	}

	// Is an array type if we get to here

	ivPos++;									// Next token (skip $var)

	if (vLineToks[ivPos].m_nType != TOK_LEFTSUBSCRIPT)
	{
		// No subscript, treat as a single entity
		vResult = *pvTemp;
		return AUT_OK;
	}

	// Is array type variant with a subscript, parse it and evaluate
	if ( AUT_FAILED(Parser_GetArrayElement(vLineToks, ivPos, &pvTemp)) )
		return AUT_ERR;

	// Assign the resulting value
	vResult = *pvTemp;

	return AUT_OK;

} // Parse_EvaluateVariable()


///////////////////////////////////////////////////////////////////////////////
// Parser_GetArrayElement()
//
// Gets a reference to an array element (variant).  Assumes that the next
// token is [.
// pvTemp [in] = pointer to array
// pvTemp [out] = pointer to array element
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::Parser_GetArrayElement(VectorToken &vLineToks, uint &ivPos, Variant **ppvTemp)
{
	Variant vTemp;
	int		nColVar = -1;
	int		nColTemp = -1;
	int		nSubScripts[VAR_SUBSCRIPT_MAX];
	int		nSub;

	// If the variant is an array, then read the subscript, and get a reference
	// We need to use a local array to store the subscript
	// and then when all subscripts have been parsed THEN we get a reference to the element, this is
	// to allow features like $test[$test[0]] - otherwise the recursive ArraySubscriptSetNext() calls
	// would get messed up.
	nSub = 0;									// Current subscript

	nColVar = vLineToks[ivPos-1].m_nCol;		// Save variable name for error messages

	while (vLineToks[ivPos].m_nType == TOK_LEFTSUBSCRIPT)
	{
		ivPos++;								// Skip [
		nColTemp = vLineToks[ivPos].m_nCol;	// Save start of expression for error messages

		// Parse expression for subscript
		if ( AUT_FAILED( Parser_EvaluateExpression(vLineToks, ivPos, vTemp) ) )
		{
			//FatalError(IDS_AUT_E_PARSESUBSCRIPT, nColTemp);
			return AUT_ERR;
		}

		// Subscript cannot be < 0
		if ( vTemp.nValue() < 0 )
		{
			FatalError(IDS_AUT_E_PARSESUBSCRIPT, nColTemp);
			return AUT_ERR;
		}

		// Next token must be ]
		if (vLineToks[ivPos].m_nType != TOK_RIGHTSUBSCRIPT)
		{
			//AUT_MSGBOX("", "Thisone")
			FatalError(IDS_AUT_E_PARSESUBSCRIPT, vLineToks[ivPos-1].m_nCol);
			return AUT_ERR;
		}

		ivPos++;								// Next token

		// Add this subscript
		nSubScripts[nSub++] = vTemp.nValue();

	} // End While


	// Based on the subscript we just parsed, get the relevant array element
	(*ppvTemp)->ArraySubscriptClear();			// Reset the subscript

	for (int i = 0; i < nSub; i++)
		(*ppvTemp)->ArraySubscriptSetNext(nSubScripts[i]);

	(*ppvTemp) = (*ppvTemp)->ArrayGetRef();

	if ((*ppvTemp) == NULL)
	{
		FatalError(IDS_AUT_E_BADSUBSCRIPT, nColVar);	// Use the initial variable for the error message
		return AUT_ERR;
	}

	return AUT_OK;

} // Parser_GetArrayElement()


///////////////////////////////////////////////////////////////////////////////
// Parser_EvaluateMacro()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::Parser_EvaluateMacro(const char *szName, Variant &vResult)
{
	AString		sMacro = szName;
	char		szValue[_MAX_PATH+1] = "";
	char		szValue2[_MAX_PATH+1] ="";
	int			i = 0;
	struct		tm *newtime;
    time_t		long_time;
	DWORD		dwTemp;
	int			nTemp;
	RECT		rTemp;
	char		szInetAddr[16];
	HDC			hdc;
	HWND		hWnd;

	time(&long_time);							// Get time as long integer
	newtime = localtime(&long_time);			// Convert to local time

	sMacro.toupper();							// Convert to uppercase for quicker comparisons (strcmp rather than stricmp)
	const char *szMacro = sMacro.c_str();		// Get a const pointer for speed


	while ( (i < M_MAX) && (strcmp(m_szMacros[i], szMacro)) )
		i++;

	if (i == M_MAX)
	{
		// No Macro match - check the variable table for a global variable of the same
		// Name (WITH the @ prefix...) - used for special vars like @ExitMethod, @ExitCode
		AString sNewMacro("@");
		Variant *pvTemp;
		bool	bConst = false;

		sNewMacro += sMacro;

		g_oVarTable.GetRef(sNewMacro, &pvTemp, bConst);
		if (pvTemp == NULL)
			return AUT_ERR;
		else
		{
			vResult = *pvTemp;
			return AUT_OK;
		}
	}


	// Return the relevant macro value
	switch (i)
	{
		case M_CR:
			vResult = "\r";
			break;
		case M_LF:
			vResult = "\n";
			break;
		case M_CRLF:
			vResult = "\r\n";
			break;
		case M_TAB:
			vResult = "\t";
			break;

		case M_ERROR:
			vResult = m_nFuncErrorCode;	// Extended function error code
			break;

		case M_EXTENDED:
			vResult = m_nFuncExtCode;	// Extended function code
			break;

		case M_SEC:
			sprintf(szValue, "%.2d", newtime->tm_sec);
			vResult = szValue;
			break;

		case M_MIN:
			sprintf(szValue, "%.2d", newtime->tm_min);
			vResult = szValue;
			break;

		case M_HOUR:
			sprintf(szValue, "%.2d", newtime->tm_hour);
			vResult = szValue;
			break;

		case M_MDAY:
			sprintf(szValue, "%.2d", newtime->tm_mday);
			vResult = szValue;
			break;

		case M_MON:
			sprintf(szValue, "%.2d", newtime->tm_mon + 1);
			vResult = szValue;
			break;

		case M_YEAR:
			vResult = int(newtime->tm_year + 1900);
			break;

		case M_WDAY:
			vResult = int(newtime->tm_wday + 1);
			break;

		case M_YDAY:
			sprintf(szValue, "%.3d", newtime->tm_yday);
			vResult = szValue;
			break;

		case M_PROGRAMFILESDIR:
			Util_RegReadString(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion", "ProgramFilesDir", _MAX_PATH, szValue);
			vResult = szValue;
			break;
		case M_COMMONFILESDIR:
			Util_RegReadString(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion", "CommonFilesDir", _MAX_PATH, szValue);
			vResult = szValue;
			break;

		case M_MYDOCUMENTSDIR:
			Util_RegReadString(HKEY_CURRENT_USER, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", "Personal", _MAX_PATH, szValue);
			Util_StripTrailingDir(szValue);		// Remove trailing blackslash
			vResult = szValue;
			break;

		case M_APPDATACOMMONDIR:
			Util_RegReadString(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", "Common AppData", _MAX_PATH, szValue);
			vResult = szValue;
			break;
		case M_DESKTOPCOMMONDIR:
			Util_RegReadString(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", "Common Desktop", _MAX_PATH, szValue);
			if (szValue[0] == '\0')
				Util_RegReadString(HKEY_CURRENT_USER, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", "Desktop", _MAX_PATH, szValue);
			vResult = szValue;
			break;
		case M_DOCUMENTSCOMMONDIR:
			Util_RegReadString(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", "Common Documents", _MAX_PATH, szValue);
			vResult = szValue;
			break;
		case M_FAVORITESCOMMONDIR:
			Util_RegReadString(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", "Common Favorites", _MAX_PATH, szValue);
			if (szValue[0] == '\0')
				Util_RegReadString(HKEY_CURRENT_USER, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", "Favorites", _MAX_PATH, szValue);
			vResult = szValue;
			break;
		case M_PROGRAMSCOMMONDIR:
			Util_RegReadString(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", "Common Programs", _MAX_PATH, szValue);
			if (szValue[0] == '\0')
				Util_RegReadString(HKEY_CURRENT_USER, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", "Programs", _MAX_PATH, szValue);
			vResult = szValue;
			break;
		case M_STARTMENUCOMMONDIR:
			Util_RegReadString(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", "Common Start Menu", _MAX_PATH, szValue);
			if (szValue[0] == '\0')
				Util_RegReadString(HKEY_CURRENT_USER, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", "Start Menu", _MAX_PATH, szValue);
			vResult = szValue;
			break;
		case M_STARTUPCOMMONDIR:
			Util_RegReadString(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", "Common Startup", _MAX_PATH, szValue);
			if (szValue[0] == '\0')
				Util_RegReadString(HKEY_CURRENT_USER, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", "Startup", _MAX_PATH, szValue);
			vResult = szValue;
			break;

		case M_APPDATADIR:
			Util_RegReadString(HKEY_CURRENT_USER, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", "AppData", _MAX_PATH, szValue);
			vResult = szValue;
			break;
		case M_DESKTOPDIR:
			Util_RegReadString(HKEY_CURRENT_USER, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", "Desktop", _MAX_PATH, szValue);
			vResult = szValue;
			break;
		case M_FAVORITESDIR:
			Util_RegReadString(HKEY_CURRENT_USER, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", "Favorites", _MAX_PATH, szValue);
			vResult = szValue;
			break;
		case M_PROGRAMSDIR:
			Util_RegReadString(HKEY_CURRENT_USER, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", "Programs", _MAX_PATH, szValue);
			vResult = szValue;
			break;
		case M_STARTMENUDIR:
			Util_RegReadString(HKEY_CURRENT_USER, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", "Start Menu", _MAX_PATH, szValue);
			vResult = szValue;
			break;
		case M_STARTUPDIR:
			Util_RegReadString(HKEY_CURRENT_USER, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", "Startup", _MAX_PATH, szValue);
			vResult = szValue;
			break;

		case M_COMPUTERNAME:
			dwTemp = _MAX_PATH;
			GetComputerName(szValue, &dwTemp);
			vResult = szValue;
			break;

		case M_WINDOWSDIR:
			GetWindowsDirectory(szValue, _MAX_PATH);
			vResult = szValue;
			break;
		case M_SYSTEMDIR:
			GetSystemDirectory(szValue, _MAX_PATH);
			vResult = szValue;
			break;

		case M_SW_HIDE:
			vResult = SW_HIDE;
			break;
		case M_SW_MINIMIZE:
			vResult = SW_MINIMIZE;
			break;
		case M_SW_MAXIMIZE:
			vResult = SW_MAXIMIZE;
			break;
		case M_SW_RESTORE:
			vResult = SW_RESTORE;
			break;
		case M_SW_SHOW:
			vResult = SW_SHOW;
			break;
		case M_SW_SHOWDEFAULT:
			vResult = SW_SHOWDEFAULT;
			break;
		case M_SW_SHOWMAXIMIZED:
			vResult = SW_SHOWMAXIMIZED;
			break;
		case M_SW_SHOWMINIMIZED:
			vResult = SW_SHOWMINIMIZED;
			break;
		case M_SW_SHOWMINNOACTIVE:
			vResult = SW_SHOWMINNOACTIVE;
			break;
		case M_SW_SHOWNA:
			vResult = SW_SHOWNA;
			break;
		case M_SW_SHOWNOACTIVATE:
			vResult = SW_SHOWNOACTIVATE;
			break;
		case M_SW_SHOWNORMAL:
			vResult = SW_SHOWNORMAL;
			break;

		case M_SCRIPTFULLPATH:
			vResult = m_sScriptFullPath.c_str();
			break;
		case M_SCRIPTNAME:
			vResult = m_sScriptName.c_str();
			break;
		case M_SCRIPTDIR:
			vResult = m_sScriptDir.c_str();
			break;
		case M_WORKINGDIR:
			GetCurrentDirectory(_MAX_PATH, szValue);
			vResult = szValue;
			break;

		case M_OSTYPE:
			if ( g_oVersion.IsWinNT() == true )
				vResult = "WIN32_NT";
			else
				vResult = "WIN32_WINDOWS";
			break;

		case M_OSVERSION:
			if ( g_oVersion.IsWinNT() == true )
			{
				if (g_oVersion.IsWin2003() == true)
					vResult = "WIN_2003";
				else if (g_oVersion.IsWinXP() == true)
					vResult = "WIN_XP";
				else if (g_oVersion.IsWin2000() == true)
					vResult = "WIN_2000";
				else
					vResult = "WIN_NT4";
			}
			else
			{
				if (g_oVersion.IsWin95() == true)
					vResult = "WIN_95";
				else if (g_oVersion.IsWin98() == true)
					vResult = "WIN_98";
				else
					vResult = "WIN_ME";
			} // End If
			break;

		case M_OSBUILD:
			vResult = (int)g_oVersion.BuildNumber();
			break;

		case M_OSSERVICEPACK:
			vResult = g_oVersion.CSD();

			break;

		case M_OSLANG:
			if ( g_oVersion.IsWinNT() == true )
			{
				if ( g_oVersion.IsWin2000orLater() == true )
					Util_RegReadString(HKEY_LOCAL_MACHINE,"SYSTEM\\CurrentControlSet\\Control\\Nls\\Language", "InstallLanguage", _MAX_PATH, szValue);
				else      // WinNT4
					Util_RegReadString(HKEY_LOCAL_MACHINE,"SYSTEM\\CurrentControlSet\\Control\\Nls\\Language", "Default", _MAX_PATH, szValue);

				vResult = szValue;
			}

			else         // Win9x
			{
				Util_RegReadString(HKEY_USERS,".DEFAULT\\Control Panel\\Desktop\\ResourceLocale", "", _MAX_PATH, szValue);
				vResult = &szValue[4];
			}

			break;


		case M_AUTOITVERSION:
			GetModuleFileName (NULL, szValue, sizeof(szValue));

			Util_GetFileVersion(szValue, szValue2);
			vResult = szValue2;
			break;

		case M_AUTOITEXE:
			GetModuleFileName (NULL, szValue, sizeof(szValue));
			vResult = szValue;
			break;


		case M_IPADDRESS1:
			Util_GetIPAddress(1, szInetAddr);
			vResult = szInetAddr;
			break;
		case M_IPADDRESS2:
			Util_GetIPAddress(2, szInetAddr);
			vResult = szInetAddr;
			break;
		case M_IPADDRESS3:
			Util_GetIPAddress(3, szInetAddr);
			vResult = szInetAddr;
			break;
		case M_IPADDRESS4:
			Util_GetIPAddress(4, szInetAddr);
			vResult = szInetAddr;
			break;

		case M_DESKTOPWIDTH:
			GetWindowRect(GetDesktopWindow(), &rTemp);
			vResult = (int)(rTemp.right);
			break;
		case M_DESKTOPHEIGHT:
			GetWindowRect(GetDesktopWindow(), &rTemp);
			vResult = (int)(rTemp.bottom);
			break;
		case M_DESKTOPDEPTH:
			hWnd = GetDesktopWindow();
			hdc = GetDC(hWnd);
			vResult = (int)GetDeviceCaps(hdc, BITSPIXEL);
			ReleaseDC(hWnd, hdc);
			break;
		case M_DESKTOPREFRESH:
			hWnd = GetDesktopWindow();
			hdc = GetDC(hWnd);
			vResult = (int)GetDeviceCaps(hdc, VREFRESH);
			ReleaseDC(hWnd, hdc);
			break;

		case M_COMSPEC:
			GetEnvironmentVariable("COMSPEC", szValue, _MAX_PATH);
			vResult = szValue;
			break;

		case M_TEMPDIR:
			GetTempPath(_MAX_PATH, szValue);
			Util_StripTrailingDir(szValue);		// Remove trailing backslash
			vResult = szValue;
			break;

		case M_USERNAME:
			dwTemp = _MAX_PATH;
			GetUserName(szValue, &dwTemp);
			vResult = szValue;
			break;

#ifndef AUTOITSC
		case M_COMPILED:
			vResult = 0;
			break;
#else
		case M_COMPILED:
			vResult = 1;
			break;
#endif

		case M_USERPROFILEDIR:
			// Deceptively difficult as all the API functions for obtaining this rely on IE4+
			if (g_oVersion.IsWinNT())
				GetEnvironmentVariable("USERPROFILE", szValue, _MAX_PATH);
			else
			{
				// Get the users desktop dir and remove the last \ char
				Util_RegReadString(HKEY_CURRENT_USER, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", "Desktop", _MAX_PATH, szValue);
				for (nTemp = (int)strlen(szValue)-1; nTemp >= 0 && szValue[nTemp] != '\\'; --nTemp);
				szValue[nTemp] = '\0';
			}

			vResult = szValue;
			break;

		case M_HOMEDRIVE:
			GetEnvironmentVariable("HOMEDRIVE", szValue, _MAX_PATH);
			vResult = szValue;
			break;

		case M_HOMEPATH:
			GetEnvironmentVariable("HOMEPATH", szValue, _MAX_PATH);
			vResult = szValue;
			break;

		case M_HOMESHARE:
			GetEnvironmentVariable("HOMESHARE", szValue, _MAX_PATH);
			vResult = szValue;
			break;

		case M_LOGONSERVER:
			GetEnvironmentVariable("LOGONSERVER", szValue, _MAX_PATH);
			vResult = szValue;
			break;

		case M_LOGONDOMAIN:
			GetEnvironmentVariable("USERDOMAIN", szValue, _MAX_PATH);
			vResult = szValue;
			break;

		case M_LOGONDNSDOMAIN:
			GetEnvironmentVariable("USERDNSDOMAIN", szValue, _MAX_PATH);
			vResult = szValue;
			break;

		case M_INETGETBYTESREAD:
			vResult = m_InetGetDetails.nBytesRead;
			break;
		case M_INETGETACTIVE:
			vResult = (int)m_InetGetDetails.bInProgress;
			break;

		case M_NUMPARAMS:
			vResult = m_nNumParams;
			break;

	} // end switch


	return AUT_OK;

} // Parse_EvaluateMacro()


///////////////////////////////////////////////////////////////////////////////
// Parser_EvaluateCondition()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::Parser_EvaluateCondition(VectorToken &vLineToks, unsigned int &ivPos, bool &bResult)
{
	Variant	vTemp;

	// Parse the expression
	if ( AUT_FAILED( Parser_EvaluateExpression(vLineToks, ivPos, vTemp) ) )
		return AUT_ERR;

	// If result is non zero, return true.  Else false
	if (vTemp.isTrue())
		bResult = true;
	else
		bResult = false;

	return AUT_OK;

} // Parse_EvaluateCondition()


///////////////////////////////////////////////////////////////////////////////
// Parser_EvaluateExpression()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::Parser_EvaluateExpression(VectorToken &vLineToks, unsigned int &ivPos, Variant &vResult)
{
	StackInt		opStack;					// Operator parsing stack
	StackVariant	valStack;					// Value (variant) parsing stack

	int				opTemp;
	Variant			vTemp;

	int				opPrev = OPR_NULL;
	bool			bLastOpWasReduce;

	int				nColTemp;


	// Initialise the operator stack with the END token
	opStack.push(OPR_END);

	// Store the column at the start of the expression for easy error output
	nColTemp = vLineToks[ivPos].m_nCol;

	for (;;)
	{
		// Must be either an operator, variant or function, otherwise end of expression reached
		// Convert to the type of tokens this routine uses.  Also execute any functions to get their
		// return values (the function eval may itself call this function for evaluation - recursion
		// rocks! ;) )
		switch ( vLineToks[ivPos].m_nType )
		{
			case TOK_LESS:
				opTemp = OPR_LESS;
				ivPos++;						// Move the position to the next token, ready for next iteration
				break;
			case TOK_GREATER:
				opTemp = OPR_GTR;
				ivPos++;
				break;
			case TOK_LESSEQUAL:
				opTemp = OPR_LESSEQUAL;
				ivPos++;
				break;
			case TOK_GREATEREQUAL:
				opTemp = OPR_GTREQUAL;
				ivPos++;
				break;
			case TOK_NOTEQUAL:
				opTemp = OPR_NOTEQUAL;
				ivPos++;
				break;
			case TOK_EQUAL:
				opTemp = OPR_EQUAL;
				ivPos++;
				break;
			case TOK_STRINGEQUAL:
				opTemp = OPR_STRINGEQUAL;
				ivPos++;
				break;
			case TOK_CONCAT:
				opTemp = OPR_CONCAT;
				ivPos++;
				break;

			case TOK_PLUS:
				// Check for unary plus
				if ( opPrev != OPR_VAL && opPrev != OPR_RPR)
					opTemp = OPR_UPL;
				else
					opTemp = OPR_ADD;
				ivPos++;
				break;

			case TOK_MINUS:
				// Check for unary minus
				if ( opPrev != OPR_VAL && opPrev != OPR_RPR)
					opTemp = OPR_UMI;
				else
					opTemp = OPR_SUB;

				ivPos++;
				break;

			case TOK_MULT:
				opTemp = OPR_MUL;
				ivPos++;
				break;
			case TOK_DIV:
				opTemp = OPR_DIV;
				ivPos++;
				break;
			case TOK_POW:
				opTemp = OPR_POW;
				ivPos++;
				break;
			case TOK_LEFTPAREN:
				opTemp = OPR_LPR;
				ivPos++;
				break;
			case TOK_RIGHTPAREN:
				opTemp = OPR_RPR;
				ivPos++;
				break;

			case TOK_STRING:
				vTemp = vLineToks[ivPos].szValue;

				if (m_bExpandEnvStrings)
					Parser_ExpandEnvString(vTemp);
				if (m_bExpandVarStrings)
					Parser_ExpandVarString(vTemp);

				opTemp = OPR_VAL;
				ivPos++;
				break;

			case TOK_INT32:
				vTemp = vLineToks[ivPos].nValue;
				opTemp = OPR_VAL;
				ivPos++;
				break;
			case TOK_INT64:
				vTemp = vLineToks[ivPos].n64Value;
				opTemp = OPR_VAL;
				ivPos++;
				break;
			case TOK_DOUBLE:
				vTemp = vLineToks[ivPos].fValue;
				opTemp = OPR_VAL;
				ivPos++;
				break;


			case TOK_KEYWORD:
				switch (vLineToks[ivPos].nValue)
				{
					case K_AND:
						opTemp = OPR_LOGAND;
						ivPos++;
						break;
					case K_OR:
						opTemp = OPR_LOGOR;
						ivPos++;
						break;
					case K_NOT:
						opTemp = OPR_NOT;
						ivPos++;
						break;
					default:
						// End of expression (non expression keyword encountered)
						opTemp = OPR_END;
						break;
				}
				break;

			case TOK_FUNCTION:
				// Call function
				if ( AUT_FAILED(Parser_FunctionCall(vLineToks, ivPos, vTemp)) )
					return AUT_ERR;

				opTemp = OPR_VAL;
				break;

			case TOK_USERFUNCTION:
				// Call user function
				if ( AUT_FAILED(Parser_UserFunctionCall(vLineToks, ivPos, vTemp)) )
					return AUT_ERR;

				opTemp = OPR_VAL;
				break;

			case TOK_VARIABLE:
				if ( AUT_FAILED(Parser_EvaluateVariable(vLineToks, ivPos, vTemp)) )
					return AUT_ERR;

				opTemp = OPR_VAL;
				break;

			case TOK_MACRO:
				if ( AUT_FAILED(Parser_EvaluateMacro(vLineToks[ivPos++].szValue, vTemp)) )
				{
					FatalError(IDS_AUT_E_MACROUNKNOWN, vLineToks[ivPos-1].m_nCol);
					return AUT_ERR;
				}

				opTemp = OPR_VAL;
				break;

			default:
				// End of expression (non expression character encountered)
				opTemp = OPR_END;
				break;

		}

		// Update previous operator (used for checking for unary minus and tracking AND/OR)
		opPrev = opTemp;



		// Input is value?
		if (opTemp == OPR_VAL)
		{
			// Shift token to value stack then continue
			valStack.push(vTemp);
			continue;
		}



		// Input is operator, check the rules table based on the last operator (top of stack)
		// and the current operator to find out what to do next

		bLastOpWasReduce = true;
		while (bLastOpWasReduce == true)
		{
			bLastOpWasReduce = false;			// Exit on next loop unless a Reduce operator occurs

			switch ( m_PrecOpRules[opStack.top()][opTemp] )
			{
				case R:						// Reduce
					// Standard Reduce
					if ( AUT_FAILED( Parser_OprReduce(opStack, valStack) ) )
					{
						// Print message and quit
						FatalError(IDS_AUT_E_EXPRESSION, nColTemp);
						return AUT_ERR;
					}
					bLastOpWasReduce = true;		// Loop again with this operator
					break;

				case RP:					// Reduce parenthesis
					// Special case for ), reduce until we find (
					while (opStack.top() != OPR_LPR)
					{
						if (opStack.top() == OPR_END)
						{
							FatalError(IDS_AUT_E_UNBALANCEDPAREN, nColTemp);
							return AUT_ERR;
						}
						else
						{
							// Standard Reduce
							if ( AUT_FAILED( Parser_OprReduce(opStack, valStack) ) )
							{
								// Print message and quit
								FatalError(IDS_AUT_E_EXPRESSION, nColTemp);
								return AUT_ERR;
							}
						}
					}
					opStack.pop();			// Pop the (
					break;

				case S:						// Shift
					opStack.push(opTemp);
					break;

				case A:						// Accept (finish)
					if (valStack.size() != 1)
					{
						// Syntax error, print message and quit
						FatalError(IDS_AUT_E_EXPRESSION, nColTemp);
						return AUT_ERR;
					}
					else
					{
						vResult = valStack.top();
						valStack.pop();
						return AUT_OK;
					}

				case E1:
					FatalError(IDS_AUT_E_RIGHTPAREN, nColTemp);
					return AUT_ERR;
				case E2:
					FatalError(IDS_AUT_E_MISSINGOP, nColTemp);
					return AUT_ERR;
				case E3:
					FatalError(IDS_AUT_E_UNBALANCEDPAREN, nColTemp);
					return AUT_ERR;

			} // End Switch

		} // End While (reducing loop)

		// We have just done an operation, which may have resulted in the stacks being reduced.
		// Check if the last operator (opPrev) was a logical comparision AND/OR.  If it was then
		// because AND/OR is the lowest precedence the stacks WILL have just been reduced above.
		// Make a note of the boolean result (either 1 or 0 based on current top of value stack)
		// and then decide if we need to skip up until the END or next boolean operation -
		// whichever comes first
		if (valStack.size() && (opPrev == OPR_LOGAND || opPrev == OPR_LOGOR) )
		{
			vTemp = valStack.top();		// Get the current top value

			// Skip the rest of the comparision if true + OR, or false + AND
			//if ( (vTemp.nValue() && opPrev == OPR_LOGOR) || (!vTemp.nValue() && opPrev == OPR_LOGAND) )
			if ( (vTemp.isTrue() && opPrev == OPR_LOGOR) || (!vTemp.isTrue() && opPrev == OPR_LOGAND) )
			{
				if ( AUT_FAILED( Parser_SkipBoolean(vLineToks, ivPos) ) )
				{
					FatalError(IDS_AUT_E_EXPRESSION, nColTemp);	// Unbalanced () usually causes an error
					return AUT_ERR;
				}

				// "Fix" our value on the value stack to be 1 or 0 (as the Reduce algorithm would have)
				valStack.pop();
				if (opPrev == OPR_LOGOR)
					vTemp = 1;					// Must be 1 if we skipped for an OR operator!
				else
					vTemp = 0;					// or 0 for an AND
				valStack.push(vTemp);			// Put the result back on the value stack

				// Remove the And/Or from the operator stack and proceed as if nothing happened
				opStack.pop();
			}
		}

	} // End While(1)

	return AUT_OK;

} // Parser_EvaluateExpression()


///////////////////////////////////////////////////////////////////////////////
// Parser_OprReduce()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::Parser_OprReduce(StackInt &opStack, StackVariant &valStack)
{
	Variant	vOp1, vOp2;

	// Check that minimum number of values are on the stack for the operator to
	// be used
	if ( (opStack.top() == OPR_NOT || opStack.top() == OPR_UMI || opStack.top() == OPR_UPL) && valStack.size() < 1 )
		return AUT_ERR;
	if ( (opStack.top() != OPR_NOT && opStack.top() != OPR_UMI && opStack.top() != OPR_UPL ) && valStack.size() < 2 )
		return AUT_ERR;


	// Now perform the required operator function
	switch ( opStack.top() )
	{
		case OPR_GTR:							// Comparision
			vOp2 = valStack.top();	valStack.pop();
			vOp1 = valStack.top();	valStack.pop();

			if (vOp1 > vOp2)
				vOp1 = 1;
			else
				vOp1 = 0;

			valStack.push(vOp1);
			break;

		case OPR_LESS:							// Comparision
			vOp2 = valStack.top();	valStack.pop();
			vOp1 = valStack.top();	valStack.pop();

			if (vOp1 < vOp2 )
				vOp1 = 1;
			else
				vOp1 = 0;

			valStack.push(vOp1);
			break;

		case OPR_GTREQUAL:						// Comparision
			vOp2 = valStack.top();	valStack.pop();
			vOp1 = valStack.top();	valStack.pop();

			if ( (vOp1 > vOp2) || (vOp1 == vOp2) )
				vOp1 = 1;
			else
				vOp1 = 0;

			valStack.push(vOp1);
			break;

		case OPR_LESSEQUAL:						// Comparision
			vOp2 = valStack.top();	valStack.pop();
			vOp1 = valStack.top();	valStack.pop();

			if ( (vOp1 < vOp2) || (vOp1 == vOp2) )
				vOp1 = 1;
			else
				vOp1 = 0;

			valStack.push(vOp1);
			break;

		case OPR_NOTEQUAL:						// Comparision
			vOp2 = valStack.top();	valStack.pop();
			vOp1 = valStack.top();	valStack.pop();

			if (vOp1 == vOp2 )
				vOp1 = 0;
			else
				vOp1 = 1;

			valStack.push(vOp1);
			break;

		case OPR_EQUAL:							// Comparision (=) not case sensitive
			vOp2 = valStack.top();	valStack.pop();
			vOp1 = valStack.top();	valStack.pop();

			if (vOp1 == vOp2)
				vOp1 = 1;
			else
				vOp1 = 0;

			valStack.push(vOp1);
			break;

		case OPR_STRINGEQUAL:					// String comparision (==) (force string and case sense)
			vOp2 = valStack.top();	valStack.pop();
			vOp1 = valStack.top();	valStack.pop();

			if ( vOp1.StringCompare(vOp2) )
				vOp1 = 1;
			else
				vOp1 = 0;

			valStack.push(vOp1);
			break;

		case OPR_CONCAT:						// Concatenation
			vOp2 = valStack.top();	valStack.pop();
			vOp1 = valStack.top();	valStack.pop();
			vOp1.Concat(vOp2);
			valStack.push(vOp1);
			break;

		case OPR_LOGAND:						// Logical AND
			vOp2 = valStack.top();	valStack.pop();
			vOp1 = valStack.top();	valStack.pop();
			if (vOp1 && vOp2)
				vOp1 = 1;
			else
				vOp1 = 0;
			valStack.push(vOp1);
			break;

		case OPR_LOGOR:							// Logical OR
			vOp2 = valStack.top();	valStack.pop();
			vOp1 = valStack.top();	valStack.pop();
			if (vOp1 || vOp2)
				vOp1 = 1;
			else
				vOp1 = 0;
			valStack.push(vOp1);
			break;

		case OPR_NOT:							// Unary NOT
			vOp1 = valStack.top();	valStack.pop();
			vOp1 = !vOp1;			valStack.push(vOp1);
			break;

		case OPR_ADD:							// Addition +
			vOp2 = valStack.top();	valStack.pop();
			vOp1 = valStack.top();	valStack.pop();
			vOp1 += vOp2;			valStack.push(vOp1);
			break;

		case OPR_UPL:							// Unary plus
			break;

		case OPR_SUB:							// Subraction -
			vOp2 = valStack.top();	valStack.pop();
			vOp1 = valStack.top();	valStack.pop();
			vOp1 -= vOp2;			valStack.push(vOp1);
			break;

		case OPR_UMI:							// Unary minus
			vOp1 = valStack.top();	valStack.pop();
			vOp2 = -1;
			vOp1 *= vOp2;			valStack.push(vOp1);	// Multiply by -1
			break;

		case OPR_MUL:							// Multiplication *
			vOp2 = valStack.top();	valStack.pop();
			vOp1 = valStack.top();	valStack.pop();
			vOp1 *= vOp2;			valStack.push(vOp1);
			break;

		case OPR_DIV:							// Division /
			vOp2 = valStack.top();	valStack.pop();
			vOp1 = valStack.top();	valStack.pop();
			vOp1 /= vOp2;			valStack.push(vOp1);
			break;

		case OPR_POW:							// ^
			vOp2 = valStack.top();	valStack.pop();
			vOp1 = valStack.top();	valStack.pop();

			if ( vOp2.fValue() == 0.0 )
				vOp1 = 1.0;
			else if ( vOp1.fValue() == 0.0 && vOp2.fValue() < 0.0)
			{
				//double fTmp = 0.0;	// needed to prevent compiler error.

				//vOp1 = 1.0/fTmp;	// invalid.  Return 1.#INF by forcing divide by 0. GIVES COMPILER WARNING
				vOp1 = 0.0;
			}
			else if ( vOp1.fValue() < 0.0 && (vOp2.type() == VAR_INT32 || vOp2.type() == VAR_INT64) ) // integer (32 or 64 bit)
			{
#ifdef _MSC_VER
				// MS Compiler
				vOp1 = qmathPow(qmathFabs(vOp1.fValue()), vOp2.fValue());
#else
				vOp1 = pow(fabs(vOp1.fValue()), vOp2.fValue());
#endif
				if (vOp2.nValue() & 1) // odd number
				{
					vOp2 = -1.0;
					vOp1 *= vOp2;
				}
			}
			else
#ifdef _MSC_VER
				// MS Compiler
				vOp1 = qmathPow(vOp1.fValue(), vOp2.fValue());
#else
				vOp1 = pow(vOp1.fValue(), vOp2.fValue());
#endif

			valStack.push(vOp1);
			break;
	}

	// Remove top operator from stack
	opStack.pop();

	return AUT_OK;

} // Parser_OprReduce()


///////////////////////////////////////////////////////////////////////////////
// Parser_SkipBoolean()
// This will skip to the next Boolean comparision operator (AND, OR) while
// keeping track of any ( )
// Relies on AND/OR being lowest precedence and DOES NOT validate the stuff
// it is skipping which could contain syntax errors...lazy.
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::Parser_SkipBoolean(VectorToken &vLineToks, unsigned int &ivPos)
{
	int nCount = 0;
	int	tok;

	// We skip to the next AND/OR operator OR until we encounter an invalid expression/END
	// We must make sure that we check open/close brackets too to avoid nested AND/ORs

	for (;;)
	{
		tok = vLineToks[ivPos].m_nType;

		// Keep track of brackets
		if (tok == TOK_LEFTPAREN)
			nCount++;
		else if (tok == TOK_RIGHTPAREN)
		{
			nCount--;
			if (nCount < 0)
				return AUT_OK;					// Unbalanced () but return OK so that (func2() OR func3()) works (0 open (, 1 close ) )
		}

		// If token is end of expression then finish (error if brackets not balanced)
		if (tok == TOK_END)
		{
			if (nCount != 0)
				return AUT_ERR;					// Unbalanced ()
			else
				return AUT_OK;
		}


		// If we are in the balanced level we end if we encounter AND/OR or a non-expression token
		if (nCount == 0)
		{
			// If token is AND/OR then end ( i know the next comparison below will do the same thing but
			// this makes a little easier to understand)
			if (tok == TOK_KEYWORD && (vLineToks[ivPos].nValue == K_AND || vLineToks[ivPos].nValue == K_OR) )
				return AUT_OK;

			// Also end if a non-expression token (in the balanced state ALL keywords are end of expression (except NOT),
			// such as THEN) pretty much every other token can be part of an expression
			if ( ( tok == TOK_KEYWORD && vLineToks[ivPos].nValue != K_NOT) || tok == TOK_COMMA)
				return AUT_OK;
		}

		ivPos++;								// Next token
	}

	return AUT_OK;

} // Parser_SkipBoolean()

