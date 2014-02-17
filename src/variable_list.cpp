
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
// variable_list.cpp
//
// A list of named variables.  (Case sensitive!)
//
///////////////////////////////////////////////////////////////////////////////


// Includes
#include "StdAfx.h"								// Pre-compiled headers

#ifndef _MSC_VER								// Includes for non-MS compilers
	#include <stdio.h>
#endif

#include "variable_list.h"


///////////////////////////////////////////////////////////////////////////////
// Constructor()
///////////////////////////////////////////////////////////////////////////////

VariableList::VariableList() : m_lpRoot(NULL)
{

} // VariableList()


///////////////////////////////////////////////////////////////////////////////
// Destructor()
///////////////////////////////////////////////////////////////////////////////

VariableList::~VariableList()
{
	removenode(m_lpRoot);

} // ~VariableList()


///////////////////////////////////////////////////////////////////////////////
// removenode()
// Removes the given node and any children
///////////////////////////////////////////////////////////////////////////////

void VariableList::removenode(VarNode *lpRoot)
{
	if (lpRoot == NULL)
		return;

	if (lpRoot->lpLeft)
		removenode(lpRoot->lpLeft);
	if (lpRoot->lpRight)
		removenode(lpRoot->lpRight);

	// All children deleted, now delete this node

	// Do we need to free the memory for the variant, if it is a reference var then
	// we don't have to as its owner will do it, if it is a normal variant then we
	// must do it.
	if (lpRoot->nType == VARTABLE_VARIANT)
		delete lpRoot->pvVariant;			// Delete the variant

	delete [] lpRoot->szName;				// Delete the string name
	delete lpRoot;							// Delete the node itself

} // removenode()


///////////////////////////////////////////////////////////////////////////////
// findvarnode()
// Lookup a variable and if found return the pointer to the NODE
///////////////////////////////////////////////////////////////////////////////

VarNode* VariableList::findvarnode(const char *szName)
{
	VarNode	*lpTemp = m_lpRoot;
	int	nRes;

	while (lpTemp != NULL)
	{
		nRes = strcmp(lpTemp->szName, szName);

		if (nRes < 0)
			lpTemp = lpTemp->lpLeft;			// Less than (left child)
		else if (nRes > 0)
			lpTemp = lpTemp->lpRight;			// Greater than (right child)
		else
			return lpTemp;						// Found
	}

	// Not found
	return NULL;

} // findvarnode()


///////////////////////////////////////////////////////////////////////////////
// findvar()
// Lookup a variable name and if found return the pointer to the actual VARIANT data
///////////////////////////////////////////////////////////////////////////////

Variant* VariableList::findvar(const char *szName, bool &bConst)
{
	VarNode *lpTemp = findvarnode(szName);

	if (lpTemp)
	{
		bConst = lpTemp->bConst;
		return lpTemp->pvVariant; 			// Found!  Return a pointer to it
	}
	else
		return NULL;

} // findvar()


///////////////////////////////////////////////////////////////////////////////
// addvar()
// Add or update a variant to the list
///////////////////////////////////////////////////////////////////////////////

void VariableList::addvar(const char *szName, const Variant &vVar, bool bConst)
{
	VarNode	*lpTemp = findvarnode(szName);

	// Does this variable already exist?
	if (lpTemp)
	{
		// Found, update entry
		*(lpTemp->pvVariant) = vVar;
		return;
	}

	// Not found, add a new node
	lpTemp				= new VarNode;

	lpTemp->szName		= new char[strlen(szName)+1];
	strcpy(lpTemp->szName, szName);

	lpTemp->nType		= VARTABLE_VARIANT;
	lpTemp->pvVariant	= new Variant;
	*(lpTemp->pvVariant)= vVar;
	lpTemp->bConst		= bConst;
	lpTemp->lpLeft		= NULL;
	lpTemp->lpRight		= NULL;

	addnode(szName, lpTemp);

} // addvar()


///////////////////////////////////////////////////////////////////////////////
// addref()
// Add or update a reference to a variant to the list
///////////////////////////////////////////////////////////////////////////////

void VariableList::addref(const char *szName, Variant *pvVar)
{
	VarNode	*lpTemp = findvarnode(szName);

	// Does this variable already exist?
	if (lpTemp)
	{
		// Found, update entry
		lpTemp->pvVariant = pvVar;
		return;
	}

	// Not found, add a new node
	lpTemp				= new VarNode;

	lpTemp->szName		= new char[strlen(szName)+1];
	strcpy(lpTemp->szName, szName);

	lpTemp->nType		= VARTABLE_REFERENCE;
	lpTemp->pvVariant	= pvVar;				// Instead add a reference
	lpTemp->bConst		= false;
	lpTemp->lpLeft		= NULL;
	lpTemp->lpRight		= NULL;

	// Add this node to the tree
	addnode(szName, lpTemp);

} // addref()


///////////////////////////////////////////////////////////////////////////////
// addnode()
// Add a new node to the tree.
// ASSUMES THE NAME DOES NOT ALREADY EXIST IN THE TREE.  MUST CHECK BEFORE
// CALLING THIS FUNCTION!
///////////////////////////////////////////////////////////////////////////////

void VariableList::addnode(const char *szName, VarNode *lpNewNode)
{
	if (m_lpRoot == NULL)
	{
		// Very first entry
		m_lpRoot = lpNewNode;
		return;
	}

	// Not the root entry, Find a place in the tree to add this node
	// Note we only have to check for < and > because we know from above that
	// the exact entry does NOT exist
	VarNode	*lpCur = m_lpRoot;
	int		nRes;

	for (;;)
	{
		nRes = strcmp(lpCur->szName, szName);

		if (nRes < 0)
		{
			// Less than (left child)
			// Another node to check or do we add the new node here?
			if (lpCur->lpLeft)
				lpCur = lpCur->lpLeft;			// Next node
			else
			{
				lpCur->lpLeft = lpNewNode;		// Add the node here
				break;
			}
		}
		else
		{
			// Greater than (right child)
			if (lpCur->lpRight)
				lpCur = lpCur->lpRight;			// Next node
			else
			{
				lpCur->lpRight = lpNewNode;		// Add the node here
				break;
			}
		}
	}

} // addnode()

