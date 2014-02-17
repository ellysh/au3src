
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
// stack_variable_list.cpp
//
// The class for a stack of VariableList classes.
//
///////////////////////////////////////////////////////////////////////////////


// Includes
#include "StdAfx.h"								// Pre-compiled headers

#ifndef _MSC_VER								// Includes for non-MS compilers
	#include <stdio.h>
#endif

#include "stack_variable_list.h"


///////////////////////////////////////////////////////////////////////////////
// Constructor()
///////////////////////////////////////////////////////////////////////////////

StackVarList::StackVarList() : m_nItems(0), m_lpTop(NULL)
{

} // StackVarList()


///////////////////////////////////////////////////////////////////////////////
// Destructor()
///////////////////////////////////////////////////////////////////////////////

StackVarList::~StackVarList()
{
	StackVarListNode	*lpTemp, *lpTemp2;

	lpTemp = m_lpTop;

	while(lpTemp != NULL)
	{
		lpTemp2 = lpTemp->lpPrev;
		delete lpTemp->lpList;					// Delete list entry
		delete lpTemp;							// Delete the actual node
		lpTemp = lpTemp2;
	}

} // ~StackVarList()


///////////////////////////////////////////////////////////////////////////////
// push()
// creates a new blank top entry
///////////////////////////////////////////////////////////////////////////////

void StackVarList::push(void)
{
	StackVarListNode	*lpTemp;

	// Create a new node, and a new blank list entry
	lpTemp			= new StackVarListNode;
	lpTemp->lpList	= new VariableList;

	// Add it to the top
	if (m_lpTop)
		lpTemp->lpPrev = m_lpTop;
	else
		lpTemp->lpPrev = NULL;					// First entry

	m_lpTop	= lpTemp;
	m_nItems++;

} // push()


///////////////////////////////////////////////////////////////////////////////
// pop()
// Pops a value from the stack
///////////////////////////////////////////////////////////////////////////////

void StackVarList::pop(void)
{
	StackVarListNode	*lpTemp;

	if (m_lpTop)
	{
		lpTemp	= m_lpTop->lpPrev;
		delete m_lpTop->lpList;					// Delete list entry
		delete m_lpTop;							// Delete the actual node
		m_lpTop = lpTemp;
		m_nItems--;
	}
	else
		return;

} // pop()


///////////////////////////////////////////////////////////////////////////////
// top()
// Returns top value from stack (or 0 if non exists)
///////////////////////////////////////////////////////////////////////////////

VariableList* StackVarList::top(void)
{
	if (!m_nItems)
		return NULL;
	else
		return m_lpTop->lpList;

} // top()

