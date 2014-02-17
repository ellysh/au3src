
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
// variabletable.cpp
//
// A class for managing AutoIt variables
//
///////////////////////////////////////////////////////////////////////////////


// Includes
#include "StdAfx.h"								// Pre-compiled headers

#ifndef _MSC_VER								// Includes for non-MS compilers
	#include <windows.h>
#endif

#include "variabletable.h"


///////////////////////////////////////////////////////////////////////////////
// Assign()
///////////////////////////////////////////////////////////////////////////////

bool VariableTable::Assign(AString sVarName, const Variant &vVariant, bool bConst, int nReqScope)
{
	sVarName.toupper();							// Always use uppercase to force case insensitive operation

	switch (nReqScope)
	{
		case VARTABLE_ANY:
			if (m_Locals.empty())
				m_Globals.addvar(sVarName.c_str(), vVariant, bConst);
			else
				m_Locals.top()->addvar(sVarName.c_str(), vVariant, bConst);
			break;

		case VARTABLE_FORCELOCAL:
			if (m_Locals.empty())
				m_Globals.addvar(sVarName.c_str(), vVariant, bConst);
			else
				m_Locals.top()->addvar(sVarName.c_str(), vVariant, bConst);
			break;

		case VARTABLE_FORCEGLOBAL:
			m_Globals.addvar(sVarName.c_str(), vVariant, bConst);
			break;
	}

	return true;

} // Assign()


///////////////////////////////////////////////////////////////////////////////
// GetRef()
///////////////////////////////////////////////////////////////////////////////

bool VariableTable::GetRef(AString sVarName, Variant **pvVariant, bool &bConst, int nReqScope)
{
	Variant *lpVar = NULL;

	sVarName.toupper();							// Always use uppercase to force case insensitive operation

	switch (nReqScope)
	{
		case VARTABLE_ANY:
			if (!m_Locals.empty())
				lpVar = m_Locals.top()->findvar(sVarName.c_str(), bConst);

			if (lpVar == NULL)
				lpVar = m_Globals.findvar(sVarName.c_str(), bConst);

			break;

		case VARTABLE_FORCELOCAL:
			if (!m_Locals.empty())
				lpVar = m_Locals.top()->findvar(sVarName.c_str(), bConst);
			else
				lpVar = m_Globals.findvar(sVarName.c_str(), bConst);

			break;

		case VARTABLE_FORCEGLOBAL:
			lpVar = m_Globals.findvar(sVarName.c_str(), bConst);
			break;
	}

	*pvVariant = lpVar;

	return (lpVar != NULL);

} // GetRef()


///////////////////////////////////////////////////////////////////////////////
// CreateRef()
//
// Designed to be used just after a ScopeIncrease() call to create a reference
// to a variable ONE SCOPE LEVEL LOWER than current (or global scope)
//
// Be careful when using references that the variant you are referencing
// actually exists!  If proper use of ScopeIncrease() Decrease() is done this
// should be OK.
//
// Create a reference to a variant (variant pointer is explicitly given).
//
///////////////////////////////////////////////////////////////////////////////

bool VariableTable::CreateRef(AString sRefName, Variant *pvVariant)
{
	sRefName.toupper();							// Always use uppercase to force case insensitive operation

	// If not in a local function, then can't execute
	if (m_Locals.empty() || pvVariant == NULL)
		return false;

	m_Locals.top()->addref(sRefName.c_str(), pvVariant);

	return true;

} // CreateRef()


///////////////////////////////////////////////////////////////////////////////
// ScopeIncrease()
///////////////////////////////////////////////////////////////////////////////

void VariableTable::ScopeIncrease(void)
{
	m_Locals.push();

} // ScopeIncrease()


///////////////////////////////////////////////////////////////////////////////
// ScopeDecrease()
///////////////////////////////////////////////////////////////////////////////

void VariableTable::ScopeDecrease(void)
{
	m_Locals.pop();

} // ScopeDecrease()


///////////////////////////////////////////////////////////////////////////////
// isDeclared()
//
// Returns true if this variant match a declared variable
//
///////////////////////////////////////////////////////////////////////////////

int VariableTable::isDeclared(AString sVarName)
{
	bool bConst = false;

	sVarName.toupper();							// Always use uppercase to force case insensitive operation

	// Look in local variables
	if (!(m_Locals.empty()) && m_Locals.top()->findvar(sVarName.c_str(), bConst) != NULL)
		return -1;								// Local

	// Look in global variables
	if (m_Globals.findvar(sVarName.c_str(), bConst) != NULL)
		return 1;								// Global

	// not found at all
	return 0;

} // isDeclared()

