
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
// script_file.cpp
//
// Contains registry handling routines.  Part of script.cpp
//
///////////////////////////////////////////////////////////////////////////////


// Includes
#include "StdAfx.h"								// Pre-compiled headers

#ifndef _MSC_VER								// Includes for non-MS compilers
	#include <windows.h>
#endif

#include "AutoIt.h"								// Autoit values, macros and config options

#include "script.h"
#include "utility.h"
#include "resources\resource.h"


///////////////////////////////////////////////////////////////////////////////
// RegSplitKey()
//
// Takes a key like "HKEY_USERS\Software\Microsoft" and splits it into key and
// subkey "HKEY_USERS", "Software\Microsoft"
//
///////////////////////////////////////////////////////////////////////////////

void AutoIt_Script::RegSplitKey(AString sFull, AString &sCname, AString &sKey, AString &sSubKey)
{
	int nPos;

	// Blank make sure the key and subkey are blank in case of error
	sCname	= "";
	sKey	= "";
	sSubKey = "";

	// Simple stuff first, change a request for (Default) into "" - removed as a key called (Default) can actually exist :(
//	if (!sValue.stricmp("(Default)"))
//		sValue = "";

	if (sFull[0] == '\\' && sFull[1] == '\\')	// Safe even if blank due to astring overrun protection
	{
		// Computername specified
		sFull.erase(0, 2);
		nPos = sFull.find_first_of("\\");
		sCname.assign(sFull, 0, nPos );
		sFull.erase(0, nPos+1);					// Remove the computername for further processing
	}
	else
		sCname = "";

	// Split the rest
	nPos = sFull.find_first_of("\\");

	sKey.assign(sFull, 0, nPos );
	sSubKey.assign(sFull, nPos+1, sFull.length());
	sSubKey.strip_trailing("\\");

//	AUT_MSGBOX("CName", sCname.c_str());
//	AUT_MSGBOX("Key", sKey.c_str());
//	AUT_MSGBOX("SubKey", sSubKey.c_str());
//	AUT_MSGBOX("Value", sValue.c_str());

} // RegSplitKey()


///////////////////////////////////////////////////////////////////////////////
// RegGetMainKey()
//
// Gets the relevant HKEY value for a given ascii string
//
///////////////////////////////////////////////////////////////////////////////

bool AutoIt_Script::RegGetMainKey(AString sKey, HKEY &hKey)
{
	sKey.toupper();

	if (sKey == "HKEY_LOCAL_MACHINE" || sKey == "HKLM")
	{
		hKey = HKEY_LOCAL_MACHINE;
		return true;
	}

	if (sKey == "HKEY_CLASSES_ROOT" || sKey == "HKCR")
	{
		hKey = HKEY_CLASSES_ROOT;
		return true;
	}

	if (sKey == "HKEY_CURRENT_CONFIG" || sKey == "HKCC")
	{
		hKey = HKEY_CURRENT_CONFIG;
		return true;
	}

	if (sKey == "HKEY_CURRENT_USER" || sKey == "HKCU")
	{
		hKey = HKEY_CURRENT_USER;
		return true;
	}

	if (sKey == "HKEY_USERS" || sKey == "HKU")
	{
		hKey = HKEY_USERS;
		return true;
	}

	return false;								// No matching key

} // RegGetMainKey()


