
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
// Contains string handling routines.  Part of script.cpp
//
///////////////////////////////////////////////////////////////////////////////


// Includes
#include "StdAfx.h"								// Pre-compiled headers

#ifndef _MSC_VER								// Includes for non-MS compilers
	#include <stdio.h>
	#include <windows.h>
	#include <ctype.h>
#endif

#include "AutoIt.h"								// Autoit values, macros and config options

#include "script.h"
#include "resources\resource.h"
#include "utility.h"


///////////////////////////////////////////////////////////////////////////////
// StringLen()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_StringLen(VectorVariant &vParams, Variant &vResult)
{
	// $var = StringLen(<string>)
	vResult = (int)strlen(vParams[0].szValue());
	return AUT_OK;

} // StringLen()


///////////////////////////////////////////////////////////////////////////////
// StringLeft()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_StringLeft(VectorVariant &vParams, Variant &vResult)
{
	// $var = StringLeft(<string>, <count>)

	AString sInput = vParams[0].szValue();
	int		nCount = vParams[1].nValue();

	if (nCount > sInput.length())
		nCount = sInput.length();

	AString sResult;
	sResult.assign( sInput, 0, nCount );

	vResult = sResult.c_str();
	return AUT_OK;

} // StringLeft()


///////////////////////////////////////////////////////////////////////////////
// StringRight()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_StringRight(VectorVariant &vParams, Variant &vResult)
{
	// $var = StringRight(<string>, <count>)

	AString sInput = vParams[0].szValue();
	int		nCount = vParams[1].nValue();

	if (nCount > sInput.length())
		nCount = sInput.length();

	AString	sResult;
	sResult.assign( sInput, sInput.length()-nCount, sInput.length() );

	vResult = sResult.c_str();
	return AUT_OK;

} // StringRight()


///////////////////////////////////////////////////////////////////////////////
// StringMid()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_StringMid(VectorVariant &vParams, Variant &vResult)
{
	// $var = StringMid(<string>, <start> [, <count>])

	AString sInput = vParams[0].szValue();
	AString	sResult;
	int		nCount =-1;
	int		nStart = vParams[1].nValue() - 1;

	if (vParams.size() > 2)
		nCount	= vParams[2].nValue();

	if ( (nCount > (sInput.length() - nStart)) || (nCount < 0) )
		nCount = sInput.length() - nStart;

	sResult.assign( sInput, nStart, nStart+nCount );

	vResult = sResult.c_str();
	return AUT_OK;

} // StringMid()


///////////////////////////////////////////////////////////////////////////////
// StringTrimLeft()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_StringTrimLeft(VectorVariant &vParams, Variant &vResult)
{
	// $var = StringTrimLeft(<string>, <count>)

	AString sInput = vParams[0].szValue();
	AString	sResult;
	int		nEnd = sInput.length();
	int		nStart = vParams[1].nValue();

	if (nStart >= nEnd)
	{
		vResult = "";
		return AUT_OK;
	}

	sResult.assign( sInput, nStart, nEnd );

	vResult = sResult.c_str();
	return AUT_OK;

} // StringTrimLeft()


///////////////////////////////////////////////////////////////////////////////
// StringTrimRight()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_StringTrimRight(VectorVariant &vParams, Variant &vResult)
{
	// $var = StringTrimRight(<string>, <count>)

	AString sInput = vParams[0].szValue();
	AString	sResult;
	int		nEnd;

	if ( vParams[1].nValue() >= sInput.length() )
	{
		vResult = "";
		return AUT_OK;
	}

	nEnd = sInput.length() - vParams[1].nValue();

	sResult.assign( sInput, 0, nEnd );

	vResult = sResult.c_str();
	return AUT_OK;

} // StringTrimRight()


