#ifndef __USERFUNCLIST_H
#define __USERFUNCLIST_H

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
// userfunction_list.h
//
// The list that is used to store details of user functions.
//
///////////////////////////////////////////////////////////////////////////////


// Includes
#include "astring_datatype.h"


typedef struct
{
	AString sName;								// Name of the user defined function
	int		nFuncLineNum;						// Line number of Func keyword (0=unused)
	int		nNumParams;							// Number of parameters this function has
	int		nNumParamsMin;						// Min Number of parameters this function has
	int		nEndFuncLineNum;					// Line number of the EndFunc keyword
} UserFuncDetails;


typedef struct _UserFuncListNode
{
	UserFuncDetails	uItem;
	struct _UserFuncListNode	*lpNext;		// Next node (or NULL)

} UserFuncListNode;


class UserFuncList
{
public:
	// Functions
	UserFuncList();								// Constructor
	~UserFuncList();							// Destructor

	void				add(const UserFuncDetails &uItem);	// Add item to the list
	UserFuncDetails*	find(AString sName);
	void				createindex(void);					// Creates the index and sorts the list - NO MORE ADDITIONS POSSIBLE

private:
	// Variables
	UserFuncListNode	*m_lpFirst;				// Pointer to first node
	UserFuncListNode	*m_lpLast;				// Pointer to last node

	UserFuncDetails		**m_Index;
	int					m_nNumItems;

};

///////////////////////////////////////////////////////////////////////////////

#endif
