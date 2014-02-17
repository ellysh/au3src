
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
// script_math.cpp
//
// Contains math/conversion routines.  Part of script.cpp
//
///////////////////////////////////////////////////////////////////////////////


// Includes
#include "StdAfx.h"								// Pre-compiled headers

#ifndef _MSC_VER								// Includes for non-MS compilers
	#include <stdio.h>
	#include <windows.h>
	#include <math.h>
	#include <limits.h>
#else
	#include "qmath.h"							// MinGW doesn't like our asm maths functions
#endif

#include "AutoIt.h"								// Autoit values, macros and config options

#include "script.h"
#include "globaldata.h"
#include "utility.h"
#include "mt19937ar-cok.h"


///////////////////////////////////////////////////////////////////////////////
// Asc()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_Asc(VectorVariant &vParams, Variant &vResult)
{
	vResult = int(uchar(vParams[0].szValue()[0]));
	return AUT_OK;

} // Asc()


///////////////////////////////////////////////////////////////////////////////
// Chr()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_Chr(VectorVariant &vParams, Variant &vResult)
{
	char szTemp[2];

	szTemp[0]	=  (char)vParams[0].nValue();
	szTemp[1]	= '\0';
	vResult		= szTemp;				// Return a string variant of this character code

	return AUT_OK;

} // Chr()


///////////////////////////////////////////////////////////////////////////////
// Dec()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_Dec(VectorVariant &vParams, Variant &vResult)
{
	int		nTemp1;

	if (Util_ConvDec(vParams[0].szValue(), nTemp1) == false)
		SetFuncErrorCode(1);			// error
	vResult = nTemp1;

	return AUT_OK;

} // Dec()


///////////////////////////////////////////////////////////////////////////////
// VarType()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_VarType(VectorVariant &vParams, Variant &vResult)
{
	vResult = vParams[0].type();

	return AUT_OK;

} // VarType()


///////////////////////////////////////////////////////////////////////////////
// Int()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_Int(VectorVariant &vParams, Variant &vResult)
{
	// If the variant is already int32 or int64 then leave alone
	// If the variant is a DOUBLE then convert to an int64
	if (vParams[0].type() == VAR_INT32 || vParams[0].type() == VAR_INT64)
	{
		vResult = vParams[0];
	}
	else if (vParams[0].type() == VAR_DOUBLE || vParams[0].type() == VAR_STRING)
	{
		vResult = vParams[0].n64Value();
	}
	else
	{
		SetFuncErrorCode(1);
		vResult = 0;							// Invalid
	}

	return AUT_OK;

} // Int()


///////////////////////////////////////////////////////////////////////////////
// IsArray()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_IsArray(VectorVariant &vParams, Variant &vResult)
{
	vResult = vParams[0].isArray();
	return AUT_OK;

} // IsArray()


///////////////////////////////////////////////////////////////////////////////
// IsString()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_IsString(VectorVariant &vParams, Variant &vResult)
{
	vResult = vParams[0].isString();
	return AUT_OK;

} // IsString()


///////////////////////////////////////////////////////////////////////////////
// IsInt()
//
// Is the base type numerical AND contains no fractional part
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_IsInt(VectorVariant &vParams, Variant &vResult)
{
	vResult = 0;

	if (vParams[0].isNumber())
	{
		if (vParams[0].type() == VAR_INT32 || vParams[0].type() == VAR_INT64)
			vResult = 1;
		else
		{										// Must be DOUBLE, check for fractions
			__int64 nTemp;

			// Convert to an __int64 and back to a double and compare
			nTemp = (__int64)vParams[0].fValue();
			if ( (double)nTemp == vParams[0].fValue() )
				vResult = 1;
		}
	}

	return AUT_OK;

} // IsInt()


///////////////////////////////////////////////////////////////////////////////
// IsFloat()
//
// Is the base type numerical AND contains a fractional part
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_IsFloat(VectorVariant &vParams, Variant &vResult)
{
	vResult = 0;

	if (vParams[0].isNumber())
	{
		if (vParams[0].type() == VAR_DOUBLE)
		{										// check for fractions
			__int64 nTemp;

			// Convert to an __int64 and back to a double and compare
			nTemp = (__int64)vParams[0].fValue();
			if ( (double)nTemp != vParams[0].fValue() )
				vResult = 1;
		}
	}

	return AUT_OK;

} // IsFloat()