///////////////////////////////////////////////////////////////////////////////
// RegRead()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_RegRead(VectorVariant &vParams, Variant &vResult)
{
	// $var = RegRead(key, valuename)

	HKEY	hRegKey, hMainKey, hRemoteKey = NULL;
	DWORD	dwRes, dwBuf, dwType;
	char	szRegBuffer[65535+2];					// Only allow reading of 64Kb from a key plus space for 2 nulls
	AString	sCname, sKey, sSubKey;
	char	*szBinary;
	int		j;
	DWORD	i, n;
	char	szHexData[] = "0123456789ABCDEF";

	// Set the default return value to ""
	vResult = "";

	RegSplitKey(vParams[0].szValue(), sCname, sKey, sSubKey);

	// Get the main key name
	if ( RegGetMainKey(sKey, hMainKey) == false)
	{
		SetFuncErrorCode(1);					// Default is 0
		return AUT_OK;
	}

	// Open the key (and remote key if required)
	if (!sCname.empty())
	{
		// Remote
		if ( RegConnectRegistry(sCname.c_str(), hMainKey, &hRemoteKey) != ERROR_SUCCESS )
		{
			SetFuncErrorCode(1);					// Default is 0
			return AUT_OK;
		}

		hMainKey = hRemoteKey;
	}

	//local
	// Open the registry key
	if ( RegOpenKeyEx(hMainKey, sSubKey.c_str(), 0, KEY_READ, &hRegKey) != ERROR_SUCCESS )
	{
		if (!sCname.empty())
			RegCloseKey(hRemoteKey);

		SetFuncErrorCode(1);					// Default is 0
		return AUT_OK;
	}

	// Read the value and determine the type
	if ( RegQueryValueEx(hRegKey, vParams[1].szValue(), NULL, &dwType, NULL, NULL) != ERROR_SUCCESS )
	{
		// Error reading key
		SetFuncErrorCode(-1);					// Default is 0
		RegCloseKey(hRegKey);
		if (!sCname.empty())
			RegCloseKey(hRemoteKey);

		// If the (default) key was requested return vResult as an empty string as (default) is usually
		// REG_SZ
		//if (vParams[1].szValue()[0] == '\0')
		//	vResult = "";

		return AUT_OK;
	}


	dwRes = 65535;								// Default size (leave space for 2 nulls for reg_multi_sz)

	// The way we read is different depending on the type of the key
	switch (dwType)
	{
		case REG_SZ:
		case REG_EXPAND_SZ:
			if (RegQueryValueEx(hRegKey, vParams[1].szValue(), NULL, NULL, (LPBYTE)szRegBuffer, &dwRes) == ERROR_SUCCESS)
			{
				// dwres is number of chars INCLUDING the null char IF present, according to docs we must ensure termination
				szRegBuffer[dwRes] = '\0';
				vResult = szRegBuffer;
			}
			else
				SetFuncErrorCode(-2);					// Default is 0

			break;

		case REG_MULTI_SZ:
			if (RegQueryValueEx(hRegKey, vParams[1].szValue(), NULL, NULL, (LPBYTE)szRegBuffer, &dwRes) == ERROR_SUCCESS)
			{
				szRegBuffer[dwRes] = '\0';	// Ensure termination

				// Skip back through ALL the null chars first
				while (szRegBuffer[dwRes] == '\0' && dwRes != 0)
					--dwRes;

				for (i=0; i<dwRes; ++i)
				{
					if (szRegBuffer[i] == '\0')
						szRegBuffer[i] = '\n'; // Convert to \n
				}

				vResult = szRegBuffer;
			}
			else
				SetFuncErrorCode(-2);     // Default is 0

			break;


		case REG_DWORD:
			dwRes = sizeof(dwBuf);
			RegQueryValueEx(hRegKey, vParams[1].szValue(), NULL, NULL, (LPBYTE)&dwBuf, &dwRes);
			vResult = (double)dwBuf;	// Unsigned DWORD possibly too big for signed int
			break;

		case REG_BINARY:
			if (RegQueryValueEx(hRegKey, vParams[1].szValue(), NULL, NULL, (LPBYTE)szRegBuffer, &dwRes) == ERROR_SUCCESS)
			{
				szBinary = new char[(dwRes * 2) + 1];	// Each byte will turned into 2 digits, plus a final null
				szBinary[0] = '\0';
				j = 0;
				for (i=0; i<dwRes; ++i)				// i and n must be unsigned to work
				{
					n = szRegBuffer[i];				// Get the value and convert to 2 digit hex
					szBinary[j+1] = szHexData[n % 16];
					n = n / 16;
					szBinary[j] = szHexData[n % 16];
					j += 2;
				}
				szBinary[j] = '\0';					// Terminate

				vResult = szBinary;					// Assign to result
				delete [] szBinary;
			}
			else
				SetFuncErrorCode(-2);					// Default is 0

			break;

		default:
			SetFuncErrorCode(-2);				// Default is 0
			break;
	}

	RegCloseKey(hRegKey);
	if (!sCname.empty())
		RegCloseKey(hRemoteKey);

	return AUT_OK;

} // RegRead()


