#ifndef __STACK_VARIANT_H
#define __STACK_VARIANT_H

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
// stack_variant_datatype.h
//
// The class for Variant stack datatype.  Not using STL because of the
// code bloat.
//
///////////////////////////////////////////////////////////////////////////////


// Includes
#include "variant_datatype.h"


// Structs for stack node
typedef struct _StackVariantNode
{
	Variant						vItem;			// The item
	struct _StackVariantNode	*lpPrev;		// Previous node (or NULL)

} StackVariantNode;


class StackVariant
{
public:
	// Functions
	StackVariant();								// Constructor
	~StackVariant();							// Destructor

	void			push(const Variant &vItem);	// Push item onto stack
	void			pop(void);					// Pop item from stack

	// Properties
	Variant&		top(void);								// Get top item from stack
	unsigned int	size(void) const { return m_nItems; }	// Return number of items on stack
	bool			empty(void) const;						// Tests if stack empty


	// Overloads

private:
	// Variables
	unsigned int		m_nItems;				// Number of items on stack
	StackVariantNode	*m_lpTop;				// Pointer to top node
	Variant				m_vNull;				// Dummy value to return on error
};

///////////////////////////////////////////////////////////////////////////////

#endif
