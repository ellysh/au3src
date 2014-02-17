
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
// stack_variant_datatype.cpp
//
// The class for a variant stack datatype.  Not using STL because of the
// code bloat.
//
///////////////////////////////////////////////////////////////////////////////


// Includes
#include "StdAfx.h"								// Pre-compiled headers

#ifndef _MSC_VER								// Includes for non-MS compilers
	#include <stdio.h>
#endif

#include "stack_variant_datatype.h"


///////////////////////////////////////////////////////////////////////////////
// Constructor()
///////////////////////////////////////////////////////////////////////////////

StackVariant::StackVariant() : m_nItems(0), m_lpTop(NULL)
{

} // StackVariant()


///////////////////////////////////////////////////////////////////////////////
// Destructor()
///////////////////////////////////////////////////////////////////////////////

StackVariant::~StackVariant()
{
	StackVariantNode	*lpTemp, *lpTemp2;

	lpTemp = m_lpTop;

	while(lpTemp != NULL)
	{
		lpTemp2 = lpTemp->lpPrev;
		delete lpTemp;
		lpTemp = lpTemp2;
	}

} // ~StackVariant()


///////////////////////////////////////////////////////////////////////////////
// push()
// Pushes a value onto the stack
///////////////////////////////////////////////////////////////////////////////

void StackVariant::push(const Variant &vItem)
{
	StackVariantNode	*lpTemp;

	// Create a new node
	lpTemp			= new StackVariantNode;
	lpTemp->vItem	= vItem;

	// Add it to the top
	if (m_lpTop)
		lpTemp->lpPrev = m_lpTop;
	else
		lpTemp->lpPrev = NULL;

	m_lpTop	= lpTemp;
	m_nItems++;

} // push()


///////////////////////////////////////////////////////////////////////////////
// pop()
// Pops a value from the stack
///////////////////////////////////////////////////////////////////////////////

void StackVariant::pop(void)
{
	StackVariantNode	*lpTemp;

	if (m_lpTop)
	{
		lpTemp	= m_lpTop->lpPrev;
		delete m_lpTop;
		m_lpTop = lpTemp;
		m_nItems--;
	}
	else
		return;

} // pop()


///////////////////////////////////////////////////////////////////////////////
// empty()
// Returns true if the stack is empty
///////////////////////////////////////////////////////////////////////////////

bool StackVariant::empty(void) const
{
	if (m_nItems)
		return false;
	else
		return true;

} // empty()


///////////////////////////////////////////////////////////////////////////////
// top()
// Returns top value from stack (or 0 if non exists)
///////////////////////////////////////////////////////////////////////////////

Variant& StackVariant::top(void)
{
	if (!m_nItems)
		return m_vNull;							// Returns a dummy variant
	else
		return m_lpTop->vItem;

} // top()