///////////////////////////////////////////////////////////////////////////////
// IsNumber()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_IsNumber(VectorVariant &vParams, Variant &vResult)
{
	vResult = vParams[0].isNumber();
	return AUT_OK;

} // IsNumber()


///////////////////////////////////////////////////////////////////////////////
// Number()
//
// Change to a numerical type - only really valid for strings
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_Number(VectorVariant &vParams, Variant &vResult)
{
	if (vParams[0].isNumber())
		vResult = vParams[0];					// Already numerical
	else
	{
		const char *szTemp = vParams[0].szValue();

		if (strstr(szTemp, "."))				// Contains a decimal point?
			vResult  = atof(szTemp);			// Convert to double
		else
		{
			__int64 n64Temp = _atoi64(szTemp);
			if (n64Temp > INT_MAX || n64Temp < INT_MIN)
				vResult  = n64Temp;				// Store as int64
			else
				vResult  = atoi(szTemp);		// Store as int32
		}
	}

	return AUT_OK;

} // Number()


///////////////////////////////////////////////////////////////////////////////
// String()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_String(VectorVariant &vParams, Variant &vResult)
{
	vResult =  vParams[0].szValue();

	return AUT_OK;

} // Number()


///////////////////////////////////////////////////////////////////////////////
// BitAND()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_BitAND(VectorVariant &vParams, Variant &vResult)
{
	uint	iNumParams = vParams.size();
	int		nRes = vParams[0].nValue();

	for (uint i = 1; i < iNumParams; ++i)
		nRes &= vParams[i].nValue();

	vResult = nRes;

	return AUT_OK;

} // BitAND()


///////////////////////////////////////////////////////////////////////////////
// BitOR()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_BitOR(VectorVariant &vParams, Variant &vResult)
{
	uint	iNumParams = vParams.size();
	int		nRes = vParams[0].nValue();

	for (uint i = 1; i < iNumParams; ++i)
		nRes |= vParams[i].nValue();

	vResult = nRes;

	return AUT_OK;

} // BitOR()


///////////////////////////////////////////////////////////////////////////////
// BitNOT()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_BitNOT(VectorVariant &vParams, Variant &vResult)
{
	vResult = ~vParams[0].nValue();
	return AUT_OK;

} // BitNOT()


///////////////////////////////////////////////////////////////////////////////
// BitXOR()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_BitXOR(VectorVariant &vParams, Variant &vResult)
{
	uint	iNumParams = vParams.size();
	int		nRes = vParams[0].nValue();

	for (uint i = 1; i < iNumParams; ++i)
		nRes ^= vParams[i].nValue();

	vResult = nRes;

	return AUT_OK;

} // BitXOR()


///////////////////////////////////////////////////////////////////////////////
// BitShift()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_BitShift(VectorVariant &vParams, Variant &vResult)
{
	if (vParams[1].nValue() >= 0)
		vResult = vParams[0].nValue() >> vParams[1].nValue();
	else
		vResult = vParams[0].nValue() << (vParams[1].nValue() * -1);
	return AUT_OK;

} // BitShift()



