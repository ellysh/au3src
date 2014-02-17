
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
// script_file.cpp
//
// Contains file handling routines.  Part of script.cpp
//
///////////////////////////////////////////////////////////////////////////////


// Includes
#include "StdAfx.h"								// Pre-compiled headers

#ifndef _MSC_VER								// Includes for non-MS compilers
	#include <stdio.h>
	#include <shlobj.h>
#endif

#include "AutoIt.h"								// Autoit values, macros and config options

#include "globaldata.h"
#include "script.h"
#include "resources\resource.h"
#include "utility.h"


///////////////////////////////////////////////////////////////////////////////
// IniRead()
// $var = IniRead(filename, sectionname, keyname, default)
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_IniRead(VectorVariant &vParams, Variant &vResult)
{
	char	szFileTemp[_MAX_PATH+1];
	char	szBuffer[65535];					// Max ini file size is 65535 under 95

	// Get the fullpathname (ini functions need a full path)
	Util_GetFullPathName(vParams[0].szValue(), szFileTemp);

	GetPrivateProfileString(vParams[1].szValue(), vParams[2].szValue(),
							vParams[3].szValue(), szBuffer, 65535, szFileTemp);

	vResult = szBuffer;							// Return the string

	return AUT_OK;

} // IniRead()


///////////////////////////////////////////////////////////////////////////////
// IniWrite()
// IniWrite(filename, sectionname, keyname, value)
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_IniWrite(VectorVariant &vParams, Variant &vResult)
{
	char	szFileTemp[_MAX_PATH+1];

	// Get the fullpathname (ini functions need a full path)
	Util_GetFullPathName(vParams[0].szValue(), szFileTemp);


	if (WritePrivateProfileString(vParams[1].szValue(), vParams[2].szValue(), vParams[3].szValue(), szFileTemp))
		WritePrivateProfileString(NULL, NULL, NULL, szFileTemp);	// Flush
	else
		vResult = 0;							// Error, default is 1

	return AUT_OK;

} // IniWrite()


///////////////////////////////////////////////////////////////////////////////
// IniDelete()
// IniDelete(filename, sectionname [,keyname] )
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_IniDelete(VectorVariant &vParams, Variant &vResult)
{
	char	szFileTemp[_MAX_PATH+1];

	// Get the fullpathname (ini functions need a full path)
	Util_GetFullPathName(vParams[0].szValue(), szFileTemp);

	// If there are only 2 parameters then assume we want to delete the entire section
	if (vParams.size() == 2)
	{
		// Delete entire section
		if (WritePrivateProfileString(vParams[1].szValue(), NULL, NULL, szFileTemp))
			WritePrivateProfileString(NULL, NULL, NULL, szFileTemp);	// Flush
		else
			vResult = 0;
	}
	else
	{
		// Just delete a key as usual
		if (WritePrivateProfileString(vParams[1].szValue(), vParams[2].szValue(), NULL, szFileTemp))
			WritePrivateProfileString(NULL, NULL, NULL, szFileTemp);	// Flush
		else
			vResult = 0;
	}

	return AUT_OK;

} // IniDelete()


//////////////////////////////////////////////////////////////////////////
// IniReadSection($sIni, $sSection)
//////////////////////////////////////////////////////////////////////////
AUT_RESULT AutoIt_Script::F_IniReadSection(VectorVariant &vParams, Variant &vResult)
{
	// Store start/end positions to speed up processing
	struct KeyValue
	{
		int iKeyStart;
		int iKeyEnd;
		int iValueStart;
		int iValueEnd;
		KeyValue *pNext;
		KeyValue() : iKeyStart(0), iKeyEnd(0), iValueStart(0), iValueEnd(0), pNext(NULL) { }
	};

	char	szFileTemp[_MAX_PATH+1];

	// Get the fullpathname (ini functions need a full path)
	Util_GetFullPathName(vParams[0].szValue(), szFileTemp);

	int		i;
	char	szBuffer[32767];	// Temporary buffer, max size of INI _section_ in 95 (See MSDN for GetPrivateProfileSection)
	int		iRes = GetPrivateProfileSection(vParams[1].szValue(), szBuffer, 32767, szFileTemp);

	if (!iRes)
		SetFuncErrorCode(1);
	else
	{
		KeyValue *pStart = new KeyValue;	// Store the key/value start/end positions
		KeyValue *pCurrent = pStart;
		pCurrent->iKeyStart = 0;

		int count = 0;
		// Loop through and count the number of key/value pairs.
		// Also build a list of key/value start/end positions.
		// This avoids looping through the entire buffer twice.
		for (i = 0; i < iRes; ++i)
		{
			if (szBuffer[i] == '=' && pCurrent->iKeyEnd == 0)	// Found key/value split
			{
				pCurrent->iKeyEnd = i;
				pCurrent->iValueStart = i+1;
			}

			if (szBuffer[i] == '\0')
			{
				if (pCurrent->iKeyEnd == 0)
				{
					szBuffer[i] = '\r';	// Replace with something so it becomes a C String, thus compatible with AString
					if (szBuffer[i+1] == '\0')
					{
						delete pCurrent;
						break;	// This just happens to leave the string NULL terminated so its now a \r delimited C-style string
					}
					else
						pCurrent->iKeyStart = i+1;
				}
				else
				{
					pCurrent->iValueEnd = i;
					++count;
					szBuffer[i] = '\r';	// Replace with something so it becomes a C String, thus compatible with AString
					if (szBuffer[i+1] == '\0')
						break;	// This just happens to leave the string NULL terminated so its now a \r delimited C-style string
					else
					{
						pCurrent->pNext = new KeyValue;
						pCurrent = pCurrent->pNext;
						pCurrent->iKeyStart = i+1;
					}
				}
			}
		}

		if (!count)
			SetFuncErrorCode(2);
		else
		{
			// Dim the array
			vResult.ArraySubscriptClear();						// Reset the subscript
			vResult.ArraySubscriptSetNext(count + 1);			// Number of elements
			vResult.ArraySubscriptSetNext(2);					// Number of elements ([0]=name. [1]=pid)
			vResult.ArrayDim();									// Dimension array

			Util_Variant2DArrayAssign<int>(&vResult, 0, 0, count);	// Set the size

			pCurrent = pStart;	// Reset our pointer

			AString sKey, sValue;
			for (i = 0; i < count; ++i)
			{
				// Key
				sKey.erase();
				sKey.assign(szBuffer, pCurrent->iKeyStart, pCurrent->iKeyEnd);
				Util_Variant2DArrayAssign(&vResult, i+1, 0, sKey.c_str());

				// Value
				sValue.erase();
				sValue.assign(szBuffer, pCurrent->iValueStart, pCurrent->iValueEnd);
				Util_Variant2DArrayAssign(&vResult, i+1, 1, sValue.c_str());

				// Set up for next loop and delete the the struct
				KeyValue *pTemp = pCurrent;
				pCurrent = pCurrent->pNext;
				delete pTemp;
			}
		}
	}
	return AUT_OK;

}	// IniReadSection


//////////////////////////////////////////////////////////////////////////
// IniReadSectionNames($sFile)
//////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_IniReadSectionNames(VectorVariant &vParams, Variant &vResult)
{
	// Store start/end positions to speed up processing
	struct Section
	{
		int iStart;
		int iEnd;
		Section *pNext;
		Section() : iStart(0), iEnd(0), pNext(NULL) { }
	};

	char	szFileTemp[_MAX_PATH+1];

	// Get the fullpathname (ini functions need a full path)
	Util_GetFullPathName(vParams[0].szValue(), szFileTemp);

	int		i;
	char	szBuffer[65535];	// Temporary buffer, max size of INI in 95 (According to F_IniRead above)
	int		iRes = GetPrivateProfileSectionNames(szBuffer, 65535, szFileTemp);

	if (!iRes)
		SetFuncErrorCode(1);
	else
	{
		Section *pStart = new Section;	// Store the section start/end positions
		Section *pCurrent = pStart;
		pCurrent->iStart = 0;

		int count = 0;
		// Loop through and count the number of sections.
		// Also build a list of section start/end positions.
		// This avoids looping through the entire buffer twice.
		for (i = 0; i < iRes; ++i)
		{
			if (szBuffer[i] == '\0')
			{
				pCurrent->iEnd = i;
				++count;
				szBuffer[i] = '\r';	// Replace with something so it becomes a C String, thus compatible with AString
				if (szBuffer[i+1] == '\0')
					break;	// This just happens to leave the string NULL terminated so its now a \r delimited C-style string
				else
				{
					pCurrent->pNext = new Section;
					pCurrent = pCurrent->pNext;
					pCurrent->iStart = i+1;
				}
			}
		}

		// Dim the array
		Util_VariantArrayDim(&vResult, count+1);		// +1 to hold size element
		Util_VariantArrayAssign<int>(&vResult, 0, count);

		pCurrent = pStart;	// Reset our pointer

		AString sSection;
		for (i = 0; i < count; ++i)
		{
			sSection.erase();
			sSection.assign(szBuffer, pCurrent->iStart, pCurrent->iEnd);
			Util_VariantArrayAssign(&vResult, i+1, sSection.c_str());

			// Set up for next loop and delete the the struct
			Section *pTemp = pCurrent;
			pCurrent = pCurrent->pNext;
			delete pTemp;
		}
	}

	return AUT_OK;

}	// IniReadSectionNames


///////////////////////////////////////////////////////////////////////////////
// FileOpen()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_FileOpen(VectorVariant &vParams, Variant &vResult)
{
	// $filehandle = FileOpen(<filename>, <mode>)
	// mode 0=read, mode1=append(write), 2=erase(write)
	// Returns -1 if error

	FILE	*fptr;
	int		nFreeHandle;
	int		nMode;

	// Have we room for another file handle?
	if (m_nNumFileHandles == AUT_MAXOPENFILES)
	{
		FatalError(IDS_AUT_E_TOOMANYFILES);
		return AUT_ERR;
	}

	// Find a free handle
	for (nFreeHandle = 0; nFreeHandle < AUT_MAXOPENFILES; ++nFreeHandle)
	{
		if (m_FileHandleDetails[nFreeHandle] == NULL)
			break;
	}


	nMode = vParams[1].nValue();

	// Open a file handle in the required way
	switch ( nMode )
	{
		case 0:									// Read
			fptr = fopen(vParams[0].szValue(), "rb");
			break;

		case 1:									// Write (append)
			fptr = fopen(vParams[0].szValue(), "a+b");
			if (fptr)
				fseek(fptr, 0, SEEK_END);
			break;

		case 2:									// Write (erase contents)
			fptr = fopen(vParams[0].szValue(), "w+b");
			break;

		default:
			vResult = -1;						// Unknown mode
			return AUT_OK;
	}


	// Check that we have a valid file handle
	if (fptr == NULL)
	{
		vResult = -1;							// -1 is error
		return AUT_OK;
	}

	// Store the file handle in our table
	m_FileHandleDetails[nFreeHandle] = new FileHandleDetails;	// Create new entry
	m_FileHandleDetails[nFreeHandle]->nType = AUT_FILEOPEN;		// File open type
	m_FileHandleDetails[nFreeHandle]->fptr = fptr;				// Store handle
	m_FileHandleDetails[nFreeHandle]->nMode = nMode;			// Store mode

	++m_nNumFileHandles;
	vResult = nFreeHandle;						// Return array position as the file handle (0-MAXOPENFILES)

	return AUT_OK;

} // FileOpen()


