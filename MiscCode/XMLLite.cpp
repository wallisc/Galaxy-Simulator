#include "stdafx.h"

//----------------------------------------------------------------------- 
// This file is part of the Windows SDK Code Samples. 
//  
// Copyright (C) Microsoft Corporation.  All rights reserved. 
//  
// This source code is intended only as a supplement to Microsoft 
// Development Tools and/or on-line documentation.  See these other 
// materials for detailed information regarding Microsoft code samples. 
//  
// THIS CODE AND INFORMATION ARE PROVIDED AS IS WITHOUT WARRANTY OF ANY 
// KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE 
// IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE. 
//----------------------------------------------------------------------- 

#include <ole2.h> 
#include <xmllite.h> 
#include <stdio.h> 
#include <shlwapi.h> 
#include <string>
#include <iostream>
#include <stdlib.h>
#include <wchar.h>

#include "atlbase.h"
#include "atlstr.h"
#include "comutil.h"

#pragma warning(disable : 4127)  // conditional expression is constant 
#define CHKHR(stmt)             do { hr = (stmt); if (FAILED(hr)) goto CleanUp; } while(0) 
#define HR(stmt)                do { hr = (stmt); goto CleanUp; } while(0) 
#define SAFE_RELEASE(I)         do { if (I){ I->Release(); } I = NULL; } while(0) 
using namespace std;

string* name;
float* mass;
float* diameter;
int* brightness;
float* xcoord;
float* ycoord;
float* zcoord;
const int maxnamesize = 200;
int g_maxparticles;


HRESULT WriteAttributes(IXmlReader* pReader)
{
	const WCHAR* pwszPrefix;
	const WCHAR* pwszLocalName;
	const WCHAR* pwszValue;
	HRESULT hr = pReader->MoveToFirstAttribute();

	if (S_FALSE == hr)
		return hr;
	if (S_OK != hr)
	{
		wprintf(L"Error moving to first attribute, error is %08.8lx", hr);
		return hr;
	}
	else
	{
		while (TRUE)
		{
			if (!pReader->IsDefault())
			{ //pReader is not default
				UINT cwchPrefix;
				if (FAILED(hr = pReader->GetPrefix(&pwszPrefix, &cwchPrefix)))
				{
					wprintf(L"Error getting prefix, error is %08.8lx", hr);
					return hr;
				}
				if (FAILED(hr = pReader->GetLocalName(&pwszLocalName, NULL)))
				{
					wprintf(L"Error getting local name, error is %08.8lx", hr);
					return hr;
				}
				if (FAILED(hr = pReader->GetValue(&pwszValue, NULL)))
				{
					wprintf(L"Error getting value, error is %08.8lx", hr);
					return hr;
				}
				if (cwchPrefix > 0)
				{
			
				wprintf(L"Attr: %s:%s=\"%s\" \n", pwszPrefix, pwszLocalName, pwszValue);
				
				}
				else
					wprintf(L"Attr: %s=\"%s\" \n", pwszLocalName, pwszValue);
			}

			if (S_OK != pReader->MoveToNextAttribute())
				break;
		}
	}
	return hr;
}

