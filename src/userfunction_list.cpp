
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
// userfunction_list.cpp
//
// The list that is used to store details of user functions.
//
///////////////////////////////////////////////////////////////////////////////


// Includes
#include "StdAfx.h"								// Pre-compiled headers

#ifndef _MSC_VER								// Includes for non-MS compilers
	#include <stdio.h>
	#include <string.h>
#endif

#include "userfunction_list.h"


///////////////////////////////////////////////////////////////////////////////
// Constructor()
///////////////////////////////////////////////////////////////////////////////

UserFuncList::UserFuncList() : m_lpFirst(NULL), m_lpLast(NULL), m_Index(NULL), m_nNumItems(0)
{

} // UserFuncList()


///////////////////////////////////////////////////////////////////////////////
// Destructor()
///////////////////////////////////////////////////////////////////////////////

UserFuncList::~UserFuncList()
{
	// Delete the index
	if (m_Index)
		delete [] m_Index;


	// Delete the actual entries
	UserFuncListNode	*lpTemp, *lpTemp2;

	lpTemp = m_lpFirst;

	while(lpTemp != NULL)
	{
		lpTemp2 = lpTemp->lpNext;
		delete lpTemp;
		lpTemp = lpTemp2;
	}

} // ~UserFuncList()


///////////////////////////////////////////////////////////////////////////////
// add()
// Add a userfunction node (doesn't check for duplicates...)
///////////////////////////////////////////////////////////////////////////////

void UserFuncList::add(const UserFuncDetails &uItem)
{
	// If there is an index then no more additions are possible
	if (m_Index)
		return;

	UserFuncListNode	*lpTemp;

	// Create a new node
	lpTemp			= new UserFuncListNode;
	lpTemp->uItem	= uItem;
	lpTemp->uItem.sName.toupper();				// Make sure funcname is stored as upper case
	lpTemp->lpNext	= NULL;

// Check that default assignment/copy constructor is indeed creating a new copy with different memory!
//	char szText[100];
//	sprintf(szText, "%p", &uItem.sName);
//	MessageBox(NULL, szText, uItem.sName.c_str(), MB_OK);
//	sprintf(szText, "%p", &lpTemp->uItem.sName);
//	MessageBox(NULL, szText, lpTemp->uItem.sName.c_str(), MB_OK);


	// Add it to the end of the list
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

	// Increase number of items
	++m_nNumItems;

} // add()


///////////////////////////////////////////////////////////////////////////////
// find()
// Lookup a function name and if found return the pointer to the details.
// Once the first find operation is performed no more additions to the list
// can be made.
///////////////////////////////////////////////////////////////////////////////

UserFuncDetails* UserFuncList::find(AString sName)
{
	if (m_nNumItems == 0)
		return NULL;							// No items to search!

	// Convert the search name to upper case (everything stored as upper case)
	sName.toupper();

	// Make sure we have an index
	if (m_Index == NULL)
	{
		// No index, slow search
		UserFuncListNode	*lpTemp = m_lpFirst;

		while(lpTemp != NULL)
		{
			if ( lpTemp->uItem.sName == sName)
				return &lpTemp->uItem;			// Found, return the UserFuncDetails
			else
				lpTemp = lpTemp->lpNext;
		}

		return NULL;
	}


	// Has index and the index is sorted so we do a quick binary search
	// The built-in function list is in alphabetical order so we can do a binary search :)
	int nFirst = 0;
	int nLast = m_nNumItems - 1;
	int nRes, i;

	while (nFirst <= nLast)
	{
		i = (nFirst + nLast) / 2;			// Truncated to an integer!

		nRes = strcmp( sName.c_str(), m_Index[i]->sName.c_str() );

		if ( nRes < 0 )
			nLast = i - 1;
		else if ( nRes > 0 )
			nFirst = i + 1;
		else
			break;								// nRes == 0
	}

	if ( nFirst <= nLast )
		return m_Index[i];
	else
		return NULL;

} // find()


///////////////////////////////////////////////////////////////////////////////
// createindex()
// Create an index and then sort it for quick searching
///////////////////////////////////////////////////////////////////////////////

void UserFuncList::createindex(void)
{
	if (m_nNumItems < 6)
		return;									// Don't bother doing an index for < 6 items :)

	int i;

	// Create an array/index of pointers
	m_Index = new UserFuncDetails*[m_nNumItems];

	UserFuncListNode	*lpTemp = m_lpFirst;
	for (i = 0; i < m_nNumItems; ++i)
	{
		m_Index[i] = &lpTemp->uItem;			// Get direct pointer to the UserFuncDetails
		lpTemp = lpTemp->lpNext;				// Next
	}


	// Bubble sort the list - yes, it is a crap sort but it is the smallest and
	// speed is NOT really important here
	bool bSwapOccured = true;
	UserFuncDetails *lpFuncTemp;

	while (bSwapOccured)
	{
		bSwapOccured = false;
		for (i = 0; i < (m_nNumItems-1); ++i)
		{
			if (m_Index[i]->sName > m_Index[i+1]->sName)
			{
				bSwapOccured = true;
				lpFuncTemp = m_Index[i];
				m_Index[i] = m_Index[i+1];
				m_Index[i+1] = lpFuncTemp;
			}
		}
	}

} // createindex()
