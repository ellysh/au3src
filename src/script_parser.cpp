
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
// script_parser.cpp
//
// Contains general parsing code.  Part of script.cpp
//
///////////////////////////////////////////////////////////////////////////////


// Includes
#include "StdAfx.h"								// Pre-compiled headers

#ifndef _MSC_VER								// Includes for non-MS compilers
	#include <windows.h>
#endif

#include "AutoIt.h"								// Autoit values, macros and config options

#include "script.h"
#include "resources\resource.h"
#include "globaldata.h"


///////////////////////////////////////////////////////////////////////////////
// Parser()
///////////////////////////////////////////////////////////////////////////////

void AutoIt_Script::Parser(VectorToken &vLineToks, int &nScriptLine)
{
	uint	ivPos;						// Position in the vector
	Variant	vTemp;						// Temp variant for operations

	// A line can either start with an assignment or a keyword or a function
	// $var =
	// If/while/endif/etc.
	// function(...)

	ivPos = 0;

	switch ( vLineToks[ivPos].m_nType )
	{
		case TOK_VARIABLE:
			Parser_StartWithVariable(vLineToks, ivPos);
			break;

		case TOK_KEYWORD:
			Parser_StartWithKeyword(vLineToks, ivPos, nScriptLine);
			break;

		case TOK_FUNCTION:
			if ( AUT_SUCCEEDED(Parser_FunctionCall(vLineToks, ivPos, vTemp) ) )
			{
				// Next token must be END (otherwise there is other crap on the line)
				if (vLineToks[ivPos].m_nType != TOK_END)
				{
					FatalError(IDS_AUT_E_EXTRAONLINE, vLineToks[ivPos].m_nCol);
					return;
				}
			}

			break;

		case TOK_USERFUNCTION:
			if ( AUT_SUCCEEDED(Parser_UserFunctionCall(vLineToks, ivPos, vTemp)) )
			{
				// Next token must be END (otherwise there is other crap on the line)
				if (vLineToks[ivPos].m_nType != TOK_END)
				{
					FatalError(IDS_AUT_E_EXTRAONLINE, vLineToks[ivPos].m_nCol);
					return;
				}
			}

			break;

		case TOK_END:
			// Do nothing - blank line/comment
			break;

		default:
			// Error
			FatalError(IDS_AUT_E_GENPARSE, vLineToks[ivPos].m_nCol);
			break;
	}

} // Parser()


///////////////////////////////////////////////////////////////////////////////
// Parser_FunctionCall()
//
// Calls a BUILT-IN function
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::Parser_FunctionCall(VectorToken &vLineToks, uint &ivPos, Variant &vResult)
{
	VectorVariant	vParams;					// Vector array of the parameters for this function
	int				nNumParams;

	// Get the function name
	int nFunction	= vLineToks[ivPos].nValue;
	int	nColTemp	= vLineToks[ivPos].m_nCol;

	// Parse the function call and populate vParams
	if ( AUT_FAILED(Parser_GetFunctionCallParams(vParams, vLineToks, ivPos, nNumParams )) )
		return AUT_ERR;

	// How many parameters did we find?
	if ( nNumParams < m_FuncList[nFunction].nMin || nNumParams > m_FuncList[nFunction].nMax)
	{
		FatalError(IDS_AUT_E_FUNCTIONNUMPARAMS, nColTemp);
		return AUT_ERR;
	}

	// Call the function
	if ( AUT_FAILED( FunctionExecute(nFunction, vParams, vResult) ) )
		return AUT_ERR;

	return AUT_OK;

} // Parser_FunctionCall()


///////////////////////////////////////////////////////////////////////////////
// Parser_GetFunctionCallParams()
//
// This generates a vectorvariant (params) for a "simple" function call (no byrefs etc)
// Used for generating the parameters for built-in and plugin functions.  User function calls
// use a different routine.
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::Parser_GetFunctionCallParams(VectorVariant &vParams, VectorToken &vLineToks, uint &ivPos, int &nNumParams)
{
	VectorToken		vFuncToks;					// Vector of tokens for THIS FUNCTION CALL
	Variant			vTemp;
	uint			ivFuncPos;
	Token			tok;
	int				nColTemp;

	// Tokens should be:
	// function ( expression , expression , ... )

	// Store the char number of the function name in case we need to generate
	// an error
	nColTemp = vLineToks[ivPos].m_nCol;

	++ivPos;									// Skip function name

	if ( vLineToks[ivPos].m_nType != TOK_LEFTPAREN )
	{
		FatalError(IDS_AUT_E_GENFUNCTION, vLineToks[ivPos-1].m_nCol);	// Error points to function name ( might = END (i.e not present)
		return AUT_ERR;
	}
	++ivPos;										// Skip (

	// Loop until we hit ) or END storing tokens as we go
	// Watch out for nested function calls
	int nFuncCount = 0;
	bool bFinished = false;
	while ( bFinished == false )
	{
		switch (vLineToks[ivPos].m_nType)
		{
			case TOK_LEFTPAREN:
				++nFuncCount;
				vFuncToks.push_back(vLineToks[ivPos]);
				++ivPos;
				break;

			case TOK_RIGHTPAREN:
				if (nFuncCount == 0)
				{
					bFinished = true;
					++ivPos;					// Skip final )
				}
				else
				{
					nFuncCount--;
					vFuncToks.push_back(vLineToks[ivPos]);
					++ivPos;
				}
				break;

			case TOK_END:
				FatalError(IDS_AUT_E_GENFUNCTION, vLineToks[ivPos-1].m_nCol);
				return AUT_ERR;

			default:
				vFuncToks.push_back(vLineToks[ivPos]);
				++ivPos;
				break;
		}
	}


	tok.settype(TOK_END);						// Add an END token
	tok.m_nCol = nColTemp;						// Just in case someone uses this value for the error message
	vFuncToks.push_back(tok);


	// Loop around until we hit END, count and store the evaluated expressions as we go
	nNumParams = 0;
	ivFuncPos = 0;
	while ( vFuncToks[ivFuncPos].m_nType != TOK_END  )
	{
		// Parse an "expression" (or parameter)
		if ( AUT_FAILED( Parser_EvaluateExpression(vFuncToks, ivFuncPos, vTemp) ) )
			return AUT_ERR;

		// Increase the number of expressions parsed and store the result in our
		// parameter vector
		++nNumParams;
		vParams.push_back(vTemp);

		// Did the parse function cause us to goto the end of our tokens?
		if ( vFuncToks[ivFuncPos].m_nType == TOK_END )
			break;

		// If the next token is a comma it means that there are more parameters to read
		if ( vFuncToks[ivFuncPos].m_nType == TOK_COMMA )
			++ivFuncPos;

	} // End While



	// vParams now contains all the parameters needed to call our function - yay

	return AUT_OK;

} // Parser_GetFunctionCallParams()


///////////////////////////////////////////////////////////////////////////////
// Parser_FindUserFunction()
//
// Checks if a given user function exists, and if so, returns details
//
///////////////////////////////////////////////////////////////////////////////

bool AutoIt_Script::Parser_FindUserFunction(const char *szName, int &nLineNum, int &nNumParams,int &nNumParamsMin, int &nEndLineNum)
{
	UserFuncDetails *uDetails = m_oUserFuncList.find(szName);

	if (uDetails == NULL)
		return false;							// Not found
	else
	{
		nNumParams		= uDetails->nNumParams;
		nNumParamsMin	= uDetails->nNumParamsMin;
		nLineNum		= uDetails->nFuncLineNum;
		nEndLineNum		= uDetails->nEndFuncLineNum;
		return true;
	}

} // Parser_FindUserFunction()