///////////////////////////////////////////////////////////////////////////////
// RegWrite()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_RegWrite(VectorVariant &vParams, Variant &vResult)
{
	// $var = RegWrite(key, valuename, type, value)

	HKEY	hRegKey, hMainKey, hRemoteKey = NULL;
	DWORD	dwRes, dwBuf;
	char	szRegBuffer[65535+2];					// Only allow writing of 64Kb to a key, include space for double null
	AString	sCname, sKey, sSubKey;
	const char	*szTemp;
	int		i, j, nLen, nVal, nMult;
	uint	iNumParams = vParams.size();

	// 1 or 4 parameters is the only acceptable number
	if (iNumParams != 1 && iNumParams != 4)
	{
		vResult = 0;							// Default is 1
		return AUT_OK;
	}


	RegSplitKey(vParams[0].szValue(), sCname, sKey, sSubKey);

	// Get the main key name
	if ( RegGetMainKey(sKey, hMainKey) == false)
	{
		vResult = 0;							// Default is 1
		return AUT_OK;
	}

	// Open/Create the registry key (remotely if required)
	if (!sCname.empty())
	{
		// Remote
		if ( RegConnectRegistry(sCname.c_str(), hMainKey, &hRemoteKey) != ERROR_SUCCESS )
		{
			vResult = 0;							// Default is 1
			return AUT_OK;
		}

		hMainKey = hRemoteKey;
	}

	//local
	// Open the registry key
	if ( RegCreateKeyEx(hMainKey, sSubKey.c_str(), 0, "", REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hRegKey, &dwRes) != ERROR_SUCCESS )
	{
		vResult = 0;							// Default is 1

		if (!sCname.empty())
			RegCloseKey(hRemoteKey);

		return AUT_OK;
	}


	// If only 1 parameter (keyname) then we are finished
	if (iNumParams == 1)
	{
		RegCloseKey(hRegKey);
		if (!sCname.empty())
			RegCloseKey(hRemoteKey);
		return AUT_OK;
	}


	// Get the length of the value (even if it's a dword...) to avoid multiple strlen calls later
	nLen = (int)strlen(vParams[3].szValue());

	// Write the registry differently depending on type of variable we are writing
	if (!stricmp(vParams[2].szValue(), "REG_EXPAND_SZ"))
	{
		strcpy(szRegBuffer, vParams[3].szValue());
		if ( RegSetValueEx(hRegKey, vParams[1].szValue(), 0, REG_EXPAND_SZ, (CONST BYTE *)vParams[3].szValue(), (DWORD)nLen+1 ) != ERROR_SUCCESS )
			vResult = 0;						// Default is 1

		RegCloseKey(hRegKey);
		if (!sCname.empty())
			RegCloseKey(hRemoteKey);
		return AUT_OK;
	}

	if (!stricmp(vParams[2].szValue(), "REG_SZ"))
	{
		strcpy(szRegBuffer, vParams[3].szValue());
		if ( RegSetValueEx(hRegKey, vParams[1].szValue(), 0, REG_SZ, (CONST BYTE *)vParams[3].szValue(), (DWORD)nLen+1 ) != ERROR_SUCCESS )
			vResult = 0;						// Default is 1

		RegCloseKey(hRegKey);
		if (!sCname.empty())
			RegCloseKey(hRemoteKey);
		return AUT_OK;
	}

	if (!stricmp(vParams[2].szValue(), "REG_MULTI_SZ"))
	{
		Util_Strncpy(szRegBuffer, vParams[3].szValue(), 65535+1);	// Leave space for another null
		// Change all \n to \0 then double null terminate
		nLen = (int)strlen(szRegBuffer);
		szRegBuffer[nLen] = '\0';				// Double null
		szRegBuffer[nLen+1] = '\0';

		for (i = 0; i<nLen; ++i)
		{
			if (szRegBuffer[i] == '\n')
				szRegBuffer[i] = '\0';
		}

		// If blank then must use nLen = 0, ignoring \0\0 (blank values not allowed)
		// Otherwise take our stringlen + 2 (double null) as the size
		if (nLen != 0)
			nLen = nLen + 2;

		if ( RegSetValueEx(hRegKey, vParams[1].szValue(), 0, REG_MULTI_SZ, (CONST BYTE *)szRegBuffer, (DWORD)nLen ) != ERROR_SUCCESS )
			vResult = 0;						// Default is 1

		RegCloseKey(hRegKey);
		if (!sCname.empty())
			RegCloseKey(hRemoteKey);
		return AUT_OK;
	}


	if (!stricmp(vParams[2].szValue(), "REG_DWORD"))
	{
		dwBuf = vParams[3].nValue(); // Safe to use nvalue as the conversion TO dword happens ok
		if ( RegSetValueEx(hRegKey, vParams[1].szValue(), 0, REG_DWORD, (CONST BYTE *)&dwBuf, sizeof(dwBuf) ) != ERROR_SUCCESS )
			vResult = 0;						// Default is 1

		RegCloseKey(hRegKey);
		if (!sCname.empty())
			RegCloseKey(hRemoteKey);
		return AUT_OK;
	}

	// REG_BINARY - eeek
	if (!stricmp(vParams[2].szValue(), "REG_BINARY"))
	{
		szTemp = vParams[3].szValue();			// Easier access to the string

		// Stringlength must be a multiple of 2
		if ( (nLen % 2) != 0 )
		{
			vResult = 0;						// Error
			RegCloseKey(hRegKey);
			return AUT_OK;
		}

		// Really crappy hex conversion
		j = 0;
		i = 0;
		while (i < nLen && j < 65535)
		{
			nVal = 0;
			for (nMult = 16; nMult >= 0; nMult = nMult - 15)
			{
				if (szTemp[i] >= '0' && szTemp[i] <= '9')
					nVal += (szTemp[i] - '0') * nMult;
				else if (szTemp[i] >= 'A' && szTemp[i] <= 'F')
					nVal += (((szTemp[i] - 'A'))+10) * nMult;
				else if (szTemp[i] >= 'a' && szTemp[i] <= 'f')
					nVal += (((szTemp[i] - 'a'))+10) * nMult;
				else
				{
					vResult = 0;				// Bad non-hex character
					RegCloseKey(hRegKey);
					if (!sCname.empty())
						RegCloseKey(hRemoteKey);
					return AUT_OK;
				}

				++i;
			}

			szRegBuffer[j++] = (char)nVal;
		}

		if ( RegSetValueEx(hRegKey, vParams[1].szValue(), 0, REG_BINARY, (CONST BYTE *)szRegBuffer, (DWORD)j ) != ERROR_SUCCESS )
			vResult = 0;						// Default is 1

		RegCloseKey(hRegKey);
		if (!sCname.empty())
			RegCloseKey(hRemoteKey);
		return AUT_OK;
	}



	// If we reached here then the requested type was not known
	RegCloseKey(hRegKey);
	if (!sCname.empty())
		RegCloseKey(hRemoteKey);

	vResult = 0;							// Default is 1
	return AUT_OK;

} // RegWrite()


