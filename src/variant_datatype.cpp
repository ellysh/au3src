
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
// variant_datatype.cpp
//
///////////////////////////////////////////////////////////////////////////////


// Includes
#include "StdAfx.h"								// Pre-compiled headers

#ifndef _MSC_VER								// Includes for non-MS compilers
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <windef.h>
#endif

#include "variant_datatype.h"


///////////////////////////////////////////////////////////////////////////////
// Copy constructor
///////////////////////////////////////////////////////////////////////////////

Variant::Variant(const Variant &vOp2):
	m_nVarType(vOp2.m_nVarType),	// Get type of var to copy
	m_szValue(NULL)					// Zero required vars
{
	//MessageBox(NULL, "Copy constructor called.", "Variant", MB_OK);

	switch(m_nVarType)
	{
		case VAR_INT32:
			m_nValue	= vOp2.m_nValue;
			break;

		case VAR_INT64:
			m_n64Value	= vOp2.m_n64Value;
			break;

		case VAR_DOUBLE:
			m_fValue	= vOp2.m_fValue;
			break;

		case VAR_STRING:
			m_nStrLen	= vOp2.m_nStrLen;
			m_nStrAlloc = m_nStrLen + 1;
			m_szValue	= new char[m_nStrAlloc];
			strcpy(m_szValue, vOp2.m_szValue);
			break;

		case VAR_REFERENCE:
			m_pValue	= vOp2.m_pValue;
			break;

		case VAR_HWND:
			m_hWnd	= vOp2.m_hWnd;
			break;

		case VAR_ARRAY:
			int i;
			m_Array = NULL;						// Must set this to NULL for ArrayDetailsCreate to work.
			ArrayDetailsCreate();				// Create our array handling structure

			// Copy the required dimension sizes
			m_Array->DimensionsCur	= vOp2.m_Array->Dimensions;
			for (i=0; i<m_Array->DimensionsCur; i++)
				m_Array->SubscriptCur[i] = vOp2.m_Array->Subscript[i];

			// Allocate this array (based on _current_ subscript)
			ArrayDim();

			// Now copy the individual variant elements (each array element is a pointer to a variant)
			for (i=0; i<m_Array->nElements; i++)
			{
				if (vOp2.m_Array->Data[i] != NULL)
				{
					m_Array->Data[i] = new Variant;	// Allocate a variant for this spot
					*(m_Array->Data[i]) = *(vOp2.m_Array->Data[i]);
				}
			}
			break;
	}
}


///////////////////////////////////////////////////////////////////////////////
// Destructor()
///////////////////////////////////////////////////////////////////////////////

Variant::~Variant()
{
	//MessageBox(NULL, "Destructor called.", "Variant", MB_OK);

	// Free up the cache string if required
	InvalidateStringValue();

	// Free any array stuff if required
	ArrayFree();
	ArrayDetailsFree();

} // ~Variant()


///////////////////////////////////////////////////////////////////////////////
// ReInit()
// This clears all data/arrays from a variant and resets to base values of 0 (int32)
///////////////////////////////////////////////////////////////////////////////

void Variant::ReInit(void)
{
	// Free up the char string if required
	InvalidateStringValue();

	// Free any array stuff if required
	ArrayFree();
	ArrayDetailsFree();

	m_nVarType		= VAR_INT32;				// Type of this variant
	m_nValue		= 0;						// Value

} // ReInit()


///////////////////////////////////////////////////////////////////////////////
// HexToDec()
///////////////////////////////////////////////////////////////////////////////

bool Variant::HexToDec(const char *szHex, int &nDec)
{
	// Really crappy hex conversion
	int i = (int)strlen(szHex) - 1;

	nDec = 0;
	int nMult = 1;
	for (int j = 0; j < 8; ++j)
	{
		if (i < 0)
			break;

		if (szHex[i] >= '0' && szHex[i] <= '9')
			nDec += (szHex[i] - '0') * nMult;
		else if (szHex[i] >= 'A' && szHex[i] <= 'F')
			nDec += (((szHex[i] - 'A'))+10) * nMult;
		else if (szHex[i] >= 'a' && szHex[i] <= 'f')
			nDec += (((szHex[i] - 'a'))+10) * nMult;
		else
		{
			nDec = 0;					// Set value as 0
			return false;
		}

		--i;
		nMult = nMult * 16;
	}

	if (i != -1)
	{
		nDec = 0;
		return false;
	}
	else
		return true;

} // Util_ConvDec()


///////////////////////////////////////////////////////////////////////////////
// szValue()
///////////////////////////////////////////////////////////////////////////////

const char * Variant::szValue(void)
{
	// If this is not a string type AND no generated string exists then
	// generate one
	if (m_nVarType != VAR_STRING && m_szValue == NULL)
		GenStringValue();						// Generate a cached string version

	return m_szValue;

} // szvalue()


///////////////////////////////////////////////////////////////////////////////
// fValue()
///////////////////////////////////////////////////////////////////////////////