///////////////////////////////////////////////////////////////////////////////
// Parser_UserFunctionCall()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::Parser_UserFunctionCall(VectorToken &vLineToks, uint &ivPos, Variant &vResult)
{
	int		nLineNum; 							// Line number of function to call
	int		nNumParamsMax;						// Max Number of parameters for function
	int		nNumParamsMin;						// Min Number of parameters for function
	int		nEndLineNum;						// Line number of endfunc

	// Save the column of the function for any error messages
	int nColTemp = vLineToks[ivPos].m_nCol;

	// Check that this user function exists, and get the details of it
	if (Parser_FindUserFunction(vLineToks[ivPos].szValue, nLineNum, nNumParamsMax, nNumParamsMin, nEndLineNum) == false)
	{
		// Wasn't a recognised user function - try and and run it as a plugin function
//		if (Parser_PluginFunctionCall(vLineToks, ivPos, vResult) == false)
//		{
			FatalError(IDS_AUT_E_UNKNOWNUSERFUNC, nColTemp);
			return AUT_ERR;
//		}
//		else
//			return AUT_OK;
	}

	VectorVariant	vParams;					// Vector array of the parameters for this function
	VectorToken		vFuncToks;					// Vector of tokens for THIS FUNCTION CALL
	VectorToken		vFuncDecToks;				// Vector of tokens for the function declaration
	int				i;
	Variant			vTemp;
	Variant			*pvTemp;
	bool			bConst = false;
	uint			ivFuncPos, ivFuncDecPos;
	uint			ivParamPos;
	Token			tok;
	AString			sLine;						// Temp line buffer
	const char		*szTempScriptLine;
	GenStatement	tFuncDetails;

	// Make a statement stack entry for this function (we will push it on stack when params verified)
	tFuncDetails.nType		= L_FUNC;
	tFuncDetails.nFunc		= nLineNum;
	tFuncDetails.nEndFunc	= nEndLineNum;


	// Tokens should be:
	// function ( expression , expression , ... )
	++ivPos;									// Skip function name
	if ( vLineToks[ivPos].m_nType != TOK_LEFTPAREN )
	{
		FatalError(IDS_AUT_E_GENFUNCTION, nColTemp) ;
		return AUT_ERR;
	}
	++ivPos;										// Skip (


	// Loop until we hit ) or END storing tokens as we go
	// Watch out for nested function calls
	int nFuncCount = 0;
	bool bFinished = false;
	while ( bFinished == false )
	{
		switch (vLineToks[ivPos].m_nType)
		{
			case TOK_LEFTPAREN:
				++nFuncCount;
				vFuncToks.push_back(vLineToks[ivPos]);
				++ivPos;
				break;

			case TOK_RIGHTPAREN:
				if (nFuncCount == 0)
				{
					bFinished = true;
					++ivPos;					// Skip final )
				}
				else
				{
					nFuncCount--;
					vFuncToks.push_back(vLineToks[ivPos]);
					++ivPos;
				}
				break;

			case TOK_END:
				FatalError(IDS_AUT_E_GENFUNCTION, nColTemp);
				return AUT_ERR;

			default:
				vFuncToks.push_back(vLineToks[ivPos]);
				++ivPos;
				break;
		}
	}

	tok.settype(TOK_END);						// Add an END token to our buffer
	vFuncToks.push_back(tok);

	//
	// vFuncToks now just contains the tokens for the parameters of a function call.
	//

	ivFuncPos = 0;				// First token

	// Ok, now we re-read the function declaration (to check for ByRefs) and make a list
	// of the values or references that we will pass.  We must make a list and not assign them
	// now as parameters may contain other function calls which would change the scope!  Ack.
	// Also to note, the function declaration was syntax checked when the script was loaded, this
	// helps.

	// Read in our function declaration
	szTempScriptLine = g_oScriptFile.GetLine(nLineNum);
	Lexer(nLineNum, szTempScriptLine, vFuncDecToks);
	ivFuncDecPos = 0;
	ivFuncDecPos += 3;							// Skip "Func", funcname and "("


	int nNumParams = 0;

	for (i=0; i<nNumParamsMax; ++i)
	{
		// If we hit an END token we have run out of parameters
		if (vFuncToks[ivFuncPos].m_nType == TOK_END)
			break;

		++nNumParams;		// increment current number

		// Is the declaration for this parameter a value or reference?
		if (vFuncDecToks[ivFuncDecPos].m_nType == TOK_KEYWORD && vFuncDecToks[ivFuncDecPos].nValue == K_BYREF)
		{
			// Reference, get a pointer to the named variable
			if (vFuncToks[ivFuncPos].m_nType != TOK_VARIABLE)
			{
				FatalError(IDS_AUT_E_FUNCTIONEXPECTEDVARIABLE, vFuncToks[ivFuncPos].m_nCol);
				return AUT_ERR;
			}

			if (g_oVarTable.GetRef(vFuncToks[ivFuncPos].szValue, &pvTemp, bConst) == false)
			{
				FatalError(IDS_AUT_E_VARNOTFOUND, vFuncToks[ivFuncPos].m_nCol);
				return AUT_ERR;
			}
			else if (bConst)
			{
				// Can't use a constant for a ByRef!
				FatalError(IDS_AUT_E_ASSIGNTOCONST, vFuncToks[ivFuncPos].m_nCol);
				return AUT_ERR;
			}

			++ivFuncPos;						// Skip $var name

			// If this variable is an array AND the next token is [ then try to get the reference
			// of the actual ELEMENT rather than the whole array
			if (pvTemp->type() == VAR_ARRAY && vFuncToks[ivFuncPos].m_nType == TOK_LEFTSUBSCRIPT)
			{
				if ( AUT_FAILED(Parser_GetArrayElement(vFuncToks, ivFuncPos, &pvTemp)) )
					return AUT_ERR;
			}

			vTemp = pvTemp;						// vTemp is now a variant that refers to another

			ivFuncDecPos += 2;					// Skip ByRef and $var in declaration

			// Make sure that the next token in the user call is a comma or END
			if ( vFuncToks[ivFuncPos].m_nType != TOK_COMMA && vFuncToks[ivFuncPos].m_nType != TOK_END)
			{
				FatalError(IDS_AUT_E_FUNCTIONEXPECTEDVARIABLE, vFuncToks[ivFuncPos].m_nCol);
				return AUT_ERR;
			}

		}
		else
		{
			// Value
			// Parse an "expression" (or parameter)
			if ( AUT_FAILED( Parser_EvaluateExpression(vFuncToks, ivFuncPos, vTemp) ) )
				return AUT_ERR;

			++ivFuncDecPos;						// Skip $var in declaration

			// If this contains a default parameter then skip "= literal" as well
			if (vFuncDecToks[ivFuncDecPos].m_nType == TOK_EQUAL)
				ivFuncDecPos +=2;
		}

		// Add the value (or reference...) onto our list of params to pass
		vParams.push_back(vTemp);

		++ivFuncDecPos;							// Skip (potential) comma in function declaration

		// If the next token is a comma (user call) it means that there are more parameters to read
		if ( vFuncToks[ivFuncPos].m_nType == TOK_COMMA )
			++ivFuncPos;

	} // End For


	// If the next token in the function call is not END, then too many parameters have been passed.
	if ( vFuncToks[ivFuncPos].m_nType != TOK_END  )
	{
		FatalError(IDS_AUT_E_FUNCTIONNUMPARAMS, vFuncToks[ivFuncPos].m_nCol);
		return AUT_ERR;
	}

	// Also check that too few parameters have not been passed
	if (nNumParams < nNumParamsMin)
	{
		// too small number of parameters
		FatalError(IDS_AUT_E_FUNCTIONNUMPARAMS, nColTemp);
		return AUT_ERR;
	}

	//Util_WinPrintf("", "%d %d", nNumParams, nNumParamsMax);


	// Ok, we have parsed the function call and have a list of the parameters (values and/or reference)
	// that we need to pass, now increase the scope and reread the function declaration and assign
	// values or references as required
	// nNumParams = the number of parameters passed
	// nNumParamsMax = the maximum number of parameters that the function can take
	// One thing to note, when the script started the StoreUserFuncs() function checked that the Func
	// declaration was correct, so if there are optional params (nNumParams < nNumParamsMax) then we can
	// assume they are properly formatted as $var = literal.
	ivFuncDecPos = 0;							// Back to the start of the declaration
	ivFuncDecPos += 3;							// Skip "Func", funcname and "("
	ivParamPos = 0;								// Back to the start of our parameter list

	g_oVarTable.ScopeIncrease();				// Increase scope

	// Create new variables with the values we worked out above
	for (i=1; i<=nNumParamsMax; ++i)
	{
		// Is the declaration for this parameter a value or reference?
		if (vFuncDecToks[ivFuncDecPos].m_nType == TOK_KEYWORD && vFuncDecToks[ivFuncDecPos].nValue == K_BYREF)
		{
			// Reference, create a reference
			++ivFuncDecPos;	// Skip ByRef keyword

			if (g_oVarTable.CreateRef(vFuncDecToks[ivFuncDecPos].szValue, vParams[ivParamPos].pValue() ) == false)
			{
				FatalError(IDS_AUT_E_VARNOTFOUND, vFuncDecToks[ivFuncDecPos].m_nCol);
				return AUT_ERR;
			}
			ivFuncDecPos += 2;					// Skip $varname and comma
		}
		else
		{
			// Passed value, or use default value
			if (i > nNumParams)
			{
				// We need to use a default value, we CAN assume that tokens are - "$var = [+ -] literal"
				VectorToken	vTempExp;
				uint		ivTempExpPos = 0;

				// Optional sign?
				if (vFuncDecToks[ivFuncDecPos+2].m_nType == TOK_MINUS || vFuncDecToks[ivFuncDecPos+2].m_nType == TOK_PLUS)
				{
					vTempExp.push_back( vFuncDecToks[ivFuncDecPos+2] );	// Sign
					vTempExp.push_back( vFuncDecToks[ivFuncDecPos+3] );	// Literal
				}
				else
					vTempExp.push_back( vFuncDecToks[ivFuncDecPos+2] );	// Literal

				// Add END token
				tok.settype(TOK_END);						// Add an END token to our buffer
				vTempExp.push_back(tok);

				// Evaluate this simple expression
				Parser_EvaluateExpression(vTempExp, ivTempExpPos, vTemp);

				g_oVarTable.Assign(vFuncDecToks[ivFuncDecPos].szValue, vTemp, false, VARTABLE_FORCELOCAL);
			}
			else
			{
				// Value
				g_oVarTable.Assign(vFuncDecToks[ivFuncDecPos].szValue, vParams[ivParamPos], false, VARTABLE_FORCELOCAL);
			}

			// We need to skip either 2 places ($var ,) or if the declaration contained a default then we
			// skip 4/5 places ($var = [+ -] literal ,)
			if (vFuncDecToks[ivFuncDecPos+1].m_nType == TOK_EQUAL)
			{
				if (vFuncDecToks[ivFuncDecPos+2].m_nType == TOK_MINUS || vFuncDecToks[ivFuncDecPos+2].m_nType == TOK_PLUS)
					ivFuncDecPos += 5;			// Skip ($var = +/- literal ,)
				else
					ivFuncDecPos += 4;			// Skip ($var = literal ,)
			}
			else
				ivFuncDecPos += 2;				// Skip ($var ,)
		}

		++ivParamPos;							// Next param

	} // End For


	// We will now continue by using a recursive call of the Execute() function.

	m_StatementStack.push(tFuncDetails);		// Store details of the func on the stack so we can track what
												// function we are currently executing
	m_nNumParams = nNumParams;					// memorize number of parameters to be return by @NUMPARAMS
	m_vUserRetVal = 0;							// Default userfunction return value is zero
	SetFuncErrorCode(0);						// As with built in functions, reset the @error values
	SetFuncExtCode(0);
	SaveExecute(nLineNum+1, false, false);		// Save state and run the user function (line after the Func declaration)
	vResult = m_vUserRetVal;					// Get the return value

	// Pop the function
	m_StatementStack.pop();

	// Descrease the variable scope - deletes function local variables
	g_oVarTable.ScopeDecrease();

	return AUT_OK;

} // Parser_UserFunctionCall()


///////////////////////////////////////////////////////////////////////////////
// Parser_PluginFunctionCall()
//
// This function is called from within Parser_UserFunctionCall()
// (Plugin functions currently assumed to be user functions)
//
///////////////////////////////////////////////////////////////////////////////

bool AutoIt_Script::Parser_PluginFunctionCall(VectorToken &vLineToks, uint &ivPos, Variant &vResult)
{
	return false;
/*
typedef int (*PLUGINFUNC)(VectorVariant &vParams, Variant &vResult, int &nError);

	VectorVariant	vParams;
	int				nNumParams;
	int				nError;

	// Parse the function call and populate vParams
	if ( AUT_FAILED(Parser_GetFunctionCallParams(vParams, vLineToks, ivPos, nNumParams )) )
		return false;



	PLUGINFUNC		lpfnPlugin;
	HINSTANCE hLib = LoadLibrary("example.dll");

	if (hLib == NULL)
	{
		AUT_MSGBOX("LoadLib failed", "")
		return false;
	}

    lpfnPlugin = (PLUGINFUNC)GetProcAddress(hLib, "PluginTest");

	if (lpfnPlugin == NULL)
	{
		AUT_MSGBOX("GetProc failed", "")
		return false;
	}

	lpfnPlugin(vParams, vResult, nError);

	AUT_MSGBOX("Result", vResult.szValue())


	return false;								// Error calling plugin / not recognised
*/

} // Parser_PluginFunctionCall()


