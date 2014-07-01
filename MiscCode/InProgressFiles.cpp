// InProgressFiles.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include "DXUT.h"
#include "DXUTgui.h"
#include "SDKmisc.h"
#include "DXUTcamera.h"
#include "DXUTsettingsdlg.h"
#include <commdlg.h>
#include "resource.h"
#include "WaitDlg.h"
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

using namespace DirectX;
using namespace std;

//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
#define GET_OBJECT_INFO 8



int NUM_PARTICLES;
string name[];
float mass[];
float diameter[];
float brightness[];
float xcoord[];
float ycoord[];
float zcoord[];



//THINGS FROM SAMPLE
struct PARTICLE
{
	XMFLOAT4 pos; //position stored as a vector
	XMFLOAT4 velo; //to be calculated elsewhere?
};

PARTICLE* g_pParticleArray = NULL;


struct PARTICLE_VERTEX
{
	XMFLOAT4 Color;
};

//THINGS FROM SAMPLE

struct PARTICLE_DETAILS
{
	string name;
	float mass;
	float diameter;
	float brightness;
};

PARTICLE_DETAILS* g_pParticleArrayTWO = NULL;

//Loads all details about each object except positon and velocity
void fillParticleDetails(PARTICLE_DETAILS particles2[], int NUM_PARTICLES, string nameIn[], float massIn[], float diameterIn[], float brightnessIn[]){
	for (int i = 0; i < NUM_PARTICLES; i++){
		particles2[i].name = nameIn[i];
		particles2[i].mass = massIn[i];
		particles2[i].diameter = diameterIn[i];
		particles2[i].brightness = brightnessIn[i];
	}
}

void fillParticles(PARTICLE particles[], int NUM_PARTICLES, float x[], float y[], float z[], XMFLOAT4 Velocity){
	//method that loads g_pParticleArray with position as a vector and velocity (that is being updated constantly?)
	//create and then call here a getVelocity methods 

	for (int i = 0; i < NUM_PARTICLES; i++){
		particles[i].pos = createPositionFloat(x[i], y[i], z[i]);
		pParticles[i].velo = Velocity;
	}
}


/////////set of get methods for object properties/////////////

//gets the name of an object using its index number
string getName(int index){
	return g_pParticleArrayTWO[index].name;
}

//gets the mass of an object using its index number
float getMass(int index){
	return g_pParticleArrayTWO[index].mass;
}

//gets the diameter of an object using its index number
float getDiameter(int index){
	return g_pParticleArrayTWO[index].diameter;
}

//gets the mass of an object using its index number
float getBrightness(int index){
	return g_pParticleArrayTWO[index].brightness;
}

//gets position of object using its index number
XMFLOAT4 getPosition(int index){				//change positionType to what the actual type is
	return g_pParticleArray[index].pos;
}

//gets velocity of the object using its index number 
velocityType getVelocity(int index){				//change velocityType to what the actual type is
	return g_pParticleArray[index].velo;
}

/////////////////////////////////////////////////////

//method for establishing the position of a celestial body
//definition for XMFLOAT4 is in the sample, line 570
XMFLOAT4 createPositionFloat(float xParticle, float yParticle, float zParticle){
	return XMFLOAT4(xParticle, yParticle, zParticle, 10000.0f * 10000.0f);
}



/////////////////////////////////////////////////////

//BUTTONS

//method for creating buttons for each object
void createObjectButtons(int NUM_PARTICLES){
	for (int i = 0; i < NUM_PARTICLES; i++){
		LPCWSTR name = getName(i);
		g_HUD.AddButton(GET_OBJECT_INFO, Lname, getPosition(i).x, getPosition(i).y, 10, 10)
	}
} 

//action for CALLBACK when button is clicked
case GET_OBJECT_INFO:
	displayObjInfo(); break;

//method to display object info
	void displayObjInfo(int index){

	}

/////////////////////////////////////////////////////

	int ParseFile(){
		HRESULT hr = S_OK;
		IStream *pFileStream = NULL;
		IXmlReader *pReader = NULL;
		XmlNodeType nodeType;
		const WCHAR* pwszPrefix;
		const WCHAR* pwszLocalName;
		const WCHAR* pwszValue;
		UINT cwchPrefix;

		if (2 != 2) //??? Currently this is where the program is breaking. Never gets past this. argc value is 1? 
		{
			wprintf(L"Usage: XmlLiteReader.exe name-of-input-file\n");
			return 0;
		}

		//Open read-only input stream 
		LPCWSTR fileName = L"C:\\Users\\t-kaver\\Source\\Repos\\Galaxy-Simulator\\GalaxySim\\XMLFileFormatTEST.xml";
		if (FAILED(hr = SHCreateStreamOnFile(fileName, STGM_READ, &pFileStream)))
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
						NUM_PARTICLES = (int)pwszLocalName;
						name = new string[NUM_PARTICLES];
						mass = new float[NUM_PARTICLES];
						diameter = new float[NUM_PARTICLES];
						brightness = new int[NUM_PARTICLES];
						xcoord = new float[NUM_PARTICLES];
						ycoord = new float[NUM_PARTICLES];
						zcoord = new float[NUM_PARTICLES];
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
							mass[count] = wcstof(pwszLocalName, NULL);
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


	/////////////////////////////////////////////////////


int _tmain(int argc, _TCHAR* argv[])
{
	int ParseFile()
	g_pParticleArray = new PARTICLE[MAX_PARTICLES];
	g_pParticleArrayTWO = new PARTICLE_DETAILS[MAX_PARTICLES];
	fillParticles(g_pParticleArray, NUM_PARTICLES, xcoord, ycoord, zcoord, XMFLOAT4(0, 0, 0, 1));
	fillParticleDetails(g_pParticleArrayTWO, MAX_PARTICLES, name, mass, diameter, brightness);
	return 0;
}

