//--------------------------------------------------------------------------------------
// File: GalaxySim.cpp
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// 
// Dev Buddies:
// Christopher Wallis
//
// Developers:
// Ana-Maria Constantin
// Kanika Verma
// Melanie Loppnow


// 
//--------------------------------------------------------------------------------------
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
#include <cstdlib>
#include <ctime>
#include <math.h>
#include <tchar.h>
#include <strsafe.h>
#include <sstream>
#include <string>

#include "atlbase.h"
#include "atlstr.h"
#include "comutil.h"

#pragma warning( disable : 4100 )
#pragma warning(disable : 4127)  // conditional expression is constant 

using namespace DirectX;
using namespace std;

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
CDXUTDialogResourceManager          g_DialogResourceManager;    // manager for shared resources of dialogs
CModelViewerCamera                  g_Camera;                   // A model viewing camera
CD3DSettingsDlg                     g_D3DSettingsDlg;           // Device settings dialog
CDXUTDialog                         g_HUD;                      // dialog for standard controls
CDXUTDialog                         g_SampleUI;                 // dialog for sample specific controls
CDXUTTextHelper*                    g_pTxtHelper = nullptr;

ID3D11VertexShader*                 g_pRenderParticlesVS = nullptr;
ID3D11GeometryShader*               g_pRenderParticlesGS = nullptr;
ID3D11PixelShader*                  g_pRenderParticlesPS = nullptr;
ID3D11SamplerState*                 g_pSampleStateLinear = nullptr;
ID3D11BlendState*                   g_pBlendingStateParticle = nullptr;
ID3D11DepthStencilState*            g_pDepthStencilState = nullptr;

ID3D11Buffer*                       g_pcbCS = nullptr;

ID3D11Buffer*                       g_pParticlePosVelo0 = nullptr;
ID3D11Buffer*                       g_pParticlePosVelo1 = nullptr;
ID3D11ShaderResourceView*           g_pParticlePosVeloRV0 = nullptr;
ID3D11ShaderResourceView*           g_pParticlePosVeloRV1 = nullptr;
ID3D11Buffer*                       g_pParticleBuffer = nullptr;
ID3D11InputLayout*                  g_pParticleVertexLayout = nullptr;

ID3D11Buffer*                       g_pcbGS = nullptr;

ID3D11ShaderResourceView*           g_pParticleTexRV = nullptr;

const float                         g_fSpread = 400.0f;

CDXUTEditBox						*g_JumpTimeInput = nullptr;

struct PARTICLE_VERTEX
{
	XMFLOAT4 Color;
};

#define MAX_PARTICLES      10000            // the max number of particles in the n-body simulation
//#define NUM_PARTICLES      5            // the number of particles in the n-body simulation
int NUM_PARTICLES;
#define MAX_FPS            60.0f            // throttle the frame rate
#define SECONDS_PER_FRAME  1.0f / MAX_FPS

#define CHKHR(stmt)             do { hr = (stmt); if (FAILED(hr)) goto CleanUp; } while(0) 
#define HR(stmt)                do { hr = (stmt); goto CleanUp; } while(0) 

struct CB_GS
{
	XMFLOAT4X4 m_WorldViewProj;
	XMFLOAT4X4 m_InvView;
};

struct CB_CS
{
	UINT param[4];
	float paramf[4];
};

struct PARTICLE
{
	XMFLOAT4 pos;
	XMFLOAT4 velo;
};

struct PARTICLE_DETAILS
{
	wstring name;
	float mass;
	float diameter;
	float brightness;
	float red;
	float green;
	float blue;
};


PARTICLE*                           g_pParticleArray = NULL;
PARTICLE_DETAILS*					g_pParticleArrayTWO = NULL;


//will point to arrays that hold info from file until it is put into above two arrays
class ObjectData
{
public:

	ObjectData() :
		m_name(L"unknown"), m_mass(0.0f), m_diameter(0.0f), m_brightness(0.0f), m_xcoord(0.0f), m_ycoord(0.0f), m_zcoord(0.0f), m_xvelo(0.0f), m_yvelo(0.0f), 
		m_zvelo(0.0f), m_red(0.0f), m_green(0.0f), m_blue(0.0f)
	{}

	ObjectData(const wstring & name, float mass, float diameter, int brightness, float x, float y, float z, float xv, float yv, float zv, float r, float g, float b) :
		m_name(name), m_mass(mass), m_diameter(diameter), m_brightness(brightness), m_xcoord(x), m_ycoord(y), m_zcoord(z), 
		m_xvelo(xv), m_yvelo(yv), m_zvelo(zv), m_red(r), m_green(g), m_blue(b)
	{
		// Could assert on the various properties to ensure they are within range
	}

	wstring   m_name;
	float     m_mass;
	float     m_diameter;
	float     m_brightness;
	float     m_xcoord;
	float     m_ycoord;
	float     m_zcoord;
	float     m_xvelo;
	float     m_yvelo;
	float     m_zvelo;
	float     m_red;
	float     m_green;
	float     m_blue;

};

std::vector<ObjectData> g_objects;

//const float g_constant = -8.644 * pow(10, -13);
const float g_constant = -8.644E-16;
const int g_cTimeStringLength = 20;

float g_red[MAX_PARTICLES];
float g_green[MAX_PARTICLES];
float g_blue[MAX_PARTICLES];
bool g_isFirst = true;
bool g_isPaused = false;
bool g_hasDisplay = false;
CDXUTEditBox *g_pTextBox;
bool g_firstTextBox = true;
XMMATRIX g_mProj;
XMMATRIX g_mView;

CB_GS * g_pCBGS;

int g_height = 600;
int g_width = 800;

bool g_relevantMouse = false;
float g_xMouse;
float g_yMouse;

bool g_loaded = false;

double g_timeValue = 0.005; //can change this to change speed of simulation, used later to do 2x and 0.5x
double g_systemTime = 0; //sets the inital system time to 0
LPWSTR g_timeString; //used later for the Jump Time In button user uses to input time to jump to.


//-------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
#define IDC_TOGGLEFULLSCREEN    1
#define IDC_TOGGLEREF           3
#define IDC_CHANGEDEVICE        4
#define IDC_RESETPARTICLES      5
#define IDC_PAUSE               7
#define IDC_DOUBLESPEED			8
#define IDC_HALFSPEED			9
#define IDC_JUMPTIMEIN			10
#define IDC_SUBMITTIMEIN		11
#define IDC_RESETCAMERA			12

//--------------------------------------------------------------------------------------
// Forward declarations 
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings(DXUTDeviceSettings* pDeviceSettings, void* pUserContext);
void CALLBACK OnFrameMove(double fTime, float fElapsedTime, void* pUserContext);
LRESULT CALLBACK MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
	void* pUserContext);
void CALLBACK OnGUIEvent(UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext);

bool CALLBACK IsD3D11DeviceAcceptable(const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo,
	DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext);
HRESULT CALLBACK OnD3D11CreateDevice(ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
	void* pUserContext);
HRESULT CALLBACK OnD3D11ResizedSwapChain(ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
	const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext);
void CALLBACK OnD3D11ReleasingSwapChain(void* pUserContext);
void CALLBACK OnD3D11DestroyDevice(void* pUserContext);
void CALLBACK OnD3D11FrameRender(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime,
	float fElapsedTime, void* pUserContext);