///////////////////////////////////////////////////////////////////////////////
// StringInStr()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_StringInStr(VectorVariant &vParams, Variant &vResult)
{
	// $var = StringInStr(<string>, <substring> [,casesense] [,occurance])

	int		nPos = 0;
	int		nOccur = 1;		// find first occurance
	bool	bCs = false;	// not case sensitive

	// Get the string into our nice string class
	AString sInput = vParams[0].szValue();

	switch (vParams.size())
	{
	case 4:
		// determine occurance
		if (vParams[3].nValue() == 0) // not valid
		{
			vResult = 0;
			SetFuncErrorCode(1);
			return AUT_OK;
		}
		else
			nOccur = vParams[3].nValue();

	case 3:
		// determine case sensitivity
		bCs = (vParams[2].nValue() != 0);

	case 2:
		// do the find.
		nPos = sInput.find_str( vParams[1].szValue(), bCs, nOccur );
	}

	if (nPos == sInput.length())
		vResult = 0;
	else
		vResult =  (int)nPos + 1;

	return AUT_OK;

} // StringInStr()


///////////////////////////////////////////////////////////////////////////////
// StringLower()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_StringLower(VectorVariant &vParams, Variant &vResult)
{
	// $var = StringLower(<string>)

	AString sString = vParams[0].szValue();
	sString.tolower();

	vResult =  sString.c_str();

	return AUT_OK;

} // StringLower()


///////////////////////////////////////////////////////////////////////////////
// StringUpper()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_StringUpper(VectorVariant &vParams, Variant &vResult)
{
	// $var = StringUpper(<string>)

	AString sString = vParams[0].szValue();
	sString.toupper();

	vResult =  sString.c_str();

	return AUT_OK;

} // StringUpper()


///////////////////////////////////////////////////////////////////////////////
// StringStripCR()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_StringStripCR(VectorVariant &vParams, Variant &vResult)
{
	// $var = StringStripCR(<string>)
	char	*szString;
	size_t	len;

	len = strlen(vParams[0].szValue());
	szString = new char[len+1];

	strcpy(szString, vParams[0].szValue());
	Util_StripCR(szString);

	vResult = szString;							// Copy to vResult

	delete [] szString;

	return AUT_OK;

} // StringStripCR()


///////////////////////////////////////////////////////////////////////////////
// StringAddCR()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_StringAddCR(VectorVariant &vParams, Variant &vResult)
{
	// $var = StringAddCR(<string>)
	char	*szOut;

	// How long will our new string buffer need to be?
	szOut = new char[Util_AddCRSize(vParams[0].szValue())];

	Util_AddCR(vParams[0].szValue(), szOut);

	vResult = szOut;							// Copy to vResult

	delete [] szOut;

	return AUT_OK;

} // StringAddCR()


///////////////////////////////////////////////////////////////////////////////
// StringReplace()
//
// $var = StringReplace(<string>, <searchstring or start>, <replacestring>, <numreplaces>, [casesense])
//
// This is a little bit inefficient as the input and output string will be
// constantly changing sizes with the AString class, but makes for easy reading :)
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_StringReplace(VectorVariant &vParams, Variant &vResult)
{
	AString sOutput;
	AString sTemp;
	bool	bCaseSense;
	int		nReplacesDone, nReplacesToDo;

	int		nPos;
	int		iSearchStrLen;

	if (vParams[1].type() == VAR_INT32 || vParams[1].type() == VAR_INT64)
	{
		nPos = vParams[1].nValue() -1;
		if ( nPos < 0 ||
				nPos + strlen(vParams[2].szValue()) > strlen(vParams[0].szValue()) )
		{
			vResult = "";
			SetFuncErrorCode(1);
			return AUT_OK;
		}

		sOutput.assign(vParams[0].szValue(), 0, nPos);
		sOutput += vParams[2].szValue();
		sTemp.assign(vParams[0].szValue(), nPos+(int)strlen(vParams[2].szValue()), (int)strlen(vParams[0].szValue()) );
		sOutput += sTemp;
	}
	else
	{
		// Search strings cannot be blank
		if (vParams[1].szValue()[0] == '\0')
			return AUT_OK;

		// Store number of replaces to do (0=all)
		if (vParams.size() >= 4)
			nReplacesToDo = vParams[3].nValue();
		else
			nReplacesToDo = 0;						// Default is all

		// Store the casesense mode
		if (vParams.size() == 5)
		{
			if (vParams[4].nValue() == 0)
				bCaseSense = false;
			else
				bCaseSense = true;
		}
		else
			bCaseSense = false;


		// Get the input string into our nice string class
		AString sInput = vParams[0].szValue();

		// Make a note of how long the search string is
		iSearchStrLen = (int)strlen(vParams[1].szValue());

		bool bLoop = true;
		nReplacesDone = 0;
		while (bLoop)
		{
			nPos = sInput.find_str(vParams[1].szValue(), bCaseSense);
			if (nPos == sInput.length())
			{
				sTemp.assign(sInput, 0, nPos);		// Copy remaining input
				sOutput += sTemp;					// And append to output
				bLoop = false;						// No more found
			}
			else
			{
				// Copy from the input up to the found string
				sTemp.assign(sInput, 0, nPos);

				// Append this to our output, and include the replace string
				sOutput += sTemp;
				sOutput += vParams[2].szValue();

				// Now erase everything up to and including our search string in sInput
				sInput.erase(0, nPos+iSearchStrLen);

				nReplacesDone++;

				// More replacing to do?
				if ( nReplacesToDo != 0 && nReplacesDone == nReplacesToDo)
				{
					sOutput += sInput;						// Append remaing input to output
					bLoop = false;							// No more to do
				}
				else
					bLoop = true;
			}
		}

		// Store the number of replacements done in @extended
		SetFuncExtCode(nReplacesDone);
	}
	vResult =  sOutput.c_str();

	return AUT_OK;

} // StringReplace()