///////////////////////////////////////////////////////////////////////////////
// RegRemovewSubkeys() - helper function for RegDelete
///////////////////////////////////////////////////////////////////////////////

static bool RegRemoveSubkeys(HKEY hRegKey)
{
	// Removes all subkeys to the given key.  Will not touch the given key.
	CHAR Name[256];
	DWORD dwNameSize;
	FILETIME ftLastWrite;
	HKEY hSubKey;
	bool Success;

	for (;;)
	{ // infinite loop
		dwNameSize=255;
		if (RegEnumKeyEx(hRegKey, 0, Name, &dwNameSize, NULL, NULL, NULL, &ftLastWrite) == ERROR_NO_MORE_ITEMS)
			break;
		if ( RegOpenKeyEx(hRegKey, Name, 0, KEY_READ, &hSubKey) != ERROR_SUCCESS )
			return false;

		Success=RegRemoveSubkeys(hSubKey);
		RegCloseKey(hSubKey);
		if (!Success)
			return false;
		else if (RegDeleteKey(hRegKey, Name) != ERROR_SUCCESS)
			return false;
	}
	return true;
}


///////////////////////////////////////////////////////////////////////////////
// RegDelete()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_RegDelete(VectorVariant &vParams, Variant &vResult)
{
	// $var = RegDelete(key[, valuename])

	uint	iNumParams = vParams.size();
	HKEY	hRegKey, hMainKey, hRemoteKey = NULL;
	AString	sCname, sKey, sSubKey, sValue;
	bool	Success;

	RegSplitKey(vParams[0].szValue(), sCname, sKey, sSubKey);

	// Get the main key name
	if ( RegGetMainKey(sKey, hMainKey) == false)
	{
		vResult = 0;							// Default is 1, 0=key did not exist
		return AUT_OK;
	}

	// Open the key we want (remote if required)
	if (!sCname.empty())
	{
		// Remote
		if ( RegConnectRegistry(sCname.c_str(), hMainKey, &hRemoteKey) != ERROR_SUCCESS )
		{
			vResult = 0;							// Default is 1, 0=key did not exist
			return AUT_OK;
		}

		hMainKey = hRemoteKey;
	}

	// Local
	// Open the key we want
	if ( RegOpenKeyEx(hMainKey, sSubKey.c_str(), 0, KEY_READ | KEY_WRITE, &hRegKey) != ERROR_SUCCESS )
	{
		vResult = 0;							// Default is 1, 0=key did not exist

		if (!sCname.empty())
			RegCloseKey(hRemoteKey);

		return AUT_OK;
	}


	switch (iNumParams)
	{
		case 1:
			// Remove Key
			Success=RegRemoveSubkeys(hRegKey);
			RegCloseKey(hRegKey);
			if (!Success)
			{
				vResult = 2;							// Default is 1, 2=error deleting
			}
			else if (RegDeleteKey(hMainKey, sSubKey.c_str()) != ERROR_SUCCESS)
			{
				vResult = 2;							// Default is 1, 2=error deleting
			}
			break;

		case 2:
			// Remove Value
			LONG lRes = RegDeleteValue(hRegKey, vParams[1].szValue());
			if (lRes != ERROR_SUCCESS)
			{
				if (lRes == ERROR_FILE_NOT_FOUND)
					vResult = 0;
				else
					vResult = 2;
			}
			RegCloseKey(hRegKey);
			break;
	}

	if (!sCname.empty())
		RegCloseKey(hRemoteKey);

	return AUT_OK;

} // RegDelete()