void CALLBACK OnMouseEvent(_In_ bool bLeftButtonDown, _In_ bool bRightButtonDown, _In_ bool bMiddleButtonDown,
	_In_ bool bSideButton1Down, _In_ bool bSideButton2Down, _In_ int nMouseWheelDelta,
	_In_ int xPos, _In_ int yPos, _In_opt_ void* pUserContext);

void InitApp();
void RenderText();
//declare WriteAttributes function here, content is below ParseFile()
HRESULT WriteAttributes(IXmlReader* pReader);
int ParseFile();
//functions associated with various buttons
void displayObjectInfo();
void doubleSpeed();
void halfSpeed();
LPWSTR GetSimTime();
void jumpTime(float newTime);


//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	DXUTSetCallbackDeviceChanging(ModifyDeviceSettings);
	DXUTSetCallbackMsgProc(MsgProc);
	DXUTSetCallbackFrameMove(OnFrameMove);

	DXUTSetCallbackD3D11DeviceAcceptable(IsD3D11DeviceAcceptable);
	DXUTSetCallbackD3D11DeviceCreated(OnD3D11CreateDevice);
	DXUTSetCallbackD3D11SwapChainResized(OnD3D11ResizedSwapChain);
	DXUTSetCallbackD3D11FrameRender(OnD3D11FrameRender);
	DXUTSetCallbackD3D11SwapChainReleasing(OnD3D11ReleasingSwapChain);
	DXUTSetCallbackD3D11DeviceDestroyed(OnD3D11DestroyDevice);
	DXUTSetCallbackMouse(OnMouseEvent);

	ParseFile();

	InitApp();

	DXUTInit(true, true);                 // Use this line instead to try to create a hardware device

	DXUTSetCursorSettings(true, true); // Show the cursor and clip it when in full screen
	DXUTCreateWindow(L"SkyX");
	DXUTCreateDevice(D3D_FEATURE_LEVEL_10_0, true, g_width, g_height);
	DXUTMainLoop();                      // Enter into the DXUT render loop

	g_objects.clear();

	return DXUTGetExitCode();
}





//--------------------------------------------------------------------------------------
// Initialize the app 
//--------------------------------------------------------------------------------------
void InitApp()
{
	g_D3DSettingsDlg.Init(&g_DialogResourceManager);
	g_HUD.Init(&g_DialogResourceManager);
	g_SampleUI.Init(&g_DialogResourceManager);

	g_HUD.SetCallback(OnGUIEvent); int iY = 10;
	g_HUD.AddButton(IDC_TOGGLEFULLSCREEN, L"Full screen (F6)", 0, iY, 170, 23, VK_F6);
	g_HUD.AddButton(IDC_TOGGLEREF, L"Toggle REF (F3)", 0, iY += 26, 170, 23, VK_F3);
	g_HUD.AddButton(IDC_CHANGEDEVICE, L"Change device (F2)", 0, iY += 26, 170, 23, VK_F2);
	g_HUD.AddButton(IDC_RESETPARTICLES, L"Reset particles (F4)", 0, iY += 26, 170, 22, VK_F4);
	g_HUD.AddButton(IDC_PAUSE, L"Pause / Unpause", 0, iY += 26, 170, 22);
	g_HUD.AddButton(IDC_DOUBLESPEED, L"Speed 2x", 0, iY += 26, 170, 23);
	g_HUD.AddButton(IDC_HALFSPEED, L"Speed 0.5x", 0, iY += 26, 170, 23);
	g_HUD.AddEditBox(IDC_JUMPTIMEIN, L"", 0, iY += 26, 170, 40, false, &g_JumpTimeInput);
	g_HUD.AddButton(IDC_SUBMITTIMEIN, L"Jump!", 0, iY += 40, 170, 40);
	g_HUD.AddButton(IDC_RESETCAMERA, L"Reset Camera Position", 0, iY += 40, 170, 23);

	g_SampleUI.SetCallback(OnGUIEvent);
}

//--------------------------------------------------------------------------------------
// Parse File 
//--------------------------------------------------------------------------------------

int ParseFile(){
	HRESULT hr = S_OK;
	IStream *pFileStream = NULL;
	IXmlReader *pReader = NULL;
	XmlNodeType nodeType;
	const WCHAR* pwszPrefix;
	const WCHAR* pwszLocalName;
	const WCHAR* pwszValue;
	UINT cwchPrefix;

	ObjectData objectData;

	//Open read-only input stream 
	LPCWSTR fileName = L"ObjectData.xml";
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

	const WCHAR * elementName = NULL;

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

				elementName = pwszLocalName;

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
				//wprintf(L"End Element: %s\n", pwszLocalName);

			}

			if (pwszLocalName != NULL && wcscmp(pwszLocalName, L"object") == 0){
				g_objects.push_back(objectData);
			}

			break;

		case XmlNodeType_Text:
			if (FAILED(hr = pReader->GetValue(&pwszValue, NULL)))
			{
				wprintf(L"Error getting value, error is %08.8lx", hr);
				HR(hr);
			}
			wprintf(L"Text: %s\n", pwszValue);

			if (elementName != NULL && wcscmp(elementName, L"name") == 0){
				objectData.m_name = pwszValue;
			}

			else if (elementName != NULL && wcscmp(elementName, L"mass") == 0){
				objectData.m_mass = wcstof(pwszValue, NULL);
			}

			else if (elementName != NULL && wcscmp(elementName, L"diameter") == 0){
				objectData.m_diameter = wcstof(pwszValue, NULL);
			}

			else if (elementName != NULL && wcscmp(elementName, L"brightness") == 0){
				objectData.m_brightness = wcstof(pwszValue, NULL);
			}

			else if (elementName != NULL && wcscmp(elementName, L"xcoord") == 0){
				objectData.m_xcoord = wcstof(pwszValue, NULL);
			}

			else if (elementName != NULL && wcscmp(elementName, L"ycoord") == 0){
				objectData.m_ycoord = wcstof(pwszValue, NULL);
			}
			else if (elementName != NULL && wcscmp(elementName, L"zcoord") == 0){
				objectData.m_zcoord = wcstof(pwszValue, NULL);
			}
			else if (elementName != NULL && wcscmp(elementName, L"xvelo") == 0){
				objectData.m_xvelo = wcstof(pwszValue, NULL);
			}
			else if (elementName != NULL && wcscmp(elementName, L"yvelo") == 0){
				objectData.m_yvelo = wcstof(pwszValue, NULL);
			}
			else if (elementName != NULL && wcscmp(elementName, L"zvelo") == 0){
				objectData.m_zvelo = wcstof(pwszValue, NULL);
			}
			else if (elementName != NULL && wcscmp(elementName, L"red") == 0){
				objectData.m_red = wcstof(pwszValue, NULL);
			}
			else if (elementName != NULL && wcscmp(elementName, L"green") == 0){
				objectData.m_green = wcstof(pwszValue, NULL);
			}
			else if (elementName != NULL && wcscmp(elementName, L"blue") == 0){
				objectData.m_blue = wcstof(pwszValue, NULL);
			}

			break;

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
	}

#if 0 // Keep this code around as example of using OutputDebugString
	for (auto object : g_objects)
	{
		char buffer[256];
		sprintf_s(buffer, sizeof(buffer), "Name Array: %s\n", object.m_name.c_str());
		::OutputDebugStringA(buffer);
	}
#endif


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