///////////////////////////////////////////////////////////////////////////////
// StringSplit()
//
// $array = StringSplit("string", "delimiters" [, Flag] )
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_StringSplit(VectorVariant &vParams, Variant &vResult)
{
	int			iCount, iElements, iIndex;
	int			nPos, nFlag;
	char const	*pcSearch, *pcDelim, *pcTmp;	// pointer can change, but the pointed at stuff will not.
	Variant		*pvTemp;

	// For speed, pre-allocate our string class to be same size as input string
	AString		sElement((int)strlen(vParams[0].szValue()));

	if (vParams.size() < 3)
		nFlag = 0;
	else
		nFlag = vParams[2].nValue();

	pcSearch	= vParams[0].szValue();
	pcDelim		= vParams[1].szValue();

	// Create array with single characters when the delimiter is empty
	if (pcDelim[0] == '\0')
	{
		iCount   = 0;
		iIndex   = 1;
		iElements = (int)strlen(pcSearch);
		Util_VariantArrayDim(&vResult, iElements+1);    // create the array , String length + 1
		pvTemp = Util_VariantArrayGetRef(&vResult, 0);  // First element set to length
		*pvTemp = iElements;
		while (iCount < iElements)
		{
			sElement.assign(pcSearch, iCount, iCount + 1);
			pvTemp = Util_VariantArrayGetRef(&vResult, iIndex++);   //Next element
			*pvTemp = sElement.c_str();
			++iCount;                  // Increase count
		}
		return AUT_OK;
	}


	switch (nFlag)
	{
	case 0:	// default method - any characters in delimeter string delimit fields
		// Count how many times the delimiters occur
		iCount = 0;
		nPos = 0;
		while (pcSearch[nPos] != '\0')
		{
			if ( strchr(pcDelim, pcSearch[nPos]) )
					++iCount;						// Increase count
			++nPos;
		}
		break;
	case 1: // use exact delimeter string to delimit fields
		iCount=0;
		while (pcSearch != NULL)
		{
			pcSearch = strstr(pcSearch, pcDelim);
			if (pcSearch != NULL) {
				++iCount;
				pcSearch += strlen(pcDelim);	// skip by the length of the delimter
			}
		}
		// reset back to starting place
		pcSearch = vParams[0].szValue();
		break;
	default:
		iCount = -1;
	}

	// Find any delimiters?
	if (iCount <= 0)
	{
		// Create array of 2 to return count = 1 and full string in element[1]
		Util_VariantArrayDim(&vResult, 2);
		pvTemp = Util_VariantArrayGetRef(&vResult, 0);	//First element
		*pvTemp = 1;

		pvTemp = Util_VariantArrayGetRef(&vResult, 1);	//Second element
		*pvTemp = pcSearch;

		SetFuncErrorCode(-iCount+1);	// 1 for no delimiters, 2 for bad flag
		return AUT_OK;
	}


	// Number of string elements will be iCount number of delimiters + 1 (3 delims = 4 elements to return)
	iElements = iCount+1;

	// Create our return array (first element is number of strings return, so we need iElements+1 elements)
	Util_VariantArrayDim(&vResult, iElements+1);

	pvTemp = Util_VariantArrayGetRef(&vResult, 0);	//First element
	*pvTemp = iElements;							// Number of elements we will return


	// Go through the loop again but this time, make a note of the strings and store in our new array
	iCount	= 0;
	nPos	= 0;
	iIndex	= 1;
	sElement = "";
	bool bStored;
	while (iCount < iElements)
	{
		switch (nFlag)
		{
		case 0:
			bStored = false;
			if ( strchr(pcDelim, pcSearch[nPos]) )	// This works as \0 is also matched so overrun is prevented
			{
				pvTemp = Util_VariantArrayGetRef(&vResult, iIndex++);	//Next element
				*pvTemp = sElement.c_str();
				sElement = "";

				bStored = true;
				++iCount;						// Increase count
			}

			if (bStored == false)
				sElement += pcSearch[nPos];

			++nPos;
			break;
		case 1:
			pcTmp = pcSearch;
			pcSearch = strstr(pcSearch, pcDelim);
			if (pcSearch != NULL) {
				sElement = "";

				// copy all elements up to the beginning of the delimiter into sElement
				for (;pcTmp != pcSearch; ++pcTmp)
					sElement += *pcTmp;

				pcSearch += strlen(pcDelim);	// skip by the length of the delimter
			}
			else
				sElement = pcTmp;
			// copy string into array
			pvTemp = Util_VariantArrayGetRef(&vResult, iIndex++);	//Next element
			*pvTemp = sElement.c_str();
			++iCount;
			break;
		}	// switch
	}	// while (iCount < iElements)

	return AUT_OK;

} // StringSplit()