///////////////////////////////////////////////////////////////////////////////
// FileClose()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_FileClose(VectorVariant &vParams, Variant &vResult)
{
	// FileClose(<filehandle>)
	// mode 0=read, mode1=write

	int nHandle = vParams[0].nValue();

	if (nHandle < 0)
		return AUT_OK;

	if (nHandle >= AUT_MAXOPENFILES || m_FileHandleDetails[nHandle] == NULL)
	{
		FatalError(IDS_AUT_E_FILEHANDLEINVALID);
		return AUT_ERR;
	}
	else
	{
		if (m_FileHandleDetails[nHandle]->nType == AUT_FILEOPEN)
		{
			fclose(m_FileHandleDetails[nHandle]->fptr);	// Close the file
		}
		else
		{
			FindClose(m_FileHandleDetails[nHandle]->hFind);
			delete [] m_FileHandleDetails[nHandle]->szFind;
		}

		delete m_FileHandleDetails[nHandle];		// Release our memory
		m_FileHandleDetails[nHandle] = NULL;		// Reset entry
		--m_nNumFileHandles;						// Reduce file count
	}

	return AUT_OK;

} // FileClose()


///////////////////////////////////////////////////////////////////////////////
// FileReadLine()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_FileReadLine(VectorVariant &vParams, Variant &vResult)
{
	// FileReadLine(<filehandle | filename>, [line] )
	// Returns line in vResult
	// @error:
	// ok = 0, 1=file not open for reading, -1=eof
	//

	int		i, nLine;
	size_t	Len;
	FILE	*fptr;
	char	szBuffer[65535];					// Read buffer
	int		nHandle;
	bool	bError = false;

	// Default return value is ""
	vResult = "";

	// Are we being passed a filename or filehandle?
	if (vParams[0].isString() == true)
	{
		// Filename being used - we must open and close this file during this function
		fptr = fopen(vParams[0].szValue(), "rb");	// Open in read mode (binary)
		if (fptr == NULL)
		{
			SetFuncErrorCode(1);					// Not open for reading
			return AUT_OK;
		}
	}
	else
	{
		// Existing file handle used
		nHandle = vParams[0].nValue();

		if (nHandle < 0)
		{
			SetFuncErrorCode(1);
			return AUT_OK;
		}

		// Does this file handle exist?
		if (nHandle >= AUT_MAXOPENFILES || m_FileHandleDetails[nHandle] == NULL)
		{
			FatalError(IDS_AUT_E_FILEHANDLEINVALID);
			return AUT_ERR;
		}

		// Is it a file open handle?
		if (m_FileHandleDetails[nHandle]->nType != AUT_FILEOPEN)
		{
			FatalError(IDS_AUT_E_FILEHANDLEINVALID);
			return AUT_ERR;
		}


		fptr = m_FileHandleDetails[nHandle]->fptr;	// Get the file handle

		// Is the file open for reading?
		if (m_FileHandleDetails[nHandle]->nMode != 0)
		{
			SetFuncErrorCode(1);					// Not open for reading
			return AUT_OK;
		}
	}


	// Read next line?  Or read a specific line?
	if (vParams.size() == 2)
	{
		nLine = vParams[1].nValue();		// Line requested to read

		// Specific line...reeeeeewind
		fseek(fptr, 0, SEEK_SET);

		for (i=0; i<nLine; ++i)
		{
			if ( Util_fgetsb(szBuffer, 65535, fptr) == NULL )
			{
				SetFuncErrorCode(-1);				// EOF reached
				bError = true;
				//return AUT_OK;
			}
		} // end for
	}
	else
	{
		// Read next line
		if (Util_fgetsb(szBuffer, 65535, fptr) == NULL)
		{
			SetFuncErrorCode(-1);				// EOF reached
			bError = true;
			//return AUT_OK;
		}
	}

	if (bError == false)
	{
		// No errors
		// Strip newline if present
		Len = strlen(szBuffer);
		if (Len && szBuffer[Len-1] == '\n')
			szBuffer[Len-1] = '\0';

		vResult = szBuffer;							// Return the line
	}


	// Errors or not, this is where we close the file if we opened it above
	if (vParams[0].isString())
		fclose(fptr);							// Close our file

	return AUT_OK;

} // FileReadLine()


///////////////////////////////////////////////////////////////////////////////
// FileRead()
// FileRead(<filehandle | filename>, chars )
// Returns line in vResult
// @error:
// ok = 0, 1=file not open for reading, -1=eof
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_FileRead(VectorVariant &vParams, Variant &vResult)
{
	size_t	Len;
	FILE	*fptr;
	char	*szBuffer;						// Read buffer
	int		nHandle;

	// Default return value is ""
	vResult = "";

	// Are we being passed a filename or filehandle?
	if (vParams[0].isString() == true)
	{
		// Filename being used - we must open and close this file during this function
		fptr = fopen(vParams[0].szValue(), "rb");	// Open in read mode (binary)
		if (fptr == NULL)
		{
			SetFuncErrorCode(1);					// Not open for reading
			return AUT_OK;
		}
	}
	else
	{
		// Existing file handle used
		nHandle = vParams[0].nValue();

		if (nHandle < 0)
		{
			SetFuncErrorCode(1);
			return AUT_OK;
		}

		// Does this file handle exist?
		if (nHandle >= AUT_MAXOPENFILES || m_FileHandleDetails[nHandle] == NULL)
		{
			FatalError(IDS_AUT_E_FILEHANDLEINVALID);
			return AUT_ERR;
		}

		// Is it a file open handle?
		if (m_FileHandleDetails[nHandle]->nType != AUT_FILEOPEN)
		{
			FatalError(IDS_AUT_E_FILEHANDLEINVALID);
			return AUT_ERR;
		}


		fptr = m_FileHandleDetails[nHandle]->fptr;	// Get the file handle

		// Is the file open for reading?
		if (m_FileHandleDetails[nHandle]->nMode != 0)
		{
			SetFuncErrorCode(1);					// Not open for reading
			return AUT_OK;
		}
	}


	// Create a buffer big enough for number of chars + \0
	szBuffer = new char[vParams[1].nValue() + 1];

	Len = fread(szBuffer, 1, vParams[1].nValue(), fptr);
	if (Len == 0)
		SetFuncErrorCode(-1);					// EOF, or error...
	else
	{
		szBuffer[Len] = '\0';					// Terminate
		vResult = szBuffer;
	}

	delete [] szBuffer;							// Free buffer

	// Errors or not, this is where we close the file if we opened it above
	if (vParams[0].isString())
		fclose(fptr);							// Close our file

	return AUT_OK;

} // FileRead()


///////////////////////////////////////////////////////////////////////////////
// F_FileWriteLine()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_FileWriteLine(VectorVariant &vParams, Variant &vResult)
{
	return FileWriteLine(vParams, vResult, true);

} // FileWriteLine()


///////////////////////////////////////////////////////////////////////////////
// FileWrite()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_FileWrite(VectorVariant &vParams, Variant &vResult)
{
	return FileWriteLine(vParams, vResult, false);

} // FileWrite()


///////////////////////////////////////////////////////////////////////////////
// FileWriteLine()
//
// If bWriteLine = true then the \r\n terminator checks will be active (
// FileWriteLine) otherwise the line won't be changed (FileWrite)
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::FileWriteLine(VectorVariant &vParams, Variant &vResult, bool bWriteLine)
{
	// FileWriteLine(<filehandle>, <line>)
	// Returns error in vResult
	// ok = 1 (default), 0=file not open for writing
	//

	FILE	*fptr;
	int		nHandle;
	int		nLen;

	// Are we being passed a filename or filehandle?
	if (vParams[0].isString() == true)
	{
		// Filename being used - we must open and close this file during this function
		fptr = fopen(vParams[0].szValue(), "a+b");	// Open in append mode
		if (fptr == NULL)
		{
			vResult = 0;						// Not open for writing
			return AUT_OK;
		}
	}
	else
	{
		// Filehandle used
		nHandle = vParams[0].nValue();

		// Does this file handle exist?
		if (nHandle >= AUT_MAXOPENFILES || nHandle < 0 || m_FileHandleDetails[nHandle] == NULL)
		{
			FatalError(IDS_AUT_E_FILEHANDLEINVALID);
			return AUT_ERR;
		}

		// Is it a file open handle?
		if (m_FileHandleDetails[nHandle]->nType != AUT_FILEOPEN)
		{
			FatalError(IDS_AUT_E_FILEHANDLEINVALID);
			return AUT_ERR;
		}

		fptr = m_FileHandleDetails[nHandle]->fptr;	// Get the file handle

		// Is the file open for writing?
		if (m_FileHandleDetails[nHandle]->nMode != 1 && m_FileHandleDetails[nHandle]->nMode != 2)
		{
			vResult = 0;							// Not open for writing
			return AUT_OK;
		}
	}

	// Append the line
	fputs(vParams[1].szValue(), fptr);

	if (bWriteLine)
	{
		// If the last character sent was NOT a CR or LF then add a standard DOS CRLF
		nLen = (int)strlen(vParams[1].szValue());
		if (nLen)
		{
			if (vParams[1].szValue()[nLen-1] != '\r' && vParams[1].szValue()[nLen-1] != '\n')
				fputs("\r\n", fptr);
		}
		else
		{
			// Blank, add DOS CRLF
			fputs("\r\n", fptr);
		}
	}

	if ( ferror(fptr) )
		vResult = 0;							// Any errors during writing?

	// Close the file if we just opened it
	if (vParams[0].isString() == true)
		fclose(fptr);

	return AUT_OK;

} // FileWriteLine()


/*
///////////////////////////////////////////////////////////////////////////////
// FileGetTime()
//
// FileGetTime("file" [,option])
//
// Returns FileTime as 6 element array 0=yyyy,1=m,2=d,3=h,4=m,5=s]
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_FileGetTime(VectorVariant &vParams, Variant &vResult)
{
	SYSTEMTIME		st;
	HANDLE			fileIn;
	FILETIME		ftCreationTime,ftLastAccessTime,ftLastWriteTime;
	int				n=0;
	char			szTime[14+1];

//	BY_HANDLE_FILE_INFORMATION		wfad;

	if ( (fileIn = CreateFile(vParams[0].szValue(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE )
	{
		if ( GetFileTime(fileIn,&ftCreationTime,&ftLastAccessTime,&ftLastWriteTime) )
		{
			if (  vParams.size() > 1 )
				n=vParams[1].nValue();
			switch ( n )
			{
				case 1:
					FileTimeToLocalFileTime(&ftCreationTime,&ftCreationTime);
					FileTimeToSystemTime(&ftCreationTime,&st);
					break;

				case 2:
					FileTimeToLocalFileTime(&ftLastAccessTime,&ftLastAccessTime);
					FileTimeToSystemTime(&ftLastAccessTime,&st);
					break;

				default:
					FileTimeToLocalFileTime(&ftLastWriteTime,&ftLastWriteTime);
					FileTimeToSystemTime(&ftLastWriteTime,&st);
			}

			if ( ( vParams.size() > 2)  && (vParams[2].nValue()==1) )
			{
				// return YYYYMMDDHHMMSS to ease comparison
				sprintf(szTime,"%4d%02d%02d%02d%02d%02d",
					st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond);

				vResult = szTime;
			}
			else
			{

				Variant	*pvTemp;

				// Setup vResult as an Array to hold the 6 values we want to return - save the values as strings
				// so that we can specify the number of digits (01 instead of 1, etc)
				Util_VariantArrayDim(&vResult, 6);

				pvTemp = Util_VariantArrayGetRef(&vResult, 0);	//	First element
				sprintf(szTime, "%4d", st.wYear);
				*pvTemp = szTime;								// Year YYYY


				pvTemp = Util_VariantArrayGetRef(&vResult, 1);
				sprintf(szTime, "%02d", st.wMonth);
				*pvTemp = szTime;								// Month MM

				pvTemp = Util_VariantArrayGetRef(&vResult, 2);
				sprintf(szTime, "%02d", st.wDay);
				*pvTemp = szTime;								// Day DD

				pvTemp = Util_VariantArrayGetRef(&vResult, 3);
				sprintf(szTime, "%02d", st.wHour);
				*pvTemp = szTime;								// Hour HH

				pvTemp = Util_VariantArrayGetRef(&vResult, 4);
				sprintf(szTime, "%02d", st.wMinute);
				*pvTemp = szTime;								// Min MM

				pvTemp = Util_VariantArrayGetRef(&vResult, 5);
				sprintf(szTime, "%02d", st.wSecond);
				*pvTemp = szTime;								// Sec SS
			}
		}
		else
			SetFuncErrorCode(1);				// Default is 0

		// Close the file
		CloseHandle(fileIn);
	}
	else
		SetFuncErrorCode(1);					// Default is 0

	return AUT_OK;

} // FileGetTime()
*/