//--------------------------------------------------------------------------------------
HRESULT CreateParticleBuffer(ID3D11Device* pd3dDevice)
{
	HRESULT hr = S_OK;

	D3D11_BUFFER_DESC vbdesc =
	{
		MAX_PARTICLES * sizeof(PARTICLE_VERTEX),
		D3D11_USAGE_DEFAULT,
		D3D11_BIND_VERTEX_BUFFER,
		0,
		0
	};
	D3D11_SUBRESOURCE_DATA vbInitData;
	ZeroMemory(&vbInitData, sizeof(D3D11_SUBRESOURCE_DATA));

	auto pVertices = new PARTICLE_VERTEX[MAX_PARTICLES];
	if (!pVertices)
		return E_OUTOFMEMORY;



	//random number generator for color values
	srand(static_cast <unsigned> (time(NULL)));

	if (g_isFirst) {
		for (UINT i = 0; i < g_objects.size(); i++) {

			if (g_objects[i].m_red < 0) {
				g_objects[i].m_red = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
			}
			if (g_objects[i].m_green < 0) {
				g_objects[i].m_green = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
			}
			if (g_objects[i].m_blue < 0) {
				g_objects[i].m_blue = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
			}
			if (g_objects[i].m_brightness < 0) {
				g_objects[i].m_brightness = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
			}

			pVertices[i].Color = XMFLOAT4(g_objects[i].m_red, g_objects[i].m_green, g_objects[i].m_blue, g_objects[i].m_brightness);
		}
		g_isFirst = false;
	}
	else {
		for (UINT i = 0; i < g_objects.size(); i++) {
			pVertices[i].Color = XMFLOAT4(g_objects[i].m_red, g_objects[i].m_green, g_objects[i].m_blue, g_objects[i].m_brightness);
		}
	}




	vbInitData.pSysMem = pVertices;
	V_RETURN(pd3dDevice->CreateBuffer(&vbdesc, &vbInitData, &g_pParticleBuffer));
	DXUT_SetDebugName(g_pParticleBuffer, "Particles");
	SAFE_DELETE_ARRAY(pVertices);

	return hr;
}


//--------------------------------------------------------------------------------------
float RPercent()
{
	float ret = (float)((rand() % 10000) - 5000);
	return ret / 5000.0f;
}


//--------------------------------------------------------------------------------------
// This helper function loads a group of particles
//--------------------------------------------------------------------------------------
/*void LoadParticles( PARTICLE* pParticles,
XMFLOAT3 Center, XMFLOAT4 Velocity, float Spread, UINT NumParticles )
{
XMVECTOR vCenter = XMLoadFloat3( &Center );

for( UINT i = 0; i < NumParticles; i++ )
{
XMVECTOR vDelta = XMVectorReplicate( Spread );

while( XMVectorGetX( XMVector3LengthSq( vDelta ) ) > Spread * Spread )
{
vDelta = XMVectorSet( RPercent() * Spread, RPercent() * Spread, RPercent() * Spread, 0.f );
}

XMVECTOR vPos = XMVectorAdd( vCenter, vDelta );

XMStoreFloat3( reinterpret_cast<XMFLOAT3*>( &pParticles[i].pos ), vPos );
pParticles[i].pos.w = 10000.0f * 10000.0f;

pParticles[i].velo = Velocity;
}
}*/

//method for establishing the position of a celestial body
//definition for XMFLOAT4 is in the sample, line 570
XMFLOAT4 createPositionFloat(float xParticle, float yParticle, float zParticle){
	return XMFLOAT4(xParticle, yParticle, zParticle, 10000.0f * 10000.0f);
}

XMFLOAT4 createVelocityFloat(float xVelocity, float yVelocity, float zVelocity) {
	return XMFLOAT4(xVelocity, yVelocity, zVelocity, 1);
}

//method for performing vector subtraction when dealing with XMFLOAT4
//definition for XMFLOAT4 is in the sample, line 570
XMFLOAT4 VectorSubtraction(XMFLOAT4 vector1, XMFLOAT4 vector2)
{
	float xcoord = vector1.x - vector2.x;
	float ycoord = vector1.y - vector2.y;
	float zcoord = vector1.z - vector2.z;
	float wcoord = vector1.w - vector2.w;
	return XMFLOAT4(xcoord, ycoord, zcoord, wcoord);
}

//method for calculating vector addition, in XMFLOAT4 format
//will be necessary when adding invidividual accelerations
XMFLOAT4 VectorAddition(XMFLOAT4 vector1, XMFLOAT4 vector2)
{
	float xcoord = vector1.x + vector2.x;
	float ycoord = vector1.y + vector2.y;
	float zcoord = vector1.z + vector2.z;
	float wcoord = vector1.w + vector2.w;
	return XMFLOAT4(xcoord, ycoord, zcoord, wcoord);
}

//method for calculating the magnitude of an XMFLOAT4 vector
//we will only take into account the x, y, z coordinates necessary for calculating the position!!
float VectorMagnitude(XMFLOAT4 vector)
{
	float magnitude = sqrt(pow(vector.x, 2) + pow(vector.y, 2) + pow(vector.z, 2));
	return magnitude;
}

//method for multiplying an XMFLOAT4 vector with a constant
//will use this for calculating acceleration and making things move when implementing physical laws
XMFLOAT4 ConstantVectorMultiplication(float constant, XMFLOAT4 vector)
{
	float xcoord = constant * vector.x;
	float ycoord = constant * vector.y;
	float zcoord = constant * vector.z;
	float wcoord = constant * vector.w;
	return XMFLOAT4(xcoord, ycoord, zcoord, wcoord);
}


//--------------------------------------------------------------------------------------
// Functions used to load particles (overarching one is fillParticles) other ones support it
//--------------------------------------------------------------------------------------

void fillPosVel(PARTICLE particles[], std::vector<ObjectData> & objects){
	//method that loads g_pParticleArray with position as a vector and velocity (that is being updated constantly?)
	//create and then call here a getVelocity methods 

	int i = 0;
	for (auto object : objects)
	{
		particles[i].pos = createPositionFloat(object.m_xcoord, object.m_ycoord, object.m_zcoord);
		particles[i].velo = createVelocityFloat(object.m_xvelo, object.m_yvelo, object.m_zvelo);
		i++;
	}

	NUM_PARTICLES = i;
}



void fillDetails(PARTICLE_DETAILS particles2[], std::vector<ObjectData> & objects){

	int i = 0;
	for (auto object : objects) {
		particles2[i].name = object.m_name;
		particles2[i].mass = object.m_mass;
		particles2[i].diameter = object.m_diameter;
		particles2[i].brightness = object.m_brightness;
		particles2[i].red = object.m_red;
		particles2[i].green = object.m_green;
		particles2[i].blue = object.m_blue;
		i++;
	}
	assert(i == NUM_PARTICLES);
	g_loaded = true;
}

void fillParticles(PARTICLE particles[], PARTICLE_DETAILS particles2[], std::vector<ObjectData> & objects){
	fillPosVel(particles, objects);
	fillDetails(particles2, objects);

}

//--------------------------------------------------------------------------------------
// Functions that help display information to the user
//-


//Function that displays object information.Gets called when user presses Display Object Info button
// Currently displays to output window, later will display to pane on the right
void displayObjectInfo(){
	for (auto object : g_objects)
	{
		wchar_t buffer[256];
		swprintf(buffer, sizeof(buffer), L"Name: %s\n", object.m_name.c_str());
		::OutputDebugString(buffer);

		swprintf(buffer, sizeof(buffer), L"Mass: %f\n", object.m_mass);
		::OutputDebugString(buffer);

		swprintf(buffer, sizeof(buffer), L"Diameter: %f\n", object.m_diameter);
		::OutputDebugString(buffer);

		swprintf(buffer, sizeof(buffer), L"Brightness: %d\n", object.m_brightness);
		::OutputDebugString(buffer);

		swprintf(buffer, sizeof(buffer), L"Position: x: %f y: %f z: %f\n\n", object.m_xcoord, object.m_ycoord, object.m_zcoord);
		::OutputDebugString(buffer);
	}
}

