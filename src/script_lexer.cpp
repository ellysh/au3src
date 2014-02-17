
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
// script_lexer.cpp
//
// Provides lexing (string into tokens).  Part of script.cpp
//
///////////////////////////////////////////////////////////////////////////////


// Includes
#include "StdAfx.h"								// Pre-compiled headers

#ifndef _MSC_VER								// Includes for non-MS compilers
	#include <windows.h>
	#include <limits.h>
#endif

#include "AutoIt.h"								// Autoit values, macros and config options

#include "script.h"
#include "resources\resource.h"
#include "utility.h"


///////////////////////////////////////////////////////////////////////////////
// Lexer()
//
// Converts the current line of script into tokens
//
// Uses static members to reduce object creation time (significant in large loops)
// Two tokens are used, one for storing "easy" tokens and the other for
// strings (when you store a variant the variant reallocates mem = slow)
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::Lexer(int nLineNum, const char *szLine, VectorToken &vLineToks)
{

#ifdef AUT_CONFIG_LEXERCACHE
	int	nCacheIdx = nLineNum & AUT_LEXER_CACHEMASK;

	// Do we have a cached copy of this line?
	if (m_LexerCache[nCacheIdx].nLineNum == nLineNum)
	{
		// Yes, copy the tokens
		vLineToks = m_LexerCache[nCacheIdx].vLine;

		return AUT_OK;
	}
#endif

	uint			iPos = 0;					// Position in the string
	uint			iPosTemp;
	char			ch;
	Token			tok;						// Token variable must be recreated before use as it has limited housekeeping
	static char		szTemp[AUT_MAX_LINESIZE+1];	// Static and preallocated to speed things up

	// Clear the line tokens vector
	vLineToks.clear();

	// The main lex loop
	while (szLine[iPos] != '\0')
	{
		// Skip whitespace
		while (szLine[iPos] == ' ' || szLine[iPos] == '\t')
			++iPos;

		// Reached the end?
		if (szLine[iPos] == '\0')
			break;

		// Save the column number of the token (used in error output)
		tok.m_nCol = iPos;

		// Zero our temp string position
		iPosTemp = 0;
		ch = szLine[iPos];						// Get the current character

		// Check if a function or keyword first (most common so speeds things up)
		if ( ((ch >= '0' && ch <= '9') || ch == '.') && Lexer_Number(szLine, iPos, tok, szTemp) == true )
		{
			// A number
			vLineToks.push_back(tok);
			continue;							// Next loop
		}
		else if ( (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || ch == '_' )
		{
			// keyword or function
			Lexer_KeywordOrFunc(szLine, iPos, tok, szTemp);
			vLineToks.push_back(tok);
			continue;							// Next loop
		}

		// Not a keyword/function, see what it is
		switch (ch)
		{
			case ';':
				// End when we reach a comment
				tok.settype(TOK_END);
				vLineToks.push_back(tok);
				return AUT_OK;

			case '$':							// Variable ($var)
				++iPos;							// Skip $

				ch = szLine[iPos];
				while ( (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
						(ch >= '0' && ch <= '9') || ch == '_')
				{
					szTemp[iPosTemp++] = ch;
					++iPos;
					ch = szLine[iPos];
				}
				szTemp[iPosTemp] = '\0';		// Terminate

				if (iPosTemp == 0)				// No variable given!
				{
					FatalError(IDS_AUT_E_VARBADFORMAT, iPos-1);
					return AUT_ERR;
				}

				tok.settype(TOK_VARIABLE);
				tok = szTemp;
				vLineToks.push_back(tok);
				break;

			case '@':							// Macro variable (@var)
				++iPos;

				ch = szLine[iPos];
				while ( (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
						(ch >= '0' && ch <= '9') || ch == '_')
				{
					szTemp[iPosTemp++] = ch;
					++iPos;
					ch = szLine[iPos];
				}
				szTemp[iPosTemp] = '\0';		// Terminate

				if (iPosTemp == 0)				// No macro given!
				{
					FatalError(IDS_AUT_E_VARBADFORMAT, iPos-1);
					return AUT_ERR;
				}

				tok.settype(TOK_MACRO);
				tok = szTemp;
				vLineToks.push_back(tok);
				break;

			case '"':
			case '\'':
				if ( AUT_FAILED( Lexer_String(szLine, iPos, szTemp) ) )
				{
					tok.settype(TOK_END);		// Add an end token for safety if someone tries to use this vector
					vLineToks.push_back(tok);
					return AUT_ERR;				// Abort
				}

				tok.settype(TOK_STRING);
				tok = szTemp;
				vLineToks.push_back(tok);
				break;

			case '+':
				tok.settype(TOK_PLUS);
				vLineToks.push_back(tok);
				++iPos;
				break;

			case '-':
				tok.settype(TOK_MINUS);
				vLineToks.push_back(tok);
				++iPos;
				break;

			case '/':
				tok.settype(TOK_DIV);
				vLineToks.push_back(tok);
				++iPos;
				break;

			case '^':
				tok.settype(TOK_POW);
				vLineToks.push_back(tok);
				++iPos;
				break;

			case '*':
				tok.settype(TOK_MULT);
				vLineToks.push_back(tok);
				++iPos;
				break;

			case '(':
				tok.settype(TOK_LEFTPAREN);
				vLineToks.push_back(tok);
				++iPos;
				break;

			case ')':
				tok.settype(TOK_RIGHTPAREN);
				vLineToks.push_back(tok);
				++iPos;
				break;

			case '=':
				++iPos;
				ch = szLine[iPos];
				switch (ch)
				{
					case '=':
						tok.settype(TOK_STRINGEQUAL);
						++iPos;
						break;

					default:
						tok.settype(TOK_EQUAL);
						break;
				}
				vLineToks.push_back(tok);
				break;

			case ',':
				tok.settype(TOK_COMMA);
				vLineToks.push_back(tok);
				++iPos;
				break;

			case '&':
				tok.settype(TOK_CONCAT);
				vLineToks.push_back(tok);
				++iPos;
				break;

			case '[':
				tok.settype(TOK_LEFTSUBSCRIPT);
				vLineToks.push_back(tok);
				++iPos;
				break;

			case ']':
				tok.settype(TOK_RIGHTSUBSCRIPT);
				vLineToks.push_back(tok);
				++iPos;
				break;

			case '<':
				++iPos;
				ch = szLine[iPos];
				switch (ch)
				{
					case '>':
						tok.settype(TOK_NOTEQUAL);
						++iPos;
						break;

					case '=':
						tok.settype(TOK_LESSEQUAL);
						++iPos;
						break;

					default:
						tok.settype(TOK_LESS);
						break;
				}
				vLineToks.push_back(tok);
				break;

			case '>':
				++iPos;
				ch = szLine[iPos];
				switch (ch)
				{
					case '=':
						tok.settype(TOK_GREATEREQUAL);
						++iPos;
						break;

					default:
						tok.settype(TOK_GREATER);
						break;
				}
				vLineToks.push_back(tok);
				break;


			default:
				// no match with anything - not good
				FatalError(IDS_AUT_E_GENPARSE, iPos);
				tok.settype(TOK_END);		// Add an end token for safety if someone tries to use this vector
				vLineToks.push_back(tok);
				return AUT_ERR;				// Abort

		} // End switch

	} // End while

	// Add the "end" token
	tok.settype(TOK_END);
	//tok.m_nCol = (int)strlen(szLine);
	tok.m_nCol = iPos;
	vLineToks.push_back(tok);

#ifdef AUT_CONFIG_LEXERCACHE
	// Cache this lexed line for future use
	m_LexerCache[nCacheIdx].nLineNum = nLineNum;
	m_LexerCache[nCacheIdx].vLine = vLineToks;

#endif

	return AUT_OK;

} // Lexer()


///////////////////////////////////////////////////////////////////////////////
// Lexer_String()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::Lexer_String(const char *szLine, uint &iPos, char *szResult)
{
	uint	iPosTemp = 0;
	bool	bComplete = false;

	// Found a " or ', skip to the next " or ' (ignore double quotes)

	char chComment = szLine[iPos];					// Get the comment char ' or "
	uint iPosStart = iPos;							// Store the first position

	++iPos;

	while (szLine[iPos] != '\0')
	{
		if (szLine[iPos] == chComment)
		{
			if ( szLine[iPos+1] != chComment)
			{
				++iPos;
				bComplete = true;
				break;								// String is terminated
			}
			else
			{
				// Double quote, just store a single quote
				szResult[iPosTemp++] = szLine[iPos];
				iPos = iPos + 2;
			}
		}
		else
		{
			szResult[iPosTemp++] = szLine[iPos];
			++iPos;
		}
	}

	if (bComplete == false)
	{
		FatalError(IDS_AUT_E_MISSINGQUOTE, iPosStart);
		return AUT_ERR;
	}

	// Terminate
	szResult[iPosTemp] = '\0';

	return AUT_OK;

} // Lexer_String()


///////////////////////////////////////////////////////////////////////////////
// Lexer_Number()
///////////////////////////////////////////////////////////////////////////////

bool AutoIt_Script::Lexer_Number(const char *szLine, uint &iPos, Token &rtok, char *szTemp)
{
	uint	iPosTemp = 0;

	if ( (szLine[iPos] == '0') && (szLine[iPos+1] == 'x' || szLine[iPos+1] == 'X') )
	{
		// Hex number
		int		nTemp;

		iPos += 2;								// Skip "0x"
		while ( (szLine[iPos] >= '0' && szLine[iPos] <= '9') || (szLine[iPos] >= 'a' && szLine[iPos] <= 'f') || (szLine[iPos] >= 'A' && szLine[iPos] <= 'F') )
			szTemp[iPosTemp++] = szLine[iPos++];

		szTemp[iPosTemp] = '\0';				// Terminate
		rtok.settype(TOK_INT32);
		//sscanf(szTemp, "%x", &nTemp);			// strtol doesn't cope with 0xffffffff = -1 (although sscanf adds 4KB to the code! :()
		if (Util_ConvDec(szTemp, nTemp) == false)
			return false;

		rtok.nValue = nTemp;
	}
	else
	{
		// float or integer
		bool	bFloat = false;

		enum  { L_DIGIT = 1, L_COMMA = 2, L_EXP = 4, L_SIGN = 8, L_MORE = 16, L_OK = 32 };
		uint  iState = (L_DIGIT | L_COMMA | L_EXP | L_MORE);
		for (;;)
		{
			switch (szLine[iPos])
			{
				case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
					// test not needed - digit acceptable in all states.
					iState = (iState & ~L_MORE) | L_OK; // remove more bit - no more chars required
					break;
				case '.':
					if (iState & L_COMMA) iState = (L_DIGIT | L_EXP | L_OK);
					bFloat = true;
					break;
				case 'e': case 'E': // more bit should be unset
					if ( (iState & (L_EXP | L_MORE)) == L_EXP ) iState = (L_DIGIT | L_SIGN | L_MORE | L_OK);
					break;
				case '+': case '-':
					if (iState & L_SIGN) iState = (L_DIGIT | L_MORE | L_OK);
					break;
			}

			if (iState & L_OK)
				szTemp[iPosTemp++] = szLine[iPos++];
			else if (iState & L_MORE)
				return false;					// results in a GENPARSE error
			else
				break; // done
			iState &= ~L_OK;
		}

		szTemp[iPosTemp] = '\0';              // Terminate

		if (bFloat)
		{
			rtok.settype(TOK_DOUBLE);
			rtok.fValue = atof(szTemp);
		}
		else
		{
			__int64 n64Temp = _atoi64(szTemp);
			if (n64Temp > INT_MAX || n64Temp < INT_MIN)
			{
				rtok.settype(TOK_INT64);
				rtok.n64Value  = n64Temp;		// Store as int64
			}
			else
			{
				rtok.settype(TOK_INT32);
				rtok.nValue  = atoi(szTemp);	// Store as int32
			}
		}
	}

	return true;								// Valid number

} // Lexer_Number()


///////////////////////////////////////////////////////////////////////////////
// Lexer_KeywordOrFunc()
///////////////////////////////////////////////////////////////////////////////

void AutoIt_Script::Lexer_KeywordOrFunc(const char *szLine, uint &iPos, Token &rtok, char *szTemp)
{
	int		i;
	uint	iPosTemp = 0;

	char ch = szLine[iPos];
	while ( (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
			(ch >= '0' && ch <= '9') || ch == '_')
	{
		szTemp[iPosTemp++] = ch;
		++iPos;
		ch = szLine[iPos];
	}

	szTemp[iPosTemp] = '\0';					// Terminate
	CharUpper(szTemp);							// Convert to upper case to speed up comparisons (strcmp rather than stricmp)

	// Is it a valid keyword?
	i = 0;
	while (i < K_MAX)
	{
		if (!strcmp(m_szKeywords[i], szTemp) )
		{
			// It's a keyword
			rtok.settype(TOK_KEYWORD);
			rtok.nValue = i;						// Order of enum in script.h must match order of list
			return;
		}

		++i;
	}


	// The built-in function list is in alphabetical order so we can do a binary search :)
	int nFirst = 0;
	int nLast = m_nFuncListSize - 1;
	int nRes;

	while (nFirst <= nLast)
	{
		i = (nFirst + nLast) / 2;			// Truncated to an integer!

		nRes = strcmp(szTemp, m_FuncList[i].szName);

		if ( nRes < 0 )
			nLast = i - 1;
		else if ( nRes > 0 )
			nFirst = i + 1;
		else
			break;								// nRes == 0
	}

	if ( nFirst <= nLast )
	{
		rtok.settype(TOK_FUNCTION);
		rtok.nValue = i;					// Save function index
	}
	else
	{
		// Invalid built in function, must be user function
		rtok.settype(TOK_USERFUNCTION);
		rtok = szTemp;
	}

} // Lexer_KeywordOrFunc()

