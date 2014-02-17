
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
// token_datatype.cpp
//
// The class for a token datatype (requires variant_datatype).
// Functions are called via operator overloading!
//
///////////////////////////////////////////////////////////////////////////////


// Includes
#include "StdAfx.h"								// Pre-compiled headers

#ifndef _MSC_VER								// Includes for non-MS compilers
	#include <stdio.h>							// Needed for __int64 reference in variant
	#include <string.h>
#endif

#include "token_datatype.h"


///////////////////////////////////////////////////////////////////////////////
// Constructor()
///////////////////////////////////////////////////////////////////////////////

Token::Token() : m_nType(TOK_UNDEFINED)
{
} // Token()


///////////////////////////////////////////////////////////////////////////////
// Copy Constructor
///////////////////////////////////////////////////////////////////////////////

Token::Token(const Token &vOp2)
{
	m_nType		= vOp2.m_nType;
	m_nCol		= vOp2.m_nCol;

	if (m_nType == TOK_STRING || m_nType == TOK_VARIABLE || m_nType == TOK_USERFUNCTION || m_nType == TOK_MACRO)
	{
		szValue = new char[strlen(vOp2.szValue)+1];
		strcpy(szValue, vOp2.szValue);
	}
	else if (m_nType == TOK_INT64)
		n64Value = vOp2.n64Value;
	else if (m_nType == TOK_DOUBLE)
		fValue = vOp2.fValue;
	else
		nValue = vOp2.nValue;
}


///////////////////////////////////////////////////////////////////////////////
// Destructor()
///////////////////////////////////////////////////////////////////////////////

Token::~Token()
{
	if (m_nType == TOK_STRING || m_nType == TOK_VARIABLE || m_nType == TOK_USERFUNCTION || m_nType == TOK_MACRO)
		delete [] szValue;

} // ~Token()


///////////////////////////////////////////////////////////////////////////////
// Overloaded operator=() for tokens
///////////////////////////////////////////////////////////////////////////////

Token& Token::operator=(const Token &vOp2)
{
	if (this != &vOp2)
	{
		if (m_nType == TOK_STRING || m_nType == TOK_VARIABLE || m_nType == TOK_USERFUNCTION || m_nType == TOK_MACRO)
			delete [] szValue;

		m_nType		= vOp2.m_nType;
		m_nCol		= vOp2.m_nCol;

		if (m_nType == TOK_STRING || m_nType == TOK_VARIABLE || m_nType == TOK_USERFUNCTION || m_nType == TOK_MACRO)
		{
			szValue = new char[strlen(vOp2.szValue)+1];
			strcpy(szValue, vOp2.szValue);
		}
		else if (m_nType == TOK_INT64)
			n64Value = vOp2.n64Value;
		else if (m_nType == TOK_DOUBLE)
			fValue = vOp2.fValue;
		else
			nValue = vOp2.nValue;
	}

	return *this;								// Return this object that generated the call

} // operator=()


///////////////////////////////////////////////////////////////////////////////
// Overloaded operator=() for C strings
//
// Use carefully AFTER setting the type to a string based type!
///////////////////////////////////////////////////////////////////////////////

Token& Token::operator=(const char *szStr)
{
	if (m_nType == TOK_STRING || m_nType == TOK_VARIABLE || m_nType == TOK_USERFUNCTION || m_nType == TOK_MACRO)
		delete [] szValue;

	szValue = new char[strlen(szStr)+1];
	strcpy(szValue, szStr);

	return *this;								// Return this object that generated the call

} // operator=()


///////////////////////////////////////////////////////////////////////////////
// Set the type of a token - will clean up any previous.  Use before assigning data to the token.
///////////////////////////////////////////////////////////////////////////////

void Token::settype(int nType)
{
	// If we are currently a string-based type - cleanup
	if (m_nType == TOK_STRING || m_nType == TOK_VARIABLE || m_nType == TOK_USERFUNCTION || m_nType == TOK_MACRO)
	{
		delete [] szValue;
		szValue = NULL;
	}

	m_nType = nType;

	if (m_nType == TOK_STRING || m_nType == TOK_VARIABLE || m_nType == TOK_USERFUNCTION || m_nType == TOK_MACRO)
		szValue = NULL;
	else if (m_nType == TOK_INT64)
		n64Value = 0;
	else if (m_nType == TOK_DOUBLE)
		fValue = 0.0;
	else
		nValue = 0;

} // settype()


///////////////////////////////////////////////////////////////////////////////
// isliteral()
///////////////////////////////////////////////////////////////////////////////

bool Token::isliteral(void)
{
	if (m_nType == TOK_STRING || m_nType == TOK_INT32 || m_nType == TOK_INT64 || m_nType == TOK_DOUBLE || m_nType == TOK_MACRO)
		return true;
	else
		return false;

} // isliteral()