double Variant::fValue(void)
{
	switch(m_nVarType)
	{
		case VAR_INT32:
			return (double)m_nValue;

		case VAR_INT64:
			return (double)m_n64Value;

		case VAR_DOUBLE:
			return m_fValue;

		case VAR_STRING:
			int nTemp;

			if ( (m_szValue[0] == '0') && (m_szValue[1] == 'x' || m_szValue[1] == 'X') )
			{
				HexToDec(&m_szValue[2], nTemp);	// Assume hex conversion
				return (double)nTemp;
			}
			else
				return atof(m_szValue);

		case VAR_REFERENCE:
		case VAR_ARRAY:
		case VAR_HWND:
			return 0.0;
	}

	return 0.0;

} // fValue()


///////////////////////////////////////////////////////////////////////////////
// nValue()
///////////////////////////////////////////////////////////////////////////////

int	Variant::nValue(void)
{
	switch(m_nVarType)
	{
		case VAR_INT32:
			return m_nValue;

		case VAR_INT64:
			return (int)m_n64Value;

		case VAR_DOUBLE:
			return (int)m_fValue;

		case VAR_STRING:
			int nTemp;

			if ( (m_szValue[0] == '0') && (m_szValue[1] == 'x' || m_szValue[1] == 'X') )
			{
				HexToDec(&m_szValue[2], nTemp);	// Assume hex conversion
				return nTemp;
			}
			else
				return atoi(m_szValue);

		case VAR_REFERENCE:
		case VAR_ARRAY:
		case VAR_HWND:
			return 0;
	}

	return 0;

} // nValue()


///////////////////////////////////////////////////////////////////////////////
// n64Value()
///////////////////////////////////////////////////////////////////////////////

__int64	Variant::n64Value(void)
{
	switch(m_nVarType)
	{
		case VAR_INT32:
			return (__int64)m_nValue;

		case VAR_INT64:
			return m_n64Value;

		case VAR_DOUBLE:
			return (__int64)m_fValue;

		case VAR_STRING:
			int nTemp;

			if ( (m_szValue[0] == '0') && (m_szValue[1] == 'x' || m_szValue[1] == 'X') )
			{
				HexToDec(&m_szValue[2], nTemp);	// Assume hex conversion
				return (__int64)nTemp;
			}
			else
				return _atoi64(m_szValue);

		case VAR_REFERENCE:
		case VAR_ARRAY:
		case VAR_HWND:
			return 0;
	}

	return 0;

} // n64Value()


///////////////////////////////////////////////////////////////////////////////
// hWnd()
///////////////////////////////////////////////////////////////////////////////

HWND	Variant::hWnd(void)
{
	if (m_nVarType == VAR_HWND)
		return m_hWnd;
	else
		return NULL;

} // hWnd()


///////////////////////////////////////////////////////////////////////////////
// pValue()
///////////////////////////////////////////////////////////////////////////////

Variant * Variant::pValue(void)
{
	if (m_nVarType == VAR_REFERENCE)
		return m_pValue;
	else
		return NULL;

} // pValue()


///////////////////////////////////////////////////////////////////////////////
// GenStringValue()
//
// This function generates a string version of the variant - note it does NOT
// change the base type.
//
///////////////////////////////////////////////////////////////////////////////

void Variant::GenStringValue(void)
{
	if (m_nVarType == VAR_STRING)
		return;									// Already a current string, nothing to do

	char	szTemp[128];						// It is unclear just how many 0000 the sprintf function can add...

	InvalidateStringValue();					// Remove previous string cache

	switch(m_nVarType)
	{
		case VAR_INT32:
			// Work out the string representation of the number
			itoa(m_nValue, szTemp, 10);
			break;

		case VAR_INT64:
			// Work out the string representation of the number
			_i64toa(m_n64Value, szTemp, 10);
			break;


		case VAR_DOUBLE:
			// Work out the string representation of the number, don't print trailing zeros
			sprintf(szTemp, "%.15g", m_fValue);		// Have at least 15 digits after the . for precision (default is 6)
			break;

		case VAR_REFERENCE:
		case VAR_ARRAY:
			szTemp[0] = '\0';
			break;

		case VAR_HWND:
			sprintf(szTemp, "%p", m_hWnd);
			break;
	}

	// Copy from szTemp
	m_nStrLen = (int)strlen(szTemp);
	m_nStrAlloc = m_nStrLen + 1;
	m_szValue = new char[m_nStrAlloc];
	strcpy(m_szValue, szTemp);

} // GenStringValue()


///////////////////////////////////////////////////////////////////////////////
// InvalidateStringValue()
//
// Call this to delete any string value (or cached string value)
// When working with non strings it should be called everytime the variant
// is modified so that a fresh cached copy can be generated upon a call to
// .szValue()
//
///////////////////////////////////////////////////////////////////////////////

void Variant::InvalidateStringValue(void)
{
	if (m_szValue)
	{
		delete [] m_szValue;
		m_szValue = NULL;
	}

} // InvalidateStringValue()


///////////////////////////////////////////////////////////////////////////////
// Overloaded operator=() for variants
///////////////////////////////////////////////////////////////////////////////

