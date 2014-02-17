#ifndef __CMDLINE_H
#define __CMDLINE_H

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
// cmdline.h
//
// A standalone class to make reading command line options a little easier.
//
///////////////////////////////////////////////////////////////////////////////


#define	CMDLINE_MAXPARAMS	64					// Max number of parameters
#define CMDLINE_MAXLEN		4096				// Each parameter can be this many characters

class CmdLine
{
public:
	CmdLine();									// Constructor
	~CmdLine();									// Destructor

	// Functions
	void	SetCmdLine(char *szCmdLine);		// Call this with the full cmd line

	const char * GetCmdLine() { return m_szCmdLineRaw; }

	int		GetNumParams(void) const {return m_nNumParameters;}
	bool	GetParam(int nParam, char *szParam) const;
	bool	GetNextParam(char *szParam);
	void	GetNextParamReset(void) {m_nCurrentParam = 0;}

private:
	// Variables
	int		m_nNumParameters;					// Number of space separated paramters on the cmdline
	int		m_nCurrentParam;					// Current param to return for GetNextParam()
	char	*m_szCmdLineRaw;					// The original command line
	char	*m_szParams[CMDLINE_MAXPARAMS];		// List of pointers to each parameter

	// Functions
	void	StoreParam(char *szParam);
	void	Reset(void);						// Frees up memory ready for reuse

};


///////////////////////////////////////////////////////////////////////////////

#endif