///////////////////////////////////////////////////////////////////////////////
// FileGetTime()
//
// FileGetTime("file" [,option [,format]])
//
// Returns FileTime as 6 element array 0=yyyy,1=m,2=d,3=h,4=m,5=s
// or as string in form "yyyymmddhhmmss"
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_FileGetTime(VectorVariant &vParams, Variant &vResult)
{
	SYSTEMTIME		st;
	HANDLE			fileIn;
	FILETIME		ftCreationTime,ftLastAccessTime,ftLastWriteTime;
	int				n=0;
 	char			szTime[14+1];
	WIN32_FIND_DATA	findData;

	fileIn = FindFirstFile(vParams[0].szValue(),&findData);
	if (fileIn != INVALID_HANDLE_VALUE)
	{
		ftCreationTime		= findData.ftCreationTime;
		ftLastAccessTime	= findData.ftLastAccessTime;
		ftLastWriteTime		= findData.ftLastWriteTime;

		FindClose(fileIn);
	}
	else
	{
		SetFuncErrorCode(1);					// Default is 0
		return AUT_OK;
	}

	if (  vParams.size() > 1 )
		n=vParams[1].nValue();
	switch ( n )
	{
		case 1:
			FileTimeToLocalFileTime(&ftCreationTime,&ftCreationTime);
			FileTimeToSystemTime(&ftCreationTime,&st);
			break;

		case 2:
			FileTimeToLocalFileTime(&ftLastAccessTime,&ftLastAccessTime);
			FileTimeToSystemTime(&ftLastAccessTime,&st);
			break;

		default:
			FileTimeToLocalFileTime(&ftLastWriteTime,&ftLastWriteTime);
			FileTimeToSystemTime(&ftLastWriteTime,&st);
	}

	if ( ( vParams.size() > 2)  && (vParams[2].nValue()==1) )
	{
		// return YYYYMMDDHHMMSS to ease comparison
		sprintf(szTime,"%4d%02d%02d%02d%02d%02d",
			st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond);
			vResult = szTime;
	}
	else
	{
		Variant	*pvTemp;

		// Setup vResult as an Array to hold the 6 values we want to return - save the values as strings
		// so that we can specify the number of digits (01 instead of 1, etc)
		Util_VariantArrayDim(&vResult, 6);

		pvTemp = Util_VariantArrayGetRef(&vResult, 0);	//	First element
		sprintf(szTime, "%4d", st.wYear);
		*pvTemp = szTime;								// Year YYYY

		pvTemp = Util_VariantArrayGetRef(&vResult, 1);
		sprintf(szTime, "%02d", st.wMonth);
		*pvTemp = szTime;								// Month MM

		pvTemp = Util_VariantArrayGetRef(&vResult, 2);
		sprintf(szTime, "%02d", st.wDay);
		*pvTemp = szTime;								// Day DD

		pvTemp = Util_VariantArrayGetRef(&vResult, 3);
		sprintf(szTime, "%02d", st.wHour);
		*pvTemp = szTime;								// Hour HH

		pvTemp = Util_VariantArrayGetRef(&vResult, 4);
		sprintf(szTime, "%02d", st.wMinute);
		*pvTemp = szTime;								// Min MM

		pvTemp = Util_VariantArrayGetRef(&vResult, 5);
		sprintf(szTime, "%02d", st.wSecond);
		*pvTemp = szTime;								// Sec SS
	}

	return AUT_OK;

} // FileGetTime()


///////////////////////////////////////////////////////////////////////////////
// FileSetTime()
//
// $result = FileSetTime("filename", "time", [type] [,1|0 recurse])
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_FileSetTime (VectorVariant &vParams, Variant &vResult)
{
	SYSTEMTIME	st;
	FILETIME	ft;

	uint		iNumParams = vParams.size();
	const char	*szTime = vParams[1].szValue();
	bool		bRecurse;
	char		szOldWorkingDir[_MAX_PATH+1];
	char		szPath[_MAX_PATH+1];
	char		szDrive[_MAX_PATH+1];
	char		szDir[_MAX_PATH+1];
	char		szFile[_MAX_PATH+1];
	char		szExt[_MAX_PATH+1];
	char		*szFilePart;

	// What time are we working with?
	int nWhichTime = 0;								// Default is modified
	if (iNumParams >= 3)
	{
		if (vParams[2].nValue() >= 0 && vParams[2].nValue() <= 2)
			nWhichTime = vParams[2].nValue();
	}

	// Recursion to be used?
	if (iNumParams >= 4 && vParams[3].nValue() == 1)
		bRecurse = true;
	else
		bRecurse = false;

	if (szTime[0] != '\0')
	{
		if (!Util_ConvSystemTime(szTime, &st, true, 0))
		{
			vResult = 0;
			return AUT_OK;
		}

	}
	else
	{
		// Time was blank - use current LOCAL time
		GetLocalTime(&st);
	}

	// Convert to a filetime
	SystemTimeToFileTime(&st, &ft);
	LocalFileTimeToFileTime(&ft, &ft);  // added this line to convert to proper UTC date/time

	// Make a copy of the requested filename and remove trailing \s
	strncpy(szPath, vParams[0].szValue(), _MAX_PATH);
	szPath[_MAX_PATH] = '\0';
	Util_StripTrailingDir(szPath);

	// Get the FULL pathname including drive directory etc
	GetFullPathName(szPath, _MAX_PATH, szPath, &szFilePart);

	// Split the target into bits
	_splitpath( szPath, szDrive, szDir, szFile, szExt );
	strcat(szDrive, szDir);
	strcat(szFile, szExt);

	// Get a copy of the current working directory and then change it to match our requested path
	GetCurrentDirectory(_MAX_PATH, szOldWorkingDir);
	SetCurrentDirectory(szDrive);


	// Is the requested filename a directory?
	if (Util_IsDir(szFile))
	{
		if (Util_FileSetTime(szFile, &ft, nWhichTime) == false)
		{
			vResult = 0;						// Error opening
			return AUT_OK;
		}

		if (bRecurse == false)
			return AUT_OK;
		else
		{
			SetCurrentDirectory(szFile);	// Go into the directory
			strcpy(szFile, "*.*");			// Match all
		}
	}

	// Do the operation
	if (FileSetTime_recurse(szFile, &ft, nWhichTime, bRecurse) == false)
		vResult = 0;							// Default is 1

	SetCurrentDirectory(szOldWorkingDir);	// Restore working directory

	return AUT_OK;

} // FileSetTime()


///////////////////////////////////////////////////////////////////////////////
// FileSetTime_recurse()
//
// Recursive helper function
///////////////////////////////////////////////////////////////////////////////