///////////////////////////////////////////////////////////////////////////////
// RegEnumKey()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_RegEnumKey(VectorVariant &vParams, Variant &vResult)
{
	// $var = RegEnumKey(key, instance)

	HKEY		hRegKey, hMainKey, hRemoteKey = NULL;
	DWORD		dwBuf=255;
	char		szRegBuffer[257];
	AString		sCname, sKey, sSubKey;
	FILETIME	ftLastWriteTime;

	// Set the default return value to ""
	vResult = "";

	RegSplitKey(vParams[0].szValue(), sCname, sKey, sSubKey);

	// Get the main key name
	if ( RegGetMainKey(sKey, hMainKey) == false)
	{
		SetFuncErrorCode(1);					// Default is 0
		return AUT_OK;
	}

	// Open the key we want (remote if required)
	if (!sCname.empty())
	{
		// Remote
		if ( RegConnectRegistry(sCname.c_str(), hMainKey, &hRemoteKey) != ERROR_SUCCESS )
		{
			SetFuncErrorCode(1);					// Default is 0
			return AUT_OK;
		}

		hMainKey = hRemoteKey;
	}

	// Open the registry key
	if ( RegOpenKeyEx(hMainKey, sSubKey.c_str(), 0, KEY_READ , &hRegKey) != ERROR_SUCCESS )
	{
		SetFuncErrorCode(1);					// Default is 0

		if (!sCname.empty())
			RegCloseKey(hRemoteKey);

		return AUT_OK;
	}

	// Read the key
	if ( RegEnumKeyEx(hRegKey, vParams[1].nValue()-1, szRegBuffer, &dwBuf, NULL, NULL, NULL, &ftLastWriteTime) != ERROR_SUCCESS )
	{
		// Error reading key
		SetFuncErrorCode(-1);					// Default is 0
		RegCloseKey(hRegKey);
		if (!sCname.empty())
			RegCloseKey(hRemoteKey);
		return AUT_OK;
	}

	vResult = szRegBuffer;					// Assign to result

	RegCloseKey(hRegKey);
	if (!sCname.empty())
		RegCloseKey(hRemoteKey);
	return AUT_OK;

} // RegEnumKey()