///////////////////////////////////////////////////////////////////////////////
// Parser_StartWithVariable()
///////////////////////////////////////////////////////////////////////////////

void AutoIt_Script::Parser_StartWithVariable(VectorToken &vLineToks, uint &ivPos)
{
	Variant	vTemp;								// Resulting variant
	Variant *pvTemp;
	bool	bConst = false;
	bool	bNeedToCreate = false;


	// Get the name of the result variable
	AString sVarName = vLineToks[ivPos].szValue;

	// Get a reference to the variable, if it doesn't exist, then create it.  If the
	// variable is a constant then give an error
	g_oVarTable.GetRef(sVarName, &pvTemp, bConst);
	if (pvTemp == NULL)
	{
		// Variable does not exist yet

		// If m_bMustDeclareVars is true then we must give a hard error here
		if (m_bMustDeclareVars == true)
		{
			FatalError(IDS_AUT_E_VARNOTFOUND, vLineToks[ivPos].m_nCol);
			return;
		}

		// Don't create if the next token was the start of an array [
		// In this case we need to give a hard error
		if (vLineToks[ivPos+1].m_nType == TOK_LEFTSUBSCRIPT)
		{
			FatalError(IDS_AUT_E_ARRAYUSEDNODIM, vLineToks[ivPos].m_nCol);
			return;
		}
		else
		{
			// Non array - we need to create the variable
			bNeedToCreate = true;
		}

		++ivPos;									// Next token (should be = )
	}
	else
	{
		// Variable already exists

		// Fail if this is a constant
		if (bConst)
		{
			FatalError(IDS_AUT_E_ASSIGNTOCONST, vLineToks[ivPos].m_nCol);
			return;
		}

		++ivPos;									// Next token (should be = or [ for an array)

		// If it is an array and a subscript token is next then get the reference of the array element we want to change
		if (pvTemp->type() == VAR_ARRAY && vLineToks[ivPos].m_nType == TOK_LEFTSUBSCRIPT)
		{
			if ( AUT_FAILED(Parser_GetArrayElement(vLineToks, ivPos, &pvTemp)) )
				return;
		}

	}

	// Next token must be equals, followed by an expression
	if ( vLineToks[ivPos].m_nType != TOK_EQUAL )
	{
		FatalError(IDS_AUT_E_EXPECTEDASSIGNMENT, vLineToks[ivPos].m_nCol);
		return;
	}

	// Skip to start of expression then evaluate
	++ivPos;
	if ( AUT_FAILED( Parser_EvaluateExpression(vLineToks, ivPos, vTemp) ) )
		return;

	// Next token must be END (otherwise there is other crap on the line)
	if (vLineToks[ivPos].m_nType != TOK_END)
	{
		FatalError(IDS_AUT_E_EXTRAONLINE, vLineToks[ivPos].m_nCol);
		return;
	}

	// Now, did the original var already exist or should we create it now?
	if (bNeedToCreate)
	{
		Variant vTempCreate;
		g_oVarTable.Assign(sVarName, vTempCreate);
		g_oVarTable.GetRef(sVarName, &pvTemp, bConst);
	}

	// Change the value in the variable table to this resulting value
	*pvTemp = vTemp;

} // Parser_StartWithVariable()


///////////////////////////////////////////////////////////////////////////////
// Parser_StartWithKeyword()
///////////////////////////////////////////////////////////////////////////////

void AutoIt_Script::Parser_StartWithKeyword(VectorToken &vLineToks, uint &ivPos, int &nScriptLine)
{
	int nTemp1, nTemp2, nTemp3, nTemp4;

	// Call the relevant keyword
	switch (vLineToks[ivPos].nValue)
	{
		case K_IF:
			Parser_Keyword_IF(vLineToks, ivPos, nScriptLine);
			break;
		case K_ELSE:
		case K_ELSEIF:
			Parser_Keyword_ELSE(nScriptLine);
			break;
		case K_ENDIF:
			Parser_Keyword_ENDIF(vLineToks, ivPos);
			break;

		case K_WHILE:
			Parser_Keyword_WHILE(vLineToks, ivPos, nScriptLine);
			break;
		case K_WEND:
			Parser_Keyword_WEND(vLineToks, ivPos, nScriptLine);
			break;

		case K_EXITLOOP:
			Parser_Keyword_EXITLOOP(vLineToks, ivPos, nScriptLine);
			break;
		case K_CONTINUELOOP:
			Parser_Keyword_CONTINUELOOP(vLineToks, ivPos, nScriptLine);
			break;

		case K_DO:
			Parser_Keyword_DO(vLineToks, ivPos, nScriptLine);
			break;
		case K_UNTIL:
			Parser_Keyword_UNTIL(vLineToks, ivPos, nScriptLine);
			break;

		case K_FOR:
			Parser_Keyword_FOR(vLineToks, ivPos, nScriptLine);
			break;
		case K_NEXT:
			Parser_Keyword_NEXT(vLineToks, ivPos, nScriptLine);
			break;

		case K_SELECT:
			Parser_Keyword_SELECT(vLineToks, ivPos, nScriptLine);
			break;
		case K_CASE:
			Parser_Keyword_CASE(nScriptLine);
			break;
		case K_ENDSELECT:
			Parser_Keyword_ENDSELECT(vLineToks, ivPos);
			break;

		case K_DIM:
			Parser_Keyword_DIM(vLineToks, ivPos, VARTABLE_ANY);
			break;
		case K_CONST:
			Parser_Keyword_CONST(vLineToks, ivPos, VARTABLE_ANY);
			break;
		case K_REDIM:
			Parser_Keyword_DIM(vLineToks, ivPos, VARTABLE_RESIZE);
			break;
		case K_LOCAL:
			Parser_Keyword_DIM(vLineToks, ivPos, VARTABLE_FORCELOCAL);
			break;
		case K_GLOBAL:
			Parser_Keyword_DIM(vLineToks, ivPos, VARTABLE_FORCEGLOBAL);
			break;

		case K_EXIT:
			Parser_Keyword_EXIT(vLineToks, ivPos);
			break;

		case K_ENDFUNC:
			m_bUserFuncReturned = true;			// Request exit from Execute()
			break;

		case K_RETURN:
			Parser_Keyword_RETURN(vLineToks, ivPos);
			m_bUserFuncReturned = true;			// Request exit from Execute()
			break;

		case K_FUNC:
			// The Func keyword will have been syntax checked in storeuserfuncs()
			++ivPos;							// Skip keyword Func
			Parser_FindUserFunction(vLineToks[ivPos].szValue, nTemp1, nTemp2, nTemp3, nTemp4);
			nScriptLine = nTemp4 + 1;		// Continue execution after EndFunc
			break;

		default:
			// Can't start with this keyword error
			FatalError(IDS_AUT_E_INVALIDSTARTKEYWORD);
			break;

	}

} // Parser_StartWithKeyword()


///////////////////////////////////////////////////////////////////////////////
// Parser_Keyword_IF()
///////////////////////////////////////////////////////////////////////////////

