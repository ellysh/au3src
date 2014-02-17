#ifndef __STACK_INT_H
#define __STACK_INT_H

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
// stack_int_datatype.h
//
// The class for an int stack datatype.  Not using STL because of the
// code bloat.
//
///////////////////////////////////////////////////////////////////////////////


// Structs for stack node
typedef struct _StackIntNode
{
	int						nItem;				// The item
	struct _StackIntNode	*lpPrev;			// Previous node (or NULL)

} StackIntNode;


class StackInt
{
public:
	// Functions
	StackInt();									// Constructor
	~StackInt();								// Destructor

	void			push(const int &nItem);		// Push item onto stack
	void			pop(void);					// Pop item from stack

	// Properties
	int&			top(void);								// Get top item from stack
	unsigned int	size(void) const { return m_nItems; }	// Return number of items on stack
	bool			empty(void) const;						// Tests if stack empty


	// Overloads

private:
	// Variables
	unsigned int	m_nItems;					// Number of items on stack
	StackIntNode	*m_lpTop;					// Pointer to top node
	int				m_nNull;					// Dummy return value
};

///////////////////////////////////////////////////////////////////////////////

#endif