//function that gets the current simulation time as a float and assigns it as a string to the string inputted as a parameter
void GetSimTime(WCHAR *currentTime){

	if (currentTime == NULL || g_systemTime == NULL){
		return;
	}

	HRESULT hr = StringCbPrintfW(currentTime, g_cTimeStringLength*sizeof(WCHAR), L"%f", g_systemTime);
}

//--------------------------------------------------------------------------------------
// Functions that allow user to change the speed of the simulation
//--------------------------------------------------------------------------------------

//doubles the value of g_timeValue so the simul speed goes 2x; called when user presses 2x button
void doubleSpeed(){
	g_timeValue = g_timeValue * 2;
}

//divides in half the value of g_timeValue so that simul speed slows down by half
//called when user presses 0.5x button
void halfSpeed(){
	g_timeValue = g_timeValue / 2;
}

//--------------------------------------------------------------------------------------
// Functions that allow user to jump in time in the simulation
//--------------------------------------------------------------------------------------


//this method calculates and updates new position and velocity for jumping in time
void jumpTime(float newTime){

	//reset all particles to their initial pos and velo
	for (int q = 0; q < NUM_PARTICLES; q++){
		g_pParticleArray[q].velo = createPositionFloat(g_objects[q].m_xvelo, g_objects[q].m_yvelo, g_objects[q].m_zvelo);
		g_pParticleArray[q].pos = createPositionFloat(g_objects[q].m_xcoord, g_objects[q].m_ycoord, g_objects[q].m_zcoord);
	}

	float timeIncrement = newTime / 2;

	for (int k = 0; k < newTime; k = k + timeIncrement){

		for (int i = 0; i < NUM_PARTICLES; i++)
		{
			////initial velocity for all particles
			//XMFLOAT4 initialVelo = XMFLOAT4(0, 0, 0, 1);

			////temp testing with different initial velo
			//if (g_pParticleArrayTWO[i].name.compare(L"Earth") == 0){
			//	initialVelo = XMFLOAT4(97480, 40178, .70917, 0);
			//}
			//if (g_pParticleArrayTWO[i].name.compare(L"Mars") == 0){
			//	initialVelo = XMFLOAT4(83338.28, -27184.865, -2615.148, 0);
			//}
			//if (g_pParticleArrayTWO[i].name.compare(L"Venus") == 0){
			//	initialVelo = XMFLOAT4(-91327.48, 86785.93, 6459.896, 0);
			//}
			//initial position of object i
			//XMFLOAT4 initialPositioni = createPositionFloat(g_objects[i].m_xcoord, g_objects[i].m_ycoord, g_objects[i].m_zcoord);

			// here I calculate acceleration for each object in particular
			//ind_acc = new XMFLOAT4[NUM_PARTICLES];
			XMFLOAT4 acceleration = XMFLOAT4(0, 0, 0, 0);

			for (int j = 0; j < NUM_PARTICLES; j++)
			{
				if (i != j)
				{
					XMFLOAT4 ijdist = VectorSubtraction(g_pParticleArray[i].pos, g_pParticleArray[j].pos);
					float ijdist_magnitude = VectorMagnitude(ijdist);

					float g_accConstant = g_constant * g_pParticleArrayTWO[j].mass / pow(ijdist_magnitude, 3);
					XMFLOAT4 g_acc = ConstantVectorMultiplication(g_accConstant, ijdist);
					acceleration = VectorAddition(acceleration, g_acc);

					//ind_acc[j] = g_acc;
				}
			}

			////update velocity and position using acceleration

			////calculates displacement between starting point and jumped time point
			//XMFLOAT4 displacement;
			////breaks x=vt+1/2at^2 into two parts
			//XMFLOAT4 vt = ConstantVectorMultiplication(newTime, initialVelo);
			//XMFLOAT4 atsquared = ConstantVectorMultiplication(0.5, ConstantVectorMultiplication(pow(newTime, 2), acceleration));
			//displacement = VectorAddition(vt, atsquared);

			////update the velocity and position of the particle
			//g_pParticleArray[i].velo = VectorAddition(initialVelo, ConstantVectorMultiplication(newTime, acceleration));
			//g_pParticleArray[i].pos = VectorAddition(initialPositioni, displacement);

			//update velocity and position using acceleration
			g_pParticleArray[i].velo = VectorAddition(g_pParticleArray[i].velo, ConstantVectorMultiplication(timeIncrement, acceleration));
			g_pParticleArray[i].pos = VectorAddition(g_pParticleArray[i].pos, ConstantVectorMultiplication(timeIncrement, g_pParticleArray[i].velo));

		}


	}
	g_systemTime = newTime;

}



//--------------------------------------------------------------------------------------
HRESULT CreateParticlePosVeloBuffers(ID3D11Device* pd3dDevice)
{
	HRESULT hr = S_OK;

	D3D11_BUFFER_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.ByteWidth = MAX_PARTICLES * sizeof(PARTICLE);
	desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	desc.StructureByteStride = sizeof(PARTICLE);
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	// Initialize the data in the buffers
	if (!g_pParticleArray)
	{
		g_pParticleArray = new PARTICLE[MAX_PARTICLES];
	}

	if (!g_pParticleArray)
		return E_OUTOFMEMORY;

	// Initialize the data in the buffers, 2nd array
	if (!g_pParticleArrayTWO)
	{
		g_pParticleArrayTWO = new PARTICLE_DETAILS[MAX_PARTICLES];
	}

	if (!g_pParticleArrayTWO)
		return E_OUTOFMEMORY;

	srand((unsigned int)GetTickCount64());

	// Disk Galaxy Formation
	//float fCenterSpread = g_fSpread * 0.50f;
	/*LoadParticles(g_pParticleArray,
	XMFLOAT3( fCenterSpread, 0, 0 ), XMFLOAT4( 0, 0, 0, 1 ),
	g_fSpread, NUM_PARTICLES );*/

	fillParticles(g_pParticleArray, g_pParticleArrayTWO, g_objects);

	//temp testing changing earth's inital velocity 
	for (int i = 0; i < NUM_PARTICLES; i++){
		if (g_pParticleArrayTWO[i].name.compare(L"Earth") == 0){
			g_pParticleArray[i].velo = XMFLOAT4(97480.40753, 40178.74, .709178, 0);
		}
		if (g_pParticleArrayTWO[i].name.compare(L"Mars") == 0){
			g_pParticleArray[i].velo = XMFLOAT4(83338.28, -27184.865, -2615.148, 0);
		}
		if (g_pParticleArrayTWO[i].name.compare(L"Venus") == 0){
			g_pParticleArray[i].velo = XMFLOAT4(-91327.48, 86785.93, 6459.896, 0);
		}
	}




	D3D11_SUBRESOURCE_DATA InitData;
	InitData.pSysMem = g_pParticleArray;
	V_RETURN(pd3dDevice->CreateBuffer(&desc, &InitData, &g_pParticlePosVelo0));
	V_RETURN(pd3dDevice->CreateBuffer(&desc, &InitData, &g_pParticlePosVelo1));
	DXUT_SetDebugName(g_pParticlePosVelo0, "PosVelo0");
	DXUT_SetDebugName(g_pParticlePosVelo1, "PosVelo1");

	D3D11_SHADER_RESOURCE_VIEW_DESC DescRV;
	ZeroMemory(&DescRV, sizeof(DescRV));
	DescRV.Format = DXGI_FORMAT_UNKNOWN;
	DescRV.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	DescRV.Buffer.FirstElement = 0;
	DescRV.Buffer.NumElements = desc.ByteWidth / desc.StructureByteStride;
	V_RETURN(pd3dDevice->CreateShaderResourceView(g_pParticlePosVelo0, &DescRV, &g_pParticlePosVeloRV0));
	V_RETURN(pd3dDevice->CreateShaderResourceView(g_pParticlePosVelo1, &DescRV, &g_pParticlePosVeloRV1));
	DXUT_SetDebugName(g_pParticlePosVeloRV0, "PosVelo0 SRV");
	DXUT_SetDebugName(g_pParticlePosVeloRV1, "PosVelo1 SRV");

	return hr;
}


