//******************************************************************************
//  Copyright (c) Microsoft Corporation.
//
// @File: cmdline.cpp
// @Author: alexbud
//
// Purpose:
//
//  Command line processing.
//  
// Notes:
//
// @EndHeader@
//******************************************************************************  

#include "common.h"
#include "cmdline.h"
#include <string.h>
#include <assert.h>

//------------------------------------------------------------------------------
// Function: peekNextTokenChar
//
// Description:
//
//  Fetch next start-of-token character in a non-destructive way.
//
// Parameters:
//
// Returns:
//
// Notes:
//
//  Assumes 'str' is positioned at one-beyond-end of prior token.
//
static char
peekNextTokenChar(
	_In_z_ const char* str)
{
	// Skip spaces.
	//
	while (isspace(*str))
	{
		++str;
	}
	return *str;
}

//------------------------------------------------------------------------------
// Function: ParseArgs
//
// Description:
//
//  Parses host-layer arguments stopping on first non-host switch.
//
// Parameters:
//
//  args - mutable copy of arguments.
//  parsedArgs - result of parsing.
//
// Returns:
//
// Notes:
//
_Check_return_ HRESULT
ParseArgs(
	_In_z_ char* args,
	_Out_ ParsedArgs* parsedArgs)
{
	HRESULT hr = S_OK;
	DbgScriptHostContext* hostCtxt = GetHostContext();
	char* nextTok = nullptr;
	
	char* tok = strtok_s(args, " \t", &nextTok);
	while (tok)
	{
		// Encountered switch terminator? '--'
		//
		if (!strcmp(tok, "--"))
		{
			// Swallow and break out.
			//
			break;
		}

		// Keep switches to ourselves.
		//
		if (tok[0] == '-')
		{
			if (!strcmp(tok, "-t"))
			{
				parsedArgs->TimeRun = true;
			}
			else if (!strcmp(tok, "-l"))
			{
				// Capture value for key.
				//
				tok = strtok_s(nullptr, " \t", &nextTok);
				if (!tok)
				{
					hr = E_INVALIDARG;
					hostCtxt->DebugControl->Output(
						DEBUG_OUTPUT_ERROR,
						"Error: -l requires a language ID.\n");
					goto exit;
				}

				// Convert ANSI to Wide and store in parsedArgs->LangId.
				//
				size_t cchConverted = 0;
				errno_t err = mbstowcs_s(
					&cchConverted,
					STRING_AND_CCH(parsedArgs->LangId),
					tok,
					_TRUNCATE);
				
				// Since we specified _TRUNCATE, EINVAL is the only possibility
				// which indicates an app bug.
				//
				assert(!err);
				assert(cchConverted > 0);
			}
			else
			{
				hr = E_INVALIDARG;
				hostCtxt->DebugControl->Output(
					DEBUG_OUTPUT_ERROR,
					"Error: Unknown switch '%hs'.\n", tok);
				goto exit;
			}
		}
		else
		{
			break;
		}

		// Don't prematurely NUL-terminate something that isn't a switch.
		//
		if (peekNextTokenChar(nextTok) != '-')
		{
			break;
		}
		
		// Advance token.
		//
		tok = strtok_s(nullptr, " \t", &nextTok);
	}

	parsedArgs->RemainingArgs = nextTok;

exit:
	return hr;
}

