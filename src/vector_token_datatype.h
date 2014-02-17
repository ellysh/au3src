#ifndef __VECTOR_TOKEN_H
#define __VECTOR_TOKEN_H

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
// vector_token_datatype.h
//
// The class for a token vector datatype.  Not using STL because of the
// code bloat.
//
///////////////////////////////////////////////////////////////////////////////


// Includes
#include "token_datatype.h"


// Structs for stack node
typedef struct _VectorTokenNode
{
	Token					tItem;			// The item
	struct _VectorTokenNode	*lpNext;		// Next node (or NULL)

} VectorTokenNode;


class VectorToken
{
public:
	// Functions
	VectorToken();							// Constructor
	VectorToken(const VectorToken &vSource);// Copy Constructor
	~VectorToken();							// Destructor

	void			push_back(const Token &tItem);	// Push item onto end of vector
	void			clear(void);					// Deletes all items


	// Properties
	unsigned int	size(void) const { return m_nItems; }	// Return number of items on stack
	bool			empty(void) const;						// Tests if stack empty


	// Overloads
	Token&			operator[](unsigned int nIndex);// Overloaded []
	VectorToken&	operator=(VectorToken &vOp2);	// Overloaded = for vectortokens


private:
	// Variables
	unsigned int	m_nItems;				// Number of items on stack
	VectorTokenNode	*m_lpFirst;				// Pointer to first node
	VectorTokenNode	*m_lpLast;				// Pointer to last node
	Token			m_tNull;

};

///////////////////////////////////////////////////////////////////////////////

#endif