///////////////////////////////////////////////////////////////////////////////
// StringStripWS()
//
// This is a little bit inefficient as the input and output string will be
// constantly changing sizes with the astring class, but makes for easy reading :)
//
// $var = StringStripWS(<string>, <flags>)
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_StringStripWS(VectorVariant &vParams, Variant &vResult)
{
	int				i, flags=vParams[1].nValue();
	register char	*pszTmp;

	if (flags == 0) {
		// no changes
		vResult = vParams[0];
		return AUT_OK;
	} else if (vParams[0].isArray()) {
		SetFuncErrorCode(1);	// invalid type
		return AUT_OK;
	}

	vResult = vParams[0];

	if (flags & 8) {	// strip all WS
		pszTmp = Util_StrCpyAlloc(vResult.szValue());

		for (i=0; pszTmp[i];) {
			if (Util_IsSpace(pszTmp[i]))
				for(int k=i; pszTmp[k]; ++k)	// copy characters up
					pszTmp[k]=pszTmp[k+1];
			else
				++i;
		}
		vResult = pszTmp;
		delete [] pszTmp;
		return AUT_OK;	// no other checks needed
	}
	if (flags & 1) {	// strip left
		pszTmp = Util_StrCpyAlloc(vResult.szValue());

		for (i=0; pszTmp[i] && Util_IsSpace(pszTmp[i]); ++i);	// loop until not whitespace
		if (i>0)
			vResult = pszTmp + i;	// pointer to beginning of string + i elements
		delete [] pszTmp;
	}
	if (flags & 2) {	// strip right
		pszTmp = Util_StrCpyAlloc(vResult.szValue());
		strcpy(pszTmp, vResult.szValue());
		for (i=(int)strlen(pszTmp)-1; i>=0 && Util_IsSpace(pszTmp[i]); --i);	// loop until not whitespace
		if (pszTmp[i+1]) {	// if I am not looking at the end of the string
			pszTmp[i+1] = '\0';	// set to be end of string;
			vResult = pszTmp;
		}
		delete [] pszTmp;
	}
	if (flags & 4) {	// strip double spaces
		pszTmp = Util_StrCpyAlloc(vResult.szValue());

		for (i=1; pszTmp[i];) {
			if (Util_IsSpace(pszTmp[i]) && Util_IsSpace(pszTmp[i-1]))
				for(int k=i; pszTmp[k]; ++k)	// copy characters up
					pszTmp[k]=pszTmp[k+1];
			else
				++i;
		}
		vResult = pszTmp;
		delete [] pszTmp;
	}
	return AUT_OK;

} // StringStripWS()