void AutoIt_Script::Parser_Keyword_IF(VectorToken &vLineToks, uint &ivPos, int &nScriptLine)
{
	bool			bCondition;
	VectorToken	vIFToks;
	uint			ivTempPos;					// Position in the vector
	GenStatement	tIFDetails;
	int				nIfCount;					// Keep track of number of nested IF statements
	bool			bElseFound = false;
	bool			bEndIfFound = false;
	const char		*szTempScriptLine;
	int				nLineToExecute = 0;

	// Zero our IF statement
	tIFDetails.nType	= L_IF;					// Not actually used for IF, but whatever


	++ivPos;									// Skip keyword and evaluate condition

	if ( AUT_FAILED( Parser_EvaluateCondition(vLineToks, ivPos, bCondition) ) )
		return;

	// For a valid statement, next token after expression must be "then" keyword
	if (vLineToks[ivPos].m_nType == TOK_KEYWORD && vLineToks[ivPos].nValue == K_THEN)
		++ivPos;								// skip Then keyword
	else
	{
		FatalError(IDS_AUT_E_MISSINGTHEN, vLineToks[ivPos].m_nCol);
		return;
	}

	// Next token must be END (otherwise it is a single line if)
	if (vLineToks[ivPos].m_nType != TOK_END)
	{
		// There is something else on the line, we assume it is a single line IF statement.
		// So we generate a new set of tokens from here to the end and recursively call
		// the parser again...eeek.  Only if the condition was true of course!
		if (bCondition)
		{
			while (vLineToks[ivPos].m_nType != TOK_END)
				vIFToks.push_back(vLineToks[ivPos++]);

			vIFToks.push_back(vLineToks[ivPos]);	// And the end token as well :)

			Parser(vIFToks, nScriptLine);			// Run the parser on this as a "new line"
		}

		return;
	}


	// Block IF statment if we are here


	// Store the line number of the IF statement
	tIFDetails.nIf = nScriptLine - 1;

	// If the main condition was true set the current line to be the one to execute (current line is line after IF)
	if (bCondition)
		nLineToExecute = nScriptLine;


	// Look for an ELSE and/or ENDIF statement (skipping nested IFs)
	nIfCount = 0;

	while ( (szTempScriptLine = g_oScriptFile.GetLine(nScriptLine++)) != NULL
			&& bEndIfFound == false)
	{
		Lexer(nScriptLine-1, szTempScriptLine, vIFToks);

		ivTempPos = 0;

		if (vIFToks[ivTempPos].m_nType == TOK_KEYWORD)
		{
			switch (vIFToks[ivTempPos].nValue)
			{
				case K_IF:
					// Nested IF
					// We can also have a groups of single line ifs on the same line, so check for a THEN
					// keywords - if it is followed by END then class as a nested IF, otherwise keep searching
					// and if we get to the end then it is NOT a nested if, it is just a single line IF
					while(++ivTempPos < (uint)vIFToks.size())
					{
						if (vIFToks[ivTempPos].m_nType == TOK_KEYWORD && vIFToks[ivTempPos].nValue == K_THEN)
						{
							// If the next token is END then this is nested
							if (vIFToks[ivTempPos+1].m_nType == TOK_END)
							{
								++nIfCount;		// Increased nested IF count
								break;			// Exit the while loop
							}
						}
					}
					break;

				case K_ELSE:
					// Ignore if this is a nested IF
					if (nIfCount != 0 )
						break;

					// Check the we haven't already had an ELSE for this IF
					if (bElseFound == true)
					{
						FatalError(IDS_AUT_E_TOOMANYELSE);
						return;
					}

					bElseFound = true;			// We have found the single else for this IF

					// Next token must be TOK_END
					++ivTempPos;
					if (vIFToks[ivTempPos].m_nType != TOK_END)
					{
						m_nErrorLine = nScriptLine - 1;	// So the error message is correct
						FatalError(IDS_AUT_E_EXTRAONLINE, vIFToks[ivTempPos].m_nCol);
						return;
					}

					// If the main IF condition was true, we don't do anything else
					if (bCondition)
						break;

					// Set the line to execute as the current one (line after the ELSE)
					nLineToExecute = nScriptLine;

					break;

				case K_ELSEIF:
					// Ignore if this is a nested IF
					if (nIfCount != 0)
						break;

					// Check the we haven't already had an ELSE for this IF
					if (bElseFound == true)
					{
						FatalError(IDS_AUT_E_TOOMANYELSE);
						return;
					}

					// If the main IF condition was true, we don't do anything else
					 if (bCondition)
						 break;

					++ivTempPos;									// Skip keyword and evaluate condition

					m_nErrorLine = nScriptLine - 1;	// So any error message is correct from the evalcondition below

					if ( AUT_FAILED( Parser_EvaluateCondition(vIFToks, ivTempPos, bCondition) ) )
						return;

					// For a valid statement, next token after expression must be "then" keyword
					if ( !(vIFToks[ivTempPos].m_nType == TOK_KEYWORD && vIFToks[ivTempPos].nValue == K_THEN) )
					{
						m_nErrorLine = nScriptLine - 1;	// So the error message is correct
						FatalError(IDS_AUT_E_MISSINGTHEN, vIFToks[ivTempPos].m_nCol);
						return;
					}

					// If condition was true, set current line to execute (line after the elseif)
					if (bCondition)
						nLineToExecute = nScriptLine;

					break;

				case K_ENDIF:
					if (nIfCount == 0)
					{
						bEndIfFound = true;		// Stop the main While loop
						tIFDetails.nEndIf = nScriptLine - 1;

						// If the condition is false and no Else processed then set current line to execute as the EndIf
						// Must set it to the EndIf (and not the line after) so that the IFstack unwinds itself properly
						// after the PUSH at the end of this function.
						// If the condition was true, or an Else was found then the next line to execute will have been handled already
						if (bCondition == false && bElseFound == false)
							nLineToExecute = nScriptLine - 1;	// The EndIf line
					}
					else
						nIfCount--;

					break;
			}
		}
	}

	// Did we process a valid If statement (bEndIfFound must be true)
	// Update: no longer need to check if we found an EndIf as all blocks are checked at load time

	nScriptLine = nLineToExecute;

	// Push details of this IF statement on the stack for later use
	m_StatementStack.push(tIFDetails);

} // Parser_Keyword_IF()


///////////////////////////////////////////////////////////////////////////////
// Parser_Keyword_ELSE()
//
// Also handles ELSEIF
///////////////////////////////////////////////////////////////////////////////

void AutoIt_Script::Parser_Keyword_ELSE(int &nScriptLine)
{
	GenStatement	tIFDetails;

	// Is there a matching IF? (No longer need this check, blocks verified on load)

	// Get details of the IF statement (leave on the stack for the EndIf processing)
	tIFDetails = m_StatementStack.top();
	AUT_ASSERT(tIFDetails.nType == L_IF)

	// The IF/ELSE/ENDIF will already have been validated so we don't need to bother now

	// Jump to the line containing the EndIF statement (next time round will be processed as the EndIf statement)
	nScriptLine = tIFDetails.nEndIf;

} // Parser_Keyword_ELSE()


///////////////////////////////////////////////////////////////////////////////
// Parser_Keyword_ENDIF()
///////////////////////////////////////////////////////////////////////////////

void AutoIt_Script::Parser_Keyword_ENDIF(VectorToken &vLineToks, uint &ivPos)
{
	// Is there a matching IF? (No longer need this check, blocks verified on load)

	++ivPos;									// Skip ENDIF keyword

	// Next token must be END (otherwise there is other crap on the line)
	if (vLineToks[ivPos].m_nType != TOK_END)
	{
		FatalError(IDS_AUT_E_EXTRAONLINE, vLineToks[ivPos].m_nCol);
		return;
	}

	// Get details of the IF statement (also pop off the stack)
	m_StatementStack.pop();

	// The IF/ELSE/ENDIF will already have been validated so we don't need to bother now

	// Do NOTHING! :)

} // Parser_Keyword_ENDIF()


///////////////////////////////////////////////////////////////////////////////
// Parser_Keyword_WHILE()
///////////////////////////////////////////////////////////////////////////////

void AutoIt_Script::Parser_Keyword_WHILE(VectorToken &vLineToks, uint &ivPos, int &nScriptLine)
{
	bool			bCondition;
	VectorToken		vWhileToks;
	uint			ivTempPos;					// Position in the vector
	GenStatement	tWHILEDetails;
	int				nWhileCount;				// Keep track of number of nested WHILE statements
	bool			bWendFound = false;
	const char		*szTempScriptLine;


	// Zero our WHILE statement
	tWHILEDetails.nType	= L_WHILE;				// WHILE loop

	++ivPos;									// Skip keyword and evaluate condition

	if ( AUT_FAILED( Parser_EvaluateCondition(vLineToks, ivPos, bCondition) ) )
		return;

	// Next token must be END (otherwise there is other crap on the line)
	if (vLineToks[ivPos].m_nType != TOK_END)
	{
		FatalError(IDS_AUT_E_EXTRAONLINE, vLineToks[ivPos].m_nCol);
		return;
	}

	// Store the line number of the WHILE statement
	tWHILEDetails.nLoopStart = nScriptLine - 1;

	// Look for an WEND statement (skipping nested WHILESs)
	nWhileCount = 0;

	while ( (szTempScriptLine = g_oScriptFile.GetLine(nScriptLine++)) != NULL
			&& bWendFound == false)
	{
		Lexer(nScriptLine-1, szTempScriptLine, vWhileToks);

		ivTempPos = 0;

		if (vWhileToks[ivTempPos].m_nType == TOK_KEYWORD)
		{
			switch (vWhileToks[ivTempPos].nValue)
			{
				case K_WHILE:
					// Nested WHILE
					++nWhileCount;
					break;

				case K_WEND:
					if (nWhileCount == 0)
					{
						bWendFound = true;
						tWHILEDetails.nLoopEnd = nScriptLine - 1;
					}
					else
						nWhileCount--;
					break;
			}
		}
	}

	// Did we process a valid While statement (bWendFound must be true)
	// Update - (No longer need this check, blocks verified on load)

	// Set the next line to execute depending on the evaluation of the condition
	if (bCondition)
	{
		// WHILE statement true (for first pass) - update the stack
		nScriptLine = tWHILEDetails.nLoopStart + 1;		// Continue on line AFTER While

		// Push details of this WHILE statement on the stack for later use in WEND.
		// Also store the number of IF and Select statements in use at his time so we
		// can ExitLoop and ContinueLoop easily even during recursion
		m_StatementStack.push(tWHILEDetails);
	}
	else
	{
		// WHILE statement false - don't put on the stack
		nScriptLine = tWHILEDetails.nLoopEnd + 1;		// Continue on line AFTER Wend
	}

} // Parser_Keyword_WHILE()


///////////////////////////////////////////////////////////////////////////////
// Parser_Keyword_WEND()
///////////////////////////////////////////////////////////////////////////////

void AutoIt_Script::Parser_Keyword_WEND(VectorToken &vLineToks, uint &ivPos, int &nScriptLine)
{
	// Is there a matching WHILE as the innermost (top) loop?
	// (No longer need this check, blocks verified on load)

	++ivPos;									// Skip WEND keyword

	// Next token must be END (otherwise there is other crap on the line)
	if (vLineToks[ivPos].m_nType != TOK_END)
	{
		FatalError(IDS_AUT_E_EXTRAONLINE, vLineToks[ivPos].m_nCol);
		return;
	}

	// The WHILE/WEND will already have been validated so we don't need to bother now

	// Execution should continue back at the  WHILE statement
	nScriptLine = m_StatementStack.top().nLoopStart;

	// Pop the while statement off the stack - the While will re-add it if required
	m_StatementStack.pop();

} // Parser_Keyword_WEND()


///////////////////////////////////////////////////////////////////////////////
// Parser_Keyword_EXITLOOP()
///////////////////////////////////////////////////////////////////////////////

void AutoIt_Script::Parser_Keyword_EXITLOOP(VectorToken &vLineToks, uint &ivPos, int &nScriptLine)
{
	int		nBreakLevel;						// Number of loops to break (default is 1)

	++ivPos;									// Skip EXITLOOP keyword

	// If the next token is END then there is no breaklevel
	if (vLineToks[ivPos].m_nType == TOK_END)
		nBreakLevel = 1;						// Just break a single level
	else
	{
		// Evaluate the expression
		Variant	vTemp;

		if ( AUT_FAILED( Parser_EvaluateExpression(vLineToks, ivPos, vTemp) ) )
			return;

		// Next token must be END (otherwise there is other crap on the line)
		if (vLineToks[ivPos].m_nType != TOK_END)
		{
			FatalError(IDS_AUT_E_EXTRAONLINE, vLineToks[ivPos].m_nCol);
			return;
		}

		nBreakLevel = vTemp.nValue();
	}


	// The WHILE/FOR/DO will already have been validated so we don't need to bother now


	int i=0;
	while (i < nBreakLevel)
	{
		// Is the loop stack empty?
		if (m_StatementStack.empty())
		{
			FatalError(IDS_AUT_E_EXITLOOP);
			return;
		}

		if (m_StatementStack.top().nType == L_FUNC)		// Trying to pop through function call!
		{
			FatalError(IDS_AUT_E_EXITLOOP);
			return;
		}
		else if (m_StatementStack.top().nType == L_WHILE || m_StatementStack.top().nType == L_DO ||
				m_StatementStack.top().nType == L_FOR)	// one of the loops
		{
			++i;
			nScriptLine = m_StatementStack.top().nLoopEnd + 1;		// Continue on line after WEND/UNTIL/NEXT
			m_StatementStack.pop();
		}
		else	// IF or SELECT.  Pop it and keep going
			m_StatementStack.pop();
	}

} // Parser_Keyword_EXITLOOP()