bool AutoIt_Script::FileSetTime_recurse (const char *szIn, FILETIME *ft, int nWhichTime, bool bRecurse)
{
	WIN32_FIND_DATA	findData;

	// Does the source file exist?
	HANDLE hSearch = FindFirstFile(szIn, &findData);

	while (hSearch != INVALID_HANDLE_VALUE)
	{
		// Make sure the returned handle is not . or ..
		if ( strcmp(findData.cFileName, ".") && strcmp(findData.cFileName, "..") )
		{
			if (Util_FileSetTime(findData.cFileName, ft, nWhichTime) == false)
			{
				FindClose(hSearch);				// Error opening
				return false;
			}

		} // End If

		if (FindNextFile(hSearch, &findData) == FALSE)
			break;

	} // End while

	FindClose(hSearch);

	// Only carry on if we need to recurse subdirectories
	if (bRecurse == false)
		return true;


	// Redo the search of this directory with *.* to find all the directories
	hSearch = FindFirstFile("*.*", &findData);

	while (hSearch != INVALID_HANDLE_VALUE)
	{
		// Make sure the returned handle is a directory (ignore . and ..)
		if ( (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY
			&& strcmp(findData.cFileName, ".") && strcmp(findData.cFileName, "..") )
		{
			SetCurrentDirectory(findData.cFileName);	// Move into new directory

			if (FileSetTime_recurse(szIn, ft, nWhichTime, bRecurse) == false)
			{
				FindClose(hSearch);
				return false;
			}

			SetCurrentDirectory("..");			// Return from directory

		} // End If

		if (FindNextFile(hSearch, &findData) == FALSE)
			break;

	} // End while

	FindClose(hSearch);

	return true;

} // FileSetTime_recurse()


///////////////////////////////////////////////////////////////////////////////
// FileGetSize()
//
// Returns FileSize as bytes
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_FileGetSize(VectorVariant &vParams, Variant &vResult)
{
	// Typical, the original version of the function works better than the newer one :)
	// The FindFirstFile also works on in-use files

	WIN32_FIND_DATA	findData;

	// Does the source file exist?
	HANDLE hSearch = FindFirstFile(vParams[0].szValue(), &findData);

	if (hSearch == INVALID_HANDLE_VALUE)
	{
		vResult = 0;
		SetFuncErrorCode(1);
	}
	else
	{
		FindClose(hSearch);
		// Store as double, or...
		//vResult = (double)(((__int64)(findData.nFileSizeHigh) << 32) | (__int64)(findData.nFileSizeLow));

		// Store as int64
		vResult = (((__int64)(findData.nFileSizeHigh) << 32) | (__int64)(findData.nFileSizeLow));

	}

	return AUT_OK;

} // FileGetSize()


///////////////////////////////////////////////////////////////////////////////
// FileOpenDialog()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_FileOpenDialog(VectorVariant &vParams, Variant &vResult)
{
	return FileDialog(vParams, vResult, vParams.size(), 1);

} // FileOpenDialog()


///////////////////////////////////////////////////////////////////////////////
// FileSaveDialog()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_FileSaveDialog(VectorVariant &vParams, Variant &vResult)
{
	return FileDialog(vParams, vResult, vParams.size(), 0);

} // FileSaveDialog()



///////////////////////////////////////////////////////////////////////////////
// FileDialog()
//
// FileOpenDialog and FileSaveDialog(<title>,<InitDir>,<filter>[,Options][,Default FileName])
// options are:
//	1 = File Must Exist
//	2 = Path Must Exist
//	4 = Allow MultiSelect -(result for multiple selections are <FULL PATH>,<FILE1>,<FILE2>,...)
//	8 = Prompt to Create New File
//	16 = Prompt to OverWrite File
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::FileDialog(VectorVariant &vParams, Variant &vResult, uint iNumParams, int nFlag)
{
	OPENFILENAME	ofn;
	char			szFile[65536] = "";	// initalize entire array with null characters
	char			szTitle[32768];
	char			szDir[_MAX_PATH+1];
	char			szFilter[(_MAX_PATH*2)+1];
	AString			sFilter, sTemp, RET;
	int				nPos1, nPos2;

	strncpy(szTitle, vParams[0].szValue(), 32767);		// Get the title
	szTitle[32767] = '\0';

	strncpy(szDir, vParams[1].szValue(), _MAX_PATH);		// Get the directory
	szDir[_MAX_PATH] = '\0';

	strncpy(szFilter, vParams[2].szValue(), _MAX_PATH);		// Get filter
	szFilter[_MAX_PATH] = '\0';

	// Work out the nightmare double nulled c string that we need for the filter
	// All Files (*.*)\0*.*\0\0
	sTemp = szFilter;

	nPos1 = sTemp.find_first_of("(");
	nPos2 = sTemp.find_first_of(")");

	if (nPos1 == sTemp.length() || nPos2 == sTemp.length() || nPos2 < nPos1)
	{
		FatalError(IDS_AUT_E_BADFILEFILTER);
		return AUT_ERR;
	}

	sFilter.assign(sTemp, nPos1+1, nPos2);
	nPos1 = (uint)strlen(szFilter);
	nPos2 = sFilter.length();
	szFilter[nPos1++] = '\0';
	strcpy(&szFilter[nPos1], sFilter.c_str());
	nPos1 = nPos1 + nPos2;
	szFilter[nPos1++] = '\0';
	szFilter[nPos1++] = '\0';

	if (iNumParams > 4)	// fifth argument is filename
	{
		strcpy(szFile, vParams[4].szValue());
	}
	else
	{
		szFile[0] = '\0';
	}
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize		= sizeof(OPENFILENAME);
	ofn.hwndOwner		= NULL;
	ofn.lpstrFilter		= szFilter;
	ofn.lpstrTitle		= szTitle;
	ofn.nMaxFile		= sizeof(szFile)-1;		// easier to maintain.  Returns memory allocated, not string size.
	ofn.lpstrFile		= szFile;
	ofn.lpstrInitialDir = szDir;				// Set working dir
	if ( iNumParams < 4 )						// Fourth argument is options
		ofn.Flags=OFN_NODEREFERENCELINKS|OFN_HIDEREADONLY|OFN_EXPLORER;	// Default
	else if( (vParams[3].nValue()<=31) && (vParams[3].nValue()>=0) )
	{
		nPos2 = OFN_NODEREFERENCELINKS|OFN_HIDEREADONLY|OFN_EXPLORER;
		if(vParams[3].nValue() & 16)
			nPos2=nPos2|OFN_OVERWRITEPROMPT;
		if(vParams[3].nValue() & 8)
			nPos2=nPos2|OFN_CREATEPROMPT;
		if(vParams[3].nValue() & 4)
			nPos2=nPos2|OFN_ALLOWMULTISELECT;
		if(vParams[3].nValue() & 2)
			nPos2=nPos2|OFN_PATHMUSTEXIST;
		if(vParams[3].nValue() & 1)
			nPos2=nPos2|OFN_FILEMUSTEXIST;
		ofn.Flags=nPos2;
	}
	else
		ofn.Flags=OFN_NODEREFERENCELINKS|OFN_HIDEREADONLY|OFN_EXPLORER;

	if(nFlag == 1)
	{
		if(GetOpenFileName(&ofn))
		{
			if(nPos2 & OFN_ALLOWMULTISELECT)
			{
				size_t len=strlen(ofn.lpstrFile);
				RET = ofn.lpstrFile;
				ofn.lpstrFile+=len+1;
				while(ofn.lpstrFile[0])
				{
					len=strlen(ofn.lpstrFile);
					RET += "|";
					RET += ofn.lpstrFile;
					ofn.lpstrFile+=len+1;
				}
				vResult = RET.c_str();
			}
			else
				vResult = ofn.lpstrFile;
		}
		else
			SetFuncErrorCode(1);					// Default is 0
	}
	else
	{
		if(GetSaveFileName(&ofn))
			vResult = ofn.lpstrFile;
		else
			SetFuncErrorCode(1);				// Default is 0
	}

	return AUT_OK;

} // FileDialog()



///////////////////////////////////////////////////////////////////////////////
// FileSelectFolder()
//
// FileSelectFolder(<Dialog Text>,<Root Dir>,<Create Folder Flag (0|1)>)
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_FileSelectFolder (VectorVariant &vParams, Variant &vResult)
{
#define BIF_NONEWFOLDERBUTTON	0x0200

#ifndef BIF_NEWDIALOGSTYLE
    #define BIF_NEWDIALOGSTYLE		0x0040
#endif

#ifndef BIF_EDITBOX
    #define BIF_EDITBOX				0x0010
#endif

#define BIF_BROWSEFORCOMPUTER	0x1000

	LPMALLOC		pMalloc;
	BROWSEINFO		browseInfo;
	LPITEMIDLIST	lpItemIDList;
	char			Result[_MAX_PATH+1] = "";
	char			szText[2048];
	char			szInitialDir[_MAX_PATH];
	int				flag = BIF_NONEWFOLDERBUTTON;
	int				iImage = 0;

    SHGetMalloc(&pMalloc);

	if (vParams[2].nValue() & 1)				// Create Folder Flag (>=IE6.0)
		(flag &= 0x00FF)|=BIF_NEWDIALOGSTYLE;	// works only with new dialog style
	if (vParams[2].nValue() & 2)				// New Dialog Style (>=IE5.0)
		flag |= BIF_NEWDIALOGSTYLE;
	if (vParams[2].nValue() & 4)				// Show Edit Control (>=IE4.0)
		flag |= BIF_EDITBOX;
//	if (vParams[2].nValue() & 8)				// Browse for computers only
//		flag |= BIF_BROWSEFORCOMPUTER;

	if (vParams[1].szValue()[0] != '\0')		// Root Folder ( blank = My Computer )
	{											// Get PIDL routine
		IShellFolder *pDF;
		if (SHGetDesktopFolder(&pDF) == NOERROR)
		{
			LPITEMIDLIST pIdl = NULL;
			ULONG        chEaten;
			ULONG        dwAttributes;
			OLECHAR olePath[MAX_PATH];			// wide-char version of path name
			MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, vParams[1].szValue(), -1, olePath, MAX_PATH);
			pDF->ParseDisplayName(NULL,NULL,olePath,&chEaten,&pIdl,&dwAttributes);
			pDF->Release();
			browseInfo.pidlRoot = pIdl;
		}
	} // Get PIDL routine
	else
		browseInfo.pidlRoot = NULL;

	browseInfo.hwndOwner		= NULL;
	browseInfo.pszDisplayName	= Result;
	strcpy(szText,vParams[0].szValue());
	browseInfo.lpszTitle		= szText;
	browseInfo.lpfn				= BrowseForFolderProc;
	strcpy(szInitialDir,vParams[3].szValue());
	browseInfo.lParam			= (LPARAM)szInitialDir;
	browseInfo.ulFlags			= flag;
	browseInfo.iImage			= iImage;

	lpItemIDList = SHBrowseForFolder(&browseInfo);// Spawn Dialog

	if(lpItemIDList != NULL)					// Folder Is Chosen
	{
		vResult = Result;						// Return Full Path	(in case of server name)
		SHGetPathFromIDList(lpItemIDList,Result);	// Doesn't work with server names! Just use return value...
		pMalloc->Free(lpItemIDList);

		if (Result[0] != '\0')
			vResult = Result;					// Return Full Path
	}
	else
	{
		SetFuncErrorCode(1);					// Folder Is NOT Chosen
		vResult = "";
	}

	pMalloc->Release();

	return AUT_OK;

} // FileSelectFolder()


///////////////////////////////////////////////////////////////////////////////
// BrowseForFolderProc()
///////////////////////////////////////////////////////////////////////////////

int CALLBACK AutoIt_Script::BrowseForFolderProc(HWND hWnd,UINT iMsg,LPARAM lParam,LPARAM lpData)
{
    switch(iMsg)
    {
        case BFFM_INITIALIZED:
            if(lpData)
                SendMessage(hWnd,BFFM_SETSELECTION,TRUE,lpData);
            break;
        default:
            break;
    }

  return 0;
} // BrowseForFolderProc


///////////////////////////////////////////////////////////////////////////////
// DriveMapAdd()
//
// DriveMapAdd( "drive", "\\server\share" [, flags] [, "user" [, "password"]]] )
//
// Maps a network drive
//
// Flags:
// CONNECT_UPDATE_PROFILE = 1
// CONNECT_INTERACTIVE = 8
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_DriveMapAdd(VectorVariant &vParams, Variant &vResult)
{
	NETRESOURCE	nr;
	DWORD		res;
	uint		iNumParams = vParams.size();
	DWORD		dwFlags = 0;
	char		szBuffer[256];
	DWORD		dwBuffersize = 256;
	DWORD		dwResult;

	// Get the basic parameters
	char *szDrive = Util_StrCpyAlloc(vParams[0].szValue());
	char *szRemote = Util_StrCpyAlloc(vParams[1].szValue());

	if (iNumParams > 2)
		dwFlags = vParams[2].nValue();

	if (!strnicmp(szDrive, "LPT", 3))
		nr.dwType = RESOURCETYPE_PRINT;
	else
		nr.dwType = RESOURCETYPE_DISK;

	nr.lpRemoteName = szRemote;
	nr.lpProvider = NULL;

	if ((szDrive[0] == '\0') || (szDrive[0] == '*'))
		nr.lpLocalName = NULL;					// No drive mapping with "" and first free with "*"
	else
		nr.lpLocalName = szDrive;

	if (szDrive[0] == '*')
		dwFlags |= CONNECT_REDIRECT;			// Use first available drive

	if (iNumParams < 4 || g_oVersion.IsWin9x())
		res = WNetUseConnection(NULL,&nr, NULL, NULL, dwFlags, szBuffer, &dwBuffersize, &dwResult); // Use current user/password
	else
	{
		if (iNumParams == 4)
			res = WNetUseConnection(NULL,&nr, NULL, vParams[3].szValue(), dwFlags, szBuffer, &dwBuffersize, &dwResult);
		else
			 res = WNetUseConnection(NULL,&nr, vParams[4].szValue(), vParams[3].szValue(), dwFlags, szBuffer, &dwBuffersize, &dwResult);
	}

	if(res != NO_ERROR)
	{
		vResult = 0;							// Default is 1

		// Use @error for extended information
		if (res == ERROR_ACCESS_DENIED)
			SetFuncErrorCode(2);				// Access denied
		else if (res == ERROR_ALREADY_ASSIGNED || res == ERROR_DEVICE_ALREADY_REMEMBERED)
			SetFuncErrorCode(3);				// Already assigned
		else if (res == ERROR_BAD_DEVICE)
			SetFuncErrorCode(4);				// Bad local device name
		else if (res == ERROR_BAD_NET_NAME)
			SetFuncErrorCode(5);				// Bad remote name
		else if (res == ERROR_INVALID_PASSWORD)
			SetFuncErrorCode(6);				// Bad password
		else
			SetFuncErrorCode(1);				// General error
	}

	// Return the drive mapped or "" if * was specified
	if (szDrive[0] == '*')
	{
		if  (dwResult & CONNECT_LOCALDRIVE)
			vResult = szBuffer;
		else
			vResult = "";						// Error
	}

	delete [] szDrive;
	delete [] szRemote;

	return AUT_OK;

} // DriveMapAdd()