///////////////////////////////////////////////////////////////////////////////
// StringIsFloat()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_StringIsFloat(VectorVariant &vParams, Variant &vResult)
{
	vResult = (int)StringIsFloat(vParams[0]);
	return AUT_OK;

} // StringIsFloat()


///////////////////////////////////////////////////////////////////////////////
// StringIsFloat()
//
// $var = StringIsFloat(<var|expre>)
//
///////////////////////////////////////////////////////////////////////////////

bool AutoIt_Script::StringIsFloat(Variant &vValue)
{
	int pos=0;
	const char *pszValue = vValue.szValue();

	// manually built finite-state machine
	enum parse_state {
		ps_begin=1, ps_whole_digit, ps_decimal, ps_fraction_digit
	} state=ps_begin;

	if (pszValue == NULL || *pszValue == '\0')	// if string is NULL or empty;
		return false;
	else if (strcmp(pszValue, ".")==0 || strcmp(pszValue, "+.")==0 || strcmp(pszValue, "-.")==0)
		return false;

	while (pszValue[pos]!='\0') {
		switch (state) {
		case ps_begin:
			switch (pszValue[pos]) {
			case '-': case '+':
				++pos;	// move to next character
			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
				state=ps_whole_digit;
				break;
			case '.':
				state=ps_decimal;
				break;
			default:
				return false;
			}
			break;
		case ps_whole_digit:
			if (isdigit(pszValue[pos]))	// is it a digit?
				++pos;
			else if (pszValue[pos]=='.')
				state=ps_decimal;
			else
				return false;
			break;
		case ps_decimal:
			++pos;
			state=ps_fraction_digit;
			break;
		case ps_fraction_digit:
			if (isdigit(pszValue[pos]))
				++pos;
			else
				return false;
			break;
		} // switch state
	} // while
	return (state == ps_fraction_digit);

} // StringIsFloat()


///////////////////////////////////////////////////////////////////////////////
// StringIsInt()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_StringIsInt(VectorVariant &vParams, Variant &vResult)
{
	vResult = (int)StringIsInt(vParams[0]);
	return AUT_OK;

} // StringIsInt()


///////////////////////////////////////////////////////////////////////////////
// StringIsInt()
//
// $var = StringIsInt(<var|expre>)
//
///////////////////////////////////////////////////////////////////////////////

bool AutoIt_Script::StringIsInt(Variant &vValue)
{
	int			pos = 0;
	const char *pszValue = vValue.szValue();

	// manually built finite-state machine
	enum parse_state {
		ps_begin=1, ps_digit
	} state=ps_begin;

	if (pszValue == NULL || *pszValue == '\0')	// if string is NULL or empty;
		return false;
	else if (strcmp(pszValue, "+")==0 || strcmp(pszValue, "-")==0)
		return false;

	while (pszValue[pos]!='\0') {
		switch (state) {
		case ps_begin:
			switch (pszValue[pos]) {
			case '-': case '+':
				++pos;	// move to next character
			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
				state=ps_digit;
				break;
			default:
				return false;
			}
			break;
		case ps_digit:
			if (isdigit(pszValue[pos]))	// is it a digit?
				++pos;
			else
				return false;
			break;
		} // switch state
	} // while
	return true;

} // StringIsInt