Variant& Variant::operator=(const Variant &vOp2)
{
	if (this == &vOp2)							// Stop copies to self!
		return *this;

	// ReInit this variant (clears all arrays/strings, resets to default)
	ReInit();

	m_nVarType	= vOp2.m_nVarType;

	m_szValue		= NULL;

	switch(m_nVarType)
	{
		case VAR_INT32:
			m_nValue	= vOp2.m_nValue;
			break;

		case VAR_INT64:
			m_n64Value	= vOp2.m_n64Value;
			break;

		case VAR_DOUBLE:
			m_fValue	= vOp2.m_fValue;
			break;

		case VAR_STRING:
			m_nStrLen = vOp2.m_nStrLen;
			m_nStrAlloc = vOp2.m_nStrAlloc;
			m_szValue = new char[m_nStrAlloc];	// Same as the ALLOCATED size of the other string
			strcpy(m_szValue, vOp2.m_szValue);
			break;

		case VAR_REFERENCE:
			m_pValue	= vOp2.m_pValue;
			break;

		case VAR_HWND:
			m_hWnd		= vOp2.m_hWnd;
			break;

		case VAR_ARRAY:
			int i;

			ArrayDetailsCreate();

			// Copy the required dimension sizes
			m_Array->DimensionsCur	= vOp2.m_Array->Dimensions;
			for (i=0; i<m_Array->DimensionsCur; i++)
				m_Array->SubscriptCur[i] = vOp2.m_Array->Subscript[i];

			// Allocate this array (based on _current_ subscript)
			ArrayDim();

			// Now copy the individual variant elements (each array element is a pointer to a variant)
			for (i=0; i<m_Array->nElements; i++)
			{
				if (vOp2.m_Array->Data[i] != NULL)
				{
					m_Array->Data[i] = new Variant;	// Allocate a variant for this spot
					*(m_Array->Data[i]) = *(vOp2.m_Array->Data[i]);
				}
			}
			break;
	}

	return *this;								// Return this object that generated the call

} // operator=()


///////////////////////////////////////////////////////////////////////////////
// Overloaded operator=() for integers
///////////////////////////////////////////////////////////////////////////////

Variant& Variant::operator=(int nOp2)
{
	// Free any local array data / zero array variables
	ReInit();

	m_nVarType	= VAR_INT32;
	m_nValue	= nOp2;

	return *this;								// Return this object that generated the call

} // operator=()


///////////////////////////////////////////////////////////////////////////////
// Overloaded operator=() for 64bit integers
///////////////////////////////////////////////////////////////////////////////

Variant& Variant::operator=(__int64 nOp2)
{
	// Free any local array data / zero array variables
	ReInit();

	m_nVarType	= VAR_INT64;
	m_n64Value	= nOp2;

	return *this;								// Return this object that generated the call

} // operator=()


///////////////////////////////////////////////////////////////////////////////
// Overloaded operator=() for HWNDs
///////////////////////////////////////////////////////////////////////////////

Variant& Variant::operator=(HWND nOp2)
{
	// Free any local array data / zero array variables
	ReInit();

	m_nVarType	= VAR_HWND;
	m_hWnd		= nOp2;

	return *this;								// Return this object that generated the call

} // operator=()


///////////////////////////////////////////////////////////////////////////////
// Overloaded operator=() for floats
///////////////////////////////////////////////////////////////////////////////

Variant& Variant::operator=(double fOp2)
{
	// Free any local array data / zero array variables
	ReInit();

	m_nVarType	= VAR_DOUBLE;
	m_fValue	= fOp2;

	return *this;								// Return this object that generated the call

} // operator=()


///////////////////////////////////////////////////////////////////////////////
// Overloaded operator=() for strings
///////////////////////////////////////////////////////////////////////////////

Variant& Variant::operator=(const char *szOp2)
{
	// Free any local array data / zero array variables
	ReInit();

	m_nVarType	= VAR_STRING;

	// Copy the string
	m_nStrLen = (int)strlen(szOp2);
	m_nStrAlloc =  m_nStrLen + 1;
	m_szValue = new char[m_nStrAlloc];
	strcpy(m_szValue, szOp2);

	return *this;								// Return this object that generated the call

} // operator=()


///////////////////////////////////////////////////////////////////////////////
// Overloaded operator=() for pointers
///////////////////////////////////////////////////////////////////////////////

Variant& Variant::operator=(Variant *pOp2)
{
	// Free any local array data / zero array variables
	ReInit();

	m_nVarType	= VAR_REFERENCE;
	m_pValue	= pOp2;

	return *this;								// Return this object that generated the call

} // operator=()


///////////////////////////////////////////////////////////////////////////////
// Overloaded operator+=()
///////////////////////////////////////////////////////////////////////////////