///////////////////////////////////////////////////////////////////////////////
// DriveMapDel()
//
// DriveMapDel( "device" )
//
// Disconnects a network drive
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_DriveMapDel(VectorVariant &vParams, Variant &vResult)
{
	DWORD res = WNetCancelConnection2(vParams[0].szValue(), CONNECT_UPDATE_PROFILE, TRUE);

	if(res != NO_ERROR)
	{
		vResult = 0;							// Default is 1

		// Use @error for extended information
	}

	return AUT_OK;

} // DriveMapDel()


///////////////////////////////////////////////////////////////////////////////
// DriveMapGet()
//
// DriveMapGet( "device" )
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_DriveMapGet(VectorVariant &vParams, Variant &vResult)
{
	char	szBuffer[1024];
	DWORD	dwLen = 1024;

	DWORD res = WNetGetConnection(vParams[0].szValue(), szBuffer, &dwLen);

	if(res != NO_ERROR)
	{
		vResult = "";							// Default is 1
		SetFuncErrorCode(1);
	}
	else
		vResult = szBuffer;

	return AUT_OK;

} // DriveMapGet()


///////////////////////////////////////////////////////////////////////////////
// DriveGetDrive()
//
// Returns a list of drives of type specified
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_DriveGetDrive(VectorVariant &vParams, Variant &vResult)
{
	char	szVat[27][4];
	AString	sBuffer;
	int     n = 0, nFound = 0;
	UINT	uiFlag, uiTemp;
	Variant	*pvTemp;

	sBuffer = vParams[0].szValue();

	if ( sBuffer.stricmp("all")==0 )
		uiFlag = 99;
	else if ( sBuffer.stricmp("cdrom")==0 )
		uiFlag = DRIVE_CDROM;
	else if ( sBuffer.stricmp("removable")==0 )
		uiFlag = DRIVE_REMOVABLE;
	else if ( sBuffer.stricmp("fixed")==0 )
		uiFlag = DRIVE_FIXED;
	else if ( sBuffer.stricmp("network")==0 )
		uiFlag = DRIVE_REMOTE;
	else if ( sBuffer.stricmp("ramdisk")==0 )
		uiFlag = DRIVE_RAMDISK;
	else if ( sBuffer.stricmp("unknown")==0 )
		uiFlag = DRIVE_UNKNOWN;
	else
	{
		SetFuncErrorCode(1);
		return AUT_OK;
	}


	for( n=97;n<=122; ++n)
    {
		sBuffer	= (char)n;
		sBuffer += ":\\";
		uiTemp = GetDriveType(sBuffer.c_str());
		if( uiTemp==uiFlag || (uiFlag==99 && uiTemp!=DRIVE_NO_ROOT_DIR) )
		{
			sBuffer.strip_trailing("\\");
			strcpy(szVat[nFound],sBuffer.c_str());
			++nFound;
		}
	}
	if ( nFound > 0 )
	{
		Util_VariantArrayDim(&vResult, nFound + 1);

		pvTemp = Util_VariantArrayGetRef(&vResult, 0);	// First element
		*pvTemp = (int)nFound;

		for( n=1;n<=nFound; ++n)
		{
			pvTemp = Util_VariantArrayGetRef(&vResult, n);
			*pvTemp = szVat[n-1];
		}
	}
	else
		SetFuncErrorCode(1);

	return AUT_OK;

} // DriveGetDrive()


///////////////////////////////////////////////////////////////////////////////
// CDTray()
//
// Opens and closes the CDROM tray
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_CDTray(VectorVariant &vParams, Variant &vResult)
{
	AString sDrive	= vParams[0].szValue();
	AString sAction = vParams[1].szValue();

	sAction.tolower();							// Convert action to lower case (for comparision)

	if (sAction == "close")
		sAction = "closed";						// Will be a common mistake...

	if ( (sAction == "open" || sAction == "closed") && GetDriveType(sDrive.c_str()) == DRIVE_CDROM )
	{
		AString sMCI = "open ";
		sMCI += sDrive;
		sMCI += " type cdaudio alias cd wait";

		if ( mciSendString(sMCI.c_str(), 0, 0, 0) == 0 )	// open handle to Drive
		{
			sMCI = "set cd door ";
			sMCI += sAction;
			sMCI += " wait";
			if ( !(mciSendString(sMCI.c_str(), 0, 0, 0) == 0) )
				vResult = 0;					// Error

			sMCI = "close cd wait";
			mciSendString(sMCI.c_str(), 0, 0, 0);			// close handle to Drive
		}
		else
			vResult = 0;						// Error
	}
	else
		vResult = 0;							// Error

	return AUT_OK;

}// CDTray()


///////////////////////////////////////////////////////////////////////////////
// DriveGetType(<path>)
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_DriveGetType(VectorVariant &vParams, Variant &vResult)
{
	UINT			uiType = DRIVE_UNKNOWN;

	SetErrorMode(SEM_FAILCRITICALERRORS);		// So a:\ does not ask for disk

	AString sTemp = vParams[0].szValue();
	if ( sTemp[sTemp.length()-1] != '\\' )	// Attempt to fix the parameter passed
		sTemp += "\\";

	if ( (uiType = GetDriveType(sTemp.c_str())) != DRIVE_NO_ROOT_DIR )
	{
		switch (uiType)
		{
			case DRIVE_REMOVABLE:
				vResult = "Removable";
				break;
			case DRIVE_FIXED:
				vResult = "Fixed";
				break;
			case DRIVE_REMOTE:
				vResult = "Network";
				break;
			case DRIVE_CDROM:
				vResult = "CDROM";
				break;
			case DRIVE_RAMDISK:
				vResult = "RAMDisk";
				break;
			default:
				vResult = "Unknown";
		}
	}
	else
		SetFuncErrorCode(1);

	return AUT_OK;

} // DriveGetType()


///////////////////////////////////////////////////////////////////////////////
// $var = DriveSpaceTotal(<path>)
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_DriveSpaceTotal(VectorVariant &vParams, Variant &vResult)
{
	SetErrorMode(SEM_FAILCRITICALERRORS);		// So a:\ does not ask for disk

	AString sTemp = vParams[0].szValue();
	if ( sTemp[sTemp.length()-1] != '\\' )	// Attemp to fix the parameter passed
		sTemp += "\\";

	ULARGE_INTEGER	uiTotal, uiFree, uiUsed;
	DWORD			dwSectPerClust, dwBytesPerSect, dwFreeClusters, dwTotalClusters;
	typedef BOOL (WINAPI *MyGetDiskFreeSpaceEx)(LPCTSTR, PULARGE_INTEGER, PULARGE_INTEGER, PULARGE_INTEGER);
	MyGetDiskFreeSpaceEx	lpfnGetDiskFreeSpaceEx;

	if ( (lpfnGetDiskFreeSpaceEx = (MyGetDiskFreeSpaceEx)GetProcAddress(GetModuleHandle("kernel32.dll"), "GetDiskFreeSpaceExA")) != NULL )
	{
		if ( lpfnGetDiskFreeSpaceEx(sTemp.c_str(),&uiFree,&uiTotal,&uiUsed) )
			vResult = double((__int64)(uiTotal.QuadPart))/(1024.*1024.);
		else
			SetFuncErrorCode(1);
	}
	else
	{
		if ( GetDiskFreeSpace(sTemp.c_str(), &dwSectPerClust, &dwBytesPerSect, &dwFreeClusters, &dwTotalClusters) )
			vResult = double((__int64)(dwTotalClusters * dwSectPerClust * dwBytesPerSect))/(1024.*1024.);
		else
			SetFuncErrorCode(1);
	}

	return AUT_OK;

} // DriveSpaceTotal()


///////////////////////////////////////////////////////////////////////////////
// $var = DriveSpaceFree(<path>)
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_DriveSpaceFree(VectorVariant &vParams, Variant &vResult)
{
	SetErrorMode(SEM_FAILCRITICALERRORS);		// So a:\ does not ask for disk

	AString sTemp = vParams[0].szValue();
	if ( sTemp[sTemp.length()-1] != '\\' )	// Attemp to fix the parameter passed
		sTemp += "\\";

	ULARGE_INTEGER	uiTotal, uiFree, uiUsed;
	DWORD			dwSectPerClust, dwBytesPerSect, dwFreeClusters, dwTotalClusters;
	typedef BOOL (WINAPI *MyGetDiskFreeSpaceEx)(LPCTSTR, PULARGE_INTEGER, PULARGE_INTEGER, PULARGE_INTEGER);
	MyGetDiskFreeSpaceEx	lpfnGetDiskFreeSpaceEx;
//	int				test = ERROR_SUCCESS;

	if ( (lpfnGetDiskFreeSpaceEx = (MyGetDiskFreeSpaceEx)GetProcAddress(GetModuleHandle("kernel32.dll"), "GetDiskFreeSpaceExA")) != NULL )
	{
		if ( lpfnGetDiskFreeSpaceEx(sTemp.c_str(),&uiFree,&uiTotal,&uiUsed) )
			vResult = double((__int64)(uiFree.QuadPart))/(1024.*1024.);
		else
			SetFuncErrorCode(1);
	}
	else
	{
		if ( GetDiskFreeSpace(sTemp.c_str(), &dwSectPerClust, &dwBytesPerSect, &dwFreeClusters, &dwTotalClusters) )
			vResult = double((__int64)(dwFreeClusters * dwSectPerClust * dwBytesPerSect))/(1024.*1024.);
		else
			SetFuncErrorCode(1);
	}

	return AUT_OK;

} // DriveSpaceFree()


///////////////////////////////////////////////////////////////////////////////
// $var = DriveStatus(<path>)
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_DriveStatus(VectorVariant &vParams, Variant &vResult)
{
	SetErrorMode(SEM_FAILCRITICALERRORS);		// So a:\ does not ask for disk

	AString sTemp = vParams[0].szValue();
	if ( sTemp[sTemp.length()-1] != '\\' )	// Attemp to fix the parameter passed
		sTemp += "\\";

	DWORD			dwSectPerClust, dwBytesPerSect, dwFreeClusters, dwTotalClusters;
	int				test = ERROR_SUCCESS;

	if ( !(GetDiskFreeSpace(sTemp.c_str(), &dwSectPerClust, &dwBytesPerSect, &dwFreeClusters, &dwTotalClusters)) )
			test = GetLastError();

	switch (test)
	{
		case ERROR_SUCCESS:
			vResult = "READY";
			break;
		case ERROR_PATH_NOT_FOUND:
			vResult = "INVALID";
			break;
		case ERROR_NOT_READY:
			vResult = "NOTREADY";
			break;
		case ERROR_WRITE_PROTECT:
			vResult = "READONLY";
			break;
		default:
			vResult = "UNKNOWN";
	}

	if ( test>0 )
			SetFuncErrorCode(1);

	return AUT_OK;

} // DriveStatus()