///////////////////////////////////////////////////////////////////////////////
// StringFormat()
// String variables are limited to 65535 as part of the buffer overrun protection
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_StringFormat(VectorVariant &vParams, Variant &vResult)
{
	uint		iNumParam = vParams.size();
	AString		sOutput(1024);					// Pre-allocated string for final output (pre-allocated 1KB - enough for most operations)
	char		szTempOutput[65535+1];			// Temporary output for each var (limited to 65535 chars)
	char		szTemp[65535+1];				// Temp variable for buffer overrun protection
	char		*szFormat;
	char		*szDelim;
	char		ch;

	int			n, j;

	int i = 0;			// to scan Format specification

	// Make a copy of the format spec - we can't just use a pointer to the vParam because
	// we modify it and that is NOT allowed (it is a const) but the compiler was not warning about
	// it in this case
	szFormat = Util_StrCpyAlloc(vParams[0].szValue());

	for (n=1; n<=(int)iNumParam; ++n)				// to loop once more for concatenating after last format specification
	{											// Append this to our output while not '%'
		j = 0;
		szTempOutput[0] = '\0';					// Zero our temp output

		ch = szFormat[i++];
		while (ch != '%' && ch != '\\' && ch != '\0')
		{
			szTempOutput[j++] = ch;
			ch = szFormat[i++];
		}
		szTempOutput[j] = '\0';					// Terminate
		sOutput += szTempOutput;				// Add to final output

		// If the last ch read was \0 then exit - we must do this before reading the next
		// char or we will be reading beyond the end of the input!
		if (ch == '\0')
			break;

		// If we are here then last char was %, next char is either % (%%) or a flag
		if (szFormat[i] != '%' && ch != '\\')
		{
			// Format flag - have we used too many parameters?
			if (n == (int)iNumParam)
				break;			// no more param or end of Format specification

			// retrieve Format specification
			szDelim = strpbrk(&szFormat[i], "diouxXeEfgGs");
			if (szDelim != NULL)
			{
				ch = szDelim[1];				// save char after end of Format specification
				szDelim[1] = '\0';

				switch (szDelim[0])
				{
					case 'd':
					case 'i':
					case 'o':
					case 'u':
					case 'x':
					case 'X':
						sprintf(szTempOutput, &szFormat[i-1], vParams[n].nValue());
						break;

					case 'e':
					case 'E':
					case 'f':
					case 'g':
					case 'G':
						sprintf(szTempOutput, &szFormat[i-1], vParams[n].fValue());
						break;

					case 's':
						strncpy(szTemp, vParams[n].szValue(), 65535);
						szTemp[65535] = '\0';		// Make sure it is terminated if larger than 65535
						sprintf(szTempOutput, &szFormat[i-1], szTemp);
						break;

				}


				i = (int)strlen(szFormat);		// skip Format specification
				szDelim[1] = ch;				// restore next char
			}
		}
		else	// case %%
		{
			--n;								// no param used
			switch (szFormat[i])
			{
				case '%':
					szTempOutput[0] = '%';
					break;
				case 'n':
					szTempOutput[0] = 10;
					break;
				case 'r':
					szTempOutput[0] = 13;
					break;
				case 't':
					szTempOutput[0] = 9;
					break;
				case '\\':
					szTempOutput[0] = '\\';
					break;
			}
			++i;								// Skip %
			szTempOutput[1] = '\0';				// Add a single %
		}

		// Add temp output to final - szTempOutput will be cleared at start of next loop
		sOutput += szTempOutput;
	}

	delete [] szFormat;							// Clean up

	vResult =  sOutput.c_str();					// Assign the final output
	return AUT_OK;

} // StringFormat()


///////////////////////////////////////////////////////////////////////////////
// StringIsAlpha()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_StringIsAlpha(VectorVariant &vParams, Variant &vResult)
{
	uchar *pChar;

	vResult = 0;						// Set default
	pChar = (uchar *)(vParams[0].szValue());
	if (*pChar == '\0')	// empty string.  Not valid
			return AUT_OK;
	for (; *pChar != '\0'; ++pChar)
		if (!IsCharAlpha(*pChar))
			return AUT_OK;
	// entire string read.  It passed.
	vResult = 1;

	return AUT_OK;

} // StringIsAlpha()