///////////////////////////////////////////////////////////////////////////////
// RegEnumVal()
///////////////////////////////////////////////////////////////////////////////

AUT_RESULT AutoIt_Script::F_RegEnumVal(VectorVariant &vParams, Variant &vResult)
{
	// $var = RegEnumKey(key, instance)

	HKEY		hRegKey, hMainKey, hRemoteKey = NULL;
	DWORD		dwBuf=16383;
	char		szRegBuffer[16385];
	AString		sCname, sKey, sSubKey;

	// Set the default return value to ""
	vResult = "";

	RegSplitKey(vParams[0].szValue(), sCname, sKey, sSubKey);

	// Get the main key name
	if ( RegGetMainKey(sKey, hMainKey) == false)
	{
		SetFuncErrorCode(1);					// Default is 0
		return AUT_OK;
	}

	// Open the key we want (remote if required)
	if (!sCname.empty())
	{
		// Remote
		if ( RegConnectRegistry(sCname.c_str(), hMainKey, &hRemoteKey) != ERROR_SUCCESS )
		{
			SetFuncErrorCode(1);					// Default is 0
			return AUT_OK;
		}

		hMainKey = hRemoteKey;
	}

	// Open the registry key
	if ( RegOpenKeyEx(hMainKey, sSubKey.c_str(), 0, KEY_READ , &hRegKey) != ERROR_SUCCESS )
	{
		SetFuncErrorCode(1);					// Default is 0
		if (!sCname.empty())
			RegCloseKey(hRemoteKey);
		return AUT_OK;
	}

	// Read the key
	if ( RegEnumValue(hRegKey, vParams[1].nValue()-1, szRegBuffer, &dwBuf, NULL, NULL, NULL, NULL) != ERROR_SUCCESS )
	{
		// Error reading key
		SetFuncErrorCode(-1);					// Default is 0
		RegCloseKey(hRegKey);
		if (!sCname.empty())
			RegCloseKey(hRemoteKey);
		return AUT_OK;
	}

	vResult = szRegBuffer;					// Assign to result

	RegCloseKey(hRegKey);
	if (!sCname.empty())
		RegCloseKey(hRemoteKey);
	return AUT_OK;

} // RegEnumVal()

