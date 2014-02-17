#ifndef __STACK_VARIABLELIST_H
#define __STACK_VARIABLELIST_H

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
// stack_variable_list.h
//
// The class for a stack of VariableList classes.
//
///////////////////////////////////////////////////////////////////////////////


// Includes
#ifndef _MSC_VER								// Includes for non-MS compilers
	#include <stdio.h>
#endif

#include "variable_list.h"


// Structs for stack node
typedef struct _StackVarListNode
{
	VariableList				*lpList;		// The item (a variable list class)
	struct _StackVarListNode	*lpPrev;		// Previous node (or NULL)

} StackVarListNode;


class StackVarList
{
public:
	// Functions
	StackVarList();								// Constructor
	~StackVarList();							// Destructor

	void	push(void);							// Create/push empty list item onto stack
	void	pop(void);							// Pop and free top list item from stack

	// Properties
	VariableList*	top(void);					// Get pointer to the top item from stack
	int				size(void) const			// Return number of items on stack
		{ return m_nItems; }
	bool			empty(void) const			// Tests if stack empty
		{ return (m_lpTop == NULL); }


private:
	// Variables
	int		m_nItems;							// Number of items on stack
	StackVarListNode	*m_lpTop;				// Pointer to top node
};

///////////////////////////////////////////////////////////////////////////////

#endif