///////////////////////////////////////////////////////////////////////////////
// Parser_Keyword_CONTINUELOOP()
///////////////////////////////////////////////////////////////////////////////

void AutoIt_Script::Parser_Keyword_CONTINUELOOP(VectorToken &vLineToks, uint &ivPos, int &nScriptLine)
{
	int		nBreakLevel;						// Number of loops to break (default is 1)
	int	i;

	++ivPos;									// Skip EXITLOOP keyword

	// If the next token is END then there is no breaklevel
	if (vLineToks[ivPos].m_nType == TOK_END)
		nBreakLevel = 1;						// Just break a single level
	else
	{
		// Evaluate the expression
		Variant	vTemp;

		if ( AUT_FAILED( Parser_EvaluateExpression(vLineToks, ivPos, vTemp) ) )
			return;

		// Next token must be END (otherwise there is other crap on the line)
		if (vLineToks[ivPos].m_nType != TOK_END)
		{
			FatalError(IDS_AUT_E_EXTRAONLINE, vLineToks[ivPos].m_nCol);
			return;
		}

		nBreakLevel = vTemp.nValue();
	}

	i=0;
	while (i < nBreakLevel)
	{
		// Is the stack empty?
		if (m_StatementStack.empty())
		{
			FatalError(IDS_AUT_E_EXITLOOP);
			return;
		}

		if (m_StatementStack.top().nType == L_FUNC)	// Trying to pop through function call!
		{
			FatalError(IDS_AUT_E_EXITLOOP);
			return;
		}
		else if (m_StatementStack.top().nType == L_WHILE || m_StatementStack.top().nType == L_DO ||
				m_StatementStack.top().nType == L_FOR)	// one of the loops
		{
			++i;
			nScriptLine = m_StatementStack.top().nLoopEnd;		// Continue on WEND/UNTIL/NEXT

			if (i != nBreakLevel)
				m_StatementStack.pop();				// Pop all BUT the last one :)
		}
		else	// IF or SELECT.  Pop it and keep going
			m_StatementStack.pop();
	}

} // Parser_Keyword_CONTINUELOOP()


///////////////////////////////////////////////////////////////////////////////
// Parser_Keyword_DO()
///////////////////////////////////////////////////////////////////////////////

void AutoIt_Script::Parser_Keyword_DO(VectorToken &vLineToks, uint &ivPos, int &nScriptLine)
{
	VectorToken		vDoToks;
	uint			ivTempPos;					// Position in the vector
	GenStatement	tDODetails;
	int				nDoCount;					// Keep track of number of nested DO statements
	bool			bUntilFound = false;
	const char		*szTempScriptLine;


	// Zero our DO statement
	tDODetails.nType	= L_DO;					// This is a DO loop

	++ivPos;									// Skip keyword

	// Next token must be END (otherwise there is other crap on the line)
	if (vLineToks[ivPos].m_nType != TOK_END)
	{
		FatalError(IDS_AUT_E_EXTRAONLINE, vLineToks[ivPos].m_nCol);
		return;
	}

	// Store the line number of the DO statement
	tDODetails.nLoopStart = nScriptLine - 1;

	// Look for an UNTIL statement (skipping nested DOs)
	nDoCount = 0;

	while ( (szTempScriptLine = g_oScriptFile.GetLine(nScriptLine++)) != NULL
			&& bUntilFound == false)
	{
		Lexer(nScriptLine-1, szTempScriptLine, vDoToks);

		ivTempPos = 0;

		if (vDoToks[ivTempPos].m_nType == TOK_KEYWORD)
		{
			switch (vDoToks[ivTempPos].nValue)
			{
				case K_DO:
					// Nested DO
					++nDoCount;
					break;

				case K_UNTIL:
					if (nDoCount == 0)
					{
						bUntilFound = true;
						tDODetails.nLoopEnd = nScriptLine - 1;	// Store line of UNTIL statement
					}
					else
						nDoCount--;
					break;
			}
		}
	}

	// Did we process a valid UNTIL statement (bUntilFound must be true)
	// (No longer need this check, blocks verified on load)

	// Push details of this DO statement on the stack for later use
	m_StatementStack.push(tDODetails);

	nScriptLine = tDODetails.nLoopStart + 1;		// Continue on line AFTER Do

} // Parser_Keyword_DO()


///////////////////////////////////////////////////////////////////////////////
// Parser_Keyword_UNTIL()
///////////////////////////////////////////////////////////////////////////////

void AutoIt_Script::Parser_Keyword_UNTIL(VectorToken &vLineToks, uint &ivPos, int &nScriptLine)
{
	bool			bCondition;
	VectorToken	vDoToks;
	AString			sTempScriptLine;

	// Is there a matching DO?
	// (No longer need this check, blocks verified on load)

	++ivPos;									// Skip keyword and evaluate condition

	if ( AUT_FAILED( Parser_EvaluateCondition(vLineToks, ivPos, bCondition) ) )
		return;

	// Next token must be END (otherwise there is other crap on the line)
	if (vLineToks[ivPos].m_nType != TOK_END)
	{
		FatalError(IDS_AUT_E_EXTRAONLINE, vLineToks[ivPos].m_nCol);
		return;
	}

	AUT_ASSERT(m_StatementStack.top().nType == L_DO);

	// Set the next line to execute depending on the evaluation of the condition
	if (bCondition == false)
	{
		// DO statement false, continue on the line AFTER the DO statement
		nScriptLine = m_StatementStack.top().nLoopStart + 1;

	}
	else
	{
		// DO statement true, pop the DO off the stack and continue at next line
		m_StatementStack.pop();
	}

} // Parser_Keyword_UNTIL()


///////////////////////////////////////////////////////////////////////////////
// Parser_Keyword_FOR()
///////////////////////////////////////////////////////////////////////////////

void AutoIt_Script::Parser_Keyword_FOR(VectorToken &vLineToks, uint &ivPos, int &nScriptLine)
{
	VectorToken		vForToks;
	uint			ivTempPos;					// Position in the vector
	GenStatement	tFORDetails;
	int				nForCount;					// Keep track of number of nested FOR statements
	bool			bNextFound = false;
	const char		*szTempScriptLine;
	AString			sVarName;
	Variant			*pvTemp;
	bool			bConst = false;
	Variant			vTemp;

	// Zero our FOR statement
	tFORDetails.nType		= L_FOR;			// This is a FOR loop
	tFORDetails.nLoopStart	= nScriptLine - 1;	// FOR line num = n1

	++ivPos;									// Skip FOR keyword

	// Next token must be a variable, get a reference to it (or create if required)
	if (vLineToks[ivPos].m_nType != TOK_VARIABLE)
	{
		FatalError(IDS_AUT_E_FORERROR, vLineToks[ivPos].m_nCol);
		return;
	}

	sVarName = vLineToks[ivPos].szValue;
	++ivPos;									// Skip variable name

	// Get a reference to the variable (must be a local), if it doesn't exist, then create it as a local.
	// If the variable is a constant then don't allow it either. Note: Even in Opt("MustDeclareVars") mode
	// we allow the automatic creation here.
	g_oVarTable.GetRef(sVarName, &pvTemp, bConst, VARTABLE_FORCELOCAL);
	if (pvTemp == NULL)
	{
		vTemp = 0;
		g_oVarTable.Assign(sVarName, vTemp, false, VARTABLE_FORCELOCAL);
		g_oVarTable.GetRef(sVarName, &pvTemp, bConst, VARTABLE_FORCELOCAL);
	}
	else if (bConst)
	{
		FatalError(IDS_AUT_E_ASSIGNTOCONST, vLineToks[ivPos-1].m_nCol);
		return;
	}

	// Next token must be =
	if (vLineToks[ivPos].m_nType != TOK_EQUAL)
	{
		FatalError(IDS_AUT_E_FORERROR, vLineToks[ivPos].m_nCol);
		return;
	}

	++ivPos;									// Skip =

	// Next token(s) are an expression
	if ( AUT_FAILED( Parser_EvaluateExpression(vLineToks, ivPos, vTemp) ) )
		return;

	// Assign the initial expression to our counter
	*pvTemp = vTemp;

	// Next token must be the keyword "To"
	if ( !(vLineToks[ivPos].m_nType == TOK_KEYWORD && vLineToks[ivPos].nValue == K_TO) )
	{
		FatalError(IDS_AUT_E_FORERROR, vLineToks[ivPos].m_nCol);
		return;
	}

	++ivPos;									// Skip TO

	// Next token(s) are an expression
	if ( AUT_FAILED( Parser_EvaluateExpression(vLineToks, ivPos, vTemp) ) )
		return;

	tFORDetails.vForTo = vTemp;					// Store the TO value

	// Next token may optionally be STEP
	if (vLineToks[ivPos].m_nType == TOK_KEYWORD && vLineToks[ivPos].nValue == K_STEP)
	{
		++ivPos;								// Skip STEP

		// Next token(s) are an expression
		if ( AUT_FAILED( Parser_EvaluateExpression(vLineToks, ivPos, vTemp) ) )
			return;

		tFORDetails.vForStep = vTemp;			// Store the STEP value
	}
	else
		tFORDetails.vForStep = 1;				// Default step is +1


	// Next token must be END (otherwise there is other crap on the line)
	if (vLineToks[ivPos].m_nType != TOK_END)
	{
		FatalError(IDS_AUT_E_EXTRAONLINE, vLineToks[ivPos].m_nCol);
		return;
	}


	// If either the counter, to value or step value is a float, make all floats
//	if (pvTemp->type() == VAR_DOUBLE || tFORDetails.vForTo.type() == VAR_DOUBLE  || tFORDetails.vForStep.type() == VAR_DOUBLE)
//	{
//		pvTemp->ChangeToDouble();
//		tFORDetails.vForTo.ChangeToDouble();
//		tFORDetails.vForStep.ChangeToDouble();
//	}


	// Look for an NEXT statement (skipping nested FORs)
	nForCount = 0;

	while ( (szTempScriptLine = g_oScriptFile.GetLine(nScriptLine++)) != NULL
			&& bNextFound == false)
	{
		Lexer(nScriptLine-1, szTempScriptLine, vForToks);

		ivTempPos = 0;

		if (vForToks[ivTempPos].m_nType == TOK_KEYWORD)
		{
			switch (vForToks[ivTempPos].nValue)
			{
				case K_FOR:
					// Nested FOR
					++nForCount;
					break;

				case K_NEXT:
					if (nForCount == 0)
					{
						bNextFound = true;
						tFORDetails.nLoopEnd = nScriptLine - 1;	// Store line # for NEXT statement
					}
					else
						nForCount--;
					break;
			}
		}
	}

	// Did we process a valid FOR statement (bNextFound must be true)
	// (No longer need this check, blocks verified on load)

	// Even though NEXT evaluates the loop, we must check here as well as it may already be
	// time to finish the loop (for i = 0 to 0 etc)

	// Test the condition before we recommit to the stack, STEP argument:
	//		Value			Loop executes if
	//		Positive or 0	$counter <= end
	//		Negative		$counter >= end
	// Note: We must reset nScriptLine as it was mangled above
	vTemp = 0.0;
	if ( (tFORDetails.vForStep > vTemp) || (tFORDetails.vForStep == vTemp) )
	{
		if ( ((*pvTemp) < tFORDetails.vForTo) || ((*pvTemp) == tFORDetails.vForTo) )
		{
			m_StatementStack.push(tFORDetails);		// Commit  to the stack
			nScriptLine = tFORDetails.nLoopStart + 1;	// Continue execution on line after FOR statement
		}
		else
			nScriptLine = tFORDetails.nLoopEnd + 1;	// Continue execution on line after NEXT statement
	}
	else
	{
		if ( ((*pvTemp) > tFORDetails.vForTo) || ((*pvTemp) == tFORDetails.vForTo) )
		{
			m_StatementStack.push(tFORDetails);		// Commit  to the stack
			nScriptLine = tFORDetails.nLoopStart + 1;	// Continue execution on line after FOR statement
		}
		else
			nScriptLine = tFORDetails.nLoopEnd + 1;	// Continue execution on line after NEXT statement
	}

} // Parser_Keyword_FOR()


