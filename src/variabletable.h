#ifndef __VARIABLETABLE_H
#define __VARIABLETABLE_H

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
// variabletable.h
//
// A class for managing AutoIt variables
//
// Note: The AutoIt lexer only allows 0-9, a-z and _ in variable names so other
// characters will never appear in a variable name.  But, we may internally store
// variables with other names in the table for special uses.  (Such as @ExitMethod)
//
///////////////////////////////////////////////////////////////////////////////


// Includes
#include "stack_variable_list.h"

// Magic numbers used in assigning/getting vars, sometimes we want force local/global operations
#define VARTABLE_ANY			0				// Any type (local first, then global)
#define VARTABLE_FORCELOCAL		1				// Force local only scope
#define VARTABLE_FORCEGLOBAL	2				// Force global only scope
#define VARTABLE_RESIZE			4				// Resize the array in the variable list.

class VariableTable
{
private:
	VariableList	m_Globals;					// global variables
	StackVarList	m_Locals;					// local variables

public:
	// Functions
	bool	Assign(AString sVarName, const Variant &vVariant, bool bConst = false, int nReqScope = VARTABLE_ANY);	// Assign variable
	bool	GetRef(AString sVarName, Variant **pvVariant, bool &bConst, int nReqScope = VARTABLE_ANY);		// Get pointer to a variable
	bool	CreateRef(AString sRefName, Variant *pvVariant);									// Create a reference variable (alt)

	void	ScopeIncrease(void);				// Increase scope (user function call)
	void	ScopeDecrease(void);				// Decrease scope (return from user function)
	int		isDeclared(AString sVarName);		// Return true if the reference variable exists (and type of variable, global/local etc)
	bool	IsGlobalLevel(void)					// Returns true when vartable is in base/global state
				{ return m_Locals.empty(); }
};

///////////////////////////////////////////////////////////////////////////////

#endif