//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings(DXUTDeviceSettings* pDeviceSettings, void* pUserContext)
{
	return true;
}

float distanceCalc(XMVECTOR center, XMVECTOR side) {
	float distance = 0;
	float centerX = XMVectorGetX(center);
	float sideX = XMVectorGetX(side);
	float centerY = XMVectorGetY(center);
	float sideY = XMVectorGetY(side);
	float centerZ = XMVectorGetZ(center);
	float sideZ = XMVectorGetZ(side);

	float xSquared = powf((sideX - centerX), 2); //***Overflow concerns?
	float ySquared = powf((sideY - centerY), 2);
	float zSquared = powf((sideZ - centerZ), 2);

	distance = sqrtf(xSquared + ySquared + zSquared);

	return distance;
}

float distanceCalc(float cx, float cy, float sx, float sy) {
	float distance = 0;

	float xSquared = powf((sx - cx), 2);
	float ySquared = powf((sy - cy), 2);

	distance = sqrtf(xSquared + ySquared);

	return distance;
}

//float distanceCalc(float cx, float cy, float sx, float sy, float cz) {
//	float distance = 0;
//
//	float xSquared = powf((sx - cx), 2);
//	float ySquared = powf((sy - cy), 2);
//	float zSquared = powf(cz, 2);
//
//	distance = sqrtf(xSquared + ySquared + zSquared);
//
//	return distance;
//}

void convertTo3x3(XMFLOAT4X4 matrix4x4) {
	g_pCBGS->m_InvView._43 = 0;
	g_pCBGS->m_InvView._14 = 0;
	g_pCBGS->m_InvView._24 = 0;
	g_pCBGS->m_InvView._34 = 0;
	g_pCBGS->m_InvView._44 = 1;
	g_pCBGS->m_InvView._42 = 0;
	g_pCBGS->m_InvView._41 = 0;
}

//--------------------------------------------------------------------------------------
// This callback function handles mouse related user input. 
//--------------------------------------------------------------------------------------
void CALLBACK OnMouseEvent(bool bLeftButtonDown, bool bRightButtonDown, bool bMiddleButtonDown,
	bool bSideButton1Down, bool bSideButton2Down, int nMouseWheelDelta,
	int xPos, int yPos, void* pUserContext)
{
	if (g_isPaused && g_hasDisplay && bLeftButtonDown) {
		g_xMouse = (float)xPos;
		g_yMouse = (float)yPos;
		g_relevantMouse = true;
	}
}

wstring concatenateObjInfo(int index) {
	wstring objectInfo(L"Name: " + g_pParticleArrayTWO[index].name + L"\nMass: " + to_wstring(g_pParticleArrayTWO[index].mass) + L"\nDiameter: " +
		to_wstring(g_pParticleArrayTWO[index].diameter) + L"\nBrightness: " + to_wstring(g_pParticleArrayTWO[index].brightness) +
		L"\nPosition:\nx: " + to_wstring(g_pParticleArray[index].pos.x) + L"\ny: " + to_wstring(g_pParticleArray[index].pos.y) +
		L"\nz: " + to_wstring(g_pParticleArray[index].pos.z));

	return objectInfo;
}

//--------------------------------------------------------------------------------------
// This callback function will be called once at the beginning of every frame. This is the
// best location for your application to handle updates to the scene, but is not 
// intended to contain actual rendering calls, which should instead be placed in the 
// OnFrameRender callback.  
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove(double fTime, float fElapsedTime, void* pUserContext)
{

	if (fElapsedTime < SECONDS_PER_FRAME)
	{
		Sleep(static_cast<DWORD>((SECONDS_PER_FRAME - fElapsedTime) * 1000.0f));
	}

	// Update the camera's position based on user input 
	g_Camera.FrameMove(fElapsedTime);

	g_mProj = g_Camera.GetProjMatrix();
	g_mView = g_Camera.GetViewMatrix();



	if (!g_isPaused)
	{
		auto pd3dImmediateContext = DXUTGetD3D11DeviceContext();

		D3D11_MAPPED_SUBRESOURCE ms;
		pd3dImmediateContext->Map(g_pParticlePosVelo0, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);

		g_systemTime = g_systemTime + g_timeValue;
		for (int i = 0; i < NUM_PARTICLES; i++)
		{

			// here I calculate acceleration for each object in particular
			//ind_acc = new XMFLOAT4[NUM_PARTICLES];
			XMFLOAT4 acceleration = XMFLOAT4(0, 0, 0, 0);

			for (int j = 0; j < NUM_PARTICLES; j++)
			{
				if (i != j)
				{
					XMFLOAT4 ijdist = VectorSubtraction(g_pParticleArray[i].pos, g_pParticleArray[j].pos);
					float ijdist_magnitude = VectorMagnitude(ijdist);

					float g_accConstant = g_constant * g_pParticleArrayTWO[j].mass / pow(ijdist_magnitude, 3);
					XMFLOAT4 g_acc = ConstantVectorMultiplication(g_accConstant, ijdist);
					acceleration = VectorAddition(acceleration, g_acc);

					//ind_acc[j] = g_acc;
				}
			}

			//update velocity and position using acceleration
			g_pParticleArray[i].velo = VectorAddition(g_pParticleArray[i].velo, ConstantVectorMultiplication(g_timeValue, acceleration));
			g_pParticleArray[i].pos = VectorAddition(g_pParticleArray[i].pos, ConstantVectorMultiplication(g_timeValue, g_pParticleArray[i].velo));
			//g_pParticleArray[i].pos.x -= 2.0f;
			//move each object's button


		}



		memcpy(ms.pData, g_pParticleArray, sizeof(PARTICLE) * NUM_PARTICLES);

		pd3dImmediateContext->Unmap(g_pParticlePosVelo0, NULL);

		std::swap(g_pParticlePosVelo0, g_pParticlePosVelo1);
		std::swap(g_pParticlePosVeloRV0, g_pParticlePosVeloRV1);
	}
	else if (g_isPaused && g_hasDisplay && g_relevantMouse) {

		float xScreenMouse;
		float yScreenMouse;
		float mouseRadius;
		XMVECTOR screenSphere;
		XMVECTOR screenObject;
		XMVECTOR worldSphere;
		float radius;
		float screenRadius;
		int foundIndex = -1;
		float zCoordHit = NULL;

		//mouse location in screen coordinates
		xScreenMouse = ((2 * g_xMouse) / (float)DXUTGetWindowWidth()) - 1;
		yScreenMouse = 1 - ((2 * g_yMouse) / (float)DXUTGetWindowHeight());

		for (int i = 0; i < NUM_PARTICLES; i++) {
			XMVECTOR worldObject = { g_pParticleArray[i].pos.x, g_pParticleArray[i].pos.y, g_pParticleArray[i].pos.z, 1.0f };

			//world view projection comes from RenderParticles
			screenObject = XMVector3TransformCoord(worldObject, XMLoadFloat4x4(&g_pCBGS->m_WorldViewProj));
			XMFLOAT4 screenCoord;
			XMStoreFloat4(&screenCoord, screenObject);

			//inverse view matrix comes from RenderParticles
			//screen space radius
			radius = 8000.0f; //TODO: Get this value from hlsl; hlsl value should in turn come from the diameter
			XMVECTOR offset = { radius, 0.0f, 0.0f, 0.0f };
			convertTo3x3(g_pCBGS->m_InvView);

			XMVECTOR worldOffset = XMVector3Transform(offset, XMLoadFloat4x4(&g_pCBGS->m_InvView));
			worldSphere = worldObject + worldOffset;
			screenSphere = XMVector3TransformCoord(worldSphere, XMLoadFloat4x4(&g_pCBGS->m_WorldViewProj));
			screenRadius = distanceCalc(screenObject, screenSphere);

			mouseRadius = distanceCalc(XMVectorGetX(screenObject), XMVectorGetY(screenObject), xScreenMouse, yScreenMouse);

			wchar_t buffer[256];
			swprintf(buffer, sizeof(buffer), L"screen: %f mouse: %f\n centerX: %f centerY %f \n mouseX: %f mouseY: %f\n\n", screenRadius, mouseRadius, XMVectorGetX(screenObject), XMVectorGetY(screenObject), xScreenMouse, yScreenMouse);
			::OutputDebugString(buffer);

			if (mouseRadius <= screenRadius) { //Checks if click is within radius

				if (foundIndex < 0) { //First hit
					zCoordHit = g_pParticleArray[i].pos.z;
					foundIndex = i;
				}
				else if (zCoordHit <= g_pParticleArray[i].pos.z) { //Additional hits. Compares z coordinates. 
					zCoordHit = g_pParticleArray[i].pos.z;
					foundIndex = i;
				}

			}
		}

		if (foundIndex < 0 && g_pTextBox != nullptr && g_hasDisplay) {
			g_pTextBox->ClearText();
			g_pTextBox->SetText(L"No object selected");
		}
		else if (g_pTextBox != nullptr && g_hasDisplay) {
			//TODO: text wrapping
			wstring objectInfo = concatenateObjInfo(foundIndex);
			g_pTextBox->ClearText();
			g_pTextBox->SetText(objectInfo.c_str());
		}
		foundIndex = -1;
	}


	g_relevantMouse = false;


}



