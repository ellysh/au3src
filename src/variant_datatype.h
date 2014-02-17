#ifndef __VARIANT_H
#define __VARIANT_H

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
// variant_datatype.h
//
// The standalone class for a variant datatype.
//
//
// A variant can be one of these main types:
//  - VAR_STRING (a string)
//  - VAR_INT32 (a 32bit int)
//  - VAR_INT64 (a 64bit int)
//  - VAR_DOUBLE (a double)
//
// The value of a variant can be read by:
//  - .fValue() for the double value
//  - .szValue() for the string value
//  - .nValue() for a 32bit integer version
//  - .n64Value() for a 64bit integer version
//
// Any of these values can be read at any time and a suitable conversion will
// be performed.  E.g. if a variant is the STRING value "10" you can use
// .fValue to read 10.0 and .nValue to read 10.
//
///////////////////////////////////////////////////////////////////////////////


// Includes
#ifndef _MSC_VER								// Includes for non-MS compilers
	#include <windef.h>							// HWND reference
#endif

// Define the types of variants that we allow
#define VAR_ERROR			0					// Invalid comparision type
#define VAR_INT32			1					// 32bit Integer
#define VAR_INT64			2					// 64bit Integer
#define VAR_DOUBLE			3					// Double float
#define VAR_STRING			4					// String
#define VAR_ARRAY			5					// Array of variants
#define VAR_REFERENCE		6					// Reference to another variant
#define VAR_HWND			7					// Handle (Window)

#define VAR_ITOA_MAX		65					// Maximum returned length of an i64toa operation
#define VAR_SUBSCRIPT_MAX	64					// Maximum number of subscripts for an array



class Variant
{

public:
	// Functions
	Variant() : m_nVarType(VAR_INT32), m_nValue(0), m_szValue(NULL) {}	// Constructor
	Variant(const Variant &vOp2);				// Copy constructor
	~Variant();									// Destructor

	Variant&	operator=(const Variant &vOp2);	// Overloaded = for variants
	Variant&	operator=(int nOp2);			// Overloaded = for integers
	Variant&	operator=(__int64 nOp2);		// Overloaded = for integers
	Variant&	operator=(HWND nOp2);			// Overloaded = for HWNDs
	Variant&	operator=(double fOp2);			// Overloaded = for floats
	Variant&	operator=(const char *szOp2);	// Overloaded = for strings
	Variant&	operator=(Variant *pOp2);		// Overloaded = for pointers/references
	Variant&	operator+=(Variant &vOp2);		// Overloaded += (addition/concatenation)
	Variant&	operator-=(Variant &vOp2);		// Overloaded -= (subtraction)
	Variant&	operator*=(Variant &vOp2);		// Overloaded *= (multiplication)
	Variant&	operator/=(Variant &vOp2);		// Overloaded /= (division)
	Variant&	operator&=(Variant &vOp2);		// Overloaded &= Bitwise AND
	Variant&	operator|=(Variant &vOp2);		// Overloaded |= Bitwise OR
	Variant&	operator!();					// Overloaded !  logical NOT

	friend bool	operator==(Variant &vOp1, Variant &vOp2);	// Overloaded == for Variants (case insensitive)
	friend bool	operator>(Variant &vOp1, Variant &vOp2);	// Overloaded > for Variants
	friend bool	operator<(Variant &vOp1, Variant &vOp2);	// Overloaded < for Variants
	friend bool operator&&(Variant &vOp1, Variant &vOp2);	// Overloaded && for variants
	friend bool operator||(Variant &vOp1, Variant &vOp2);	// Overloaded || for variants

	bool		StringCompare(Variant &vOp2);			// Compare two strings with case sense
	bool		HexToDec(const char *szHex, int &nDec);	// Convert hex string to an integer
	void		Concat(Variant &vOp2);					// Concats two variants (forces string if possible)
	void		ChangeToDouble(void);					// Convert variant to a DOUBLE
	void		ChangeToString(void);					// Convert variant to a STRING
	void		ChangeToInt32(void);					// Convert variant to a INT32

