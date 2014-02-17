#ifndef __ASTRING_H
#define __ASTRING_H

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
// astring_datatype.h
//
// The standalone class for a String datatype.  Not using STL because of the
// code bloat.
//
///////////////////////////////////////////////////////////////////////////////


// Includes
#ifndef _MSC_VER								// Includes for non-MS compilers
	#include <windows.h>
	#include <limits.h>
#endif


class AString
{
public:
	// Functions
	AString();									// Constructor (blank string)
	AString(const char *szStr);					// Constructor (with string)
	AString(int iLen);							// Constructor with pre-allocation
	AString(const AString &sSource);			// Copy constructor
	~AString() { delete m_szText; }				// Destructor (inline)

	void		erase(void);					// Erase entire string
	void		erase(int nStart, int nEnd);	// Erase a range from the string
	void		erase(int nStart);				// Erase a from start to end

	const char*	c_str(void) const { return m_szText; }	// Return C style string
	void		tolower(void) { CharLower(m_szText); }	// Convert string to lowercase
	void		toupper(void) { CharUpper(m_szText); }	// Convert string to uppercase
	void		assign(const AString &sStr,
					int nStart, int nEnd);				// Assign chars from one str to another

	int			find_first_not_of(const char *szInput) const;							// Find first char not in input
	int			find_last_not_of(const char *szInput) const;							// Find last char not in input
	int			find_first_of(const char *szInput) const;								// Find first char in input
	int			find_last_of(const char *szInput) const;								// Find last char in input
	int			find_str(const char *szInput, bool bCaseSense, int nOccurance=1) const;	// Find given string in input

	void		strip_leading(const char *szInput);		// Strip leading chars
	void		strip_trailing(const char *szInput);	// Strip trailing chars

	int			strcmp(const AString &sStr) const		// Use C strcmp() routine (compare with AString)
					{ return ::strcmp(m_szText, sStr.m_szText); }

	int			strcmp(const char *szStr) const			// Use C strcmp() routine (compare with C string)
					{ return ::strcmp(m_szText, szStr); }

	int			stricmp(const AString &sStr) const		// Use C stricmp() routine (compare with AString)
					{ return ::stricmp(m_szText, sStr.m_szText); }

	int			stricmp(const char *szStr) const		// Use C stricmp() routine (compare with C string)
					{ return ::stricmp(m_szText, szStr); }


	// Properties
	int			length(void) const { return m_length; }	// Return string length
	bool		empty(void) const;						// Tests if string empty

	// Substrings
	AString		left(int nSize) const;						// left-most characters
	AString		right(int nSize) const;						// right-most characters
	AString		mid(int nStart, int nSize = INT_MAX) const;	// middle characters

	// Overloads
	char&		operator[](int nIndex);					// Overloaded []
	char		operator[](int nIndex) const;			// Overloaded [] for const
	AString&	operator=(const AString &sOp2);			// Overloaded = for AString
	AString&	operator=(const char *szOp2);			// Overloaded = for C string
	AString&	operator=(const char ch);				// Overloaded = for a single char
	AString&	operator+=(const AString &sOp2);		// Overloaded += for AString (append)
	AString&	operator+=(const char *szOp2);			// Overloaded += for C string (append)
	AString&	operator+=(const char ch);				// Overloaded += for a single char (append)
	AString		operator+(const AString &sOp2) const;	// Overloaded + for AString (append)
	AString		operator+(const char ch) const;			// Overloaded + for a single char (append)

	friend bool	operator==(const AString &sOp1, const AString &sOp2);	// Overloaded == for AStrings (case sensitive)
	friend bool	operator!=(const AString &sOp1, const AString &sOp2);	// Overloaded != for AStrings (case sensitive)
	friend bool	operator <(const AString &sOp1, const AString &sOp2);	// Overloaded < for AStrings (case sensitive)
	friend bool	operator<=(const AString &sOp1, const AString &sOp2);	// Overloaded <= for AStrings (case sensitive)
	friend bool	operator >(const AString &sOp1, const AString &sOp2);	// Overloaded > for AStrings (case sensitive)
	friend bool	operator>=(const AString &sOp1, const AString &sOp2);	// Overloaded >= for AStrings (case sensitive)


private:
	// Variables
	char		*m_szText;						// The actual string of characters
	int			m_length;						// String length
	int			m_allocated;					// Number of bytes allocated for this string
};

///////////////////////////////////////////////////////////////////////////////

#endif
