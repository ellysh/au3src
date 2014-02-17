#ifndef __SCRIPTFILE_H
#define __SCRIPTFILE_H

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
// scriptfile.h
//
// The script file object.  This object handles all requests to the script file
// that was read in.
//
///////////////////////////////////////////////////////////////////////////////


// Includes

// Define our structure for holding each line of text from the script
typedef struct larray
{
	char	*szLine;							// Text of the script line
	int 	nLineNum;							// The line number (from the .au3 file)
	int		nIncludeID;							// ID of the file this line is from
	struct 	larray *lpNext;						// Next entry in linked list

} LARRAY;


// Define our structure for keeping track of unique include names
#define AUT_MAX_INCLUDE_IDS		64				// Max number of Include filenames

#define AUT_DIRECTIVE_ERROR				0		// Error processing directive (usually bad include filename)
#define AUT_DIRECTIVE_STRIP				1		// Strip directive/Already processed
#define AUT_DIRECTIVE_PASSTHROUGH		2		// Add the directive text
#define AUT_DIRECTIVE_NODIRECTIVE		3		// Line contains no directive
#define AUT_DIRECTIVE_INCLUDEDECLINED	4		// Attempt to include #include-once file more than once - ignore rest of file


class AutoIt_ScriptFile
{
public:
	// Functions
	AutoIt_ScriptFile();						// Constructor
	~AutoIt_ScriptFile();						// Destructor
	bool			LoadScript(char *szFile);	// Loads a script into memory
	void			PrepareScript(void);		// Prepares a loaded script for speed
	void			UnloadScript(void);			// Removes a script from memory
	const char *	GetLine(int nLineNum);			// Retrieve line x
	int				GetAutLineNumber(int nLineNum);	// Get the aut script file line number for line x
	int				GetNumScriptLines() { return m_nScriptLines; }	// Returns number of lines in the script
	void			AddLine(int nLineNum,  const char *szLine, int nIncludeID);
	const char *	GetIncludeName(int nIncludeID);
	const char *	GetIncludeFileName(int nIncludeID);
	int				GetIncludeID(int nLineNum);


private:
	// Variables
	LARRAY			*m_lpScript;				// Start of the linked list
	LARRAY			*m_lpScriptLast;			// Last node of the list
	int				m_nScriptLines;				// Number of lines in the list
	char			**m_szScriptLines;			// Array of char * for each line of the script

	char			*m_szIncludeIDs[AUT_MAX_INCLUDE_IDS];
	int				m_nIncludeCounts[AUT_MAX_INCLUDE_IDS];
	int				m_nNumIncludes;

	char			**m_pIncludeDirs;
	unsigned int	m_nIncludeDirs;

	// Functions
	void			AppendLastLine(const char *szLine);
	bool			Include(const char *szFileName, int nIncludeID);
	int				AddIncludeName(const char *szFileName);
	void			StripLeading(char *szLine);
	void			StripTrailing(char *szLine);
	bool			IncludeParse(const char *szLine, char *szTemp);

	#ifdef AUTOITSC
		int		CheckDirective(UCHAR *lpData, char *szLine, ULONG &nPos, ULONG nDataSize);
		bool	ExtractLine(UCHAR *lpData, char *szOut, ULONG &nPos, ULONG nDataSize);
	#else
		int		CheckDirective(char *szLine, const char *szFullFileName, int &nLineNum, FILE *fIn, int nIncludeID);
	#endif
};


///////////////////////////////////////////////////////////////////////////////

#endif
