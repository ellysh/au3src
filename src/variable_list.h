#ifndef __VARIABLELIST_H
#define __VARIABLELIST_H

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
// variable_list.h
//
// A list of named variables.  (Case sensitive!)
//
///////////////////////////////////////////////////////////////////////////////


// Includes
#include "astring_datatype.h"
#include "variant_datatype.h"

#define VARTABLE_VARIANT		0				// Type: Variant
#define VARTABLE_REFERENCE		1				// Type: Reference to a variant

/*
typedef struct _VarEntry
{
	AString			sName;						// Name of this variable
	int				nType;						// Variant OR reference to a variant
	Variant			*pvVariant;					// Pointer to a variant (either a reference to another, or one we create!)
	bool			bConst;						// True is this is a const

	_VarEntry		*lpNext;					// Next entry in linked list (or NULL)

} VarEntry;
*/

typedef struct _VarNode
{
	char			*szName;					// Name of this variable
	int				nType;						// Variant OR reference to a variant
	Variant			*pvVariant;					// Pointer to a variant (either a reference to another, or one we create!)
	bool			bConst;						// True is this is a const

	_VarNode		*lpLeft, *lpRight;			// Left/Right nodes, or NULL if no child

} VarNode;



class VariableList
{
public:
	// Functions
	VariableList();								// Constructor
	~VariableList();							// Destructor

	void		addvar(const char *szName, const Variant &vVar, bool bConst);	// Add/update variant to the list
	void		addref(const char *szName, Variant *pvVar);						// Add/update variant REFERENCE to the list
	Variant*	findvar(const char *szName, bool &bConst);						// Find a variable in the list

private:
	void		removenode(VarNode *lpRoot);
	VarNode*	findvarnode(const char *szName);
	void		addnode(const char *szName, VarNode *lpNewNode);

	// Variables
	VarNode		*m_lpRoot;						// Pointer to root node, or NULL if none yet.

};

///////////////////////////////////////////////////////////////////////////////

#endif
