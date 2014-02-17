#ifndef __STACK_STATEMENT_H
#define __STACK_STATEMENT_H

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
// stack_statement_datatype.h
//
// The class for a general statement stack datatype.  Not using STL because of the
// code bloat.
//
// Although it is intended for loops, the IF statement also uses a single stack
// of this type, the loops (while, do, for) share a single stack.
//
///////////////////////////////////////////////////////////////////////////////

// Includes
#include "variant_datatype.h"


// statement types
enum
{
	L_IF, L_WHILE, L_DO, L_FOR, L_SELECT,
	L_FUNC
};


// Using a simple struct and relying on the default copy constructor to be sufficient -
// should be ok, just using ints (no copy cons required) and Variants (has a copy constructor)
typedef struct _GenStatement
{
	int		nType;								// L_IF, L_WHILE, etc

	// The different types of structure that this is used for, a union can be used
	// to reduce space as only one set will be used at once for the "start" and "end"
	union
	{
		int		nIf;							// Line numbers for IF/ELSE/ENDIF
		int		nLoopStart;						// Start and End linenumbers of loop (while.wend.do.until.for.next)
		int		nSelect;						// Line numbers for SELECT/ENDSELECT
		int		nFunc;							// Line numbers for Func/EndFunc
	};
	union
	{
		int		nEndIf;							// Line numbers for IF/ELSE/ENDIF
		int		nLoopEnd;						// Start and End linenumbers of loop (while.wend.do.until.for.next)
		int		nEndSelect;						// Line numbers for SELECT/ENDSELECT
		int		nEndFunc;						// Line numbers for Func/EndFunc
	};

	// Specifics for FOR loops
	Variant	vForTo, vForStep;					// To and Step values for a "for" loop

} GenStatement;


// Structs for stack node
typedef struct _StackStatementNode
{
	GenStatement				Item;			// The item
	struct _StackStatementNode	*lpPrev;		// Previous node (or NULL)

} StackStatementNode;


class StackStatement
{
public:
	// Functions
	StackStatement();							// Constructor
	~StackStatement();							// Destructor

	void			push(const GenStatement &Item);	// Push item onto stack
	void			pop(void);						// Pop item from stack

	// Properties
	GenStatement&	top(void);								// Get top item from stack
	unsigned int	size(void) const { return m_nItems; }	// Return number of items on stack
	bool			empty(void) const;						// Tests if stack empty


	// Overloads

private:
	// Variables
	unsigned int		m_nItems;				// Number of items on stack
	StackStatementNode	*m_lpTop;				// Pointer to top node
	GenStatement		m_sNull;				// Dummy return value
};

///////////////////////////////////////////////////////////////////////////////

#endif
