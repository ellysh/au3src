
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
// cmdline.cpp
//
// A standalone class to make reading command line options a little easier.
//
///////////////////////////////////////////////////////////////////////////////


// Includes
#include "StdAfx.h"								// Pre-compiled headers

#ifndef _MSC_VER								// Includes for non-MS compilers
	#include <stdio.h>
	#include <string.h>
#endif

#include "cmdline.h"
#include "utility.h"


///////////////////////////////////////////////////////////////////////////////
// Constructor()
///////////////////////////////////////////////////////////////////////////////

CmdLine::CmdLine() : m_nNumParameters(0), m_nCurrentParam(0), m_szCmdLineRaw(NULL)
{
//	m_nNumParameters	= 0;					// Number of space separated paramters on the cmdline
//	m_nCurrentParam		= 0;					// Current param to return for GetNextParam()

	// Ensure all pointers are NULL
	for (int i=0; i<CMDLINE_MAXPARAMS; i++)
		m_szParams[i] = NULL;
}


///////////////////////////////////////////////////////////////////////////////
// Destructor()
///////////////////////////////////////////////////////////////////////////////

CmdLine::~CmdLine()
{
	Reset();

}


///////////////////////////////////////////////////////////////////////////////
// SetCmdLine()
///////////////////////////////////////////////////////////////////////////////

void CmdLine::SetCmdLine(char *szCmdLine)
{
	Reset();									// Reset any previous command lines

	// Store the raw command line
	m_szCmdLineRaw = Util_StrCpyAlloc(szCmdLine);

	// Command line parameters are separated by spaces
	// If spaces are required in a parameter, it should be surrounded by quotes
	// If quotes are required, they must be doubled up eg "" is one quote

	int			i = 0;							// In string position
	int			iParam = 0;						// Out param position
	bool		bQuote = false;					// Quoting is inactive
	char		szParam[CMDLINE_MAXLEN+1];		// Store our temp parameter
	char		ch;

	// Whenever we hit a space and we are NOT in quote mode
	// - Store param
	// - Skip spaces

	// Skip leading spaces
	while ( (ch = szCmdLine[i]) == ' ' || ch  == '\t')
		i++;

	while ( ((ch = szCmdLine[i++]) != '\0')  &&  (iParam < CMDLINE_MAXLEN) )
	{
		if ( ch == ' '|| ch  == '\t')
		{
			// Param separator found - are we in quote mode?
			if ( bQuote == true )
			{
				szParam[iParam++] = ch;			// Store as normal - not param separator
			}
			else
			{
				szParam[iParam] = '\0';
				StoreParam(szParam);
				iParam = 0;
				// We are starting a param, skip all spaces
				while ( (ch = szCmdLine[i]) == ' '|| ch  == '\t' )
					i++;
			}
		}
		else
		{
			if ( ch == '"' )
			{
				if (szCmdLine[i] == '"')
				{
					szParam[iParam++] = '"';
					i++;
				}
				else
					bQuote = !bQuote;					// Quote found - toggle quote mode
			}
			else
				szParam[iParam++] = ch;				// Simply store character
		}

	} // End While

	// Check iParam, if there was a construct in progress, finish and store
	if ( iParam != 0 )
	{
		szParam[iParam] = '\0';
		StoreParam(szParam);
	}


} // SetCmdLine()


///////////////////////////////////////////////////////////////////////////////
// StoreParam()
///////////////////////////////////////////////////////////////////////////////

void CmdLine::StoreParam(char *szParam)
{
	// Create enough space to store our line + \0
	//m_szParams[m_nNumParameters] = (char *)malloc((strlen(szParam)+1) * sizeof(char));
	m_szParams[m_nNumParameters++] = Util_StrCpyAlloc(szParam);

} // StoreParam()

///////////////////////////////////////////////////////////////////////////////
// Reset()
///////////////////////////////////////////////////////////////////////////////

void CmdLine::Reset(void)
{
	// Ensure all pointers are NULL
	for (int i=0; i<m_nNumParameters; i++)
	{
		//free(m_szParams[i]);
		delete [] m_szParams[i];				// Harmless if already NULL
		m_szParams[i] = NULL;
	}

	// Delete the raw command line
	delete [] m_szCmdLineRaw;					// Harmless if already NULL

	m_nNumParameters	= 0;
	m_nCurrentParam		= 0;

} // Reset()


///////////////////////////////////////////////////////////////////////////////
// GetParam()
///////////////////////////////////////////////////////////////////////////////

bool CmdLine::GetParam(int nParam, char *szParam) const
{
	// Ensure we don't pass back crap if there is an error
	szParam[0] = '\0';

	if (nParam >= m_nNumParameters)
		return false;							// Invalid request
	else
	{
		if (m_szParams[nParam] == NULL)
			return false;
		else
		{
			strcpy(szParam, m_szParams[nParam]);
			return true;
		}
	}

} // GetParam()


///////////////////////////////////////////////////////////////////////////////
// GetNextParam()
///////////////////////////////////////////////////////////////////////////////

bool CmdLine::GetNextParam(char *szParam)
{
	// Ensure we don't pass back crap if there is an error
	szParam[0] = '\0';

	if (m_nCurrentParam >= m_nNumParameters)
		return false;							// Invalid request
	else
	{
		if (m_szParams[m_nCurrentParam] == NULL)
			return false;
		else
		{
			strcpy(szParam, m_szParams[m_nCurrentParam]);
			m_nCurrentParam++;
			return true;
		}
	}

} // GetNextParam()
