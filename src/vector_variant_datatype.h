#ifndef __VECTOR_VARIANT_H
#define __VECTOR_VARIANT_H

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
#include "variant_datatype.h"


// Structs for stack node
typedef struct _VectorVariantNode
{
	Variant						vItem;			// The item
	struct _VectorVariantNode	*lpNext;		// Next node (or NULL)

} VectorVariantNode;


class VectorVariant
{
public:
	// Functions
	VectorVariant();							// Constructor
	VectorVariant(const VectorVariant &vSource);// Copy Constructor
	~VectorVariant();							// Destructor

	void			push_back(const Variant &tItem);	// Push item onto end of vector
	void			clear(void);						// Deletes all items


	// Properties
	unsigned int	size(void) const { return m_nItems; }	// Return number of items on stack
	bool			empty(void) const;						// Tests if stack empty


	// Overloads
	Variant&		operator[](unsigned int nIndex);		// Overloaded []

private:
	// Variables
	unsigned int		m_nItems;				// Number of items on stack
	VectorVariantNode	*m_lpFirst;				// Pointer to first node
	VectorVariantNode	*m_lpLast;				// Pointer to last node
	Variant				m_vNull;				// Dummy variant to return if [] is out of bounds
};

///////////////////////////////////////////////////////////////////////////////

#endif