///////////////////////////////////////////////////////////////////////////////
// Parser_Keyword_NEXT()
///////////////////////////////////////////////////////////////////////////////

void AutoIt_Script::Parser_Keyword_NEXT(VectorToken &vLineToks, uint &ivPos, int &nScriptLine)
{
	GenStatement	tFORDetails;
	VectorToken		vForToks;
	const char		*szTempScriptLine;
	Variant			*pvTemp;
	bool			bConst = false;
	Variant			vTemp;


	// Is there a matching FOR?
	// (No longer need this check, blocks verified on load)

	++ivPos;									// Skip NEXT keyword

	// Next token must be END (otherwise there is other crap on the line)
	if (vLineToks[ivPos].m_nType != TOK_END)
	{
		FatalError(IDS_AUT_E_EXTRAONLINE, vLineToks[ivPos].m_nCol);
		return;
	}

	// Update the counter and check the for condition
	// We now have a reference to the count variable, are we setting up the FOR statement
	tFORDetails = m_StatementStack.top();	// Get a copy of the details from the stack
	m_StatementStack.pop();					// Remove it from stack (we will push back on if required)


	// Now we have to read in the for line in order to get the variable name - we know that
	// the for structure is perfect so we can go right to the token we need and assume that
	// it is a valid and assigned variable
	szTempScriptLine = g_oScriptFile.GetLine(tFORDetails.nLoopStart);
	Lexer(tFORDetails.nLoopStart, szTempScriptLine, vForToks);
	AString sVarName = vForToks[1].szValue;	// FOR = 0, Var = 1,
	g_oVarTable.GetRef(sVarName, &pvTemp, bConst);


	(*pvTemp) += tFORDetails.vForStep;	// Increment with the STEP value

	// Test the condition before we recommit to the stack, STEP argument:
	//		Value			Loop executes if
	//		Positive or 0	$counter <= end
	//		Negative		$counter >= end
	// Note: If we wish to end the loop then just do nothing and the next line executed
	// will be the one following the NEXT statement
	vTemp = 0.0;

	if ( (tFORDetails.vForStep > vTemp) || (tFORDetails.vForStep == vTemp) )
	{
//		Util_WinPrintf("", "pvtemp = %f, forto = %f", (*pvTemp).fValue(), tFORDetails.vForTo.fValue());
//		(*pvTemp).ChangeToString();
//		(*pvTemp) = (*pvTemp).fValue();

		if ( ((*pvTemp) < tFORDetails.vForTo) || ((*pvTemp) == tFORDetails.vForTo) )
		{

			m_StatementStack.push(tFORDetails);			// Commit back to the stack
			nScriptLine = tFORDetails.nLoopStart + 1;	// Continue execution on line after FOR statement
		}
	}
	else
	{
		if ( ((*pvTemp) > tFORDetails.vForTo) || ((*pvTemp) == tFORDetails.vForTo) )
		{
			m_StatementStack.push(tFORDetails);			// Commit back to the stack
			nScriptLine = tFORDetails.nLoopStart + 1;	// Continue execution on line after FOR statement
		}
	}

} // Parser_Keyword_NEXT()


///////////////////////////////////////////////////////////////////////////////
// Parser_Keyword_SELECT()
///////////////////////////////////////////////////////////////////////////////

void AutoIt_Script::Parser_Keyword_SELECT(VectorToken &vLineToks, uint &ivPos, int &nScriptLine)
{
	VectorToken	vSELECTToks;
	uint			ivTempPos;					// Position in the vector
	GenStatement	tSELECTDetails;
	int				nSelectCount;				// Keep track of number of nested SELECT statements
	bool			bCaseFound = false;
	bool			bCaseExecuted = false;
	bool			bEndSelectFound = false;
	int				nCaseToExecute = 1;
	const char		*szTempScriptLine;
	bool			bCondition;


	// Zero our SELECT statement
	tSELECTDetails.nType	= L_SELECT;			// Not actually used for SELECT, but whatever

	++ivPos;									// Skip SELECT keyword

	// Next token must be END (otherwise there is other crap on the line)
	if (vLineToks[ivPos].m_nType != TOK_END)
	{
		FatalError(IDS_AUT_E_EXTRAONLINE, vLineToks[ivPos].m_nCol);
		return;
	}

	// Store the line number of the SELECT statement
	tSELECTDetails.nSelect = nScriptLine - 1;

	// Look for an CASE and/or ENDSELECT statement (skipping nested SELECTs)
	nSelectCount = 0;

	while ( (szTempScriptLine = g_oScriptFile.GetLine(nScriptLine++)) != NULL
	   && bEndSelectFound == false)
	{
		m_nErrorLine = nScriptLine-1;			// So any errors in case statements look correct

		Lexer(nScriptLine-1, szTempScriptLine, vSELECTToks);

		ivTempPos = 0;

		if (vSELECTToks[ivTempPos].m_nType == TOK_KEYWORD)
		{
			switch (vSELECTToks[ivTempPos].nValue)
			{
				case K_SELECT:
					// Nested SELECT
					++nSelectCount;
					break;

				case K_CASE:
					// Ignore if this is a nested SELECT, or a CASE statement has already been selected
					// for execution
					if (nSelectCount == 0 && bCaseExecuted == false)
					{
						bCaseFound = true;

						// Evaluate the condition
						++ivTempPos;			// Skip CASE keyword

						// Is this the default case?
						if (vSELECTToks[ivTempPos].m_nType == TOK_KEYWORD && vSELECTToks[ivTempPos].nValue == K_ELSE)
						{
							bCondition = true;	// Default case always matches
							++ivTempPos;		// Skip Default
						}
						else
						{
							if ( AUT_FAILED( Parser_EvaluateCondition(vSELECTToks, ivTempPos, bCondition) ) )
								return;
						}

						// Next token must be END (otherwise there is other crap on the line)
						if (vSELECTToks[ivTempPos].m_nType != TOK_END)
						{
							FatalError(IDS_AUT_E_EXTRAONLINE, vSELECTToks[ivTempPos].m_nCol);
							return;
						}

						if (bCondition)
						{
							bCaseExecuted = true;
							// Continue execution on line after this Case statement
							nCaseToExecute = nScriptLine;
						}
					}
					break;

				case K_ENDSELECT:
					if (nSelectCount == 0)
					{
						bEndSelectFound = true;
						tSELECTDetails.nEndSelect = nScriptLine - 1;
					}
					else
						nSelectCount--;
					break;
			}
		}
	}

	// Did we process a valid EndSelect statement and were there 1+ case statements?
	if (bEndSelectFound == false || bCaseFound == false)
	{
		m_nErrorLine = tSELECTDetails.nSelect;		// Make the error look correct
		FatalError(IDS_AUT_E_MISSINGENDSELECTORCASE);
		return;
	}


	m_StatementStack.push(tSELECTDetails);			// Push this select statement onto the stack

	// Are we going to execute any of the case statements?
	if (bCaseExecuted)
		nScriptLine = nCaseToExecute;				// Continue execution on line after selected case statement
	else
		nScriptLine = tSELECTDetails.nEndSelect;	// Continue on the EndSelect keyword

} // Parser_Keyword_SELECT()


///////////////////////////////////////////////////////////////////////////////
// Parser_Keyword_CASE()
///////////////////////////////////////////////////////////////////////////////

void AutoIt_Script::Parser_Keyword_CASE(int &nScriptLine)
{
	// Is there a matching SELECT?
	// (No longer need this check, blocks verified on load)

	// If we have reached a CASE statement then we just continue execution at the
	// ENDSELECT  line because it just means that a previous case statement has
	// finished executing
	AUT_ASSERT(m_StatementStack.top().nType == L_SELECT);
	nScriptLine = m_StatementStack.top().nEndSelect;

} // Parser_Keyword_CASE()


///////////////////////////////////////////////////////////////////////////////
// Parser_Keyword_ENDSELECT()
///////////////////////////////////////////////////////////////////////////////

void AutoIt_Script::Parser_Keyword_ENDSELECT(VectorToken &vLineToks, uint &ivPos)
{
	// Is there a matching SELECT?
	// (No longer need this check, blocks verified on load)

	++ivPos;									// Skip ENDSELECT keyword

	// Next token must be END (otherwise there is other crap on the line)
	if (vLineToks[ivPos].m_nType != TOK_END)
	{
		FatalError(IDS_AUT_E_EXTRAONLINE, vLineToks[ivPos].m_nCol);
		return;
	}

	// Remove the SELECT statement from the stack
	m_StatementStack.pop();

	// Do NOTHING! :)

} // Parser_Keyword_ENDSELECT()