///////////////////////////////////////////////////////////////////////////////
// Random()
// $var = Random( [Min [, Max [, flag]]] )
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_Random(VectorVariant &vParams, Variant &vResult)
{
	double	fTemp1;
	int		nTemp1;
	bool	bInteger = false;					// Defaults to floats for backwards compatibility
	uint	iNumParams = vParams.size();

	// Integer flag?
	if (iNumParams == 3 && vParams[2].nValue() == 1)
		bInteger = true;

	// Default result is 0
	vResult = 0;

	switch(vParams.size())
	{
		case 0:	// return [0, 1]
			vResult = genrand_real2();
			break;

		case 1: // return [0, n]
			fTemp1 = vParams[0].fValue();
			if (fTemp1 <= 0.0)
				SetFuncErrorCode(1);
			else
				vResult = genrand_real2() * fTemp1;	// return [0,1] * fTemp1
			break;

		case 2:	// return [n, m] for int and [n, m] for float
		case 3:
			if (vParams[0].isArray() || vParams[1].isArray())
				SetFuncErrorCode(1);
			else if (bInteger)
			{
				// Integer (32 bit)
				int nTemp2 = vParams[1].nValue();

				nTemp1 = vParams[0].nValue();
				if (nTemp1 > nTemp2)
					SetFuncErrorCode(1);	// invalid range
				else
					vResult = (int)(genrand_int31()%(nTemp2-nTemp1+1) + nTemp1);
			}
			else
			{
				// Float
				double fTemp2 = vParams[1].fValue();

				fTemp1 = vParams[0].fValue();
				if (fTemp1 >= fTemp2)
					SetFuncErrorCode(1);	// invalid range
				else
					vResult = genrand_real2()*(fTemp2-fTemp1) + fTemp1;
			}
			break;
	}	// switch vParams.size()

	return AUT_OK;

} // Random()


/*
///////////////////////////////////////////////////////////////////////////////
// Random()
// $var = Random([[min] ,max]])
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_Random(VectorVariant &vParams, Variant &vResult)
{
	double	fTemp1;

	switch(vParams.size())
	{
		case 0:	// return [0, 1)
			vResult = genrand_real2();
			break;

		case 1: // return [0, n]
			switch(vParams[0].type())
			{
				case VAR_DOUBLE:
					fTemp1 = vParams[0].fValue();
					if (fTemp1 <= 0.0)
						SetFuncErrorCode(2);
					else
						vResult=genrand_real2()*fTemp1;	// return [0,1)
					break;
				default:
					SetFuncErrorCode(1);	// invalid argument type
			}
			break;

		case 2:	// return [n, m] for int and [n, m) for float
			if (vParams[0].isArray() || vParams[1].isArray())
				SetFuncErrorCode(1);
			else
			{
				// Float
				double fTemp2=vParams[1].fValue();

				fTemp1=vParams[0].fValue();
				if (fTemp1>=fTemp2)
					SetFuncErrorCode(2);	// invalid range
				else
					vResult = genrand_real2()*(fTemp2-fTemp1) + fTemp1;
			}
			break;
	}
	return AUT_OK;

} // Random()

*/


///////////////////////////////////////////////////////////////////////////////
// Exp()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_Exp(VectorVariant &vParams, Variant &vResult)
{
#ifdef _MSC_VER
		// MS Compiler
	vResult = qmathExp(vParams[0].fValue());
#else
		// Non MS/Ming
	vResult = exp(vParams[0].fValue());
#endif

	return AUT_OK;

} // Exp()


///////////////////////////////////////////////////////////////////////////////
// Log()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_Log(VectorVariant &vParams, Variant &vResult)
{
#ifdef _MSC_VER
		// MS Compiler
	vResult = qmathLog(vParams[0].fValue());
#else
		// Non MS/Ming
	vResult = log(vParams[0].fValue());
#endif

	return AUT_OK;

} // Log()


///////////////////////////////////////////////////////////////////////////////
// ATan()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_ATan(VectorVariant &vParams, Variant &vResult)
{
#ifdef _MSC_VER
		// MS Compiler
	vResult = qmathAtan(vParams[0].fValue());
#else
		// Non MS/Ming
	vResult = atan(vParams[0].fValue());
#endif

	return AUT_OK;

} // ATab()


///////////////////////////////////////////////////////////////////////////////
// ACos()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_ACos(VectorVariant &vParams, Variant &vResult)
{
#ifdef _MSC_VER
		// MS Compiler
	vResult = qmathAcos(vParams[0].fValue());
#else
		// Non MS/Ming
	vResult = acos(vParams[0].fValue());
#endif

	return AUT_OK;

} // ACos()


///////////////////////////////////////////////////////////////////////////////
// ASin()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_ASin(VectorVariant &vParams, Variant &vResult)
{
#ifdef _MSC_VER
		// MS Compiler
	vResult = qmathAsin(vParams[0].fValue());
#else
		// Non MS/Ming
	vResult = asin(vParams[0].fValue());
#endif

	return AUT_OK;

} // ASin()