///////////////////////////////////////////////////////////////////////////////
// DriveGetFileSystem(<drive>)
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_DriveGetFileSystem(VectorVariant &vParams, Variant &vResult)
{
	char			szBuffer[256];
	char			szFileSystem[256];
	DWORD			dwVolumeSerial,dwMaxCL,dwFSFlags;

	SetErrorMode(SEM_FAILCRITICALERRORS);		// So a:\ does not ask for disk

	AString sTemp = vParams[0].szValue();
	if ( sTemp[sTemp.length()-1] != '\\' )	// Attemp to fix the parameter passed
		sTemp += "\\";

	if ( GetVolumeInformation(sTemp.c_str(),szBuffer,255,&dwVolumeSerial,&dwMaxCL,&dwFSFlags,szFileSystem,255) )
		vResult = szFileSystem;
	else
		SetFuncErrorCode(1);

	return AUT_OK;

} // DriveGetFileSystem()


///////////////////////////////////////////////////////////////////////////////
// DriveGetSerial(<drive>)
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_DriveGetSerial(VectorVariant &vParams, Variant &vResult)
{
	char			szBuffer[256];
	char			szFileSystem[256];
	DWORD			dwVolumeSerial,dwMaxCL,dwFSFlags;

	SetErrorMode(SEM_FAILCRITICALERRORS);		// So a:\ does not ask for disk

	AString sTemp = vParams[0].szValue();
	if ( sTemp[sTemp.length()-1] != '\\' )	// Attemp to fix the parameter passed
		sTemp += "\\";

	if ( GetVolumeInformation(sTemp.c_str(),szBuffer,255,&dwVolumeSerial,&dwMaxCL,&dwFSFlags,szFileSystem,255) )
	{
		sprintf(szBuffer,"%lu",dwVolumeSerial);
		vResult = szBuffer;
	}
	else
		SetFuncErrorCode(1);

	return AUT_OK;

} // DriveGetSerial()


///////////////////////////////////////////////////////////////////////////////
// DriveGetLabel(<drive>)
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_DriveGetLabel(VectorVariant &vParams, Variant &vResult)
{
	char			szBuffer[256];
	char			szFileSystem[256];
	DWORD			dwVolumeSerial,dwMaxCL,dwFSFlags;

	SetErrorMode(SEM_FAILCRITICALERRORS);		// So a:\ does not ask for disk

	AString sTemp = vParams[0].szValue();
	if ( sTemp[sTemp.length()-1] != '\\' )	// Attemp to fix the parameter passed
		sTemp += "\\";

	if ( GetVolumeInformation(sTemp.c_str(),szBuffer,255,&dwVolumeSerial,&dwMaxCL,&dwFSFlags,szFileSystem,255) )
		vResult = szBuffer;
	else
		SetFuncErrorCode(1);

	return AUT_OK;

} // DriveGetLabel()


///////////////////////////////////////////////////////////////////////////////
// DriveSetLabel(<drive>,<label>)
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_DriveSetLabel(VectorVariant &vParams, Variant &vResult)
{
	AString sTemp = vParams[0].szValue();
	if ( sTemp[sTemp.length()-1] != '\\' )	// Attemp to fix the parameter passed
		sTemp += "\\";

	SetErrorMode(SEM_FAILCRITICALERRORS);		// So a:\ does not ask for disk

	if ( !SetVolumeLabel(sTemp.c_str(),vParams[1].szValue()) )
		vResult = 0;							// Default is 1

	return AUT_OK;

} // DriveSetLabel()


///////////////////////////////////////////////////////////////////////////////
// FileCreateShortcut()
// Creates a shortcut (.lnk) toi a file
// FileCreateShortcut(<file>,<lnk>[,<workdir>,<args>,<desc>,<icon>,<hotkey>,<iconindex>,<windowstate>])
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_FileCreateShortcut(VectorVariant &vParams, Variant &vResult)
{
	uint			iNumParams = vParams.size();
	bool			bShift, bControl, bAlt, bWin;
	UINT			mods = 0;
	UINT			vk;
	IShellLink*		psl;
	IPersistFile*	ppf;
	WORD			wsz[MAX_PATH];
	AString			sLink = vParams[1].szValue();

	// Add .lnk to the end of the lnk file - unless already present
	if (sLink.find_str(".lnk", false) == sLink.length() )
		sLink += ".lnk";

	CoInitialize(NULL);
	if( SUCCEEDED(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID *)&psl)) )
	{
		psl->SetPath(vParams[0].szValue());
		if ( iNumParams > 2 )
			psl->SetWorkingDirectory(vParams[2].szValue());

		if ( iNumParams > 3 )
			psl->SetArguments(vParams[3].szValue());

		if ( iNumParams > 4 )
			psl->SetDescription(vParams[4].szValue());

		if ( iNumParams > 5 )
			psl->SetIconLocation(vParams[5].szValue(),0);

		if ( iNumParams > 6 && vParams[6].isTrue())		// Hotkey may be blank
		{
			// Get the virtual key code and modifiers
			if (m_oSendKeys.GetSingleVKandMods(vParams[6].szValue(), vk, bShift, bControl, bAlt, bWin) == true)
			{
				if (bShift)
					mods |= HOTKEYF_SHIFT;

				// Make sure that CTRL+ALT is selected (otherwise invalid)
				mods |= HOTKEYF_CONTROL|HOTKEYF_ALT;

				psl->SetHotkey((WORD)(vk | (mods << 8)));	// Vk in low 8 bits, mods in high 8
			}
		}

		if ( iNumParams > 7 )
			psl->SetIconLocation(vParams[5].szValue(),vParams[7].nValue());

		if ( iNumParams > 8 )
			psl->SetShowCmd(vParams[8].nValue());

		if( SUCCEEDED(psl->QueryInterface(IID_IPersistFile,(LPVOID *)&ppf)) )
		{
			MultiByteToWideChar(CP_ACP,0,sLink.c_str(),-1,(LPWSTR)wsz,MAX_PATH);
			if ( FAILED(ppf->Save((LPCWSTR)wsz,TRUE)) )
				vResult = 0;
			ppf->Release();
		}
		psl->Release();
	}
	else
		vResult = 0;							// Error, default is 1

	CoUninitialize();

	return	 AUT_OK;

} // FileCreateShortcut()


///////////////////////////////////////////////////////////////////////////////
// FileGetShortcut()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_FileGetShortcut(VectorVariant &vParams, Variant &vResult)
{
	IShellLink		*psl;
	IPersistFile	*ppf;
	WORD			wsz[MAX_PATH+1];
	Variant			*pvTemp;
	char			szGotPath[MAX_PATH+1];
	char			szArguments[MAX_PATH+1];
	char			szWorkingDir[MAX_PATH+1];
	char			szDescription[MAX_PATH+1];
	char			szIconLocation[MAX_PATH+1];
	int				nIconIndex;
	int				nShowCmd;
	AString			sLink = vParams[0].szValue();

	// Add .lnk to the end of the lnk file - unless already present
	if (sLink.find_str(".lnk", false) == sLink.length())
		sLink += ".lnk";

	SetFuncErrorCode(1);
	if (Util_DoesFileExist(sLink.c_str()))
	{
		CoInitialize(NULL);
		if( SUCCEEDED(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID *)&psl)) )
		{
			if (SUCCEEDED(psl->QueryInterface( IID_IPersistFile, (LPVOID *) &ppf)))
			{
				MultiByteToWideChar(CP_ACP,0,sLink.c_str(),-1,(LPWSTR)wsz,MAX_PATH);
				if (SUCCEEDED(ppf->Load((const WCHAR*)wsz,0)))
				{
					Util_VariantArrayDim(&vResult,7);

					psl->GetPath(szGotPath,MAX_PATH,NULL,SLGP_UNCPRIORITY);
					pvTemp = Util_VariantArrayGetRef(&vResult,0);
					*pvTemp = szGotPath;

					psl->GetWorkingDirectory(szWorkingDir,MAX_PATH);
					pvTemp = Util_VariantArrayGetRef(&vResult,1);
					*pvTemp = szWorkingDir;

					psl->GetArguments(szArguments,MAX_PATH);
					pvTemp = Util_VariantArrayGetRef(&vResult,2);
					*pvTemp = szArguments;

					psl->GetDescription(szDescription,MAX_PATH);
					pvTemp = Util_VariantArrayGetRef(&vResult,3);
					*pvTemp = szDescription;

					psl->GetIconLocation(szIconLocation,MAX_PATH,&nIconIndex);
					pvTemp = Util_VariantArrayGetRef(&vResult,4);
					*pvTemp = szIconLocation;
					pvTemp = Util_VariantArrayGetRef(&vResult,5);
					*pvTemp = nIconIndex;

					psl->GetShowCmd(&nShowCmd);
					pvTemp = Util_VariantArrayGetRef(&vResult,6);
					*pvTemp = nShowCmd;
					SetFuncErrorCode(0);
				}
				ppf->Release();
			}
			psl->Release();
		}
		CoUninitialize();
	}

	return AUT_OK;

} // FileGetShortcut()


#ifdef AUTOITSC

///////////////////////////////////////////////////////////////////////////////
// FileInstall()
//
// SELFCONTAINED AUTOIT VERSION
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_FileInstall(VectorVariant &vParams, Variant &vResult)
{
	bool			bOverwrite;
	bool			bDestExists;
	HS_EXEArc_Read	oRead;
	char			szEXE[_MAX_PATH+1];
	char			szDest[_MAX_PATH+1];
	char			szExpandedDest[_MAX_PATH+1];
	char			szDrive[_MAX_PATH+1];
	char			szDir[_MAX_PATH+1];
	char			szFile[_MAX_PATH+1];
	char			szExt[_MAX_PATH+1];

	if (vParams.size() == 3 && vParams[2].nValue() != 0)
		bOverwrite = true;
	else
		bOverwrite = false;


	// Get this EXEs filename
	GetModuleFileName(NULL, szEXE, _MAX_PATH);

	// Open the archive in this compiled exe
	if ( oRead.Open(szEXE, "") != HS_EXEARC_E_OK)
	{
		vResult = 0;							// Error (default is 1)
		return AUT_OK;
	}


	// Split dest into file and extension
	_splitpath( vParams[1].szValue(), szDrive, szDir, szFile, szExt );

	// If the filename and extension are both blank, sub with *.*
	if (szFile[0] == '\0' && szFile[0] == '\0')
	{
		strcpy(szFile, "*");
		strcpy(szExt,".*");
	}

	strcpy(szDest, szDrive);	strcat(szDest, szDir);
	strcat(szDest, szFile);		strcat(szDest, szExt);

	// Expand the destination based on source/dest
	Util_ExpandFilenameWildcard(vParams[0].szValue(), szDest, szExpandedDest);


	// Does the destination exist?
	bDestExists = Util_DoesFileExist(szExpandedDest);

	// Extract the file - maybe
	if ( (bDestExists == true  && bOverwrite == true) ||
		 (bDestExists == false) )
	{
		// Perform the extraction
		if ( oRead.FileExtract(vParams[0].szValue(), szExpandedDest) != HS_EXEARC_E_OK)
			vResult = 0;						// Error (default is 1)
	}
	else
		vResult = 0;							// File was NOT copied

	oRead.Close();								// Close the archive

	return AUT_OK;

} // FileInstall();

#else // AUTOITSC