Variant& Variant::operator+=(Variant &vOp2)
{
	__int64 n64Temp;
	int		nTemp;

	switch (m_nVarType)
	{
		case VAR_INT32:
			if (vOp2.m_nVarType == VAR_INT32)
			{
				nTemp = m_nValue + vOp2.m_nValue;
				n64Temp = (__int64)(m_nValue) + (__int64)(vOp2.m_nValue);

				if ( (__int64)(nTemp) != n64Temp )
				{
					// Promote to int64 because of int32 overflow
					m_nVarType = VAR_INT64;
					m_n64Value = n64Temp;
				}
				else
					m_nValue = nTemp;
			}
			else if (vOp2.m_nVarType == VAR_INT64)
			{
				// As 2nd operation is a 64bit, promote anyway
				m_n64Value = (__int64)(m_nValue) + vOp2.m_n64Value;
				m_nVarType = VAR_INT64;
			}
			else
			{
				ChangeToDouble();
				m_fValue += vOp2.fValue();
			}
			break;

		case VAR_INT64:
			if (vOp2.m_nVarType == VAR_INT32 || vOp2.m_nVarType == VAR_INT64)
				m_n64Value += vOp2.n64Value();
			else
			{
				ChangeToDouble();
				m_fValue += vOp2.fValue();
			}
			break;

		case VAR_DOUBLE:
			m_fValue += vOp2.fValue();
			break;

		case VAR_STRING:
			ChangeToDouble();
			m_fValue += vOp2.fValue();
			break;
	}

	// Erase any cached string version of this variant
	InvalidateStringValue();

	return *this;

} // operator+=()


///////////////////////////////////////////////////////////////////////////////
// Overloaded operator-=()
///////////////////////////////////////////////////////////////////////////////

Variant& Variant::operator-=(Variant &vOp2)
{
	__int64 n64Temp;
	int		nTemp;

	switch (m_nVarType)
	{
		case VAR_INT32:
			if (vOp2.m_nVarType == VAR_INT32)
			{
				nTemp = m_nValue - vOp2.m_nValue;
				n64Temp = (__int64)(m_nValue) - (__int64)(vOp2.m_nValue);

				if ( (__int64)(nTemp) != n64Temp )
				{
					// Promote to int64 because of int32 overflow
					m_nVarType = VAR_INT64;
					m_n64Value = n64Temp;
				}
				else
					m_nValue = nTemp;
			}
			else if (vOp2.m_nVarType == VAR_INT64)
			{
				// As 2nd operation is a 64bit, promote anyway
				m_n64Value = (__int64)(m_nValue) - vOp2.m_n64Value;
				m_nVarType = VAR_INT64;
			}
			else
			{
				ChangeToDouble();
				m_fValue -= vOp2.fValue();
			}
			break;

		case VAR_INT64:
			if (vOp2.m_nVarType == VAR_INT32 || vOp2.m_nVarType == VAR_INT64)
				m_n64Value -= vOp2.n64Value();
			else
			{
				ChangeToDouble();
				m_fValue -= vOp2.fValue();
			}
			break;

		case VAR_DOUBLE:
			m_fValue -= vOp2.fValue();
			break;

		case VAR_STRING:
			ChangeToDouble();
			m_fValue -= vOp2.fValue();
			break;
	}

	// Erase any cached string version of this variant
	InvalidateStringValue();

	return *this;

} // operator-=()


///////////////////////////////////////////////////////////////////////////////
// Overloaded operator*=()
///////////////////////////////////////////////////////////////////////////////

Variant& Variant::operator*=(Variant &vOp2)
{
	__int64 n64Temp;
	int		nTemp;

	switch (m_nVarType)
	{
		case VAR_INT32:
			if (vOp2.m_nVarType == VAR_INT32)
			{
				nTemp = m_nValue * vOp2.m_nValue;
				n64Temp = (__int64)(m_nValue) * (__int64)(vOp2.m_nValue);

				if ( (__int64)(nTemp) != n64Temp )
				{
					// Promote to int64 because of int32 overflow
					m_nVarType = VAR_INT64;
					m_n64Value = n64Temp;
				}
				else
					m_nValue = nTemp;
			}
			else if (vOp2.m_nVarType == VAR_INT64)
			{
				// As 2nd operation is a 64bit, promote anyway
				m_n64Value = (__int64)(m_nValue) * vOp2.m_n64Value;
				m_nVarType = VAR_INT64;
			}
			else
			{
				ChangeToDouble();
				m_fValue *= vOp2.fValue();
			}
			break;

		case VAR_INT64:
			if (vOp2.m_nVarType == VAR_INT32 || vOp2.m_nVarType == VAR_INT64)
				m_n64Value *= vOp2.n64Value();
			else
			{
				ChangeToDouble();
				m_fValue *= vOp2.fValue();
			}
			break;

		case VAR_DOUBLE:
			m_fValue *= vOp2.fValue();
			break;

		case VAR_STRING:
			ChangeToDouble();
			m_fValue *= vOp2.fValue();
			break;
	}

	// Erase any cached string version of this variant
	InvalidateStringValue();

	return *this;

} // operator*=()


///////////////////////////////////////////////////////////////////////////////
// Overloaded operator/=()
///////////////////////////////////////////////////////////////////////////////

Variant& Variant::operator/=(Variant &vOp2)
{
	// Must be of double type first
	ChangeToDouble();

	m_fValue = m_fValue / vOp2.fValue();	// FLOAT

	// Erase any cached string version of this variant
	InvalidateStringValue();

	return *this;

} // operator/=()


///////////////////////////////////////////////////////////////////////////////
// Overloaded operator&=()
///////////////////////////////////////////////////////////////////////////////

Variant& Variant::operator&=(Variant &vOp2)
{
	// Must be of int32 type first
	ChangeToInt32();

	// Do bitwise AND
	m_nValue	= m_nValue & vOp2.nValue();

	// Erase any cached string version of this variant
	InvalidateStringValue();

	return *this;

} // operator&=()