//--------------------------------------------------------------------------------------
// Before handling window messages, DXUT passes incoming windows 
// messages to the application through this callback function. If the application sets 
// *pbNoFurtherProcessing to TRUE, then DXUT will not process this message.
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
	void* pUserContext)
{
	// Pass messages to dialog resource manager calls so GUI state is updated correctly
	*pbNoFurtherProcessing = g_DialogResourceManager.MsgProc(hWnd, uMsg, wParam, lParam);
	if (*pbNoFurtherProcessing)
		return 0;

	// Pass messages to settings dialog if its active
	if (g_D3DSettingsDlg.IsActive())
	{
		g_D3DSettingsDlg.MsgProc(hWnd, uMsg, wParam, lParam);
		return 0;
	}

	// Give the dialogs a chance to handle the message first
	*pbNoFurtherProcessing = g_HUD.MsgProc(hWnd, uMsg, wParam, lParam);
	if (*pbNoFurtherProcessing)
		return 0;
	*pbNoFurtherProcessing = g_SampleUI.MsgProc(hWnd, uMsg, wParam, lParam);
	if (*pbNoFurtherProcessing)
		return 0;

	// Pass all windows messages to camera so it can respond to user input
	g_Camera.HandleMessages(hWnd, uMsg, wParam, lParam);

	return 0;
}

void pauseControl() {
		if (g_hasDisplay) {
			g_pTextBox->SetVisible(false);
			g_hasDisplay = false;
		}
		if (!g_isPaused) {
			DXUTPause(false, false);
			g_isPaused = true;
		}
		else {
			DXUTPause(true, false);
			g_isPaused = false;
		}

		LPCWSTR welcomeMessage = L"Select an object\nto see information\ndisplayed\n(but actually press\nthe button)";
		if (g_isPaused && !g_hasDisplay && g_firstTextBox) { //always the first case; text box pointer gets assignment here
			g_HUD.AddEditBox(11, welcomeMessage, 0, 295, 160, 300);
			g_pTextBox = g_HUD.GetEditBox(11);
			g_hasDisplay = true;
			g_firstTextBox = false;
		}
		else if (g_isPaused && !g_hasDisplay)
		{
			g_pTextBox->SetText(welcomeMessage);
			g_pTextBox->SetVisible(true);
			g_hasDisplay = true;
		}
		else if (g_hasDisplay) {
			g_pTextBox->SetVisible(false);
			g_hasDisplay = false;
		}
}


//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent(UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext)
{
	switch (nControlID)
	{
	case IDC_TOGGLEFULLSCREEN:
		DXUTToggleFullScreen(); break;
	case IDC_TOGGLEREF:
	{
		DXUTPause(false, false);
		g_isPaused = false;
		DXUTToggleREF(); break;
	}
	case IDC_CHANGEDEVICE:
		g_D3DSettingsDlg.SetActive(!g_D3DSettingsDlg.IsActive()); break;

	case IDC_RESETPARTICLES:
	{
		SAFE_RELEASE(g_pParticlePosVelo0);
		SAFE_RELEASE(g_pParticlePosVelo1);
		SAFE_RELEASE(g_pParticlePosVeloRV0);
		SAFE_RELEASE(g_pParticlePosVeloRV1);
		CreateParticlePosVeloBuffers(DXUTGetD3D11Device());
		break;
	}
	case IDC_PAUSE:
	{
		pauseControl();
	}
	case IDC_DOUBLESPEED:
		doubleSpeed(); break;
	case IDC_HALFSPEED:
		halfSpeed(); break;
	case IDC_SUBMITTIMEIN:
	{
		LPCWSTR timeStr;
		float timeFloat;
		timeStr = g_JumpTimeInput->GetText();
		if (timeStr == NULL){
			break;
		}
		timeFloat = wcstof(timeStr, NULL);
		jumpTime(timeFloat); break;
	}
	case IDC_RESETCAMERA:
	{
		g_Camera.Reset();
		break;
	}

	}
}


//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D11DeviceAcceptable(const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo,
	DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext)
{
	// reject any device which doesn't support CS4x
	if (DeviceInfo->ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x == FALSE)
		return false;

	return true;
}