///////////////////////////////////////////////////////////////////////////////
// FileInstall()
//
// NORMAL AUTOIT VERSION
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_FileInstall(VectorVariant &vParams, Variant &vResult)
{
	bool	bOverwrite;

	if (vParams.size() == 3 && vParams[2].nValue() != 0)
		bOverwrite = true;
	else
		bOverwrite = false;

	if (Util_CopyFile(vParams[0].szValue(), vParams[1].szValue(), bOverwrite, false) == false)
		vResult = 0;							// Error (default is 1)

	return AUT_OK;

} // FileInstall();

#endif // AUTOITSC


///////////////////////////////////////////////////////////////////////////////
// FileRecycle()
// Send a file to the recycle bin, if possible
//
// $var = FileRecycle("file(s)")
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_FileRecycle(VectorVariant &vParams, Variant &vResult)
{
	SHFILEOPSTRUCT	FileOp;

	char			szFileTemp[_MAX_PATH+2];

	// Get the fullpathname - required for UNDO to work
	Util_GetFullPathName(vParams[0].szValue(), szFileTemp);

	// We must also make it a double nulled string *sigh*
	szFileTemp[strlen(szFileTemp)+1] = '\0';

	// set to known values - Corrects crash
	FileOp.hNameMappings = NULL;
	FileOp.lpszProgressTitle = NULL;
	FileOp.fAnyOperationsAborted = FALSE;
	FileOp.hwnd = NULL;
	FileOp.pTo = NULL;

	FileOp.pFrom = szFileTemp;
	FileOp.wFunc = FO_DELETE;
	FileOp.fFlags = FOF_SILENT | FOF_ALLOWUNDO | FOF_NOCONFIRMATION;
	if ( SHFileOperation(&FileOp) )
		vResult = 0;								// Error, default is 1

	return AUT_OK;

} // FileRecycle()


///////////////////////////////////////////////////////////////////////////////
// FileRecycleEmpty()
// Empty the recycle bin (IE4+ required) - dynamically load to support non IE4
//
// $var = FileRecycleEmpty(["drive"])
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_FileRecycleEmpty(VectorVariant &vParams, Variant &vResult)
{
typedef HRESULT (WINAPI *MySHEmptyRecycleBin)(HWND, LPCTSTR, DWORD);

	const char			*szPath = NULL;
	MySHEmptyRecycleBin	lpfnEmpty;
	HINSTANCE			hinstLib;

	hinstLib = LoadLibrary("shell32.dll");
	if (hinstLib == NULL)
	{
		vResult = 0;
		return AUT_OK;
	}

	// Get the address of all the functions we require
 	lpfnEmpty		= (MySHEmptyRecycleBin)GetProcAddress(hinstLib, "SHEmptyRecycleBinA");
	if (lpfnEmpty == NULL)
	{
		vResult = 0;
		return AUT_OK;
	}

	if (vParams.size() != 0)
		szPath = vParams[0].szValue();

	if (lpfnEmpty(NULL, szPath, SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI | SHERB_NOSOUND) != S_OK)
		vResult = 0;								// Error, default is 1

	return AUT_OK;

} // FileRecycleEmpty()


///////////////////////////////////////////////////////////////////////////////
// FileGetAttrib()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_FileGetAttrib (VectorVariant &vParams, Variant &vResult)
{
	DWORD		dwTemp;
	AString		aRet;

	//aRet = "";

	dwTemp = GetFileAttributes(vParams[0].szValue());
	if ( dwTemp != (DWORD)-1 )
	{
		if (dwTemp & FILE_ATTRIBUTE_READONLY)
			aRet += "R";
		if (dwTemp & FILE_ATTRIBUTE_ARCHIVE)
			aRet += "A";
		if (dwTemp & FILE_ATTRIBUTE_SYSTEM)
			aRet += "S";
		if (dwTemp & FILE_ATTRIBUTE_HIDDEN)
			aRet += "H";
		if (dwTemp & FILE_ATTRIBUTE_NORMAL)
			aRet += "N";
		if (dwTemp & FILE_ATTRIBUTE_DIRECTORY)
			aRet += "D";
		if (dwTemp & FILE_ATTRIBUTE_OFFLINE)
			aRet += "O";
		if (dwTemp & FILE_ATTRIBUTE_COMPRESSED)
			aRet += "C";
		if (dwTemp & FILE_ATTRIBUTE_TEMPORARY)
			aRet += "T";
	}
	else
		SetFuncErrorCode(1);

	vResult = aRet.c_str();
	return AUT_OK;

} // FileGetAttrib()


///////////////////////////////////////////////////////////////////////////////
// FileSetAttrib()
//
// $result = FileSetAttrib("filename", "attribs" [,1|0 recurse])
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_FileSetAttrib (VectorVariant &vParams, Variant &vResult)
{
	DWORD		dwTemp;
	DWORD		dwAdd = 0;
	DWORD		dwRemove = 0;

	const char	*szAttribs = vParams[1].szValue();
	int			nPos = 0;
	int			nOp = 1;						// Default is +
	bool		bRecurse;
	char		szOldWorkingDir[_MAX_PATH+1];
	char		szPath[_MAX_PATH+1];
	char		szDrive[_MAX_PATH+1];
	char		szDir[_MAX_PATH+1];
	char		szFile[_MAX_PATH+1];
	char		szExt[_MAX_PATH+1];
	char		*szFilePart;


	if (vParams.size() >= 3 && vParams[2].nValue() == 1)
		bRecurse = true;
	else
		bRecurse = false;

	// Work out what attributes are required
	while (szAttribs[nPos] != '\0')
	{
		if (szAttribs[nPos] == '+')
		{
			++nPos;
			nOp = 1;
		}
		else if (szAttribs[nPos] == '-')
		{
			++nPos;
			nOp = 0;
		}

		dwTemp = 0;

		if (szAttribs[nPos] == 'R' || szAttribs[nPos] == 'r')
			dwTemp |= FILE_ATTRIBUTE_READONLY;
		else if (szAttribs[nPos] == 'A' || szAttribs[nPos] == 'a')
			dwTemp |= FILE_ATTRIBUTE_ARCHIVE;
		else if (szAttribs[nPos] == 'S' || szAttribs[nPos] == 's')
			dwTemp |= FILE_ATTRIBUTE_SYSTEM;
		else if (szAttribs[nPos] == 'H' || szAttribs[nPos] == 'h')
			dwTemp |= FILE_ATTRIBUTE_HIDDEN;
		else if (szAttribs[nPos] == 'N' || szAttribs[nPos] == 'n')
			dwTemp |= FILE_ATTRIBUTE_NORMAL;
		else if (szAttribs[nPos] == 'O' || szAttribs[nPos] == 'o')
			dwTemp |= FILE_ATTRIBUTE_OFFLINE;
		else if (szAttribs[nPos] == 'T' || szAttribs[nPos] == 't')
			dwTemp |= FILE_ATTRIBUTE_TEMPORARY;
		else
		{
			vResult = 0;
			return AUT_OK;						// Bad attrib
		}

		if (nOp == 1)
			dwAdd = dwAdd | dwTemp;
		else
			dwRemove = dwRemove | dwTemp;

		++nPos;
	}


	// Make a copy of the requested filename and remove trailing \s
	strncpy(szPath, vParams[0].szValue(), _MAX_PATH);
	szPath[_MAX_PATH] = '\0';
	Util_StripTrailingDir(szPath);

	// Get the FULL pathname including drive directory etc
	GetFullPathName(szPath, _MAX_PATH, szPath, &szFilePart);

	// Split the target into bits
	_splitpath( szPath, szDrive, szDir, szFile, szExt );
	strcat(szDrive, szDir);
	strcat(szFile, szExt);

	// Get a copy of the current working directory and then change it to match our requested path
	GetCurrentDirectory(_MAX_PATH, szOldWorkingDir);
	SetCurrentDirectory(szDrive);


	// Is the requested filename a directory?
	if (Util_IsDir(szFile))
	{
		dwTemp = GetFileAttributes(szFile);
		dwTemp = dwTemp | dwAdd;
		dwTemp = dwTemp & (~dwRemove);
		if (!SetFileAttributes(szFile, dwTemp))
		{
			SetCurrentDirectory(szOldWorkingDir);	// Restore working directory
			vResult = 0;						// Error setting attribs
			return AUT_OK;
		}

		if (bRecurse == false)
			return AUT_OK;
		else
		{
			SetCurrentDirectory(szFile);	// Go into the directory
			strcpy(szFile, "*.*");			// Match all
		}
	}

	// Do the operation
	if (FileSetAttrib_recurse(szFile, dwAdd, dwRemove, bRecurse) == false)
		vResult = 0;							// Default is 1

	SetCurrentDirectory(szOldWorkingDir);	// Restore working directory

	return AUT_OK;

} // FileSetAttrib()


///////////////////////////////////////////////////////////////////////////////
// FileSetAttrib_recurse()
//
// Recursive helper function
///////////////////////////////////////////////////////////////////////////////