	// Array functions
	void		ArraySubscriptClear(void);			// Reset the current subscript
	bool		ArraySubscriptSetNext(int iSub);	// Set next subscript
	bool		ArrayDim(void);						// Allocate memory for array
	void		ArrayFree(void);					// Releases all memory in the array and resets
	Variant*	ArrayGetRef(bool bCreate=true);		// Returns a pointer to cur array element, creating them if not present
	int			ArrayGetBound(int iSub);			// Returns size of dimension.  returns -1 if not defined
	bool		ArrayCopy(Variant &other);			// Copies the given array into the current variant, minding array bounds

	// Properties
	int		type(void) const { return m_nVarType; }	// Returns variant type
	const char	*szValue(void);						// Returns string value
	double		fValue(void);						// Returns float (double) value
	int			nValue(void);						// Returns int value
	__int64		n64Value(void);						// Returns int64 value
	Variant		*pValue(void);						// Returns a variant pointer
	HWND		hWnd(void);							// Returns a window handle

	bool		isTrue(void) const;					// Returns true if variant is non-zero (string or number)
	bool		isNumber(void) const;				// Returns true if INT32, INT64 or DOUBLE
	bool		isString(void) const;				// Returns true if VAR_STRING
	bool		isArray(void) const;				// Returns true if VAR_ARRAY
	bool		isHWND(void) const;					// Returns true if VAR_HWND


private:
	// Note the order of these variables is critical for the best mix of storage space used and speed!
	// These is due to the way structs and variables are "padded" and accessed.  Ints on 4 byte boundaries
	// give the best speed which is to be expected on 32bit machines
	// Currently uses 24 bytes per variant

	// Structure used for storing array details
	typedef struct
	{
		Variant	**Data;							// Memory area for the array (array of Variant pointers) (NULL = not used)

		int		nElements;						// Actual number of elements in array ([10][10] = 100 elements)
		int		Subscript[VAR_SUBSCRIPT_MAX];	// Subscript details
		int		SubscriptCur[VAR_SUBSCRIPT_MAX];	// Current subscript
		char	Dimensions;						// Number of dimensions/subscripts
		char	DimensionsCur;					// Current number of dimensions

	} VariantArrayDetails;


	// Variables

	// Single variant variables
	// Only one of these values should be used at any given time (space saving)
	// Biggest value is double/int64 = 8 bytes - needs to be on 8 byte boundary for performance
	// so easiest to place it at the top
	union
	{
		int					m_nValue;			// Value of int32 (for VAR_INT32)
		__int64				m_n64Value;			// Value of int64 (for VAR_INT64)
		double				m_fValue;			// Value of double (for VAR_DOUBLE)
		Variant				*m_pValue;			// Value of pointer (for VAR_REFERENCE)
		VariantArrayDetails	*m_Array;			// Value of array (for VAR_ARRAY)
		HWND				m_hWnd;				// Value of handle (for VAR_HWND)
	};

	// There is always a string value even if not a string type so that
	// szValue() can return a const pointer
	char		*m_szValue;						// Value of string (NULL = not avail)
	int			m_nStrLen;						// Length of the string (not including \0 - same as strlen() )
	int			m_nStrAlloc;					// Amount of bytes allocated for the string

	int			m_nVarType;						// Type of this variant

	// Functions
	void		ReInit(void);					// Reset a variant to initial values
	void		GenStringValue(void);			// Generate internal values as required
	void		InvalidateStringValue(void);	// Invalidate the cached string value
	int			GetComparisionType(int nOp1, int nOp2) const;
	void		ArrayDetailsCreate();
	void		ArrayDetailsFree();
	bool		ArrayBoundsCheck(void);			// Checks if requested subscript is in range
	int			ArrayGetElem(void);				// Returns which element of the array corresponds to current array values;
};

///////////////////////////////////////////////////////////////////////////////

#endif