///////////////////////////////////////////////////////////////////////////////
// Overloaded operator|=()
///////////////////////////////////////////////////////////////////////////////

Variant& Variant::operator|=(Variant &vOp2)
{
	// Must be of int32 type first
	ChangeToInt32();

	// Do bitwise OR
	m_nValue	= m_nValue | vOp2.nValue();

	// Erase any cached string version of this variant
	InvalidateStringValue();

	return *this;

} // operator|=()


///////////////////////////////////////////////////////////////////////////////
// Overloaded operator!()
///////////////////////////////////////////////////////////////////////////////

Variant& Variant::operator!()
{
	// Must be of int32 type first
	ChangeToInt32();

	m_nValue	= !m_nValue;

	// Erase any cached string version of this variant
	InvalidateStringValue();

	return *this;

} // operator!()


///////////////////////////////////////////////////////////////////////////////
//
// GetComparisionType()
//
// Used in == != > < operations to determine the best way to compare two
// operands
//
///////////////////////////////////////////////////////////////////////////////

int Variant::GetComparisionType(int nOp1, int nOp2) const
{
	switch (nOp1)
	{
		case VAR_INT32:
			switch (nOp2)
			{
				case VAR_INT32:
					return VAR_INT32;
				case VAR_INT64:
					return VAR_INT64;
				case VAR_DOUBLE:
					return VAR_DOUBLE;
				case VAR_STRING:
					return VAR_DOUBLE;
			}
			break;

		case VAR_INT64:
			switch (nOp2)
			{
				case VAR_INT32:
					return VAR_INT64;
				case VAR_INT64:
					return VAR_INT64;
				case VAR_DOUBLE:
					return VAR_DOUBLE;
				case VAR_STRING:
					return VAR_DOUBLE;
			}
			break;

		case VAR_DOUBLE:
			switch (nOp2)
			{
				case VAR_INT32:
					return VAR_DOUBLE;
				case VAR_INT64:
					return VAR_DOUBLE;
				case VAR_DOUBLE:
					return VAR_DOUBLE;
				case VAR_STRING:
					return VAR_DOUBLE;
			}
			break;

		case VAR_STRING:
			switch (nOp2)
			{
				case VAR_INT32:
					return VAR_DOUBLE;
				case VAR_INT64:
					return VAR_DOUBLE;
				case VAR_DOUBLE:
					return VAR_DOUBLE;
				case VAR_STRING:
					return VAR_STRING;
			}
			break;

		case VAR_HWND:
			if (nOp2 == VAR_HWND)
				return VAR_HWND;
			break;
	}

	// Everything else is undefined
	return VAR_ERROR;

} // GetComparisionType()


///////////////////////////////////////////////////////////////////////////////
// Overloaded operator==() for Variants
//
// Is case insensitive with strings
//
///////////////////////////////////////////////////////////////////////////////

bool operator==(Variant &vOp1, Variant &vOp2)
{
	switch( vOp1.GetComparisionType(vOp1.m_nVarType, vOp2.m_nVarType) )
	{
		case VAR_STRING:
			// Do string conparision
			if (!stricmp(vOp1.m_szValue, vOp2.m_szValue) )
				return true;
			else
				return false;

		case VAR_INT32:
			if (vOp1.nValue() == vOp2.nValue())
				return true;
			else
				return false;

		case VAR_INT64:
			if (vOp1.n64Value() == vOp2.n64Value())
				return true;
			else
				return false;

		case VAR_DOUBLE:
			if (vOp1.fValue() == vOp2.fValue())
				return true;
			else
				return false;

		case VAR_HWND:
			if (vOp1.hWnd() == vOp2.hWnd())
				return true;
			else
				return false;
	}

	return false;

} // operator==()


///////////////////////////////////////////////////////////////////////////////
// StringCompare()
//
// Compares 2 strings and is case sensitive
//
///////////////////////////////////////////////////////////////////////////////

bool Variant::StringCompare(Variant &vOp2)
{
	// Compare the two string portions - even if they aren't string variants
	// Do string comparision
	if (!strcmp(szValue(), vOp2.szValue()) )
		return true;
	else
		return false;

} // StringCompare()


///////////////////////////////////////////////////////////////////////////////
// Overloaded operator>() for Variants
///////////////////////////////////////////////////////////////////////////////

bool operator>(Variant &vOp1, Variant &vOp2)
{
	switch( vOp1.GetComparisionType(vOp1.m_nVarType, vOp2.m_nVarType) )
	{
		case VAR_STRING:
			// Do string conparision
			if (stricmp(vOp1.m_szValue, vOp2.m_szValue) > 0)
				return true;
			else
				return false;

		case VAR_INT32:
			if (vOp1.nValue() > vOp2.nValue())
				return true;
			else
				return false;

		case VAR_INT64:
			if (vOp1.n64Value() > vOp2.n64Value())
				return true;
			else
				return false;

		case VAR_DOUBLE:
			if (vOp1.fValue() > vOp2.fValue())
				return true;
			else
				return false;
	}

	return false;

} // operator>()