///////////////////////////////////////////////////////////////////////////////
// StringIsAlnum()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_StringIsAlnum(VectorVariant &vParams, Variant &vResult)
{
	uchar *pChar;

	vResult = 0;						// Set default
	pChar = (uchar *)(vParams[0].szValue());
	if (*pChar == '\0')	// empty string.  Not valid
			return AUT_OK;
	for (; *pChar != '\0'; ++pChar)
		if (!IsCharAlphaNumeric(*pChar))
			return AUT_OK;
	// entire string read.  It passed.
	vResult = 1;
	return AUT_OK;

} // StringIsAlnum()


///////////////////////////////////////////////////////////////////////////////
// StringIsDigit()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_StringIsDigit(VectorVariant &vParams, Variant &vResult)
{
	uchar *pChar;

	vResult = 0;						// Set default
	pChar = (uchar *)(vParams[0].szValue());
	if (*pChar == '\0')	// empty string.  Not valid
		return AUT_OK;
	for (; *pChar != '\0'; ++pChar)
		if (!isdigit(*pChar))
			return AUT_OK;
	// entire string read.  It passed.
	vResult = 1;
	return AUT_OK;

} // StringIsDigit()


///////////////////////////////////////////////////////////////////////////////
// StringIsXDigit()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_StringIsXDigit(VectorVariant &vParams, Variant &vResult)
{
	uchar *pChar;

	vResult = 0;						// Set default
	pChar = (uchar *)(vParams[0].szValue());
	if (*pChar == '\0')	// empty string.  Not valid
			return AUT_OK;
	for (; *pChar != '\0'; ++pChar)
		if (!isxdigit(*pChar))
			return AUT_OK;
	// entire string read.  It passed.
	vResult = 1;
	return AUT_OK;

} // StringIsXDigit()


///////////////////////////////////////////////////////////////////////////////
// StringIsLower()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_StringIsLower(VectorVariant &vParams, Variant &vResult)
{
	uchar *pChar;

	vResult = 0;						// Set default
	pChar = (uchar *)(vParams[0].szValue());
	if (*pChar == '\0')	// empty string.  Not valid
			return AUT_OK;
	for (; *pChar != '\0'; ++pChar)
		if (!IsCharLower(*pChar))
			return AUT_OK;
	// entire string read.  It passed.
	vResult = 1;
	return AUT_OK;

} // StringIsLower()


///////////////////////////////////////////////////////////////////////////////
// StringIsUpper()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_StringIsUpper(VectorVariant &vParams, Variant &vResult)
{
	uchar *pChar;

	vResult = 0;						// Set default
	pChar = (uchar *)(vParams[0].szValue());
	if (*pChar == '\0')	// empty string.  Not valid
			return AUT_OK;
	for (; *pChar != '\0'; ++pChar)
		if (!IsCharUpper(*pChar))
			return AUT_OK;
	// entire string read.  It passed.
	vResult = 1;
	return AUT_OK;

} // StringIsUpper()


///////////////////////////////////////////////////////////////////////////////
// StringIsSpace()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_StringIsSpace(VectorVariant &vParams, Variant &vResult)
{
	uchar *pChar;

	vResult = 0;						// Set default
	pChar = (uchar *)(vParams[0].szValue());
	if (*pChar == '\0')	// empty string.  Not valid
		return AUT_OK;
	for (; *pChar != '\0'; ++pChar)
		if (!Util_IsSpace(*pChar))
			return AUT_OK;
	// entire string read.  It passed.
	vResult = 1;
	return AUT_OK;

} // StringIsSpace()


///////////////////////////////////////////////////////////////////////////////
// StringIsASCII()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_StringIsASCII(VectorVariant &vParams, Variant &vResult)
{
	uchar *pChar;

	vResult = 0;						// Set default
	pChar = (uchar *)(vParams[0].szValue());
	if (*pChar == '\0')	// empty string.  Not valid
			return AUT_OK;
	for (; *pChar != '\0'; ++pChar)
		if (!isascii(*pChar))
			return AUT_OK;
	// entire string read.  It passed.
	vResult = 1;
	return AUT_OK;

} // StringIsASCII()

