#ifndef __TOKEN_H
#define __TOKEN_H

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
// token_datatype.h
//
// The class for a token datatype (requires variant_datatype).
// Functions are called via operator overloading!
//
///////////////////////////////////////////////////////////////////////////////


// Includes

// Token types
#define	TOK_UNDEFINED		-1					// Used when initally created and all values are rubbish

#define TOK_KEYWORD			0					// If, else...
#define TOK_FUNCTION		1					// chr, user, send, winwait...
#define TOK_EQUAL			2					// =
#define TOK_GREATER			3					// >
#define TOK_LESS			4					// <
#define TOK_NOTEQUAL		5					// <>
#define TOK_GREATEREQUAL	6					// >=
#define TOK_LESSEQUAL		7					// <=
#define TOK_LEFTPAREN		8					// (
#define TOK_RIGHTPAREN		9					// )
#define TOK_PLUS			10					// +
#define TOK_MINUS			11					// -
#define TOK_DIV				12					// /
#define TOK_MULT			13					// *
#define TOK_VARIABLE		14					// var ($var) (STRING based type)
#define TOK_END				15					// End of tokens
#define TOK_USERFUNCTION	16					// User defined function (STRING based type)
#define TOK_COMMA			17					// ,
#define TOK_CONCAT			18					// &
#define TOK_LEFTSUBSCRIPT	19					// [
#define TOK_RIGHTSUBSCRIPT	20					// ]
#define TOK_MACRO			21					// predefined var (@var) (STRING based type)
#define TOK_STRINGEQUAL		22					// ==
#define TOK_POW				23					// ^

#define TOK_STRING			24
#define TOK_INT32			25
#define TOK_INT64			26
#define TOK_DOUBLE			27

class Token
{
public:
	// Functions
	Token();									// Constructor
	Token(const Token &vOp2);					// Copy constructor
	~Token();									// Destructor
	void		settype(int nType);				// Set new type
	bool		isliteral(void);				// Returns true if the token is a literal (string, int, int64, double)

	Token&		operator=(const Token &vOp2);	// Overloaded = for tokens
	Token&		operator=(const char *szStr);	// Overloaded = for C strings

	// Variables
	// Total size is 16 bytes per token + any string length
	int 		m_nType;						// Token type
	int			m_nCol;							// Column number this token came from

	union
	{
		int		nValue;
		__int64	n64Value;
		double	fValue;
		char	*szValue;
	};
};

///////////////////////////////////////////////////////////////////////////////

#endif