///////////////////////////////////////////////////////////////////////////////
// Overloaded operator<() for Variants
///////////////////////////////////////////////////////////////////////////////

bool operator<(Variant &vOp1, Variant &vOp2)
{
	switch( vOp1.GetComparisionType(vOp1.m_nVarType, vOp2.m_nVarType) )
	{
		case VAR_STRING:
			// Do string conparision
			if (stricmp(vOp1.m_szValue, vOp2.m_szValue) < 0)
				return true;
			else
				return false;

		case VAR_INT32:
			if (vOp1.nValue() < vOp2.nValue())
				return true;
			else
				return false;

		case VAR_INT64:
			if (vOp1.n64Value() < vOp2.n64Value())
				return true;
			else
				return false;

		case VAR_DOUBLE:
			if (vOp1.fValue() < vOp2.fValue())
				return true;
			else
				return false;
	}

	return false;

} // operator<()


///////////////////////////////////////////////////////////////////////////////
// Overloaded operator&&() for Variants
///////////////////////////////////////////////////////////////////////////////

bool operator&&(Variant &vOp1, Variant &vOp2)
{
	if (vOp1.isTrue() && vOp2.isTrue())
		return true;
	else
		return false;

} // operator&&()


///////////////////////////////////////////////////////////////////////////////
// Overloaded operator||() for Variants
///////////////////////////////////////////////////////////////////////////////

bool operator||(Variant &vOp1, Variant &vOp2)
{
	if (vOp1.isTrue() || vOp2.isTrue())
		return true;
	else
		return false;

} // operator||()


///////////////////////////////////////////////////////////////////////////////
// isTrue()
// Returns true if the variant is non-zero
///////////////////////////////////////////////////////////////////////////////

bool Variant::isTrue(void) const
{
	switch (m_nVarType)
	{
		case VAR_INT32:
			if (m_nValue)
				return true;
			break;

		case VAR_INT64:
			if (m_n64Value)
				return true;
			break;

		case VAR_DOUBLE:
			if (m_fValue)
				return true;
			break;

		case VAR_STRING:
			if (m_szValue[0] != '\0')
				return true;
			break;

		case VAR_HWND:
			if (m_hWnd != NULL)
				return true;
			break;
	}

	return false;

} // isTrue()


///////////////////////////////////////////////////////////////////////////////
// isNumber()
//
// Returns true if this variant is a number type
//
///////////////////////////////////////////////////////////////////////////////

bool Variant::isNumber(void) const
{
	if (m_nVarType == VAR_DOUBLE || m_nVarType == VAR_INT32 || m_nVarType == VAR_INT64)
		return true;
	else
		return false;

} // isNumber()


///////////////////////////////////////////////////////////////////////////////
// isString()
//
// Returns true if this variant is a string
//
///////////////////////////////////////////////////////////////////////////////

bool Variant::isString(void) const
{
	if (m_nVarType == VAR_STRING)
		return true;
	else
		return false;

} // isString()


///////////////////////////////////////////////////////////////////////////////
// isArray()
//
// Returns true if this variant is an array
//
///////////////////////////////////////////////////////////////////////////////

bool Variant::isArray(void) const
{
	if (m_nVarType == VAR_ARRAY)
		return true;
	else
		return false;

} // isArray()


///////////////////////////////////////////////////////////////////////////////
// isHWND()
//
// Returns true if this variant is a HWND
//
///////////////////////////////////////////////////////////////////////////////

bool Variant::isHWND(void) const
{
	if (m_nVarType == VAR_HWND)
		return true;
	else
		return false;

} // isHWND()


///////////////////////////////////////////////////////////////////////////////
// ChangeToDouble()
///////////////////////////////////////////////////////////////////////////////

void Variant::ChangeToDouble(void)
{
	if (m_nVarType == VAR_DOUBLE)
		return;									// Nothing to do

	// Generate a double value
	double fTemp	= fValue();

	// Delete and reinit everything (includes strings/array)
	ReInit();

	m_fValue	= fTemp;
	m_nVarType	= VAR_DOUBLE;

} // ChangeToDouble()


///////////////////////////////////////////////////////////////////////////////
// ChangeToInt32()
///////////////////////////////////////////////////////////////////////////////

void Variant::ChangeToInt32(void)
{
	if (m_nVarType == VAR_INT32)
		return;									// Nothing to do

	// Generate an int32 value
	int nTemp	= nValue();

	// Delete and reinit everything (includes strings/array)
	ReInit();

	m_nValue	= nTemp;
	m_nVarType	= VAR_INT32;

} // ChangeToInt32()


///////////////////////////////////////////////////////////////////////////////
// ChangeToString()
///////////////////////////////////////////////////////////////////////////////

void Variant::ChangeToString(void)
{
	if (m_nVarType == VAR_STRING)
		return;									// Nothing to do

	GenStringValue();							// Generate a string value of this variant

	// Delete any array data if required
	ArrayFree();
	ArrayDetailsFree();

	// Finally change the type to a string
	m_nVarType		= VAR_STRING;

} // ChangeToString()


///////////////////////////////////////////////////////////////////////////////
// Concat()
///////////////////////////////////////////////////////////////////////////////