///////////////////////////////////////////////////////////////////////////////
// Parser_Keyword_DIM()
//
// Actually, this routine is used for Dim, Local and Global which are effectively
// the same function just with a difference of scope.
//
// These keywords can be used to pre-declare a variable OR to dimension an array.
//
// JON: REDIM needs splitting out of this function into its own - too crowded
//
///////////////////////////////////////////////////////////////////////////////

void AutoIt_Script::Parser_Keyword_DIM(VectorToken &vLineToks, uint &ivPos, int nReqScope)
{
	Variant			vTemp;
	Variant			vOld;	// stores the array during resizing
	Variant			*pvTemp;
	bool			bConst = false;
	int				nNumSubscripts;
	int				nSubScriptDetails[VAR_SUBSCRIPT_MAX];
	int				nColTemp;
	bool			bReDim = ((nReqScope & VARTABLE_RESIZE) != 0);

	nReqScope &= VARTABLE_ANY | VARTABLE_FORCELOCAL | VARTABLE_FORCEGLOBAL;	// leave scope values only

	++ivPos;									// Skip DIM/LOCAL/GLOBAL/REDIM keyword

	// If the next keyword is Const then jump to our CONST routine
	if ( vLineToks[ivPos].m_nType == TOK_KEYWORD && vLineToks[ivPos].nValue == K_CONST )
	{
		Parser_Keyword_CONST(vLineToks, ivPos, nReqScope);
		return;
	}

	for (;;)
	{
		// Get variable name
		if (vLineToks[ivPos].m_nType != TOK_VARIABLE)
		{
			FatalError(IDS_AUT_E_DIMWITHOUTVAR, vLineToks[ivPos-1].m_nCol);
			return;
		}

		// Get a reference to the variable in the requested scope, if it doesn't exist, then create it.
		AString sVarName = vLineToks[ivPos].szValue;
		g_oVarTable.GetRef(sVarName, &pvTemp, bConst, nReqScope);
		if (pvTemp == NULL)
		{
			if (bReDim)
			{
				// The array to redim must already exist
				FatalError(IDS_AUT_E_VARNOTFOUND, vLineToks[ivPos].m_nCol);
				return;
			}

			vTemp = "";								// Let the uninitialised value be "" (equates to 0.0 for numbers)
			g_oVarTable.Assign(sVarName, vTemp, false, nReqScope);
			g_oVarTable.GetRef(sVarName, &pvTemp, bConst, nReqScope);
		}
		else if (bConst)
		{
			// Can't Dim a constant!
			FatalError(IDS_AUT_E_ASSIGNTOCONST, vLineToks[ivPos].m_nCol);
			return;
		}

		// If redim then the variable must already be an array - otherwise what is the point?
		if (bReDim)
		{
			if ( !(pvTemp->isArray()) )
			{
				FatalError(IDS_AUT_E_REDIMUSEDONNONARRAY, vLineToks[ivPos].m_nCol);
				return;
			}
		}

		++ivPos;									// Skip variable name

		if (vLineToks[ivPos].m_nType == TOK_LEFTSUBSCRIPT)
		{
			// Read the subscripts
			// Store the old value while the it is resized.
			if (bReDim)
				vOld = *pvTemp;

			nNumSubscripts = 0;
			nColTemp = vLineToks[ivPos].m_nCol;		// Store for error output
			while (vLineToks[ivPos].m_nType == TOK_LEFTSUBSCRIPT)
			{
				++ivPos;								// Skip [
				nColTemp = vLineToks[ivPos].m_nCol;		// Store for error output

				// Parse expression for subscript
				if ( AUT_FAILED( Parser_EvaluateExpression(vLineToks, ivPos, vTemp) ) )
				{
					FatalError(IDS_AUT_E_PARSESUBSCRIPT, nColTemp);
					return;
				}

				// Subscript cannot be less than or equal to 0
				if ( vTemp.nValue() <= 0 )
				{
					FatalError(IDS_AUT_E_PARSESUBSCRIPT, nColTemp);
					return;
				}

				// Next token must be ]
				if (vLineToks[ivPos].m_nType != TOK_RIGHTSUBSCRIPT)
				{
					FatalError(IDS_AUT_E_PARSESUBSCRIPT, nColTemp);
					return;
				}

				++ivPos;								// Next token

				// Add this subscript and increase the number of subscripts
				nSubScriptDetails[nNumSubscripts++] = vTemp.nValue();

			} // End While

			// Must have at least one subscript for a valid array
			if (nNumSubscripts < 1)
			{
				FatalError(IDS_AUT_E_TOOFEWSUBSCRIPTS, nColTemp);
				return;
			}

			// Now run through and set the various subscripts (must be done after the above to fix
			// cases like ReDim $a[$a[0]]  )
			pvTemp->ArraySubscriptClear();				// Reset the subscript
			for (int i=0; i<nNumSubscripts; ++i)
			{
				if ( pvTemp->ArraySubscriptSetNext( nSubScriptDetails[i] ) == false )
				{
					FatalError(IDS_AUT_E_TOOMANYSUBSCRIPTS, nColTemp);
					return;
				}
			}

			// Ok, valid subscripts, dimension the variant into an array
			if ( pvTemp->ArrayDim() == false )
			{
				FatalError(IDS_AUT_E_ARRAYALLOC, nColTemp);
				return;
			}

			// copy old array values into new array
			if (bReDim && vOld.ArrayGetBound(0)>0)
				pvTemp->ArrayCopy(vOld);

		} // TOK_LEFTSUBSCRIPT
		else if (vLineToks[ivPos].m_nType == TOK_EQUAL)
		{
			++ivPos;		// skip =  to evaluate and store the result
			nColTemp = vLineToks[ivPos].m_nCol;		// Store for error output

			// Parse expression for value
			if ( AUT_FAILED( Parser_EvaluateExpression(vLineToks, ivPos, vTemp) ) )
			{
				FatalError(IDS_AUT_E_EXPRESSION, nColTemp);
				return;
			}

			// initialize the variable
			(*pvTemp)=vTemp;
		} // TOK_EQUAL

		// If the next token is END, then we finish
		if (vLineToks[ivPos].m_nType == TOK_END)
			break;
		else if (vLineToks[ivPos].m_nType != TOK_COMMA)
		{
			FatalError(IDS_AUT_E_DIMWITHOUTVAR, vLineToks[ivPos].m_nCol);
			return;
		}
		++ivPos;		// skip COMMA  to check next variable to declare
	}	// while looping on variable declaration

} // Parser_Keyword_DIM()


///////////////////////////////////////////////////////////////////////////////
// Parser_Keyword_CONST()
//
// Similar to Parser_Keyword_DIM() but just for constants
//
///////////////////////////////////////////////////////////////////////////////

void AutoIt_Script::Parser_Keyword_CONST(VectorToken &vLineToks, uint &ivPos, int nReqScope)
{
	Variant			vTemp;
	Variant			*pvTemp;
	bool			bConst = false;
	int				nColTemp;

	++ivPos;									// Skip Const keyword

	for (;;)
	{
		// Get variable name
		if (vLineToks[ivPos].m_nType != TOK_VARIABLE)
		{
			FatalError(IDS_AUT_E_DIMWITHOUTVAR, vLineToks[ivPos-1].m_nCol);
			return;
		}

		// Get a reference to the variable in the requested scope, if it doesn't exist, then create it.
		AString sVarName = vLineToks[ivPos].szValue;
		g_oVarTable.GetRef(sVarName, &pvTemp, bConst, nReqScope);
		if (pvTemp == NULL)
		{
			// Doesn't already exist
			vTemp = "";								// Let the uninitialised value be "" (equates to 0.0 for numbers)
			g_oVarTable.Assign(sVarName, vTemp, true, nReqScope);
			g_oVarTable.GetRef(sVarName, &pvTemp, bConst, nReqScope);
		}
		else
		{
			// Already exists
			FatalError(IDS_AUT_E_CONSTONEXISTING, vLineToks[ivPos].m_nCol);
			return;
		}


		++ivPos;									// Skip variable name

		// Must be a TOK_EQUAL next
		if (vLineToks[ivPos].m_nType != TOK_EQUAL)
		{
			FatalError(IDS_AUT_E_EXPECTEDASSIGNMENT, vLineToks[ivPos].m_nCol);
			return;
		}

		++ivPos;		// skip =  to evaluate and store the result
		nColTemp = vLineToks[ivPos].m_nCol;		// Store for error output

		// Parse expression for value
		if ( AUT_FAILED( Parser_EvaluateExpression(vLineToks, ivPos, vTemp) ) )
		{
			FatalError(IDS_AUT_E_EXPRESSION, nColTemp);
			return;
		}

		// initialize the variable
		(*pvTemp) = vTemp;

		// If the next token is END, then we finish
		if (vLineToks[ivPos].m_nType == TOK_END)
			break;
		else if (vLineToks[ivPos].m_nType != TOK_COMMA)
		{
			FatalError(IDS_AUT_E_DIMWITHOUTVAR, vLineToks[ivPos].m_nCol);
			return;
		}

		++ivPos;		// skip COMMA  to check next variable to declare

	}	// while looping on variable declaration

} // Parser_Keyword_CONST()


///////////////////////////////////////////////////////////////////////////////
// Parser_Keyword_RETURN()
///////////////////////////////////////////////////////////////////////////////

void AutoIt_Script::Parser_Keyword_RETURN(VectorToken &vLineToks, uint &ivPos)
{
	++ivPos;									// Skip RETURN keyword

	if (vLineToks[ivPos].m_nType != TOK_END)
	{
		// Evaluate the expression
		if ( AUT_FAILED( Parser_EvaluateExpression(vLineToks, ivPos, m_vUserRetVal) ) )
			return;
	}

	// Next token must be END (otherwise there is other crap on the line)
	if (vLineToks[ivPos].m_nType != TOK_END)
	{
		FatalError(IDS_AUT_E_EXTRAONLINE, vLineToks[ivPos].m_nCol);
		return;
	}

} // Parser_Keyword_RETURN()


///////////////////////////////////////////////////////////////////////////////
// Parser_Keyword_EXIT()
///////////////////////////////////////////////////////////////////////////////

