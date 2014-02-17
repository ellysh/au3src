
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
// vector_token_datatype.cpp
//
// The class for a token vector datatype.  Not using STL because of the
// code bloat.
//
///////////////////////////////////////////////////////////////////////////////


// Includes
#include "StdAfx.h"								// Pre-compiled headers

#ifndef _MSC_VER								// Includes for non-MS compilers
	#include <stdio.h>
#endif

#include "vector_variant_datatype.h"


///////////////////////////////////////////////////////////////////////////////
// Constructor()
///////////////////////////////////////////////////////////////////////////////

VectorVariant::VectorVariant() : m_nItems(0), m_lpFirst(NULL), m_lpLast(NULL)
{

} // VectorVariant()


///////////////////////////////////////////////////////////////////////////////
// Copy Constructor()
///////////////////////////////////////////////////////////////////////////////

VectorVariant::VectorVariant(const VectorVariant &vSource) : m_nItems(0), m_lpFirst(NULL), m_lpLast(NULL)
{
	VectorVariantNode	*lpTemp;

	lpTemp = vSource.m_lpFirst;
	while(lpTemp != NULL)
	{
		push_back(lpTemp->vItem);
		lpTemp = lpTemp->lpNext;
	}
}


///////////////////////////////////////////////////////////////////////////////
// Destructor()
///////////////////////////////////////////////////////////////////////////////

VectorVariant::~VectorVariant()
{
	VectorVariantNode	*lpTemp, *lpTemp2;

	lpTemp = m_lpFirst;

	while(lpTemp != NULL)
	{
		lpTemp2 = lpTemp->lpNext;
		delete lpTemp;
		lpTemp = lpTemp2;
	}

} // ~VectorVariant()


///////////////////////////////////////////////////////////////////////////////
// Overloaded operator[]()
///////////////////////////////////////////////////////////////////////////////

Variant& VectorVariant::operator[](unsigned int nIndex)
{
	// Check bounds
	if (nIndex >= m_nItems)
	{
//		Variant vTemp;
//		vTemp = 0;
//		return vTemp;							// return 0
		return m_vNull;							// Return our dummy variant
	}


	// Loop through and return the correct token
	unsigned int	i;
	VectorVariantNode	*lpTemp;

	lpTemp = m_lpFirst;

	for (i=0; i<nIndex; i++)
		lpTemp = lpTemp->lpNext;

	return lpTemp->vItem;

} // operator[]()


///////////////////////////////////////////////////////////////////////////////
// push_back()
// Pushes a value onto the end of the vector
///////////////////////////////////////////////////////////////////////////////

void VectorVariant::push_back(const Variant &vItem)
{
	VectorVariantNode	*lpTemp;

	// Create a new node
	lpTemp			= new VectorVariantNode;
	lpTemp->vItem	= vItem;
	lpTemp->lpNext	= NULL;

	// Add it to the end of the vector
	if (m_lpLast)
	{
		m_lpLast->lpNext	= lpTemp;
		m_lpLast			= lpTemp;
	}
	else
	{
		// First entry
		m_lpFirst = m_lpLast = lpTemp;
	}

	m_nItems++;

} // push_back()


///////////////////////////////////////////////////////////////////////////////
// empty()
// Returns true if the stack is empty
///////////////////////////////////////////////////////////////////////////////

bool VectorVariant::empty(void) const
{
	if (m_nItems)
		return false;
	else
		return true;

} // empty()


///////////////////////////////////////////////////////////////////////////////
// clear()
// Clears all items in the vector
///////////////////////////////////////////////////////////////////////////////

void VectorVariant::clear(void)
{
	VectorVariantNode	*lpTemp, *lpTemp2;

	lpTemp = m_lpFirst;

	while(lpTemp != NULL)
	{
		lpTemp2 = lpTemp->lpNext;
		delete lpTemp;
		lpTemp = lpTemp2;
	}

	m_nItems	= 0;
	m_lpFirst	= NULL;
	m_lpLast	= NULL;

} // clear()