//--------------------------------------------------------------------------------------
// Create any D3D11 resources that aren't dependant on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11CreateDevice(ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
	void* pUserContext)
{
	HRESULT hr;

	static bool bFirstOnCreateDevice = true;

	// Warn the user that in order to support CS4x, a non-hardware device has been created, continue or quit?
	if (DXUTGetDeviceSettings().d3d11.DriverType != D3D_DRIVER_TYPE_HARDWARE && bFirstOnCreateDevice)
	{
		if (MessageBox(0, L"CS4x capability is missing. "\
			L"In order to continue, a non-hardware device has been created, "\
			L"it will be very slow, continue?", L"Warning", MB_ICONEXCLAMATION | MB_YESNO) != IDYES)
			return E_FAIL;
	}

	CWaitDlg CompilingShadersDlg;
	CompilingShadersDlg.ShowDialog(L"Compiling Shaders...");

	bFirstOnCreateDevice = false;

	D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS ho;
	V_RETURN(pd3dDevice->CheckFeatureSupport(D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, &ho, sizeof(ho)));

	auto pd3dImmediateContext = DXUTGetD3D11DeviceContext();
	V_RETURN(g_DialogResourceManager.OnD3D11CreateDevice(pd3dDevice, pd3dImmediateContext));
	V_RETURN(g_D3DSettingsDlg.OnD3D11CreateDevice(pd3dDevice));
	g_pTxtHelper = new CDXUTTextHelper(pd3dDevice, pd3dImmediateContext, &g_DialogResourceManager, 15);

	ID3DBlob* pBlobRenderParticlesVS = nullptr;
	ID3DBlob* pBlobRenderParticlesGS = nullptr;
	ID3DBlob* pBlobRenderParticlesPS = nullptr;

	// Create the shaders
	V_RETURN(DXUTCompileFromFile(L"GalaxyDraw.hlsl", nullptr, "VSGalaxyDraw", "vs_4_0",
		D3DCOMPILE_ENABLE_STRICTNESS, 0, &pBlobRenderParticlesVS));
	V_RETURN(DXUTCompileFromFile(L"GalaxyDraw.hlsl", nullptr, "GSGalaxyDraw", "gs_4_0",
		D3DCOMPILE_ENABLE_STRICTNESS, 0, &pBlobRenderParticlesGS));
	V_RETURN(DXUTCompileFromFile(L"GalaxyDraw.hlsl", nullptr, "PSGalaxyDraw", "ps_4_0",
		D3DCOMPILE_ENABLE_STRICTNESS, 0, &pBlobRenderParticlesPS));

	V_RETURN(pd3dDevice->CreateVertexShader(pBlobRenderParticlesVS->GetBufferPointer(), pBlobRenderParticlesVS->GetBufferSize(), nullptr, &g_pRenderParticlesVS));
	DXUT_SetDebugName(g_pRenderParticlesVS, "VSGalaxyDraw");

	V_RETURN(pd3dDevice->CreateGeometryShader(pBlobRenderParticlesGS->GetBufferPointer(), pBlobRenderParticlesGS->GetBufferSize(), nullptr, &g_pRenderParticlesGS));
	DXUT_SetDebugName(g_pRenderParticlesGS, "GSGalaxyDraw");

	V_RETURN(pd3dDevice->CreatePixelShader(pBlobRenderParticlesPS->GetBufferPointer(), pBlobRenderParticlesPS->GetBufferSize(), nullptr, &g_pRenderParticlesPS));
	DXUT_SetDebugName(g_pRenderParticlesPS, "PSGalaxyDraw");


	// Create our vertex input layout
	const D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	V_RETURN(pd3dDevice->CreateInputLayout(layout, sizeof(layout) / sizeof(layout[0]),
		pBlobRenderParticlesVS->GetBufferPointer(), pBlobRenderParticlesVS->GetBufferSize(), &g_pParticleVertexLayout));
	DXUT_SetDebugName(g_pParticleVertexLayout, "Particles");

	SAFE_RELEASE(pBlobRenderParticlesVS);
	SAFE_RELEASE(pBlobRenderParticlesGS);
	SAFE_RELEASE(pBlobRenderParticlesPS);

	V_RETURN(CreateParticleBuffer(pd3dDevice));
	V_RETURN(CreateParticlePosVeloBuffers(pd3dDevice));

	// Setup constant buffer
	D3D11_BUFFER_DESC Desc;
	Desc.Usage = D3D11_USAGE_DYNAMIC;
	Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Desc.MiscFlags = 0;
	Desc.ByteWidth = sizeof(CB_GS);
	V_RETURN(pd3dDevice->CreateBuffer(&Desc, nullptr, &g_pcbGS));
	DXUT_SetDebugName(g_pcbGS, "CB_GS");

	Desc.ByteWidth = sizeof(CB_CS);
	V_RETURN(pd3dDevice->CreateBuffer(&Desc, nullptr, &g_pcbCS));
	DXUT_SetDebugName(g_pcbCS, "CB_CS");

	// Load the Particle Texture
	V_RETURN(DXUTCreateShaderResourceViewFromFile(pd3dDevice, L"misc\\Particle.dds", &g_pParticleTexRV));

	D3D11_SAMPLER_DESC SamplerDesc;
	ZeroMemory(&SamplerDesc, sizeof(SamplerDesc));
	SamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	V_RETURN(pd3dDevice->CreateSamplerState(&SamplerDesc, &g_pSampleStateLinear));
	DXUT_SetDebugName(g_pSampleStateLinear, "Linear");

	D3D11_BLEND_DESC BlendStateDesc;
	ZeroMemory(&BlendStateDesc, sizeof(BlendStateDesc));
	BlendStateDesc.RenderTarget[0].BlendEnable = TRUE;
	BlendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	BlendStateDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	BlendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	BlendStateDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	BlendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
	BlendStateDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	BlendStateDesc.RenderTarget[0].RenderTargetWriteMask = 0x0F;
	V_RETURN(pd3dDevice->CreateBlendState(&BlendStateDesc, &g_pBlendingStateParticle));
	DXUT_SetDebugName(g_pBlendingStateParticle, "Blending");

	D3D11_DEPTH_STENCIL_DESC DepthStencilDesc;
	ZeroMemory(&DepthStencilDesc, sizeof(DepthStencilDesc));
	DepthStencilDesc.DepthEnable = FALSE;
	DepthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	pd3dDevice->CreateDepthStencilState(&DepthStencilDesc, &g_pDepthStencilState);
	DXUT_SetDebugName(g_pDepthStencilState, "DepthOff");

	// Setup the camera's view parameters

	XMVECTOR vecEye = XMVectorSet(0, 0, -g_fSpread * 1000, 0.f);
	g_Camera.SetViewParams(vecEye, g_XMZero);

	CompilingShadersDlg.DestroyDialog();

	return S_OK;
}


//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11ResizedSwapChain(ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
	const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext)
{
	HRESULT hr;

	V_RETURN(g_DialogResourceManager.OnD3D11ResizedSwapChain(pd3dDevice, pBackBufferSurfaceDesc));
	V_RETURN(g_D3DSettingsDlg.OnD3D11ResizedSwapChain(pd3dDevice, pBackBufferSurfaceDesc));

	// Setup the camera's projection parameters
	float fAspectRatio = pBackBufferSurfaceDesc->Width / (FLOAT)pBackBufferSurfaceDesc->Height;
	g_Camera.SetProjParams(XM_PI / 4, fAspectRatio, 10.0f, 500000.0f);
	g_Camera.SetWindow(pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height);
	g_Camera.SetButtonMasks(0, MOUSE_WHEEL, MOUSE_LEFT_BUTTON | MOUSE_MIDDLE_BUTTON | MOUSE_RIGHT_BUTTON);

	g_HUD.SetLocation(pBackBufferSurfaceDesc->Width - 170, 0);
	g_HUD.SetSize(170, 170);
	g_SampleUI.SetLocation(pBackBufferSurfaceDesc->Width - 170, pBackBufferSurfaceDesc->Height - 300);
	g_SampleUI.SetSize(170, 300);

	return hr;
}