bool AutoIt_Script::FileSetAttrib_recurse (const char *szIn, DWORD dwAdd, DWORD dwRemove, bool bRecurse)
{
	WIN32_FIND_DATA	findData;
	DWORD			dwTemp;

	// Does the source file exist?
	HANDLE hSearch = FindFirstFile(szIn, &findData);

	while (hSearch != INVALID_HANDLE_VALUE)
	{
		// Make sure the returned handle is not . or ..
		if ( strcmp(findData.cFileName, ".") && strcmp(findData.cFileName, "..") )
		{
			dwTemp = GetFileAttributes(findData.cFileName);
			dwTemp = dwTemp | dwAdd;
			dwTemp = dwTemp & (~dwRemove);
			if (!SetFileAttributes(findData.cFileName, dwTemp))
			{
				FindClose(hSearch);
				return false;
			}

		} // End If

		if (FindNextFile(hSearch, &findData) == FALSE)
			break;

	} // End while

	FindClose(hSearch);

	// Only carry on if we need to recurse subdirectories
	if (bRecurse == false)
		return true;


	// Redo the search of this directory with *.* to find all the directories
	hSearch = FindFirstFile("*.*", &findData);

	while (hSearch != INVALID_HANDLE_VALUE)
	{
		// Make sure the returned handle is a directory (ignore . and ..)
		if ( (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY
			&& strcmp(findData.cFileName, ".") && strcmp(findData.cFileName, "..") )
		{
			SetCurrentDirectory(findData.cFileName);	// Move into new directory

			if (FileSetAttrib_recurse(szIn, dwAdd, dwRemove, bRecurse) == false)
			{
				FindClose(hSearch);
				return false;
			}

			SetCurrentDirectory("..");			// Return from directory

		} // End If

		if (FindNextFile(hSearch, &findData) == FALSE)
			break;

	} // End while

	FindClose(hSearch);

	return true;

} // FileSetAttrib_recurse()


///////////////////////////////////////////////////////////////////////////////
// FileGetVersion(<file>)
// Returns Major and Minor, File and Product versions as an array
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_FileGetVersion (VectorVariant &vParams, Variant &vResult)
{
	char	szVersion[43+1];					// See Util_FileGetVersion for 43+1

	char *szFile = Util_StrCpyAlloc(vParams[0].szValue());

	if (Util_GetFileVersion(szFile, szVersion) == true)
	{
		vResult = szVersion;
	}
	else
	{
		vResult = "0.0.0.0";
		SetFuncErrorCode(1);
	}

	delete [] szFile;

	return AUT_OK;

} // FileGetVersion()


///////////////////////////////////////////////////////////////////////////////
// FileFindFirstFile(<file>)
// Returns a handle to be used in FileFindNextFile.
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT	AutoIt_Script::F_FileFindFirstFile(VectorVariant &vParams, Variant &vResult)
{
	WIN32_FIND_DATA		wfd;
	int					nFreeHandle;

	HANDLE hFind = FindFirstFile(vParams[0].szValue(), &wfd);

	if ( hFind == INVALID_HANDLE_VALUE )
	{
		vResult = -1;							// -1 is error
		return AUT_OK;
	}

	// File was found

	// Have we room for another file handle?
	if (m_nNumFileHandles == AUT_MAXOPENFILES)
	{
		FatalError(IDS_AUT_E_TOOMANYFILES);
		return AUT_ERR;
	}

	// Find a free handle
	for (nFreeHandle = 0; nFreeHandle < AUT_MAXOPENFILES; ++nFreeHandle)
	{
		if (m_FileHandleDetails[nFreeHandle] == NULL)
			break;
	}

	// Save the handle and the result of the first find
	m_FileHandleDetails[nFreeHandle] = new FileHandleDetails;	// Create new entry
	m_FileHandleDetails[nFreeHandle]->nType = AUT_FILEFIND;		// File find type
	m_FileHandleDetails[nFreeHandle]->hFind = hFind;			// Store handle

	char *szFind = Util_StrCpyAlloc(wfd.cFileName);					// Store the first find result
	m_FileHandleDetails[nFreeHandle]->szFind = szFind;			// Store string

	++m_nNumFileHandles;

	vResult = nFreeHandle;

	return AUT_OK;

} // FileFindFirstFile()


///////////////////////////////////////////////////////////////////////////////
// FileFindNextFile(handle)
// Returns the next file found according to a previous call to FileFindFirstFile.
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT	AutoIt_Script::F_FileFindNextFile(VectorVariant &vParams, Variant &vResult)
{
	WIN32_FIND_DATA		wfd;
	int					nHandle = vParams[0].nValue();

	// Does this file handle exist?
	if (nHandle >= AUT_MAXOPENFILES || nHandle < 0 || m_FileHandleDetails[nHandle] == NULL)
	{
		FatalError(IDS_AUT_E_FILEHANDLEINVALID);
		return AUT_ERR;
	}

	// Is it a file find handle?
	if (m_FileHandleDetails[nHandle]->nType != AUT_FILEFIND)
	{
		FatalError(IDS_AUT_E_FILEHANDLEINVALID);
		return AUT_ERR;
	}

	// Have we used the first search value?  If not, use and then set to NULL
	if (m_FileHandleDetails[nHandle]->szFind != NULL)
	{
		vResult = m_FileHandleDetails[nHandle]->szFind;
		delete [] m_FileHandleDetails[nHandle]->szFind;
		m_FileHandleDetails[nHandle]->szFind = NULL;
		return AUT_OK;
	}

	// Get the next file
	if ( !FindNextFile(m_FileHandleDetails[nHandle]->hFind, &wfd) )
	{
		SetFuncErrorCode(1);					// No more files
		vResult = "";
	}
	else
		vResult = wfd.cFileName;

	return AUT_OK;

} // FileFindNextFile()


///////////////////////////////////////////////////////////////////////////////
// FileGetLongName()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_FileGetLongName(VectorVariant &vParams, Variant &vResult)
{
	char	szLFN[_MAX_PATH+1];

	if (Util_GetLongFileName(vParams[0].szValue(), szLFN) == true)
		vResult = szLFN;
	else
	{
		vResult = vParams[0].szValue();
		SetFuncErrorCode(1);					// Error
	}

	return AUT_OK;

} // FileGetLongName()


///////////////////////////////////////////////////////////////////////////////
// FileGetShortName()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_FileGetShortName(VectorVariant &vParams, Variant &vResult)
{
	char	szBuffer[_MAX_PATH+1];

	if ( GetShortPathName(vParams[0].szValue(),szBuffer,_MAX_PATH) )
		vResult = szBuffer;
	else
	{
		SetFuncErrorCode(1);
		vResult = vParams[0].szValue();
	}

	return AUT_OK;

} // FileGetShortName()



///////////////////////////////////////////////////////////////////////////////
// DirCopy()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_DirCopy(VectorVariant &vParams, Variant &vResult)
{
	bool	bTemp = false;

	if (vParams.size() >= 3 && vParams[2].nValue() != 0)
		bTemp = true;

	if (Util_CopyDir(vParams[0].szValue(), vParams[1].szValue(), bTemp) == false)
		vResult = 0;				// Error, default is 1

	return AUT_OK;

} // DirCopy()


///////////////////////////////////////////////////////////////////////////////
// FileExists()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_FileExists(VectorVariant &vParams, Variant &vResult)
{
	if (Util_DoesFileExist(vParams[0].szValue()) == false)
		vResult = 0;					// Default is 1
	return AUT_OK;

} // FileExists()


///////////////////////////////////////////////////////////////////////////////
// DirCreate()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_DirCreate(VectorVariant &vParams, Variant &vResult)
{
	if (Util_CreateDir(vParams[0].szValue()) == false)
		vResult = 0;					// Error, default is 1
	return AUT_OK;

} // DirCreate()


///////////////////////////////////////////////////////////////////////////////
// DirRemove()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_DirRemove(VectorVariant &vParams, Variant &vResult)
{
	bool	bTemp = false;

	if (vParams.size() >= 2 && vParams[1].nValue() != 0)
		bTemp = true;

	if (Util_RemoveDir(vParams[0].szValue(), bTemp) == false)
		vResult = 0;				// Error, default is 1

		return AUT_OK;

} // DirRemove()


///////////////////////////////////////////////////////////////////////////////
// FileCopy()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_FileCopy(VectorVariant &vParams, Variant &vResult)
{
	bool bTemp = false;

	if (vParams.size() >= 3 && vParams[2].nValue() != 0)
		bTemp = true;

	if (Util_CopyFile(vParams[0].szValue(), vParams[1].szValue(), bTemp, false) == false)
		vResult = 0;				// Error, default is 1

	return AUT_OK;

} // FileCopy()


///////////////////////////////////////////////////////////////////////////////
// FileDelete()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_FileDelete(VectorVariant &vParams, Variant &vResult)
{
	if (Util_DeleteFile(vParams[0].szValue()) == false)
		vResult = 0;							// Error (default is 1)
	return AUT_OK;

} // FileDelete()


///////////////////////////////////////////////////////////////////////////////
// FileMove()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_FileMove(VectorVariant &vParams, Variant &vResult)
{
	bool	bTemp = false;

	if (vParams.size() >= 3 && vParams[2].nValue() != 0)
		bTemp = true;

	if (Util_CopyFile(vParams[0].szValue(), vParams[1].szValue(), bTemp, true) == false)
		vResult = 0;				// Error, default is 1

	return AUT_OK;

} // FileMove()


///////////////////////////////////////////////////////////////////////////////
// FileChangeDir()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_FileChangeDir(VectorVariant &vParams, Variant &vResult)
{
	if ( SetCurrentDirectory(vParams[0].szValue()) == 0 )
		vResult = 0;						// Failed, default is 1
	return AUT_OK;

} // FileChangeDir()


///////////////////////////////////////////////////////////////////////////////
// DirMove()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_DirMove(VectorVariant &vParams, Variant &vResult)
{
	bool bTemp = false;

	if (vParams.size() >= 3 && vParams[2].nValue() != 0)
		bTemp = true;

	if (Util_MoveDir(vParams[0].szValue(), vParams[1].szValue(), bTemp) == false)
		vResult = 0;				// Error, default is 1

	return AUT_OK;

} // DirMove()


///////////////////////////////////////////////////////////////////////////////
// DirGetSize(<path> [,parameter])
//
// parameter:	1 - Extended mode -> returns array with information
//				2 - Don't search in subfolders
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_DirGetSize(VectorVariant &vParams, Variant &vResult)
{
	uint	iNumParams	= vParams.size();
	AString sInputPath	= vParams[0].szValue();
	bool	bExt		= false;					// Extended mode
	bool	bRec		= true;						// Recurse subdirectories
	__int64	nSize		= 0;
	__int64	nFiles		= 0;
	__int64	nDirs		= 0;

	SetErrorMode(SEM_FAILCRITICALERRORS);

	if (sInputPath[sInputPath.length()-1] != '\\')	// Attempt to fix the parameter passed
		sInputPath += "\\";

	if (!Util_IsDir(sInputPath.c_str()))
	{
		vResult = -1;
		SetFuncErrorCode(1);
		return AUT_OK;
	}

	if (iNumParams > 1)
	{
		if (vParams[1].nValue() & 1)
			bExt = true;

		if (vParams[1].nValue() & 2)
			bRec = false;
	}

	if (GetDirSize(sInputPath.c_str(), nSize, nFiles, nDirs, bExt, bRec) == false)
	{
		// Script must have quit to return false
		return AUT_ERR;							// Emergency exit
	}

	if (bExt)
	{
		Variant	*pvTemp;
		Util_VariantArrayDim(&vResult,3);

		pvTemp = Util_VariantArrayGetRef(&vResult,0);
		*pvTemp = nSize;

		pvTemp = Util_VariantArrayGetRef(&vResult,1);
		*pvTemp = nFiles;

		pvTemp = Util_VariantArrayGetRef(&vResult,2);
		*pvTemp = nDirs;
	}
	else
		vResult = nSize;

	return AUT_OK;

} // F_DirGetSize()


///////////////////////////////////////////////////////////////////////////////
// GetDirSize()
///////////////////////////////////////////////////////////////////////////////

bool AutoIt_Script::GetDirSize(const char *szInputPath, __int64 &nSize, __int64 &nFiles, __int64 &nDirs,  bool bExt, bool bRec)
{
	WIN32_FIND_DATA	FindFileData;
	HANDLE			hFind;
	AString			sTemp	= szInputPath;
	AString			sPathbak= sTemp;
	sTemp += "*.*";
	int				nMsg;
	bool			bRes = true;

	hFind = FindFirstFile(sTemp.c_str(),&FindFileData);

	while (hFind != INVALID_HANDLE_VALUE)
	{
		// As this function may take ages to execute, keep the message loop going and handle
		// any pause/quit requests
		nMsg = ProcessMessages();
		if (nMsg == AUT_QUIT)
		{
			bRes = false;
			break;								// Exit while loop
		}
		else if (nMsg == AUT_PAUSED)
		{
			Sleep(AUT_IDLE);
			continue;							// Restart while loop
		}

		if (!(!strcmp(FindFileData.cFileName, ".") || !strcmp(FindFileData.cFileName, "..")))
		{
			sTemp = sPathbak;
			sTemp += FindFileData.cFileName;

			if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if (bExt)
					nDirs++;
				if (bRec)
				{
					sTemp += "\\";
					bRes = GetDirSize(sTemp.c_str(),nSize,nFiles,nDirs,bExt,bRec);
					if (bRes == false)
						break;					// Exit while loop
				}
			}
			else
			{
				nSize += (((__int64)(FindFileData.nFileSizeHigh) << 32) | (__int64)(FindFileData.nFileSizeLow));
				if (bExt)
					nFiles++;
			}
		}
		if (!FindNextFile(hFind,&FindFileData))
			break;
	}
	FindClose(hFind);

	return bRes;

} // GetDirSize