void Variant::Concat(Variant &vOp2)
{
	char	*szTempString;
	const char	*szOp2;

	// This must be a string type
	ChangeToString();

	// Ensure that the other variant has a valid string value and
	// the m_nStrLen m_nStrAlloc variables - VERY IMPORTANT
	szOp2 = vOp2.szValue();

	// Get new total string length
	m_nStrLen += vOp2.m_nStrLen;

	// Do we have enough space for the concat?
	if ( (m_nStrLen+1) > m_nStrAlloc)			// +1 for \0
	{
		// Create DOUBLE the space we need (room to grow)
		m_nStrAlloc = (m_nStrLen << 1) + 1;

		szTempString	= new char[m_nStrAlloc];

		strcpy(szTempString, m_szValue);
		strcat(szTempString, szOp2);

		delete [] m_szValue;
		m_szValue	= szTempString;
	}
	else
		strcat(m_szValue, szOp2);		// We have the space - no need to realloc


} // Concat()


///////////////////////////////////////////////////////////////////////////////
// ArrayDetailsCreate()
//
// This creates the structure that holds details on a future array - at this
// point the variant should be classed as a VAR_ARRAY type
///////////////////////////////////////////////////////////////////////////////

void Variant::ArrayDetailsCreate(void)
{
	if (m_nVarType != VAR_ARRAY || m_Array == NULL)
	{
		m_Array = new VariantArrayDetails;

		m_Array->Data			= NULL;
		m_Array->nElements		= 0;
		m_Array->Dimensions		= 0;
		m_Array->DimensionsCur	= 0;

		m_nVarType = VAR_ARRAY;
	}

} // ArrayDetailsCreate()


///////////////////////////////////////////////////////////////////////////////
// ArrayDetailsFree()
///////////////////////////////////////////////////////////////////////////////

void Variant::ArrayDetailsFree(void)
{
	if (m_nVarType == VAR_ARRAY && m_Array)
	{
		delete m_Array;								// Will be OK if NULL
		m_Array = NULL;
	}

} // ArrayDetailsFree()


///////////////////////////////////////////////////////////////////////////////
// ArraySubscriptClear()
// This must always be called first - even on first use of an array as it
// is repsonsible for initialising the ArrayDetails structure
///////////////////////////////////////////////////////////////////////////////

void Variant::ArraySubscriptClear(void)
{
	// Change to an array type if required
	ArrayDetailsCreate();

	// Reset the current subscript details
	m_Array->DimensionsCur	= 0;

} // ArraySubscriptClear()


///////////////////////////////////////////////////////////////////////////////
// ArraySubscriptSetNext()
//
///////////////////////////////////////////////////////////////////////////////

bool Variant::ArraySubscriptSetNext(int iSub)
{
	// Any room for another subscript?
	if (m_Array->DimensionsCur >= VAR_SUBSCRIPT_MAX)
		return false;

	m_Array->SubscriptCur[m_Array->DimensionsCur] = iSub;
	m_Array->DimensionsCur++;

	return true;

} // ArraySubscriptSetNext()


///////////////////////////////////////////////////////////////////////////////
// ArrayFree()
//
// Free the actual data (variants) from the array and zero the ArrayDetails
// structure for reuse.  But don't reset the current subscript details as they
// may be about to be used for a Dim
//
///////////////////////////////////////////////////////////////////////////////

void Variant::ArrayFree(void)
{
	if (m_nVarType != VAR_ARRAY || m_Array == NULL)
		return;							// Not an array or no array details

	if (m_Array->Data)					// Only delete if used
	{
		// Delete all the individual variants' in the array
		for (int i=0; i<m_Array->nElements; i++)
			delete m_Array->Data[i];

		// Delete the array
		delete [] m_Array->Data;
		m_Array->Data = NULL;
	}

	// Zero everything for possible reuse
	m_Array->nElements		= 0;
	m_Array->Dimensions		= 0;
	//DimensionsCur	= 0;				// DO NOT UNCOMMENT

} // ArrayFree()


///////////////////////////////////////////////////////////////////////////////
// ArrayDim()
//
// Create an array based on the current subscript details
///////////////////////////////////////////////////////////////////////////////

bool Variant::ArrayDim(void)
{
	int	i;

	if (m_nVarType != VAR_ARRAY)
		return false;							// ArrayCreateDetails not been called!

	// Delete any cached string values and any previous array DATA - Do not use
	// ReInit() as the SubScriptCur contains valid data that would be destroyed
	InvalidateStringValue();
	ArrayFree();

	// Copy the subscripts required and work out the total number of elements
	m_Array->Dimensions	= m_Array->DimensionsCur;
	m_Array->nElements	= 1;
	for (i=0; i<m_Array->Dimensions; i++)
	{
		m_Array->Subscript[i] = m_Array->SubscriptCur[i];
		m_Array->nElements = m_Array->nElements * m_Array->Subscript[i];

		// Check if the array is too big (4096*4096 (0x1000000) elements is 64MB JUST FOR THE TABLE!)
		if ( m_Array->nElements >= 0x1000000 )
			return false;						// Abort
	}

	// Create space for the array (effectively an array of Variant POINTERS)
	m_Array->Data = new Variant *[m_Array->nElements];

	// We will allocate array entries when required, so just NULL for now
	for (i=0; i<m_Array->nElements; i++)
		m_Array->Data[i] = NULL;

	return true;

} // ArrayDim()