///////////////////////////////////////////////////////////////////////////////
// Tan()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_Tan(VectorVariant &vParams, Variant &vResult)
{
#ifdef _MSC_VER
		// MS Compiler
	vResult = qmathTan(vParams[0].fValue());
#else
		// Non MS/Ming
	vResult = tan(vParams[0].fValue());
#endif

	return AUT_OK;

} // Tan()


///////////////////////////////////////////////////////////////////////////////
// Cos()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_Cos(VectorVariant &vParams, Variant &vResult)
{
#ifdef _MSC_VER
		// MS Compiler
	vResult = qmathCos(vParams[0].fValue());
#else
		// Non MS/Ming
	vResult = cos(vParams[0].fValue());
#endif

	return AUT_OK;

} // Cos()


///////////////////////////////////////////////////////////////////////////////
// Sin()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_Sin(VectorVariant &vParams, Variant &vResult)
{
#ifdef _MSC_VER
		// MS Compiler
	vResult = qmathSin(vParams[0].fValue());
#else
		// Non MS/Ming
	vResult = sin(vParams[0].fValue());
#endif

	return AUT_OK;

} // Sin()


///////////////////////////////////////////////////////////////////////////////
// Abs()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_Abs(VectorVariant &vParams, Variant &vResult)
{
#ifdef _MSC_VER
		// MS Compiler
	vResult = qmathFabs(vParams[0].fValue());

#else
		// Non MS/Ming
	vResult = fabs(vParams[0].fValue());
#endif

	return AUT_OK;

} // Abs()


///////////////////////////////////////////////////////////////////////////////
// Mod()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_Mod(VectorVariant &vParams, Variant &vResult)
{
#ifdef _MSC_VER
		// MS Compiler
	vResult = qmathFmod(vParams[0].fValue(), vParams[1].fValue());
#else
		// Non MS/Ming
	vResult = fmod(vParams[0].fValue(), vParams[1].fValue());
#endif

	return AUT_OK;

} // Mod()


///////////////////////////////////////////////////////////////////////////////
// Sqrt()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_Sqrt(VectorVariant &vParams, Variant &vResult)
{
#ifdef _MSC_VER
		// MS Compiler
	vResult = qmathSqrt(vParams[0].fValue());
#else
		// Non MS/Ming
	vResult = sqrt(vParams[0].fValue());
#endif

	return AUT_OK;

} // Sqrt()


///////////////////////////////////////////////////////////////////////////////
// Round()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_Round(VectorVariant &vParams, Variant &vResult)
{
	int		nTemp1;
	double	fTemp1;

#ifdef _MSC_VER
		// MS Compiler
	if (vParams.size() == 1)
		nTemp1 = 0;
	else
		nTemp1 = vParams[1].nValue();

	fTemp1 = qmathPow(10.0, nTemp1);
	if (vParams[0].fValue() >= 0.0)
		vResult = qmathFloor(vParams[0].fValue()*fTemp1+0.5)/fTemp1;
	else
		vResult = qmathCeil(vParams[0].fValue()*fTemp1-0.5)/fTemp1;

#else
		// Non MS/Ming
	if (vParams.size() == 1)
		nTemp1 = 0;
	else
		nTemp1 = vParams[1].nValue();

	fTemp1 = pow(10.0, nTemp1);
	if (vParams[0].fValue() >= 0.0)
		vResult = floor(vParams[0].fValue()*fTemp1+0.5)/fTemp1;
	else
		vResult = ceil(vParams[0].fValue()*fTemp1-0.5)/fTemp1;

#endif

	return AUT_OK;

} // Round()


///////////////////////////////////////////////////////////////////////////////
// Hex()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_Hex(VectorVariant &vParams, Variant &vResult)
{
	char szTemp1[1024+1];

	if (vParams[1].nValue() > 8)
	{
		vResult = "";
		SetFuncErrorCode(1);
		return AUT_OK;
	}
	if (Util_ConvHex(vParams[0].nValue(), szTemp1, vParams[1].nValue()) == false)
		SetFuncErrorCode(1);			// left overs / not enough digits
	vResult = szTemp1;

	return AUT_OK;

} // Hex()

