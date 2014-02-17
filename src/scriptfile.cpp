
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
// scriptfile.cpp
//
// The script file object.  This object handles all requests to the script file
// that was read in.
//
// The order must always contain all these calls:
//   1. LoadScript() (or just manually call AddLine)
//   2. PrepareScript()
//   3. UnloadScript()
//
//
///////////////////////////////////////////////////////////////////////////////


// Includes
#include "StdAfx.h"								// Pre-compiled headers

#ifndef _MSC_VER								// Includes for non-MS compilers
	#include <stdio.h>
	#include <windows.h>
#endif

#include "AutoIt.h"								// Autoit values, macros and config options

#include "globaldata.h"
#include "scriptfile.h"
#include "utility.h"

#include "resources\resource.h"


///////////////////////////////////////////////////////////////////////////////
// Constructor()
///////////////////////////////////////////////////////////////////////////////

AutoIt_ScriptFile::AutoIt_ScriptFile()
{
	m_pIncludeDirs	= new char*[256];
	char			szTemp[_MAX_PATH+1];
	char			szRegBuffer[65535+2];	// 64 KB + double null
	m_nIncludeDirs	= 0;

	m_lpScript		= NULL;						// Start of the linked list
	m_lpScriptLast	= NULL;						// Last node of the list
	m_nScriptLines	= 0;						// Number of lines in the list
	m_szScriptLines	= NULL;

	// Zero our include IDs
	m_nNumIncludes	= 0;

	// See if we can support standard includes
	DWORD	dwRes;
	HKEY	hRegKey;

	if ( RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\AutoIt v3\\AutoIt", 0, KEY_READ, &hRegKey) == ERROR_SUCCESS )
	{
		// Get the install directory's include path and add it as the first directory to search.
		dwRes = _MAX_PATH;
		if ( RegQueryValueEx(hRegKey, "InstallDir", NULL, NULL, (LPBYTE)szTemp, &dwRes) == ERROR_SUCCESS )
		{
			// Add a trailing \ if required
			if (strlen(szTemp) && szTemp[strlen(szTemp)-1] != '\\')
				strcat(szTemp, "\\");
			strcat(szTemp, "Include\\");
			m_pIncludeDirs[m_nIncludeDirs] = new char[_MAX_PATH+1];
			strcpy(m_pIncludeDirs[m_nIncludeDirs++], szTemp);
		}
		else
		{
			m_pIncludeDirs[m_nIncludeDirs] = new char[_MAX_PATH+1];
			strcpy(m_pIncludeDirs[m_nIncludeDirs++], "Include\\");
		}
		// Look for a key called "Include" which is REG_MULTI_SZ and lists some user-defined include directories
		dwRes = 65535;
		if (RegQueryValueEx(hRegKey, "Include", NULL, NULL, (LPBYTE)szRegBuffer, &dwRes) == ERROR_SUCCESS)
		{
			szRegBuffer[dwRes] = '\0';
//			szRegBuffer[dwRes+1] = '\0';		// Ensure double null termination

			int i = 0;
			szTemp[0] = '\0';
			char c[2];	// for strcat
			for (;;)
			{
				c[0] = szRegBuffer[i];
				c[1] = '\0';
				if (szRegBuffer[i] == '\0' || szRegBuffer[i] == ';')
				{
					if (strlen(szTemp) && szTemp[strlen(szTemp)-1] != '\\')
						strcat(szTemp, "\\");
					m_pIncludeDirs[m_nIncludeDirs] = new char[_MAX_PATH+1];
					strncpy(m_pIncludeDirs[m_nIncludeDirs++], szTemp, _MAX_PATH);
					szTemp[0] = '\0';
					if (szRegBuffer[i] == '\0')	// End of the line so break
						break;
				}
				else
					strcat(szTemp, c);
				++i;
			}
		}

		RegCloseKey(hRegKey);
	}
	else
	{
		m_pIncludeDirs[m_nIncludeDirs] = new char[_MAX_PATH+1];
		strcpy(m_pIncludeDirs[m_nIncludeDirs++], "Include\\");
	}

} // AutoIt_ScriptFile()


///////////////////////////////////////////////////////////////////////////////
// Destructor()
///////////////////////////////////////////////////////////////////////////////

AutoIt_ScriptFile::~AutoIt_ScriptFile()
{
	for (unsigned int i = 0; i < m_nIncludeDirs; ++i)
		delete[] m_pIncludeDirs[i];
	delete[] m_pIncludeDirs;

} // Destructor()


///////////////////////////////////////////////////////////////////////////////
// AddLine()
///////////////////////////////////////////////////////////////////////////////

void AutoIt_ScriptFile::AddLine(int nLineNum,  const char *szLine, int nIncludeID)
{
	LARRAY	*lpTemp;
	LARRAY	*lpLast;
	char	*szTemp;

	lpLast	= m_lpScriptLast;						// Make a note of the current last position (or NULL)

	// Do we need to start linked list?
	if ( m_lpScript == NULL )
	{
		m_lpScript				= new LARRAY;
		m_lpScriptLast			= m_lpScript;
	}
	else
	{
		// Only allocate a new line if the last line wasn't a waste (leading spaces already stripped)
		if (m_lpScriptLast->szLine[0] != ';' && m_lpScriptLast->szLine[0] != '\0')
		{
			lpTemp					= new LARRAY;
			m_lpScriptLast->lpNext	= lpTemp;			// Next
			m_lpScriptLast			= lpTemp;
		}
		else
		{
			delete [] m_lpScriptLast->szLine;	// Delete useless line
			--m_nScriptLines;					// Remove line (will re-add below)
		}
	}

	m_lpScriptLast->lpNext		= NULL;				// Next

	// Store our line
	szTemp = Util_StrCpyAlloc(szLine);
	m_lpScriptLast->szLine		= szTemp;
	m_lpScriptLast->nLineNum	= nLineNum;
	m_lpScriptLast->nIncludeID	= nIncludeID;

	// Increase the number of lines
	m_nScriptLines++;


} // AddLine()


///////////////////////////////////////////////////////////////////////////////
// AppendLastLine()
//
// Same as AddLine except it adds the text onto the LAST line added
///////////////////////////////////////////////////////////////////////////////

void AutoIt_ScriptFile::AppendLastLine(const char *szLine)
{
	char	*szTemp;
	size_t	CombinedLen;

	if (m_nScriptLines == 0)
		return;

	// How big are both lines added together?
	CombinedLen = strlen(m_lpScriptLast->szLine) + strlen(szLine);

	// Create a big enough space for the combined line and copy it there
	szTemp = new char[CombinedLen+1];
	strcpy(szTemp, m_lpScriptLast->szLine);
	strcat(szTemp, szLine);

	// The appending may have gone over the max line size, so just do a dirty hack and
	// enforce the line size by inserting a \0
	if (strlen(szTemp) > AUT_MAX_LINESIZE)
		szTemp[AUT_MAX_LINESIZE] = '\0';

	// Now free the existing line and replace with the new one
	delete [] m_lpScriptLast->szLine;
	m_lpScriptLast->szLine = szTemp;

} // AppendLastLine()


///////////////////////////////////////////////////////////////////////////////
// AddIncludeName()
///////////////////////////////////////////////////////////////////////////////

int AutoIt_ScriptFile::AddIncludeName(const char *szFileName)
{
	char	szFullPath[_MAX_PATH+1];
	char	*szTemp;

	if (m_nNumIncludes >= AUT_MAX_INCLUDE_IDS)
		return -1;

	GetFullPathName(szFileName, _MAX_PATH, szFullPath, &szTemp);

	// Does this file already exist?
	for (int i=0; i < m_nNumIncludes; ++i)
	{
		if (!stricmp(m_szIncludeIDs[i], szFullPath))
		{
			m_nIncludeCounts[i]++;				// Increase the count for this file
			return i;
		}
	}


	// New entry
	szTemp = Util_StrCpyAlloc(szFullPath);

	m_szIncludeIDs[m_nNumIncludes] = szTemp;
	m_nIncludeCounts[m_nNumIncludes] = 1;

	++m_nNumIncludes;

	return m_nNumIncludes-1;

} // AddIncludeName()


///////////////////////////////////////////////////////////////////////////////
// GetIncludeID()
///////////////////////////////////////////////////////////////////////////////

int AutoIt_ScriptFile::GetIncludeID(int nLineNum)
{
	int		i;
	LARRAY	*lpTemp = m_lpScript;

	// Do we have this many lines?
	if (nLineNum > m_nScriptLines || nLineNum <= 0)
		return -1;							// Nope

	for (i=0; i<nLineNum-1; i++)
		lpTemp = lpTemp->lpNext;

	return lpTemp->nIncludeID;

} // GetIncludeID()


///////////////////////////////////////////////////////////////////////////////
// GetIncludeName()
///////////////////////////////////////////////////////////////////////////////

const char * AutoIt_ScriptFile::GetIncludeName(int nIncludeID)
{
	if (nIncludeID >= AUT_MAX_INCLUDE_IDS || nIncludeID < 0)
		return NULL;
	else
		return m_szIncludeIDs[nIncludeID];

} // GetIncludeName()


///////////////////////////////////////////////////////////////////////////////
// GetIncludeFileName()
///////////////////////////////////////////////////////////////////////////////

const char * AutoIt_ScriptFile::GetIncludeFileName(int nIncludeID)
{
	if (nIncludeID >= AUT_MAX_INCLUDE_IDS || nIncludeID < 0)
		return NULL;
	else
	{
		char *szInclude = (char *)g_oScriptFile.GetIncludeName(nIncludeID);
		char		*szFilePart;
		GetFullPathName(szInclude, _MAX_PATH, szInclude, &szFilePart);
		return szFilePart;
	}

} // GetIncludeFileName()


///////////////////////////////////////////////////////////////////////////////
// UnloadScript()
///////////////////////////////////////////////////////////////////////////////

void AutoIt_ScriptFile::UnloadScript(void)
{
	LARRAY	*lpTemp, *lpTemp2;

	// Unloading the script is simply a matter of freeing all the memory
	// that we allocated in the linked lists and random access array

	delete [] m_szScriptLines;

	lpTemp = m_lpScript;
	while (lpTemp != NULL)
	{
		lpTemp2 = lpTemp->lpNext;
		delete [] lpTemp->szLine;				// Free the string
		delete lpTemp;							// Free the node
		lpTemp = lpTemp2;
	}

	// Ensure everything is zeroed in case we load another script
	m_lpScript		= NULL;						// Start of the linked list
	m_lpScriptLast	= NULL;						// Last node of the list
	m_nScriptLines	= 0;						// Number of lines in the list
	m_szScriptLines	= NULL;						// Random access array

	// Delete all our includes
	for (int i=0; i<m_nNumIncludes; ++i)
		delete [] m_szIncludeIDs[i];

	m_nNumIncludes = 0;

} // UnloadScript()


///////////////////////////////////////////////////////////////////////////////
// GetLine()
//
// NOTE: Line 1 is the first line (not zero)
//
///////////////////////////////////////////////////////////////////////////////

const char * AutoIt_ScriptFile::GetLine(int nLineNum)
{
	// Do we have this many lines?
	if (nLineNum > m_nScriptLines || nLineNum <= 0)
		return NULL;							// Nope

	return m_szScriptLines[nLineNum-1];			// 1 = array element 0

} // GetLine()


///////////////////////////////////////////////////////////////////////////////
// GetAutLineNumber()
//
// NOTE: Line 1 is the first line (not zero)
//
///////////////////////////////////////////////////////////////////////////////

int AutoIt_ScriptFile::GetAutLineNumber(int nLineNum)
{
	int		i;
	LARRAY	*lpTemp = m_lpScript;

	// Do we have this many lines?
	if (nLineNum > m_nScriptLines || nLineNum <= 0)
		return -1;							// Nope

	for (i=0; i<nLineNum-1; i++)
		lpTemp = lpTemp->lpNext;

	return lpTemp->nLineNum;

} // GetAutLineNumber()

///////////////////////////////////////////////////////////////////////////////
// IncludeParse()
//
// Checks a line of text to see if it is an #include directive, if so passes
// back the string changed to contain the filename of the file to include
///////////////////////////////////////////////////////////////////////////////

bool AutoIt_ScriptFile::IncludeParse(const char *szLine, char *szTemp)
{
	int		i,j;
	i = 0;
	bool bRSearch = false;
	char type;
	char	*szTemp2;

	// Skip whitespace
	while (szLine[i] == ' ' || szLine[i] == '\t')
		i++;

	// Is it < or "
	if (szLine[i] == '"')
	{
		type = '"';
		bRSearch = true;
	}
	else if (szLine[i] == '\'')
	{
		type = '\'';
		bRSearch = true;
	}
	else if (szLine[i] == '<')
	{
		type = '>';
		bRSearch = false;
	}
	else
		return false;

	// Copy until next " (should be the filename)
	i++;
	j = 0;
	while (szLine [i] != type && szLine[i] != '\0')
		szTemp[j++] = szLine[i++];
	szTemp[j] = '\0';							// Terminate

	szTemp2 = Util_StrCpyAlloc(szTemp);

	// This can be confusing, here's what's going on:
	// if bRSearch is true, we perform a reverse search.  This means we start in the local directory, then
	// move out to user specified directories.  If the file exists in the local directory, we free the memory
	// and return immediately, otherwise, try searching other directories.  If the file is found, break out of
	// of the search.  If it's not found, copy szTemp back to the file name and allow fall through.
	// The other search starts with the user-specified directories and lastly tries the local directory.
	// In all cases, this function returns true, if the file can't be found, it'll still be copied to the buffer and
	// fopen will fail inside Include() and catch it.
	if (bRSearch)
	{
		if (Util_DoesFileExist(szTemp))
		{
			delete[] szTemp2;
			return true;
		}

		for (int i = m_nIncludeDirs - 1; i >= 0; --i)
		{
			strcpy(szTemp, m_pIncludeDirs[i]);
			strcat(szTemp, szTemp2);
			if (Util_DoesFileExist(szTemp))
				break;
			else
				strcpy(szTemp, szTemp2);	// Reset back to the filename so Include()'s failure will contain a good message
		}

	}
	else
	{
		for (unsigned int i = 0; i < m_nIncludeDirs; ++i)
		{
			strcpy(szTemp, m_pIncludeDirs[i]);
			strcat(szTemp, szTemp2);
			if (Util_DoesFileExist(szTemp))
				break;
			else
				strcpy(szTemp, szTemp2);	// Just copy it into the buffer and let Include fail on it if it doesn't exist
		}

	}

	delete[] szTemp2;
	return true;

} // IncludeParse()


///////////////////////////////////////////////////////////////////////////////
// StripTrailing()
// Strips trailing spaces, tabs, \n and \r
///////////////////////////////////////////////////////////////////////////////

void AutoIt_ScriptFile::StripTrailing(char *szLine)
{
	int i;
	int	nLen;

	nLen = (int)strlen(szLine);					// Get length

	if (nLen == 0)
		return;									// Nothing to do

	// Strip trailing whitespace and newline
	i = nLen - 1;
	while ( i >= 0 && (szLine[i] == ' ' || szLine[i] == '\t' || szLine[i] == '\n' || szLine[i] == '\r') )
		i--;

	// Remove trailing
	szLine[i+1] = '\0';

} // StripTrailing()


///////////////////////////////////////////////////////////////////////////////
// StripLeading()
///////////////////////////////////////////////////////////////////////////////

void AutoIt_ScriptFile::StripLeading(char *szLine)
{
	int i, j;

	i = 0;
	while ( szLine[i] == ' ' || szLine[i] == '\t' )
		i++;

	if (szLine[i] == '\0')
		return;									// Nothing to do

	j = 0;
	while (szLine[i] != '\0')
		szLine[j++] = szLine[i++];

	szLine[j] = '\0';							// Terminate

} // StripLeading()


///////////////////////////////////////////////////////////////////////////////
// PrepareScript()
//
// Takes the populated linked-list script and re-stores it as a random access
// array for speed.  The orignal memory for the scipt line is used, just pointed
// to from a random access array.
//
// In addition, when compiled for AutoIt blank and single comment lines are also
// removed and leading whitespace is stripped
//
///////////////////////////////////////////////////////////////////////////////

void AutoIt_ScriptFile::PrepareScript(void)
{
	LARRAY	*lpTemp;

	if (m_nScriptLines == 0)
		return;									// Nothing to do

	lpTemp = m_lpScript;

	// Create an array of char * large enough for all the stored lines
	m_szScriptLines = new char*[m_nScriptLines];

	// Now store them all
	for (int i=0; i<m_nScriptLines; ++i)
	{
		m_szScriptLines[i] = lpTemp->szLine;
		lpTemp = lpTemp->lpNext;			// Next
	}

} // PrepareScript()







#ifndef AUTOITSC

///////////////////////////////////////////////////////////////////////////////
// LoadScript()
//
// NATIVE VERSION
//
///////////////////////////////////////////////////////////////////////////////

bool AutoIt_ScriptFile::LoadScript(char *szFile)
{
	OPENFILENAME	ofn;

//	strcpy(szFile, "bin\\test.au3");

	// Was a script file specified? If not, bring up the fileopen dialog
    if (szFile[0] == '\0')
    {
		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		ofn.lStructSize		= sizeof(OPENFILENAME);
		ofn.lpstrTitle		= "Run Script:\0";
		ofn.hwndOwner		= NULL;
		ofn.lpstrFile		= szFile;
		ofn.nMaxFile		= _MAX_PATH;
		ofn.lpstrFilter		= "AutoIt files (*.au3)\0*.au3\0All files (*.*)\0*.*\0";
		ofn.nFilterIndex	= 1;
		ofn.Flags			= OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
		ofn.lpstrDefExt		= "au3";

		// Check that the user successfully selected a scriptfile
		if (!GetOpenFileName(&ofn))
			return false;
	}

	// Get the full LFN pathname (it is passed back for other uses)
	Util_GetFullPathName(szFile, szFile);
	Util_GetLongFileName(szFile, szFile);

	// Read in the script and any include files
    return Include(szFile, AddIncludeName(szFile));

} // LoadScript()


///////////////////////////////////////////////////////////////////////////////
// Include()
///////////////////////////////////////////////////////////////////////////////

bool AutoIt_ScriptFile::Include(const char *szFileName, int nIncludeID)
{
	char	szDrive[_MAX_DRIVE+1];
	char	szDir[_MAX_DIR+1];
	char	szFname[_MAX_FNAME+1];
	char	szExt[_MAX_EXT+1];
	char	*szFilePart;
	char	szOldWorkingDir[_MAX_PATH+1];
	char	szWorkingDir[_MAX_PATH+1];
	char	szFullFileName[_MAX_PATH+1];		// Full path of current file to include

	char	szBuffer[AUT_MAX_LINESIZE+1];
	int		nLineNum = 0;						// Line# in .aut file, starts at 1, increment immediately after reading new line
	FILE	*fptr;
	bool	bErr = true;
	bool	bContinue = true;
	int		nDirectiveRes;
	bool	bNextLineAppend = false;			// If true the next line read should be appended
	bool	bContinuationFound;
	int		nLen;


	// Open a handle to the script file
	fptr = fopen(szFileName, "r");				// ASCII read
	if ( fptr == NULL  )						// File error
	{
		strcpy(szBuffer, "Error reading the file:\n\n");
		strcat(szBuffer, szFileName);

		Util_FatalError("AutoIt", szBuffer, NULL);
		return false;
	}

	// Get the current working directory
	GetCurrentDirectory(_MAX_PATH, szOldWorkingDir);

	// Get the FULL PATH of the filename we are working with (for future use with IsInlcudedOnceOnly)
	GetFullPathName(szFileName, _MAX_PATH, szFullFileName, &szFilePart);

	// Set the working directory based on the filename we just opened - NOTE this will BREAK any further calls
	// to GetFullPathName for the currently loaded file which uses the working directory to resolve!
	_splitpath( szFullFileName, szDrive, szDir, szFname, szExt );
	strcpy(szWorkingDir, szDrive);
	strcat(szWorkingDir, szDir);
	SetCurrentDirectory(szWorkingDir);


	// Read in lines of text until EOF is reached or error occurs
	while ( bErr == true && bContinue == true && fgets(szBuffer, AUT_MAX_LINESIZE, fptr) )
	{
		++nLineNum;								// Increase line count

		// Enforce our maximum line length (must be smaller than szBuffer!) and then
		// strip all trailing whitespace
		szBuffer[AUT_MAX_LINESIZE] = '\0';
		StripLeading(szBuffer);					// Strip leading space
		StripTrailing(szBuffer);				// Strip trailing spaces and newline chars

		// Check if the last character is requesting a continuation
		nLen = (int)strlen(szBuffer);
		if (nLen && szBuffer[nLen-1] == '_')
		{
			szBuffer[nLen-1] = '\0';		// Erase the continuation char
			bContinuationFound = true;
		}
		else
			bContinuationFound = false;


		if (bNextLineAppend == true)
		{
			// Continued line
			bNextLineAppend = false;
			AppendLastLine(szBuffer);
		}
		else
		{
			// New line
			nDirectiveRes = CheckDirective(szBuffer, szFullFileName, nLineNum, fptr, nIncludeID);

			switch (nDirectiveRes)
			{
				case AUT_DIRECTIVE_NODIRECTIVE:
				case AUT_DIRECTIVE_PASSTHROUGH:
					AddLine(nLineNum, szBuffer, nIncludeID );
					break;

				case AUT_DIRECTIVE_ERROR:
					bErr = false;
					break;

				case AUT_DIRECTIVE_INCLUDEDECLINED:
					bContinue = false;				// Already included this file so stop reading anymore
					break;
			}
		}

		// Setup next loop continuation if required
		if (bContinuationFound == true)
			bNextLineAppend = true;
		else
			bNextLineAppend = false;

	} // End While


	// Close our script file
	fclose(fptr);

	SetCurrentDirectory(szOldWorkingDir);		// Restore the old working directory

	return bErr;

} // Include()


///////////////////////////////////////////////////////////////////////////////
// CheckDirective()
///////////////////////////////////////////////////////////////////////////////

int AutoIt_ScriptFile::CheckDirective(char *szLine, const char *szFullFileName, int &nLineNum, FILE *fIn, int nIncludeID)
{
	char	szTemp[AUT_MAX_LINESIZE+1];
	int		nCommentsGroup = 0;

	// Note in AutoIt the line is pre-stripped (leading and trailing)

	// Does this line contain a directive
	if (szLine[0] != '#')
		return AUT_DIRECTIVE_NODIRECTIVE;

	// Check for NoTrayIcon
	if (strnicmp(szLine, "#notrayicon", 11) == 0)
	{
		g_bTrayIconInitial = false;
		return AUT_DIRECTIVE_STRIP;
	}


	// Must check #include-once first as it contains "include" and would be incorrectly detected as #include below
	if (strnicmp(szLine, "#include-once", 13) == 0)
	{
		// Does this include already exist in our store?
		for (int i=0; i < m_nNumIncludes; ++i)
		{
			if (!stricmp(m_szIncludeIDs[i], szFullFileName))
			{
				// It exists, is the count > 1 (1=first include i.e. This include)
				if (m_nIncludeCounts[i] > 1)
					return AUT_DIRECTIVE_INCLUDEDECLINED;	// Previously included!
				else
					return AUT_DIRECTIVE_STRIP;			// Remove the #include-once text
			}
		}

		return AUT_DIRECTIVE_STRIP;
	}

	if (strnicmp(szLine, "#include", 8) == 0)
	{
		// check for #include if filename is OK
		if (IncludeParse(&szLine[8], szTemp) == true )
		{
			if ( Include(szTemp, AddIncludeName(szTemp)) )	// Get the include file
				return AUT_DIRECTIVE_STRIP;				// File successfully included so ignore/remove the #include text
			else
				return AUT_DIRECTIVE_ERROR;
		}
		else
			return AUT_DIRECTIVE_ERROR;
	}

	if (strnicmp(szLine, "#comments-start", 15) == 0 || strnicmp(szLine, "#cs", 3) == 0)
	{
		// In Aut2Exe we must include the commented text without modification
		for (;;)
		{
			if (fgets(szLine, AUT_MAX_LINESIZE, fIn) == false)
				return AUT_DIRECTIVE_STRIP;			// end of File so stop reading or exit the outermost nesting

			++nLineNum;								// Increase line count

			StripTrailing(szLine);					// Strip trailing spaces and newline
			StripLeading(szLine);

			if (strnicmp(szLine, "#comments-start", 15) == 0 || strnicmp(szLine, "#cs", 3) == 0)		// new CommentsGroup
				++nCommentsGroup;							// increments nCommentsGroup nesting
			else if (strnicmp(szLine, "#comments-end", 13) == 0 || strnicmp(szLine, "#ce", 3) == 0)		// end CommentsGroup
			{
				if (--nCommentsGroup < 0)					// decrements nCommentsGroup nesting
					return AUT_DIRECTIVE_STRIP;
			}
		}
	}

	return AUT_DIRECTIVE_STRIP;				// Remove unknown directives

} // CheckDirective()



#endif // AUTOITSC