//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11ReleasingSwapChain(void* pUserContext)
{
	g_DialogResourceManager.OnD3D11ReleasingSwapChain();
}


//--------------------------------------------------------------------------------------
void RenderText()
{
	g_pTxtHelper->Begin();
	g_pTxtHelper->SetInsertionPos(2, 0);
	g_pTxtHelper->SetForegroundColor(Colors::Yellow);
	g_pTxtHelper->DrawTextLine(DXUTGetFrameStats(DXUTIsVsyncEnabled()));
	g_pTxtHelper->DrawTextLine(DXUTGetDeviceStats());
	g_pTxtHelper->DrawTextLine(L"Time:");
	WCHAR currentTime[g_cTimeStringLength];
	GetSimTime(currentTime);
	g_pTxtHelper->DrawTextLine(currentTime);
	g_pTxtHelper->End();
}


//--------------------------------------------------------------------------------------
bool RenderParticles(ID3D11DeviceContext* pd3dImmediateContext, CXMMATRIX mView, CXMMATRIX mProj)
{
	ID3D11BlendState *pBlendState0 = nullptr;
	ID3D11DepthStencilState *pDepthStencilState0 = nullptr;
	UINT SampleMask0, StencilRef0;
	XMFLOAT4 BlendFactor0;
	pd3dImmediateContext->OMGetBlendState(&pBlendState0, &BlendFactor0.x, &SampleMask0);
	pd3dImmediateContext->OMGetDepthStencilState(&pDepthStencilState0, &StencilRef0);

	pd3dImmediateContext->VSSetShader(g_pRenderParticlesVS, nullptr, 0);
	pd3dImmediateContext->GSSetShader(g_pRenderParticlesGS, nullptr, 0);
	pd3dImmediateContext->PSSetShader(g_pRenderParticlesPS, nullptr, 0);

	pd3dImmediateContext->IASetInputLayout(g_pParticleVertexLayout);

	// Set IA parameters
	ID3D11Buffer* pBuffers[1] = { g_pParticleBuffer };
	UINT stride[1] = { sizeof(PARTICLE_VERTEX) };
	UINT offset[1] = { 0 };
	pd3dImmediateContext->IASetVertexBuffers(0, 1, pBuffers, stride, offset);
	pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

    // Use the back buffer of the 2 particle PosVelo buffers
	ID3D11ShaderResourceView* aRViews[1] = { g_pParticlePosVeloRV1 };
	pd3dImmediateContext->VSSetShaderResources(0, 1, aRViews);

	D3D11_MAPPED_SUBRESOURCE MappedResource;
	pd3dImmediateContext->Map(g_pcbGS, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
	g_pCBGS = reinterpret_cast<CB_GS*>(MappedResource.pData);
	XMStoreFloat4x4(&g_pCBGS->m_WorldViewProj, XMMatrixMultiply(mView, mProj));
	XMStoreFloat4x4(&g_pCBGS->m_InvView, XMMatrixInverse(nullptr, mView));
	pd3dImmediateContext->Unmap(g_pcbGS, 0);
	pd3dImmediateContext->GSSetConstantBuffers(0, 1, &g_pcbGS);

	pd3dImmediateContext->PSSetShaderResources(0, 1, &g_pParticleTexRV);
	pd3dImmediateContext->PSSetSamplers(0, 1, &g_pSampleStateLinear);

	float bf[] = { 0.f, 0.f, 0.f, 0.f };
	pd3dImmediateContext->OMSetBlendState(g_pBlendingStateParticle, bf, 0xFFFFFFFF);
	pd3dImmediateContext->OMSetDepthStencilState(g_pDepthStencilState, 0);

	pd3dImmediateContext->Draw(NUM_PARTICLES, 0);

	ID3D11ShaderResourceView* ppSRVNULL[1] = { nullptr };
	pd3dImmediateContext->VSSetShaderResources(0, 1, ppSRVNULL);
	pd3dImmediateContext->PSSetShaderResources(0, 1, ppSRVNULL);

	/*ID3D11Buffer* ppBufNULL[1] = { nullptr };
	pd3dImmediateContext->GSSetConstantBuffers( 0, 1, ppBufNULL );*/

	pd3dImmediateContext->GSSetShader(nullptr, nullptr, 0);
	pd3dImmediateContext->OMSetBlendState(pBlendState0, &BlendFactor0.x, SampleMask0); SAFE_RELEASE(pBlendState0);
	pd3dImmediateContext->OMSetDepthStencilState(pDepthStencilState0, StencilRef0); SAFE_RELEASE(pDepthStencilState0);

	return true;
}


//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime,
	float fElapsedTime, void* pUserContext)
{
	// If the settings dialog is being shown, then render it instead of rendering the app's scene
	if (g_D3DSettingsDlg.IsActive())
	{
		g_D3DSettingsDlg.OnRender(fElapsedTime);
		return;
	}

	ID3D11RenderTargetView* pRTV = DXUTGetD3D11RenderTargetView();
	pd3dImmediateContext->ClearRenderTargetView(pRTV, Colors::Black);
	ID3D11DepthStencilView* pDSV = DXUTGetD3D11DepthStencilView();
	pd3dImmediateContext->ClearDepthStencilView(pDSV, D3D11_CLEAR_DEPTH, 1.0, 0);

	// Get the projection & view matrix from the camera class -> now defined in OnFrameMove


	// Render the particles
	RenderParticles(pd3dImmediateContext, g_mView, g_mProj);


	DXUT_BeginPerfEvent(DXUT_PERFEVENTCOLOR, L"HUD / Stats");
	g_HUD.OnRender(fElapsedTime);
	g_SampleUI.OnRender(fElapsedTime);
	RenderText();
	DXUT_EndPerfEvent();

	// The following could be used to output fps stats into debug output window,
	// which is useful because you can then turn off all UI rendering as they cloud performance
	/*static DWORD dwTimefirst = GetTickCount();
	if ( GetTickCount() - dwTimefirst > 5000 )
	{
	OutputDebugString( DXUTGetFrameStats( DXUTIsVsyncEnabled() ) );
	OutputDebugString( L"\n" );
	dwTimefirst = GetTickCount();
	}*/
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice(void* pUserContext)
{
	g_DialogResourceManager.OnD3D11DestroyDevice();
	g_D3DSettingsDlg.OnD3D11DestroyDevice();
	DXUTGetGlobalResourceCache().OnDestroyDevice();
	SAFE_DELETE(g_pTxtHelper);
	SAFE_DELETE_ARRAY(g_pParticleArray);
	SAFE_DELETE_ARRAY(g_pParticleArrayTWO);

	SAFE_RELEASE(g_pParticleBuffer);
	SAFE_RELEASE(g_pParticleVertexLayout);

	SAFE_RELEASE(g_pParticlePosVelo0);
	SAFE_RELEASE(g_pParticlePosVelo1);
	SAFE_RELEASE(g_pParticlePosVeloRV0);
	SAFE_RELEASE(g_pParticlePosVeloRV1);

	SAFE_RELEASE(g_pcbGS);
	SAFE_RELEASE(g_pcbCS);

	SAFE_RELEASE(g_pParticleTexRV);

	SAFE_RELEASE(g_pRenderParticlesVS);
	SAFE_RELEASE(g_pRenderParticlesGS);
	SAFE_RELEASE(g_pRenderParticlesPS);
	SAFE_RELEASE(g_pSampleStateLinear);
	SAFE_RELEASE(g_pBlendingStateParticle);
	SAFE_RELEASE(g_pDepthStencilState);
}