void AutoIt_Script::Parser_Keyword_EXIT(VectorToken &vLineToks, uint &ivPos)
{
	Variant	vReturn;

	++ivPos;									// Skip EXIT keyword

	// Set our exit method
	g_nExitMethod	= AUT_EXITBY_EXITKEYWORD;
	m_nCurrentOperation = AUT_QUIT;		// Quit

	// If the next token is END then there is no exit code
	if (vLineToks[ivPos].m_nType == TOK_END)
		return;

	// Evaluate the expression
	if ( AUT_FAILED( Parser_EvaluateExpression(vLineToks, ivPos, vReturn) ) )
		return;

	// Next token must be END (otherwise there is other crap on the line)
	if (vLineToks[ivPos].m_nType != TOK_END)
	{
		FatalError(IDS_AUT_E_EXTRAONLINE, vLineToks[ivPos].m_nCol);
		return;
	}

	// Set our global value for the windows return code
	g_nExitCode		= vReturn.nValue();

} // Parser_Keyword_EXIT()


///////////////////////////////////////////////////////////////////////////////
// Parser_VerifyBlockStructure()
//
// This checks that all Func, Select, IF, While, For, Do have a correct
// block structure.  It should be called as soon as the script is loaded
// and allows the code that handles these keywords to make the assumption
// that the block structure is correct saving lots of error checking code
//
// It will also check bad nesting such as:
// While 1
// Do
// Wend
// Until 1 = 1
//
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::Parser_VerifyBlockStructure(void)
{
	uint			ivPos;
	uint			ivLast;						// Last non-end position
	VectorToken	LineTokens;					// Vector (array) of tokens for a line of script
	int				nScriptLine = 1;			// 1 = first line
	const char		*szScriptLine;
	bool			bThen;

	int				nIf = 0;
	int				nSelect = 0;
	int				nFunc = 0;
	int				nWhile = 0;
	int				nFor = 0;
	int				nDo = 0;

	StackInt		stkIf;
	StackInt		stkSelect;
	StackInt		stkWhile;
	StackInt		stkFor;
	StackInt		stkDo;

	int				nSeq = 0;
	int				nSeqTst;

	int				nKey;


	// Read in each line of the script
	while ( (szScriptLine = g_oScriptFile.GetLine(nScriptLine)) != NULL )
	{
		m_nErrorLine = nScriptLine;				// Keep track for errors
		++nScriptLine;							// Next line

		// Convert the line

		if ( AUT_FAILED( Lexer(nScriptLine-1, szScriptLine, LineTokens) ) )
			return AUT_ERR;						// Bad line

		ivPos = 0;
		ivLast = 0;								// Last non-end token

		// Find the last token before TOK_END
		while (LineTokens[ivLast].m_nType != TOK_END)
			++ivLast;
		if (ivLast > 0)
			--ivLast;

		// Does the line start with a keyword?  If not, continue the main loop
		if (LineTokens[0].m_nType != TOK_KEYWORD)
			continue;							// Read next line and start again

		// What keyword?
		switch (LineTokens[0].nValue)
		{
			case K_DO:
				++nDo;
				stkDo.push(nSeq++);
				break;

			case K_WHILE:
				++nWhile;
				stkWhile.push(nSeq++);
				break;

			case K_FOR:
				++nFor;
				stkFor.push(nSeq++);
				break;

			case K_SELECT:
				++nSelect;
				stkSelect.push(nSeq++);
				break;

			case K_IF:							// Only take notice of multiline IFs
				// Do a very basic check to make sure that the Then keyword exists somewhere on the line - very crude
				bThen = false;
				while (LineTokens[ivPos].m_nType != TOK_END && bThen == false)
				{
					if (LineTokens[ivPos].m_nType == TOK_KEYWORD && LineTokens[ivPos].nValue == K_THEN)
					{
						bThen = true;
						break;					// Leave the loop with ivPos pointing to the FIRST THEN keyword
					}
					++ivPos;
				}

				if (bThen == false)
				{
					FatalError(IDS_AUT_E_MISSINGTHEN);
					return AUT_ERR;
				}

				// Is THEN used at the end of the line?  (Multiline If statement)
				if (LineTokens[ivLast].m_nType == TOK_KEYWORD && LineTokens[ivLast].nValue == K_THEN)
				{
					++nIf;
					stkIf.push(nSeq++);
				}

				// There are more tokens left so check that this line does not contain any other
				// keywords that shouldn't be used after a THEN keyword
				++ivPos;						// Skip THEN keyword
				if (LineTokens[ivPos].m_nType == TOK_KEYWORD)
				{
					nKey = LineTokens[ivPos].nValue;
					if (nKey != K_EXITLOOP && nKey != K_CONTINUELOOP && nKey != K_DIM && nKey != K_REDIM
						&& nKey != K_LOCAL && nKey != K_GLOBAL && nKey != K_EXIT && nKey != K_RETURN)
					{
						FatalError(IDS_AUT_E_KEYWORDAFTERTHEN, LineTokens[ivPos].m_nCol);
						return AUT_ERR;
					}
				}

				break;

			case K_FUNC:
				++nFunc;
				if (nFunc > 1)
				{
					FatalError(IDS_AUT_E_MISSINGENDFUNC);
					return AUT_ERR;
				}
				// As we are at the start of a function all the other tests must now be equal
				// to 0 otherwise there is an error/bad nesting/missing statements
				if ( AUT_FAILED( Parser_VerifyBlockStructure2(nDo, nWhile, nFor, nSelect, nIf) ) )
					return AUT_ERR;

				break;

			case K_UNTIL:
				--nDo;
				--nSeq;
				if (stkDo.empty())
					nSeqTst = nSeq+1;	// Make sure they not equal
				else
				{
					nSeqTst = stkDo.top();
					stkDo.pop();
				}

				if (nDo < 0 || nSeq != nSeqTst)
				{
					FatalError(IDS_AUT_E_UNTILNOMATCHINGDO);
					return AUT_ERR;
				}
				break;

			case K_WEND:
				--nWhile;
				--nSeq;
				if (stkWhile.empty())
					nSeqTst = nSeq+1;	// Make sure they not equal
				else
				{
					nSeqTst = stkWhile.top();
					stkWhile.pop();
				}

				if (nWhile < 0 || nSeq != nSeqTst)
				{
					FatalError(IDS_AUT_E_WENDNOMATCHINGWHILE);
					return AUT_ERR;
				}
				break;

			case K_NEXT:
				--nFor;
				--nSeq;
				if (stkFor.empty())
					nSeqTst = nSeq+1;	// Make sure they not equal
				else
				{
					nSeqTst = stkFor.top();
					stkFor.pop();
				}

				if (nFor < 0 || nSeq != nSeqTst)
				{
					FatalError(IDS_AUT_E_NEXTNOMATCHINGFOR);
					return AUT_ERR;
				}
				break;

			case K_ENDSELECT:
				--nSelect;
				--nSeq;
				if (stkSelect.empty())
					nSeqTst = nSeq+1;	// Make sure they not equal
				else
				{
					nSeqTst = stkSelect.top();
					stkSelect.pop();
				}

				if (nSelect < 0 || nSeq != nSeqTst)
				{
					FatalError(IDS_AUT_E_ENDSELECTNOMATCHINGSELECT);
					return AUT_ERR;
				}
				break;

			case K_CASE:
				if (stkSelect.empty())
				{
					FatalError(IDS_AUT_E_CASENOMATCHINGSELECT);
					return AUT_ERR;
				}
				else
					nSeqTst = stkSelect.top();

				if ((nSeq-1) != nSeqTst )
				{
					FatalError(IDS_AUT_E_CASENOMATCHINGSELECT);
					return AUT_ERR;
				}

				break;

			case K_ENDIF:
				--nIf;
				--nSeq;
				if (stkIf.empty())
					nSeqTst = nSeq+1;	// Make sure they not equal
				else
				{
					nSeqTst = stkIf.top();
					stkIf.pop();
				}

				if (nIf < 0 || nSeq != nSeqTst)
				{
					FatalError(IDS_AUT_E_ENDIFNOMATCHINGIF);
					return AUT_ERR;
				}
				break;

			case K_ELSE:
			case K_ELSEIF:
				if (nIf == 0)
				{
					FatalError(IDS_AUT_E_ELSENOMATCHINGIF);
					return AUT_ERR;
				}
				break;

			case K_CONTINUELOOP:
			case K_EXITLOOP:
				if (nDo == 0 && nWhile == 0 && nFor == 0)
				{
					FatalError(IDS_AUT_E_EXITLOOP);
					return AUT_ERR;
				}
				break;

			case K_ENDFUNC:
				--nFunc;
				if (nFunc != 0)
				{
					FatalError(IDS_AUT_E_MISSINGENDFUNC);
					return AUT_ERR;
				}
				// As we are at the end of a function all the other tests above must now be equal
				// to 0 otherwise there is an error/bad nesting/missing statements
				if ( AUT_FAILED( Parser_VerifyBlockStructure2(nDo, nWhile, nFor, nSelect, nIf) ) )
					return AUT_ERR;

				break;

		} // End Switch for keyword

	} // End While read each line

	// End of script - only thing left to check for is unmatched statements
	if ( AUT_FAILED( Parser_VerifyBlockStructure2(nDo, nWhile, nFor, nSelect, nIf) ) )
		return AUT_ERR;

	if (nFunc != 0)
	{
		FatalError(IDS_AUT_E_MISSINGENDFUNC);
		return AUT_ERR;
	}

	//AUT_MSGBOX("", "Initial block structure check OK")
	return AUT_OK;

} // Parser_VerifyBlockStructure()


///////////////////////////////////////////////////////////////////////////////
// Parser_VerifyBlockStructure2()
//
// Helper function
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::Parser_VerifyBlockStructure2(int nDo, int nWhile, int nFor, int nSelect, int nIf)
{
	if (nDo != 0)
	{
		FatalError(IDS_AUT_E_MISSINGUNTIL);
		return AUT_ERR;
	}
	else if (nWhile != 0)
	{
		FatalError(IDS_AUT_E_MISSINGWEND);
		return AUT_ERR;
	}
	else if (nFor != 0)
	{
		FatalError(IDS_AUT_E_MISSINGNEXT);
		return AUT_ERR;
	}
	else if (nSelect != 0)
	{
		FatalError(IDS_AUT_E_MISSINGENDSELECTORCASE);
		return AUT_ERR;
	}
	else if (nIf != 0)
	{
		FatalError(IDS_AUT_E_MISSINGENDIF);
		return AUT_ERR;
	}

	return AUT_OK;

} // Parser_VerifyBlockStructure2()