///////////////////////////////////////////////////////////////////////////////
// ArrayGetElem()
//
// Returns the index to the current array element
//
//[2][2][2]
// a  b  c
// n = 3
//
// s1 s2 s3
// 0  0  0		element 0
// 0  0  1		element 1
// 0  1  0 		element 2
// 0  1  1		element 3
// 1  0  0		element 4
// 1  0  1		element 5
// 1  1  0 		element 6
// 1  1  1		element 7
//
// index  = 0
// index += s1 * b * c
// index += s2 * c
// index += s3
//
// e.g. [1][0][1]
// index  = 0
// index += 1 * 2 * 2
// index += 0 * 2
// index += 1
// index equals element 5
//
///////////////////////////////////////////////////////////////////////////////
int Variant::ArrayGetElem(void)
{
	int i, j;
	int index, mult;

	// Do we have array data?
	if (m_nVarType != VAR_ARRAY)
		return -1;

	// Get for correct number of subscripts/size etc
	if (ArrayBoundsCheck() == false)
		return -1;

	// Convert our multidimensional array to an element in our internal single dimension array
	index = 0;
	for (i=0; i<m_Array->Dimensions; i++)
	{
		mult = 1;
		for (j=i+1; j<m_Array->Dimensions; j++)
			mult = mult * m_Array->Subscript[j];

		mult = mult * m_Array->SubscriptCur[i];
		index += mult;
	}
	return index;
}


///////////////////////////////////////////////////////////////////////////////
// ArrayGetRef()
//
// Returns a pointer to the current array element
// See GetArrayElem() for mapping info.
//
// By default bCreate is true so the array entry is created.
///////////////////////////////////////////////////////////////////////////////

Variant* Variant::ArrayGetRef(bool bCreate)
{
	int	index;

	index = ArrayGetElem();

	if (index < 0)
	// not a valid element
		return NULL;

	// index is the entry we need to return, if required, allocate the entry
	// otherwise return previously allocated entry

	if (m_Array->Data[index] == NULL && bCreate)
		m_Array->Data[index] = new Variant;

	return m_Array->Data[index];

} // ArrayGetRef()


///////////////////////////////////////////////////////////////////////////////
// ArrayBoundsCheck()
///////////////////////////////////////////////////////////////////////////////

bool Variant::ArrayBoundsCheck(void)
{
	int	i;

	if (m_Array->Dimensions != m_Array->DimensionsCur)
		return false;

	for (i=0; i<m_Array->Dimensions; i++)
	{
		if (m_Array->SubscriptCur[i] < 0 || m_Array->SubscriptCur[i] >= m_Array->Subscript[i])
			return false;
	}

	// If we get here then the requested subscript is in bounds
	return true;

} // ArrayBoundsCheck()


///////////////////////////////////////////////////////////////////////////////
// ArrayGetBound(int)
//
// Returns the size of an array dimension
///////////////////////////////////////////////////////////////////////////////

int Variant::ArrayGetBound(int iSub)
{
	if (m_Array == NULL)
		// not an array.  No dimensions and all dimensions read 0.
		return 0;
	else if (iSub == 0)
		// If asks for 0, return number of dimensions
		return m_Array->Dimensions;
	else if (iSub < 0 || iSub > m_Array->Dimensions)
		// Out of bounds.  Array does not have that dimension
		return 0;								// failure
	else
		return m_Array->Subscript[iSub-1];

} // ArrayGetBound()



///////////////////////////////////////////////////////////////////////////////
// ArrayCopy(Variant &)
//
// Copy the other array into the current one, matching array locations.
// To be used after ArrayDim() in REDIM keyword
///////////////////////////////////////////////////////////////////////////////
bool Variant::ArrayCopy(Variant &other)
{
	int i;
	Variant *vOldValue, *vLocal;

	// make sure that this is an array and that the other is the same size.
	if (m_Array->Dimensions == 0 || m_Array->Dimensions != other.m_Array->Dimensions)
		return false;
	// reset subscripts for comparison
	for (i=0; i<m_Array->Dimensions; ++i) {
		m_Array->SubscriptCur[i]=0;
		other.m_Array->SubscriptCur[i]=0;
	}
	// start copying
	do {
		// check if variant in other exists
		i = other.ArrayGetElem();
		if (i < 0)
			vOldValue = NULL;
		else
			vOldValue = other.m_Array->Data[i];
		if (vOldValue != NULL) {
			// copy variant
			vLocal = ArrayGetRef();
			if (vLocal != NULL)
				*vLocal = *vOldValue;
		}
		// increment search locations
		for (i=0; i<m_Array->Dimensions; ++i) {
			++m_Array->SubscriptCur[i];
			++other.m_Array->SubscriptCur[i];
			if (ArrayBoundsCheck() && other.ArrayBoundsCheck())
				break;
			m_Array->SubscriptCur[i]=0;
			other.m_Array->SubscriptCur[i]=0;
		}
	} while (i<m_Array->Dimensions);

	return false;

} // ArrayCopy