int __cdecl wmain(int argc, _In_reads_(argc) WCHAR* argv[])
{
	HRESULT hr = S_OK;
	IStream *pFileStream = NULL;
	IXmlReader *pReader = NULL;
	XmlNodeType nodeType;
	const WCHAR* pwszPrefix;
	const WCHAR* pwszLocalName;
	const WCHAR* pwszValue;
	UINT cwchPrefix;

	if (argc != 2) //??? Currently this is where the program is breaking. Never gets past this. argc value is 1? 
	{
		wprintf(L"Usage: XmlLiteReader.exe name-of-input-file\n");
		return 0; 
	}

	//Open read-only input stream 
	if (FAILED(hr = SHCreateStreamOnFile(argv[1], STGM_READ, &pFileStream)))
	{
		wprintf(L"Error creating file reader, error is %08.8lx", hr);
		HR(hr);
	}

	if (FAILED(hr = CreateXmlReader(__uuidof(IXmlReader), (void**)&pReader, NULL)))
	{
		wprintf(L"Error creating xml reader, error is %08.8lx", hr);
		HR(hr);
	}

	if (FAILED(hr = pReader->SetProperty(XmlReaderProperty_DtdProcessing, DtdProcessing_Prohibit)))
	{
		wprintf(L"Error setting XmlReaderProperty_DtdProcessing, error is %08.8lx", hr);
		HR(hr);
	}

	if (FAILED(hr = pReader->SetInput(pFileStream)))
	{
		wprintf(L"Error setting input for reader, error is %08.8lx", hr);
		HR(hr);
	}

	//read until there are no more nodes 
	//Pretty sure this is where we would change the printf's outside the if's so they go to an array? probably? 
	int first = 0;
	int var = 0;
	int count = 0;
	while (S_OK == (hr = pReader->Read(&nodeType)))
	{
		switch (nodeType)
		{
		case XmlNodeType_XmlDeclaration:
			wprintf(L"XmlDeclaration\n");
			if (FAILED(hr = WriteAttributes(pReader)))
			{
				wprintf(L"Error writing attributes, error is %08.8lx", hr);
				HR(hr);
			}
			break;
		case XmlNodeType_Element:
			if (FAILED(hr = pReader->GetPrefix(&pwszPrefix, &cwchPrefix)))
			{
				wprintf(L"Error getting prefix, error is %08.8lx", hr);
				HR(hr);
			}
			if (FAILED(hr = pReader->GetLocalName(&pwszLocalName, NULL)))
			{
				wprintf(L"Error getting local name, error is %08.8lx", hr);
				HR(hr);
			}
			if (cwchPrefix > 0){
				wprintf(L"Element: %s:%s\n", pwszPrefix, pwszLocalName);
			}
			else {
				//wprintf(L"Element: %s\n", pwszLocalName);
				if (first == 0)
				{
					g_maxparticles = (int)pwszLocalName;
					name = new string [g_maxparticles];
					mass = new float [g_maxparticles];
					diameter = new float [g_maxparticles];
					brightness = new int [g_maxparticles];
					xcoord = new float [g_maxparticles];
					ycoord = new float [g_maxparticles];
					zcoord = new float [g_maxparticles];
				}
				else
				{
					if (var == 0)
					{
						char buffer[maxnamesize];
						wcstombs_s(NULL, buffer, sizeof(char) * maxnamesize, pwszLocalName, maxnamesize);
						name[count] = buffer;
					}
					else if (var == 1)
						mass[count] = wcstof (pwszLocalName, NULL);
					else if (var == 2)
						diameter[count] = wcstof(pwszLocalName, NULL);
					else if (var == 3)
						brightness[count] = int(pwszLocalName);
					else if (var == 4)
						xcoord[count] = wcstof(pwszLocalName, NULL);
					else if (count == 5)
						ycoord[count] = wcstof(pwszLocalName, NULL);
					else if (var == 6)
						zcoord[count] = wcstof(pwszLocalName, NULL);
					var++;
				}
			}
			if (FAILED(hr = WriteAttributes(pReader)))
			{
				wprintf(L"Error writing attributes, error is %08.8lx", hr);
				HR(hr);
			}

			if (pReader->IsEmptyElement())
				wprintf(L" (empty)");
			break;
		case XmlNodeType_EndElement:
			if (FAILED(hr = pReader->GetPrefix(&pwszPrefix, &cwchPrefix)))
			{
				wprintf(L"Error getting prefix, error is %08.8lx", hr);
				HR(hr);
			}
			if (FAILED(hr = pReader->GetLocalName(&pwszLocalName, NULL)))
			{
				wprintf(L"Error getting local name, error is %08.8lx", hr);
				HR(hr);
			}
			if (cwchPrefix > 0)
				wprintf(L"End Element: %s:%s\n", pwszPrefix, pwszLocalName);
			else
			{
				wprintf(L"End Element: %s\n", pwszLocalName);
				if (first == 0)
				{
					first = 1;
				}
				else
				{
					count++;
					var = 0;
				}
			}
			break;
		case XmlNodeType_Text:
		case XmlNodeType_Whitespace:
			if (FAILED(hr = pReader->GetValue(&pwszValue, NULL)))
			{
				wprintf(L"Error getting value, error is %08.8lx", hr);
				HR(hr);
			}
			wprintf(L"Text: %s\n", pwszValue);
			break;
		case XmlNodeType_CDATA:
			if (FAILED(hr = pReader->GetValue(&pwszValue, NULL)))
			{
				wprintf(L"Error getting value, error is %08.8lx", hr);
				HR(hr);
			}
			wprintf(L"CDATA: %s\n", pwszValue);
			break;
		case XmlNodeType_ProcessingInstruction:
			if (FAILED(hr = pReader->GetLocalName(&pwszLocalName, NULL)))
			{
				wprintf(L"Error getting name, error is %08.8lx", hr);
				HR(hr);
			}
			if (FAILED(hr = pReader->GetValue(&pwszValue, NULL)))
			{
				wprintf(L"Error getting value, error is %08.8lx", hr);
				HR(hr);
			}
			wprintf(L"Processing Instruction name:%s value:%s\n", pwszLocalName, pwszValue);
			break;
		case XmlNodeType_Comment:
			if (FAILED(hr = pReader->GetValue(&pwszValue, NULL)))
			{
				wprintf(L"Error getting value, error is %08.8lx", hr);
				HR(hr);
			}
			wprintf(L"Comment: %s\n", pwszValue);
			break;
		case XmlNodeType_DocumentType:
			wprintf(L"DOCTYPE is not printed\n");
			break;
		}
		//printf("%d\n", g_maxparticles);
	}

CleanUp:
	SAFE_RELEASE(pFileStream);
	SAFE_RELEASE(pReader);
	return hr;
}

