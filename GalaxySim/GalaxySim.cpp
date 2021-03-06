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
#include <fstream>
#include <iomanip>
#include <windows.h>
#include <Lmcons.h>
#include "atlbase.h"
#include "atlstr.h"
#include "comutil.h"
#include <vector>

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

CDXUTEditBox						*g_JumpTimeInputBox = nullptr;
CDXUTEditBox						*g_IterationsPerFrameInBox = nullptr;
CDXUTEditBox						*g_DeleteObjInBox = nullptr;

CDXUTTimer							g_timer;

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

	ObjectData(const wstring & name, float mass, float diameter, float brightness, float x, float y, float z, float xv, float yv, float zv, float r, float g, float b) :
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
std::vector<ObjectData> g_knownValues50; //vector that will contain known solar system database values for time=50 
std::vector<ObjectData> g_knownValues365; //vector that will contain known solar system database values for time=365
std::vector<ObjectData> g_knownValues730; //vector that will contain known solar system database values for time=730 
std::vector<double> g_timeTestResults; //stores individual time test results so they can be totalled later

//const float g_constant = -8.644 * pow(10, -13);
//const float g_constant = -8.644E-16;
const float g_constant = -8.644E-16;
const int g_cFloatStringLength = 20;
const int g_cIntStringLength = 20;

float g_red[MAX_PARTICLES];
float g_green[MAX_PARTICLES];
float g_blue[MAX_PARTICLES];

//object data display variables
bool g_isFirst = true;
bool g_isPaused = false;
bool g_hasDisplay = false;
CDXUTEditBox *g_pObjectDataDisplay;
bool g_firstTextBox = true;
XMMATRIX g_mProj;
XMMATRIX g_mView;

CB_GS * g_pCBGS;

int g_height = 600;
int g_width = 800;

bool g_relevantMouse = false;
float g_xMouse;
float g_yMouse;

//variables for object adding UI
CDXUTEditBox *g_pNameBox = nullptr;
CDXUTEditBox *g_pMassBox = nullptr;
CDXUTEditBox *g_pDiameterBox = nullptr;
CDXUTEditBox *g_pBrightnessBox = nullptr;
CDXUTEditBox *g_pXPosBox = nullptr;
CDXUTEditBox *g_pYPosBox = nullptr;
CDXUTEditBox *g_pZPosBox = nullptr;
CDXUTEditBox *g_pXVelBox = nullptr;
CDXUTEditBox *g_pYVelBox = nullptr;
CDXUTEditBox *g_pZVelBox = nullptr;
CDXUTEditBox *g_pRedBox = nullptr;
CDXUTEditBox *g_pGreenBox = nullptr;
CDXUTEditBox *g_pBlueBox = nullptr;
CDXUTButton *g_pSubmitObjectButton = nullptr;
bool g_firstObjectAddition = true;
bool g_addingObject = false;



bool g_loaded = false;

double g_timeValue = 0.001; //can change this to change speed of simulation, used later to do 2x and 0.5x
double g_timeValueToHoursConversion = g_timeValue * 3800; //number of hours represented by the value of timeValue
int g_iterationsPerFrame = 10;
double g_systemTime = 0; //sets the inital system time to 0
LPWSTR g_timeString; //used later for the Jump Time In button user uses to input time to jump to.

//testing constants
bool g_isTest; //true means test mode is on
int g_step = 1; //determines which test from automated test suite is run
double g_jumpSpeedTest; //collects speed of jumpTime for automated test
double g_oneFrameTime; //collects time for one frame
double g_elapsedTimeAt1000Days;
wofstream g_dataFile;
const LPCWSTR g_localFileName = L"SkyXTelemetryData.csv";
const LPCWSTR g_currentDxDiagName = L"dxdiag.txt";
double g_elapsedTimeAt365Days;
double g_deltatResetParticles;
double g_deltatResetCamera;
double g_totalTime;

//testing variable helpers
double g_beginStartTime;
double g_endStartTime;

double g_hitTestStart;

int g_averageFPSCounter = 0;
int g_frameCounter = 0;

//printed testing variables
double g_startUpTime;
double g_pauseTime;
double g_unPauseTime;
double g_pauseFullScreenTime;
double g_unPauseFullScreenTime;
double g_winToFullTime;
double g_fullToWinTime;
double g_hitTestTime = 0; // will need to be an average, or there will be a lot of these
int g_numHitTests = 0;
float g_averageFPS = 0;

//-------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
#define IDC_TOGGLEFULLSCREEN    1
#define IDC_TOGGLEREF           3
#define IDC_CHANGEDEVICE        4
#define IDC_RESETPARTICLES      5
#define IDC_DISPLAYINFO			6
#define IDC_PAUSE               7
#define IDC_JUMPTIMEIN			10
#define IDC_SUBMITTIMEIN		11
#define IDC_RESETCAMERA			12
#define IDC_ITERATEPERFRAMEIN   13
#define	IDC_SUBMITITERATEIN     14

#define IDC_ADDOBJECT			15

#define IDC_SUBMITOBJECT		16
#define IDC_NAME				17
#define IDC_MASS				18
#define IDC_DIAMETER			19
#define IDC_BRIGHTNESS			20
#define IDC_XPOS				21
#define IDC_YPOS				22
#define IDC_ZPOS				23
#define IDC_XVEL				24
#define IDC_YVEL				25
#define IDC_ZVEL				26
#define IDC_RED					27
#define IDC_GREEN				28
#define IDC_BLUE				29
#define IDC_OUTPUTINFO          30
#define IDC_DELETEOBJIN			31
#define IDC_SUBMITDELETEOBJ		32
#define IDC_DISPLAYINFOBOX      33



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
LPWSTR GetSimTime();
void jumpTime(float newTime);
void GravityMotionIteration(float timeIncrement);
void pauseControl();


//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	g_beginStartTime = g_timer.GetAbsoluteTime();
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


	g_endStartTime = g_timer.GetAbsoluteTime();
	g_startUpTime = g_endStartTime - g_beginStartTime;

	
	HWND hwnd = FindWindow(NULL, TEXT("SkyX"));

	int selection = MessageBox(hwnd, L"Would you like to run SkyX in telemetry mode?", L"Start Up Options", MB_YESNOCANCEL);
	if (selection == IDYES) {
		g_isTest = true;
	}
	else if (selection == IDNO) {
		g_isTest = false;
	}
	else {
		exit(0);
	}

	DXUTMainLoop(); // Enter into the DXUT render loop

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

	g_HUD.SetCallback(OnGUIEvent); int iY = 10; int iYEnd = 575;
	g_HUD.AddButton(IDC_TOGGLEFULLSCREEN, L"Full screen (F6)", 0, iY, 170, 23, VK_F6);
	g_HUD.AddButton(IDC_TOGGLEREF, L"Toggle REF (F3)", 0, iY += 26, 170, 23, VK_F3);
	g_HUD.AddButton(IDC_CHANGEDEVICE, L"Change device (F2)", 0, iY += 26, 170, 23, VK_F2);
	g_HUD.AddButton(IDC_RESETPARTICLES, L"Reset particles (F4)", 0, iY += 26, 170, 22, VK_F4);
	g_HUD.AddButton(IDC_RESETCAMERA, L"Reset Camera Position", 0, iY += 26, 170, 23);
	g_HUD.AddEditBox(IDC_ITERATEPERFRAMEIN, L"", 0, iY += 26, 170, 40, false, &g_IterationsPerFrameInBox);
	g_HUD.AddButton(IDC_SUBMITITERATEIN, L"Enter Iterations/Frame", 0, iY += 40, 170, 23);
	g_HUD.AddEditBox(IDC_JUMPTIMEIN, L"", 0, iY += 26, 170, 40, false, &g_JumpTimeInputBox);
	g_HUD.AddButton(IDC_SUBMITTIMEIN, L"Jump!", 0, iY += 40, 170, 23);
	g_HUD.AddButton(IDC_PAUSE, L"Pause / Unpause", 0, iY += 26, 170, 22);
	g_HUD.AddButton(IDC_ADDOBJECT, L"Add Object", 0, iY += 26, 170, 22);
	g_HUD.AddButton(IDC_SUBMITDELETEOBJ, L"Delete Object", -180, iYEnd, 170, 22);
	g_HUD.AddEditBox(IDC_DELETEOBJIN, L"", -180, iYEnd -= 40, 170, 40, false, &g_DeleteObjInBox);
	g_HUD.AddButton(IDC_OUTPUTINFO, L"Output Object Data", -180, iYEnd -= 26, 170, 22);
	g_HUD.AddEditBox(IDC_DISPLAYINFOBOX, L"", 0, 325, 160, 270, false, &g_pObjectDataDisplay);
	//g_pObjectDataDisplay->SetVisible(false);

	//left hand UI
	int yPos = 100;
	int xPos = -630;
	int width = 170;
	int height = 35;

	g_HUD.AddEditBox(IDC_NAME, L"", xPos, yPos, width, height, false, &g_pNameBox);
	g_pNameBox->SetVisible(false);

	g_HUD.AddEditBox(IDC_MASS, L"", xPos, yPos += height, width, height, false, &g_pMassBox);
	g_pMassBox->SetVisible(false);

	g_HUD.AddEditBox(IDC_DIAMETER, L"", xPos, yPos += height, width, height, false, &g_pDiameterBox);
	g_pDiameterBox->SetVisible(false);

	g_HUD.AddEditBox(IDC_BRIGHTNESS, L"", xPos, yPos += height, width, height, false, &g_pBrightnessBox);
	g_pBrightnessBox->SetVisible(false);

	g_HUD.AddEditBox(IDC_XPOS, L"", xPos, yPos += height, width, height, false, &g_pXPosBox);
	g_pXPosBox->SetVisible(false);

	g_HUD.AddEditBox(IDC_YPOS, L"", xPos, yPos += height, width, height, false, &g_pYPosBox);
	g_pYPosBox->SetVisible(false);

	g_HUD.AddEditBox(IDC_ZPOS, L"", xPos, yPos += height, width, height, false, &g_pZPosBox);
	g_pZPosBox->SetVisible(false);

	g_HUD.AddEditBox(IDC_XVEL, L"", xPos, yPos += height, width, height, false, &g_pXVelBox);
	g_pXVelBox->SetVisible(false);

	g_HUD.AddEditBox(IDC_YVEL, L"", xPos, yPos += height, width, height, false, &g_pYVelBox);
	g_pYVelBox->SetVisible(false);

	g_HUD.AddEditBox(IDC_ZVEL, L"", xPos, yPos += height, width, height, false, &g_pZVelBox);
	g_pZVelBox->SetVisible(false);

	g_HUD.AddEditBox(IDC_RED, L"", xPos, yPos += height, width, height, false, &g_pRedBox);
	g_pRedBox->SetVisible(false);

	g_HUD.AddEditBox(IDC_GREEN, L"", xPos, yPos += height, width, height, false, &g_pGreenBox);
	g_pGreenBox->SetVisible(false);

	g_HUD.AddEditBox(IDC_BLUE, L"", xPos, yPos += height, width, height, false, &g_pBlueBox);
	g_pBlueBox->SetVisible(false);

	g_HUD.AddButton(IDC_SUBMITOBJECT, L"Create", xPos, yPos += height, width, height, NULL, true, &g_pSubmitObjectButton);
	g_pSubmitObjectButton->SetVisible(false);
	

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

PARTICLE_VERTEX* updateVertices() {
	
	auto pTempVertices = new PARTICLE_VERTEX[MAX_PARTICLES];
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

			pTempVertices[i].Color = XMFLOAT4(g_objects[i].m_red, g_objects[i].m_green, g_objects[i].m_blue, g_objects[i].m_brightness);
		}
		g_isFirst = false;
	}
	else {
		for (UINT i = 0; i < g_objects.size(); i++) {
			pTempVertices[i].Color = XMFLOAT4(g_objects[i].m_red, g_objects[i].m_green, g_objects[i].m_blue, g_objects[i].m_brightness);
		}
	}

	return pTempVertices;
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

	auto pVertices = updateVertices();
	if (!pVertices)
		return E_OUTOFMEMORY;







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


//Function that displays object information to an output window.
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

//--------------------------------------------------------------------------------------
// Functions that help with type conversions
//--------------------------------------------------------------------------------------

//input a WCHAR string and a float, assigns value of float to the WCHAR string
//if the conversion fails in some way, the string is set to NULL
void GetWCharFromFloat(WCHAR *string, float inputFloat){
	if (string == NULL){
		return;
	}

	HRESULT hr = StringCbPrintfW(string, g_cFloatStringLength*sizeof(WCHAR), L"%f", inputFloat);

	if (hr != S_OK){
		string = NULL;
	}
}

//input a WCHAR string and an int, assigns value of int to the WCHAR string
//if the conversion fails in some way, the string is set to NULL
void GetWCharFromInt(WCHAR *string, int inputInt){
	if (string == NULL){
		return;
	}

	HRESULT hr = StringCbPrintfW(string, g_cIntStringLength*sizeof(WCHAR), L"%i", inputInt);

	if (hr != S_OK){
		string = NULL;
	}
}

//--------------------------------------------------------------------------------------
// Functions related to motion of objects according to physical laws
//--------------------------------------------------------------------------------------

//takes an increment of time in system time. This time is later converted to real time
//moves the objects by timeIncrement according to gravitational physical laws
void GravityMotionIteration(float timeIncrement){
	for (int i = 0; i < NUM_PARTICLES; i++)
	{

		//calculates acceleration for each object in particular
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
			}
		}

		//update velocity and position using acceleration
		g_pParticleArray[i].velo = VectorAddition(g_pParticleArray[i].velo, ConstantVectorMultiplication(timeIncrement, acceleration));
		g_pParticleArray[i].pos = VectorAddition(g_pParticleArray[i].pos, ConstantVectorMultiplication(timeIncrement, g_pParticleArray[i].velo));
	}
}

//--------------------------------------------------------------------------------------
// Functions that allow user to jump in time in the simulation
//--------------------------------------------------------------------------------------

//this function takes real time in days, within the function it converts it to hours and later iterates based on the systemTime increment units
//this method calculates and updates new position and velocity for jumping in time
void jumpTime(float newTime){

	double jumpTimeStart = g_timer.GetAbsoluteTime();

	//convert back to hours
	newTime = newTime * 24;

	//iterate through time by increments of time Value


	/*by having it go forward from the current time for moving forward instead of resetting to 0 first like it
	does with moving backward, we save time in some cases. This could also work with everything resetting to zero and then building 
	from there like it does inside the else if (newTime< g_systemTime) block. */
	//move forward to a time
	if (newTime > g_systemTime){
		for (float k = g_systemTime; k < newTime; k = k + g_timeValueToHoursConversion){

			GravityMotionIteration(g_timeValue);

		}

	}
	//move backward to a time
	else if (newTime < g_systemTime){

		OnGUIEvent(0, IDC_RESETPARTICLES, NULL, NULL);

		for (float k = g_systemTime; k < newTime; k = k + g_timeValueToHoursConversion){

			GravityMotionIteration(g_timeValue);

		}

	}

	//uncomment to test where the particle is when you jump to a time
	//put a breakpoint at float acoord=1.0 and see values of x, y, and zcoord as you iterate through all of the objects in the debugger
	/*for (int i = 0; i < NUM_PARTICLES; i++){
		wstring name = g_pParticleArrayTWO[i].name;
		float xcoord = g_pParticleArray[i].pos.x;
		float ycoord = g_pParticleArray[i].pos.y;
		float zcoord = g_pParticleArray[i].pos.z;
		float xvelo = g_pParticleArray[i].velo.x;
		float yvelo = g_pParticleArray[i].velo.y;
		float zvelo = g_pParticleArray[i].velo.z;
		float acoord = 1.0;
	}*/

	g_systemTime = newTime;

	g_jumpSpeedTest = g_timer.GetAbsoluteTime() - jumpTimeStart;

}

//--------------------------------------------------------------------------------------
// Functions that help with testing and automated telemetry
//--------------------------------------------------------------------------------------



//fills arrays with hardcoded real values from NASA JPL Database 
//position in km, velocity in km/hr
//depending on the input parameter timeInDays it can input time=50, 365, or 730 values
void loadKnownValues(float timeInDays){
	if (timeInDays == 50){
		for (int i = 0; i < 9; i++){
			if (i == 0){
				ObjectData object;
				object.m_name = L"Sun";
				object.m_xcoord = 0;
				object.m_ycoord = 0;
				object.m_zcoord = 0;
				object.m_xvelo = 0;
				object.m_yvelo = 0;
				object.m_zvelo = 0;
				g_knownValues50.push_back(object);
			}
			if (i == 1){
				ObjectData object;
				object.m_name = L"Mercury";
				object.m_xcoord = -41474483.07;
				object.m_ycoord = -54103558.57;
				object.m_zcoord = -615489.7885;
				object.m_xvelo = 103666.4762;
				object.m_yvelo = -98496.68804;
				object.m_zvelo = -17559.11238;
				g_knownValues50.push_back(object);
			}
			if (i == 2){
				ObjectData object;
				object.m_name = L"Venus";
				object.m_xcoord = -64449331.34;
				object.m_ycoord = 85873531.11;
				object.m_zcoord = 4896238.182;
				object.m_xvelo = -101273.7503;
				object.m_yvelo = -76361.15814;
				object.m_zvelo = 4797.897862;
				g_knownValues50.push_back(object);
			}
			if (i == 3){
				ObjectData object;
				object.m_name = L"Earth";
				object.m_xcoord = 142026764.7;
				object.m_ycoord = -51038184.77;
				object.m_zcoord = 949.4655597;
				object.m_xvelo = 34483.13481;
				object.m_yvelo = 100524.2038;
				object.m_zvelo = -5.366305188;
				g_knownValues50.push_back(object);
			}
			if (i == 4){
				ObjectData object;
				object.m_name = L"Mars";
				object.m_xcoord = 16377975.27;
				object.m_ycoord = -215684754.5;
				object.m_zcoord = -4921153.108;
				object.m_xvelo = 90280.36324;
				object.m_yvelo = 14107.55284;
				object.m_zvelo = -1920.324435;
				g_knownValues50.push_back(object);
			}
			if (i == 5){
				ObjectData object;
				object.m_name = L"Jupiter";
				object.m_xcoord = -452147786;
				object.m_ycoord = 647743728;
				object.m_zcoord = 7427418.211;
				object.m_xvelo = -39170.87777;
				object.m_yvelo = -24722.02667;
				object.m_zvelo = 979.2257568;
				g_knownValues50.push_back(object);
			}
			if (i == 6){
				ObjectData object;
				object.m_name = L"Saturn";
				object.m_xcoord = -885761459.9;
				object.m_ycoord = -1191572769;
				object.m_zcoord = 55978831.8;
				object.m_xvelo = 25987.33133;
				object.m_yvelo = -20856.51129;
				object.m_zvelo = -674.5191272;
				g_knownValues50.push_back(object);
			}
			if (i == 7){
				ObjectData object;
				object.m_name = L"Uranus";
				object.m_xcoord = 2906149695;
				object.m_ycoord = 720636051.6;
				object.m_zcoord = -34960348.43;
				object.m_xvelo = -6107.200651;
				object.m_yvelo = 22625.66929;
				object.m_zvelo = 163.5649798;
				g_knownValues50.push_back(object);
			}
			if (i == 8){
				ObjectData object;
				object.m_name = L"Neptune";
				object.m_xcoord = 4095814959;
				object.m_ycoord = -1823544579;
				object.m_zcoord = -56824695.64;
				object.m_xvelo = 7799.867938;
				object.m_yvelo = 17962.08121;
				object.m_zvelo = -552.7257902;
				g_knownValues50.push_back(object);
			}
		}
	}

	if (timeInDays == 365){
		for (int i = 0; i < 9; i++){
			if (i == 0){
				ObjectData object;
				object.m_name = L"Sun";
				object.m_xcoord = 0;
				object.m_ycoord = 0;
				object.m_zcoord = 0;
				object.m_xvelo = 0;
				object.m_yvelo = 0;
				object.m_zvelo = 0;
				g_knownValues365.push_back(object);
			}
			if (i == 1){
				ObjectData object;
				object.m_name = L"Mercury";
				object.m_xcoord = 18766692.93;
				object.m_ycoord = 42132885.04;
				object.m_zcoord = 1720914.548;
				object.m_xvelo = -195263.0601;
				object.m_yvelo = 78163.14287;
				object.m_zvelo = 24301.05807;
				g_knownValues365.push_back(object);
			}
			if (i == 2){
				ObjectData object;
				object.m_name = L"Venus";
				object.m_xcoord = 4049817.258;
				object.m_ycoord = -108684566;
				object.m_zcoord = -1723364.905;
				object.m_xvelo = 125141.8566;
				object.m_yvelo = 4238.53855;
				object.m_zvelo = -7163.653228;
				g_knownValues365.push_back(object);
			}
			if (i == 3){
				ObjectData object;
				object.m_name = L"Earth";
				object.m_xcoord = 56994443.45;
				object.m_ycoord = -140984104.3;
				object.m_zcoord = 4927.148079;
				object.m_xvelo = 97722.13926;
				object.m_yvelo = 39796.94078;
				object.m_zvelo = -1.87339952;
				g_knownValues365.push_back(object);
			}
			if (i == 4){
				ObjectData object;
				object.m_name = L"Mars";
				object.m_xcoord = -32940093.43;
				object.m_ycoord = 235322170.5;
				object.m_zcoord = 5739286.75;
				object.m_xvelo = -83070.91655;
				object.m_yvelo = -4686.335825;
				object.m_zvelo = 1940.697842;
				g_knownValues365.push_back(object);
			}
			if (i == 5){
				ObjectData object;
				object.m_name = L"Jupiter";
				object.m_xcoord = -695245821.269348;
				object.m_ycoord = 404435299.857454;
				object.m_zcoord = 13877495.7147923;
				object.m_xvelo = -24233.09268;
				object.m_yvelo = -38475.33429;
				object.m_zvelo = 702.0001936;
				g_knownValues365.push_back(object);
			}
			if (i == 6){
				ObjectData object;
				object.m_name = L"Saturn";
				object.m_xcoord = -677061600.325424;
				object.m_ycoord = -1330824838.97;
				object.m_zcoord = 50087359.9067295;
				object.m_xvelo = 29086.44475;
				object.m_yvelo = -15901.13502;
				object.m_zvelo = -881.4727019;
				g_knownValues365.push_back(object);
			}
			if (i == 7){
				ObjectData object;
				object.m_name = L"Uranus";
				object.m_xcoord = 2854731950.96709;
				object.m_ycoord = 890210291.13439;
				object.m_zcoord = -33661204.0427779;
				object.m_xvelo = 0.004810591;
				object.m_yvelo = 124604248.2;
				object.m_zvelo = -537.4215605;
				g_knownValues365.push_back(object);
			}
			if (i == 8){
				ObjectData object;
				object.m_name = L"Neptune";
				object.m_xcoord = 4152581019;
				object.m_ycoord = -1686816831;
				object.m_zcoord = -60953508.09;
				object.m_xvelo = 7219.25696;
				object.m_yvelo = 18208.39271;
				object.m_zvelo = -540.1008551;
				g_knownValues365.push_back(object);
			}
		}
	}

	if (timeInDays == 730){
		for (int i = 0; i < 9; i++){
			if (i == 0){
				ObjectData object;
				object.m_name = L"Sun";
				object.m_xcoord = 0;
				object.m_ycoord = 0;
				object.m_zcoord = 0;
				object.m_xvelo = 0;
				object.m_yvelo = 0;
				object.m_zvelo = 0;
				g_knownValues730.push_back(object);
			}
			if (i == 1){
				ObjectData object;
				object.m_name = L"Mercury";
				object.m_xcoord = -44417188.49;
				object.m_ycoord = 26479749.56;
				object.m_zcoord = 6240000;
				object.m_xvelo = -125744.561;
				object.m_yvelo = -143282.4045;
				object.m_zvelo = -171.736307;
				g_knownValues730.push_back(object);
			}
			if (i == 2){
				ObjectData object;
				object.m_name = L"Venus";
				object.m_xcoord = -79882107.38;
				object.m_ycoord = 71696040.49;
				object.m_zcoord = 5592730.546;
				object.m_xvelo = -84674.20188;
				object.m_yvelo = -94480.81107;
				object.m_zvelo = 3590.777918;
				g_knownValues730.push_back(object);
			}
			if (i == 3){
				ObjectData object;
				object.m_name = L"Earth";
				object.m_xcoord = 58728268.08;
				object.m_ycoord = -140259837.9;
				object.m_zcoord = 4525.505711;
				object.m_xvelo = 97135.72709;
				object.m_yvelo = 41039.16657;
				object.m_zvelo = -3.099869692;
				g_knownValues730.push_back(object);
			}
			if (i == 4){
				ObjectData object;
				object.m_name = L"Mars";
				object.m_xcoord = 3374109.437;
				object.m_ycoord = -217370516.2;
				object.m_zcoord = -4637944.001;
				object.m_xvelo = 90504.56687;
				object.m_yvelo = 8858.20124;
				object.m_zvelo = -2035.550651;
				g_knownValues730.push_back(object);
			}
			if (i == 5){
				ObjectData object;
				object.m_name = L"Jupiter";
				object.m_xcoord = -813800809.4;
				object.m_ycoord = 30590499.83;
				object.m_zcoord = 18082783.82;
				object.m_xvelo = -2334.424351;
				object.m_yvelo = -44828.00906;
				object.m_zvelo = 238.1777941;
				g_knownValues730.push_back(object);
			}
			if (i == 6){
				ObjectData object;
				object.m_name = L"Saturn";
				object.m_xcoord = -409839199.6;
				object.m_ycoord = -1443313913;
				object.m_zcoord = 41401603.8;
				object.m_xvelo = 31559.59078;
				object.m_yvelo = -9643.498461;
				object.m_zvelo = -1087.899625;
				g_knownValues730.push_back(object);
			}
			if (i == 7){
				ObjectData object;
				object.m_name = L"Uranus";
				object.m_xcoord = 2781978360;
				object.m_ycoord = 1083012788;
				object.m_zcoord = -32000122.47;
				object.m_xvelo = -9067.271116;
				object.m_yvelo = 21658.78742;
				object.m_zvelo = 198.6202743;
				g_knownValues730.push_back(object);
			}
			if (i == 8){
				ObjectData object;
				object.m_name = L"Neptune";
				object.m_xcoord = 4212998727;
				object.m_ycoord = -1525719745;
				object.m_zcoord = -65670512.68;
				object.m_xvelo = 6539.544931;
				object.m_yvelo = 18471.16251;
				object.m_zvelo = -533.8532016;
				g_knownValues730.push_back(object);
			}
		}
	}
}

//compares positions of real and simulated values
//expects that order of planets in XML is real order starting from Sun
float comparePosVal(vector<ObjectData> &realValues){
	float avgPosDiff = 0;
	for (int i = 1; i < 9; i++){
		float xDiff = ((realValues[i].m_xcoord - g_pParticleArray[i].pos.x) / realValues[i].m_xcoord) * 100;
		float yDiff = ((realValues[i].m_ycoord - g_pParticleArray[i].pos.y) / realValues[i].m_ycoord) * 100;
		float zDiff = ((realValues[i].m_zcoord - g_pParticleArray[i].pos.z) / realValues[i].m_zcoord) * 100;
		float posDiff = (xDiff + yDiff + zDiff) / 3;
		avgPosDiff = (avgPosDiff*(i - 1) + posDiff) / (i);
	}
	return avgPosDiff;
}

//compares velocities of real and simulated values
//expects that order of planets in XML is real order starting from Sun
float compareVeloVal(vector<ObjectData> &realValues){
	float avgVeloDiff = 0;
	for (int i = 1; i < 9; i++){
		float xvDiff = ((realValues[i].m_xvelo - g_pParticleArray[i].velo.x) / realValues[i].m_xvelo) * 100;
		float yvDiff = ((realValues[i].m_yvelo - g_pParticleArray[i].velo.y) / realValues[i].m_yvelo) * 100;
		float zvDiff = ((realValues[i].m_zvelo - g_pParticleArray[i].velo.z) / realValues[i].m_zvelo) * 100;
		float veloDiff = (xvDiff + yvDiff + zvDiff) / 3;
		avgVeloDiff = (avgVeloDiff*(i - 1) + veloDiff) / (i);
	}
	return avgVeloDiff;
}

//unit test tests accuracy of jumpTime feature for time=1 year etc.
void testJumpTimeAccuracy(){
	float avgPosDiff;
	float avgVeloDiff;

	OnGUIEvent(0, IDC_RESETPARTICLES, NULL, NULL);

	//test for time=50 days
	loadKnownValues(50);
	jumpTime(50);
	pauseControl();
	float posDiff50 = comparePosVal(g_knownValues50);
	float veloDiff50 = compareVeloVal(g_knownValues50);
	pauseControl();

	//test for time=365 days
	loadKnownValues(365);
	jumpTime(365);
	pauseControl();
	float posDiff365 = comparePosVal(g_knownValues365);
	float veloDiff365 = compareVeloVal(g_knownValues365);
	pauseControl();

	//test for time=730 days
	loadKnownValues(730);
	jumpTime(730);
	pauseControl();
	float posDiff730 = comparePosVal(g_knownValues730);
	float veloDiff730 = compareVeloVal(g_knownValues730);
	pauseControl();

	//average the different tests
	avgPosDiff = (posDiff50 + posDiff365 + posDiff730) / 3; //add in other tests as they get added
	avgVeloDiff = (veloDiff50 + veloDiff365 + veloDiff730) / 3; //add in other tests as they get added

	char buffer[256];
	sprintf_s(buffer, sizeof(buffer), "Avg Position Percent Difference %f\n", avgPosDiff);
	::OutputDebugStringA(buffer);

	sprintf_s(buffer, sizeof(buffer), "Avg Velocity Percent Difference %f\n", avgVeloDiff);
	::OutputDebugStringA(buffer);

}

//test the fast forward accuracy
//fast forward to time=50 then time=365 then time=730 and compare results
//TODO not full finished yet
void testFastFwdAccuracy(){
	OnGUIEvent(0, IDC_RESETPARTICLES, NULL, NULL);
	int initialIterations = g_iterationsPerFrame;
	g_iterationsPerFrame = 20;
	//go to certain point and test accuracy
	g_iterationsPerFrame = initialIterations;

}

//test jump time accuracy
//see how long it takes to jump in time to 365 days
double testJumpTimeSpeed(){
	OnGUIEvent(0, IDC_RESETPARTICLES, NULL, NULL);
	jumpTime(365);
	double testTime = g_jumpSpeedTest;
	return testTime;
}

//see how long it takes for 1 frame at 1 iteration per frame
double testSpeed1IterationsPerFrame(){

	g_iterationsPerFrame = 1;

	double oneIterationTime = g_oneFrameTime;

	return oneIterationTime;
}

//see how long it takes for 1 frame at 100 iterations per frame
double testSpeed100IterationsPerFrame(){

	g_iterationsPerFrame = 100;

	double hundredIterationsTime = g_oneFrameTime;

	return hundredIterationsTime;
}

//see how long it takes to get to 1000 days while running the simulation at 10 iterations/frame
double testRegularSpeed(){

	double timeElapsed = g_elapsedTimeAt1000Days;
	return timeElapsed;

}

double getStartTime() {
	return g_startUpTime;
}

double getPauseTime() {
	OnGUIEvent(0, IDC_PAUSE, NULL, NULL);
	return g_pauseTime;
}

double getUnPauseTime() {
	OnGUIEvent(0, IDC_PAUSE, NULL, NULL);
	return g_unPauseTime;
}

double getPauseFullScreenTime() {
	OnGUIEvent(0, IDC_PAUSE, NULL, NULL);
	return g_pauseFullScreenTime;
}

double getUnPauseFullScreenTime() {
	OnGUIEvent(0, IDC_PAUSE, NULL, NULL);
	return g_unPauseFullScreenTime;
}

//see how long it takes to reset particles to their initial position
double testResetParticles(){
	OnGUIEvent(0, IDC_RESETPARTICLES, NULL, NULL);
	return g_deltatResetParticles;
}

//see how long it takes to Reset the Camera to its initial position
double testResetCamera(){
	OnGUIEvent(0, IDC_RESETCAMERA, NULL, NULL);
	return g_deltatResetCamera;
}

double getWinToFullTime() {
	OnGUIEvent(0, IDC_TOGGLEFULLSCREEN, NULL, NULL);
	return g_winToFullTime;
}

double getFullToWinTime() {
	OnGUIEvent(0, IDC_TOGGLEFULLSCREEN, NULL, NULL);
	return g_fullToWinTime;
}

double getHitTestTime() {
	return g_hitTestTime / g_numHitTests;
}

float getAverageFPS() {
	return g_averageFPS / g_averageFPSCounter;
}

void initializeFile() {
	g_dataFile.open(g_localFileName);
}

wstring getTelemetryResults(LPCWSTR grade, double fps) {
	wostringstream wss;
	wss << L"Computer Grade: " << grade << "\n";
	wss << L"     Average FPS: " << fps << "\n";
	wss << L"     Time Sum (sec): " << g_totalTime << "\n";
	wss << L"If you would like to see what the grade means and how your results \ncompare to our telemetry data, please see the README.txt\n\n";

	wstring telemetryResults = wss.str().c_str();

	return telemetryResults;
}

void copyFile(LPCWSTR grade, double fps) {
	//copy data csv
	srand(time(NULL));
	int num = rand();
	wostringstream wss;
	wss << L"\\\\davis\\public\\GRFXExplorerInternship\\Telemetry\\" << num << g_localFileName;
	const wstring& wstr = wss.str();
	const LPCWSTR copyName = wstr.c_str();
	bool copiedCSV = CopyFileW(g_localFileName, copyName, true);

	//copy dxdiag txt
	wostringstream wss2;
	wss2 << L"\\\\davis\\public\\GRFXExplorerInternship\\Telemetry\\" << num << g_currentDxDiagName;
	const wstring& wstrDiag = wss2.str();
	const LPCWSTR copyDiagName = wstrDiag.c_str();
	bool copiedDxDiag = CopyFileW(g_currentDxDiagName, copyDiagName, true);

	HWND hwnd = FindWindow(NULL, TEXT("SkyX"));
	
	wstring telemetryResults = getTelemetryResults(grade, fps);

	wostringstream wss3;

	if (!copiedCSV && !copiedDxDiag) {
		wss3 << L"The telemetry data and DXDiag files were not successfully copied to the share. \nPlease email the files called SkyXTelemetryData.csv and dxdiag.txt\n(in the same folder as the executable) to t-mellop@microsoft.com\nThank you so much for your help!\n\n";
		wss3 << telemetryResults;
		const wstring& wstrMessage = wss3.str();
		const LPCWSTR failureMessageBoth = wstrMessage.c_str();
		MessageBoxW(hwnd, failureMessageBoth, NULL, MB_OK);
	}
	else if (!copiedCSV && copiedDxDiag) {
		wss3 << L"The telemetry data file was not successfully copied to the share. \nPlease email the file called SkyXTelemetryData.csv\n(in the same folder as the executable) to t-mellop@microsoft.com\nThank you so much for your help!\n\n";
		wss3 << telemetryResults;
		const wstring& wstrMessage = wss3.str();
		LPCWSTR failureMessageCSV = wstrMessage.c_str();
		MessageBoxW(hwnd, failureMessageCSV, NULL, MB_OK);
}
	else if (copiedCSV && !copiedDxDiag) {
		wss3 << L"The DXDiag file was not successfully copied to the share. \nPlease email the file called dxdiag.txt\n(in the same folder as the executable) to t-mellop@microsoft.com\nThank you so much for your help!\n\n";
		wss3 << telemetryResults;
		const wstring& wstrMessage = wss3.str();
		LPCWSTR failureMessageDiag = wstrMessage.c_str();
		MessageBoxW(hwnd, failureMessageDiag, NULL, MB_OK);
	}
	else {
		wss3 << L"The telemetry data and DXDiag files were successfully copied to the share! \nThank you so much for your help!\n\n";
		wss3 << telemetryResults;
		const wstring& wstrMessage = wss3.str();
		LPCWSTR successMessage = wstrMessage.c_str();
		LPCWSTR boxTitle = L"Upload Complete";
		MessageBoxW(hwnd, successMessage, boxTitle, MB_OK);
	}
}

void getUsername() {
	wchar_t username[UNLEN + 1];
	DWORD username_len = UNLEN + 1;
	GetUserNameW(username, &username_len);

	wostringstream wss;
	wss << username; 

	const wstring& wstr = wss.str();
	const LPCWSTR usernameCopy = wstr.c_str();

	g_dataFile << usernameCopy << endl;

}



//--------------------------------------------------------------------------------------
// Functions for deleting objects through the UI
//--------------------------------------------------------------------------------------

//returns the index of the object given the name
//returns null if it can't find the object
int getObjectIndex(wstring name){
	for (int i = 0; i < NUM_PARTICLES; i++){
		if (g_pParticleArrayTWO[i].name == name){
			return i;
		}
	}
	return -1;
}

void deleteObject(wstring name){
	int index = getObjectIndex(name);
	HWND hwnd = FindWindow(NULL, TEXT("SkyX"));
	LPCWSTR message = L"No objects were deleted. Please enter a valid object name.";

	if (index == -1){
		MessageBox(hwnd, message, NULL, MB_OK | MB_ICONWARNING);
		return;
	}

	//delete object from vector
	vector<ObjectData>::const_iterator it;
	it = g_objects.cbegin();
	for (int i = 0; i < index; i++){
		it++;
	}

	g_objects.erase(it);

	//delete object from arrays

	for (int i = index; i < NUM_PARTICLES; i++){
		g_pParticleArray[i] = g_pParticleArray[i + 1];
		g_pParticleArrayTWO[i] = g_pParticleArrayTWO[i + 1];
	}
	NUM_PARTICLES--;

}

//--------------------------------------------------------------------------------------
// Functions for grade calculation in test mode
//--------------------------------------------------------------------------------------
//evaluates how well the computer did based on externally decided thresholds
//return -1 means it failed in this category
//return 0 means it was adequate in this category
//return 1 means it was excellent in this category
int timeGrade(){
	double totalTime = 0;
	for (int i = 0; i < g_timeTestResults.size(); i++){
		totalTime = totalTime + g_timeTestResults[i];
	}
	g_totalTime = totalTime;
	if (totalTime > 14.60263){
		return -1;
	}
	if (totalTime < 6.251098989){
		return 1;
	}
	else{
		return 0;
	}
}


int fpsGrade(double fps) {
	if (fps > 88.67604) {
		return 1;
	}
	else if (fps < 58.79577) {
		return -1;
	}
	else {
		return 0;
	}
}

LPCWSTR totalGrade(double fps) {
	int fpsAssessment = fpsGrade(fps);
	int timeAssessment = timeGrade();
	LPCWSTR grade;

	if (fpsAssessment == 1 && timeAssessment == 1) {
		grade = L"Excellent (4/4)";
		return grade;
	}
	else if ((fpsAssessment == 0 && timeAssessment == 1) || (fpsAssessment == 1 && timeAssessment == 0)) {
		grade = L"Good (3/4)";
		return grade;
	}
	else if (fpsAssessment == 0 && timeAssessment == 0) {
		grade = L"Adequate (2/4)";
		return grade;
	}
	else {
		grade = L"Fail (1/4)";
		return grade;
	}
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
		g_pObjectDataDisplay->SetEnabled(false); //prevents accidental alteration of editbox
		g_hitTestStart = g_timer.GetAbsoluteTime();
	}

}

wstring concatenateObjInfo(int index) {
	wstring objectInfo(L"Name: " + g_pParticleArrayTWO[index].name + L"\nMass: " + to_wstring(g_pParticleArrayTWO[index].mass) + L"\nDiameter: " +
		to_wstring(g_pParticleArrayTWO[index].diameter) + L"\nBrightness: " + to_wstring(g_pParticleArrayTWO[index].brightness) +
		L"\nPosition:\nx: " + to_wstring(g_pParticleArray[index].pos.x) + L"\ny: " + to_wstring(g_pParticleArray[index].pos.y) +
		L"\nz: " + to_wstring(g_pParticleArray[index].pos.z) +
		L"\nVelocity:\nx: " + to_wstring(g_pParticleArray[index].velo.x) + L"\ny: " + to_wstring(g_pParticleArray[index].velo.y) +
		L"\nz: " + to_wstring(g_pParticleArray[index].velo.z));

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
	double oneFrameTimeStart = g_timer.GetAbsoluteTime();



	if (fElapsedTime < SECONDS_PER_FRAME)
	{
		Sleep(static_cast<DWORD>((SECONDS_PER_FRAME - fElapsedTime) * 1000.0f));
	}

	if (g_frameCounter % 100 == 0) {
		g_averageFPS += DXUTGetFPS();
		g_averageFPSCounter++;

	}

	g_frameCounter++;


	// Update the camera's position based on user input 
	g_Camera.FrameMove(fElapsedTime);

	g_mProj = g_Camera.GetProjMatrix();
	g_mView = g_Camera.GetViewMatrix();



	if (!g_isPaused)
	{


		for (int i = 0; i < g_iterationsPerFrame; i++){



			auto pd3dImmediateContext = DXUTGetD3D11DeviceContext();

			D3D11_MAPPED_SUBRESOURCE ms;
			pd3dImmediateContext->Map(g_pParticlePosVelo0, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);

			g_systemTime = g_systemTime + g_timeValueToHoursConversion;

			GravityMotionIteration(g_timeValue);

			//this section helps with the testRegularSpeed() function
			int systemTimeAt1000 = round(1000 * 24);

			if (g_systemTime < systemTimeAt1000 + 1 && g_systemTime > systemTimeAt1000 - 1){
				g_elapsedTimeAt1000Days = g_timer.GetAbsoluteTime();
			}


			memcpy(ms.pData, g_pParticleArray, sizeof(PARTICLE) * NUM_PARTICLES);

			pd3dImmediateContext->Unmap(g_pParticlePosVelo0, NULL);

			std::swap(g_pParticlePosVelo0, g_pParticlePosVelo1);
			std::swap(g_pParticlePosVeloRV0, g_pParticlePosVeloRV1);
		}

	}
	if (g_isPaused && g_hasDisplay && g_relevantMouse) {

	

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

			/*	wchar_t buffer[256];
			swprintf(buffer, sizeof(buffer), L"screen: %f mouse: %f\n centerX: %f centerY %f \n mouseX: %f mouseY: %f\n\n", screenRadius, mouseRadius, XMVectorGetX(screenObject), XMVectorGetY(screenObject), xScreenMouse, yScreenMouse);
			::OutputDebugString(buffer);*/

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

		if (foundIndex < 0 && g_pObjectDataDisplay != nullptr && g_hasDisplay) {
			g_pObjectDataDisplay->ClearText();
			g_pObjectDataDisplay->SetText(L"No object selected");
		}
		else if (g_pObjectDataDisplay != nullptr && g_hasDisplay) {
			//TODO: text wrapping
			wstring objectInfo = concatenateObjInfo(foundIndex);
			g_pObjectDataDisplay->ClearText();
			g_pObjectDataDisplay->SetText(objectInfo.c_str());
		}
		foundIndex = -1;
		double hitTestEnd = g_timer.GetAbsoluteTime();
		g_hitTestTime += hitTestEnd - g_hitTestStart;
		g_numHitTests++;


	}


	g_relevantMouse = false;


	g_oneFrameTime = g_timer.GetAbsoluteTime() - oneFrameTimeStart;

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

void addPosVel(PARTICLE particles[], int i, std::vector<ObjectData> & object) {
	particles[i].pos = createPositionFloat(object[i].m_xcoord, object[i].m_ycoord, object[i].m_zcoord);
	particles[i].velo = createVelocityFloat(object[i].m_xvelo, object[i].m_yvelo, object[i].m_zvelo);
}

void addDetails(PARTICLE_DETAILS particles[], int i, std::vector<ObjectData> &object) {
	particles[i].name = object[i].m_name;
	particles[i].mass = object[i].m_mass;
	particles[i].diameter = object[i].m_diameter;
	particles[i].brightness = object[i].m_brightness;
	particles[i].red = object[i].m_red;
	particles[i].green = object[i].m_green;
	particles[i].blue = object[i].m_blue;
}

void clearAddObjectMenu() {
	g_pNameBox->ClearText();
	g_pMassBox->ClearText();
	g_pDiameterBox->ClearText();
	g_pBrightnessBox->ClearText();
	g_pXPosBox->ClearText();
	g_pYPosBox->ClearText();
	g_pZPosBox->ClearText();
	g_pXVelBox->ClearText();
	g_pYVelBox->ClearText();
	g_pZVelBox->ClearText();
	g_pRedBox->ClearText();
	g_pGreenBox->ClearText();
	g_pBlueBox->ClearText();
}

void addObject(wstring name, float mass, float diameter, float brightness, float x, float y, float z, float xv, float yv, float zv, float r, float g, float b) {
	ObjectData newObject(name, mass, diameter, brightness, x, y, z, xv, yv, zv, r, g, b);

	//add to vector
	g_objects.push_back(newObject);

	//add to arrays
	NUM_PARTICLES++;
	addPosVel(g_pParticleArray, NUM_PARTICLES - 1, g_objects);
	addDetails(g_pParticleArrayTWO, NUM_PARTICLES - 1, g_objects);

	clearAddObjectMenu();
	
}

void toggleAddObjectMenuVisibility(bool change) { //true reveals menu; false hides menu
	g_addingObject = change;
	g_pNameBox->SetVisible(change);
	g_pMassBox->SetVisible(change);
	g_pDiameterBox->SetVisible(change);
	g_pBrightnessBox->SetVisible(change);
	g_pXPosBox->SetVisible(change);
	g_pYPosBox->SetVisible(change);
	g_pZPosBox->SetVisible(change);
	g_pXVelBox->SetVisible(change);
	g_pYVelBox->SetVisible(change);
	g_pZVelBox->SetVisible(change);
	g_pRedBox->SetVisible(change);
	g_pGreenBox->SetVisible(change);
	g_pBlueBox->SetVisible(change);
	g_pSubmitObjectButton->SetVisible(change);
}

void pauseControl() {
	double pauseStart;
	double unPauseStart;

	if (!g_isPaused) {
		pauseStart = g_timer.GetAbsoluteTime();
	}
	else {
		unPauseStart = g_timer.GetAbsoluteTime();
	}


	if (!g_isPaused) {
		//DXUTPause(true, false);
		g_isPaused = true;
	}
	else {
		//DXUTPause(false, false);
		g_isPaused = false;
		toggleAddObjectMenuVisibility(false);
		clearAddObjectMenu();
	}

	LPCWSTR welcomeMessage = L"Select an object\nto see information\ndisplayed\n";
	if (g_isPaused && !g_hasDisplay && g_firstTextBox) { //always the first case; text box pointer gets assignment here
		g_pObjectDataDisplay->SetVisible(true);
		g_pObjectDataDisplay->SetText(welcomeMessage);
		g_hasDisplay = true;
		g_firstTextBox = false;
	}
	else if (g_isPaused && !g_hasDisplay)
	{
		g_pObjectDataDisplay->SetText(welcomeMessage);
		//g_pObjectDataDisplay->SetVisible(true);
		g_hasDisplay = true;
	}
	else if (g_hasDisplay) {
		//g_pObjectDataDisplay->SetVisible(false);
		g_pObjectDataDisplay->SetText(L"");
		g_hasDisplay = false;
	}

	if (g_isPaused) {
		double pauseEnd = g_timer.GetAbsoluteTime();
		if (DXUTIsWindowed()) {
			g_pauseTime = pauseEnd - pauseStart;
		}
		else {
			g_pauseFullScreenTime = pauseEnd - pauseStart;
		}

	}
	else {
		double unPauseEnd = g_timer.GetAbsoluteTime();
		if (DXUTIsWindowed()) {
			g_unPauseTime = unPauseEnd - unPauseStart;
		}
		else {
			g_unPauseFullScreenTime = unPauseEnd - unPauseStart;
		}
	}



}



//Add object helper methods
wstring processName(LPCWSTR tempStr) {

	wstring name(tempStr);

	return name;
}

//returns wether or not the field is empty
bool canProcessInput(LPCWSTR tempStr, bool time) {
	HWND hwnd = FindWindow(NULL, TEXT("SkyX"));
	LPCWSTR emptyFieldMessage = L"Please enter a value in all fields";
	LPCWSTR emptyFieldMessageTime = L"Please enter a value";
	if (!time) {
		if (tempStr == NULL || wcslen(tempStr) == 0) {
			MessageBox(hwnd, emptyFieldMessage, NULL, MB_OK | MB_ICONWARNING);
			return false;
		}
	}
	else {
	if (tempStr == NULL || wcslen(tempStr) == 0) {
			MessageBox(hwnd, emptyFieldMessageTime, NULL, MB_OK | MB_ICONWARNING);
		return false;
	}
	}
	
	return true;
}

//returns whether or not input is numerical
bool canConvertFloatInput(LPCWSTR tempStr, bool time) {
	LPCWSTR invalidInputMessage = L"Please make sure valid input was entered in all fields";
	LPCWSTR invalidInputMessageTime = L"Please make sure valid input was entered";
	bool generalProcess = canProcessInput(tempStr, time);
	if (!generalProcess) {
		return false;
	}

		bool hasDecimal = false;
		int decimalCount = 0;
		bool hasNegative = false;
		int negativeCount = 0;
	HWND hwnd = FindWindow(NULL, TEXT("SkyX"));

	//check if alphanumeric
	for (int i = 0; i < wcslen(tempStr); i++) {
		

		if (tempStr[i] == '.') {
			hasDecimal = true;
			decimalCount++;
		}

		if (tempStr[i] == '-') {
			hasNegative = true;
			negativeCount++;
		}

		
		
		if (!time) {
			if (!(isdigit(tempStr[i])) || !hasDecimal && decimalCount > 1 || !hasNegative && negativeCount > 1) {
				MessageBox(hwnd, invalidInputMessage, NULL, MB_OK | MB_ICONWARNING);
			return false;
		}
		}
		else {
			if (!(isdigit(tempStr[i])) || !hasDecimal && decimalCount > 1 || hasNegative) {
				MessageBox(hwnd, invalidInputMessageTime, NULL, MB_OK | MB_ICONWARNING);
				return false;
			}
		}


	}

	return true;
}

//converts numerical string to a float
float convertFloatInput(LPCWSTR tempStr) {
	float converted = wcstof(tempStr, NULL);

	return converted;
}





//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent(UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext)
{
	double tstart;
	double tend;
	switch (nControlID)
	{
	case IDC_TOGGLEFULLSCREEN:
	{
		double winToFullStart;
		double winToFullEnd;
		double fullToWinStart;
		double fullToWinEnd;
		if (DXUTIsWindowed()) {
			winToFullStart = g_timer.GetAbsoluteTime();
		}
		else {
			fullToWinStart = g_timer.GetAbsoluteTime();
		}

		DXUTToggleFullScreen();

		if (!DXUTIsWindowed()) {
			winToFullEnd = g_timer.GetAbsoluteTime();
			g_winToFullTime = winToFullEnd - winToFullStart;
		}
		else {
			fullToWinEnd = g_timer.GetAbsoluteTime();
			g_fullToWinTime = fullToWinEnd - fullToWinStart;

		}

		break;
	}
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
		tstart = g_timer.GetAbsoluteTime();
		SAFE_RELEASE(g_pParticlePosVelo0);
		SAFE_RELEASE(g_pParticlePosVelo1);
		SAFE_RELEASE(g_pParticlePosVeloRV0);
		SAFE_RELEASE(g_pParticlePosVeloRV1);
		CreateParticlePosVeloBuffers(DXUTGetD3D11Device());
		tend = g_timer.GetAbsoluteTime();
		g_systemTime = 0;

		g_deltatResetParticles = tend - tstart;

		wchar_t buffer[256];
		swprintf(buffer, sizeof(buffer), L"%f\n", g_deltatResetParticles);
		::OutputDebugString(buffer);
		break;
	}
	case IDC_PAUSE:
	{
		pauseControl();
		break;
	}
	case IDC_SUBMITITERATEIN:
	{
		LPCWSTR iterateStr;
		float iterateFloat;
		iterateStr = g_IterationsPerFrameInBox->GetText();
		if (iterateStr == NULL){
			break;
		}
		bool canProcess = canConvertFloatInput(iterateStr, true);
		if (canProcess) {
		iterateFloat = wcstof(iterateStr, NULL);
		int iterateInt = (int)(iterateFloat + 0.5);
			g_iterationsPerFrame = iterateInt;
	}
		 break;
	}

	case IDC_SUBMITTIMEIN:
	{
		LPCWSTR timeStr;
		float timeFloat;
		timeStr = g_JumpTimeInputBox->GetText();
		if (timeStr == NULL){
			break;
		}
		bool canProcess = canConvertFloatInput(timeStr, true);
		if (canProcess) {
			timeFloat = convertFloatInput(timeStr);
		jumpTime(timeFloat);
		}
		break;
	}
	case IDC_RESETCAMERA:
	{
		tstart = g_timer.GetAbsoluteTime();
		g_Camera.Reset();
		tend = g_timer.GetAbsoluteTime();
		g_deltatResetCamera = tend - tstart;

		wchar_t buffer[256];
		swprintf(buffer, sizeof(buffer), L"%f\n", g_deltatResetCamera);
		::OutputDebugString(buffer);

		break;
	}
	case IDC_ADDOBJECT: {
		if (g_addingObject) {
			clearAddObjectMenu();
			toggleAddObjectMenuVisibility(false);
			//g_pObjectDataDisplay->SetVisible(true);
			break;
		}
		HWND hwnd = FindWindow(NULL, TEXT("SkyX"));
		LPCWSTR warningMessage = L"Please pause to add an object.";
		if (!g_isPaused) {
			MessageBox(hwnd, warningMessage, NULL, MB_OK | MB_ICONWARNING);
		}
		else {
			//g_pObjectDataDisplay->SetVisible(false);
			toggleAddObjectMenuVisibility(true);
		}
		break;
	}
	case IDC_SUBMITOBJECT: {
		wstring newName = L"";
		float newMass;
		float newDiameter;
		float newBrightness;
		float newXPos;
		float newYPos;
		float newZPos;
		float newXVel;
		float newYVel;
		float newZVel;
		float newRed;
		float newBlue;
		float newGreen;

		//process name
		LPCWSTR nameStr = g_pNameBox->GetText();
		bool canProcess = canProcessInput(nameStr, false);
		if (canProcess) {
			newName = processName(nameStr);
		}
		else {
			break;
		}

		//process mass
		LPCWSTR massStr = g_pMassBox->GetText();
		canProcess = canConvertFloatInput(massStr, false);
		if (canProcess) {
			newMass = convertFloatInput(massStr);
		}
		else {
			break;
		}

		//process diameter
		LPCWSTR diameterStr = g_pDiameterBox->GetText();
		canProcess = canConvertFloatInput(diameterStr, false);
		if (canProcess) {
			newDiameter = convertFloatInput(diameterStr);
		}
		else {
			break;
		}

		//process brightness
		LPCWSTR brightnessStr = g_pBrightnessBox->GetText();
		canProcess = canConvertFloatInput(brightnessStr, false);
		if (canProcess) {
			newBrightness = convertFloatInput(brightnessStr);
		}
		else {
			break;
		}

		//process position
		LPCWSTR xPosStr = g_pXPosBox->GetText();
		canProcess = canConvertFloatInput(xPosStr, false);
		if (canProcess) {
			newXPos = convertFloatInput(xPosStr);
		}
		else {
			break;
		}

		LPCWSTR yPosStr = g_pYPosBox->GetText();
		canProcess = canConvertFloatInput(yPosStr, false);
		if (canProcess) {
			newYPos = convertFloatInput(yPosStr);
		}
		else {
			break;
		}

		LPCWSTR zPosStr = g_pZPosBox->GetText();
		canProcess = canConvertFloatInput(zPosStr, false);
		if (canProcess) {
			newZPos = convertFloatInput(zPosStr);
		}
		else {
			break;
		}

		//process velocity
		LPCWSTR xVelStr = g_pXVelBox->GetText();
		canProcess = canConvertFloatInput(xVelStr, false);
		if (canProcess) {
			newXVel = convertFloatInput(xVelStr);
		}
		else {
			break;
		}

		LPCWSTR yVelStr = g_pYVelBox->GetText();
		canProcess = canConvertFloatInput(yVelStr, false);
		if (canProcess) {
			newYVel = convertFloatInput(yVelStr);
		}
		else {
			break;
		}

		LPCWSTR zVelStr = g_pZVelBox->GetText();
		canProcess = canConvertFloatInput(zVelStr, false);
		if (canProcess) {
			newZVel = convertFloatInput(zVelStr);
		}
		else {
			break;
		}
		

		//process colors

		LPCWSTR redStr = g_pRedBox->GetText();
		canProcess = canConvertFloatInput(redStr, false);
		if (canProcess) {
			newRed = convertFloatInput(redStr);
		}
		else {
			break;
		}

		LPCWSTR greenStr = g_pGreenBox->GetText();
		canProcess = canConvertFloatInput(greenStr, false);
		if (canProcess) {
			newGreen = convertFloatInput(greenStr);
		}
		else {
			break;
		}

		LPCWSTR blueStr = g_pBlueBox->GetText();
		canProcess = canConvertFloatInput(blueStr, false);
		if (canProcess) {
			newBlue = convertFloatInput(blueStr);
		}
		else {
			break;
		}

		addObject(newName, newMass, newDiameter, newBrightness, newXPos, newYPos, newZPos, newXVel, newYVel, newZVel, newRed, newGreen, newBlue);


		auto pd3dImmediateContext = DXUTGetD3D11DeviceContext();
		auto pVertices = updateVertices();
		pd3dImmediateContext->UpdateSubresource(g_pParticleBuffer, 0, 0, pVertices, 0, 0);
		SAFE_DELETE_ARRAY(pVertices);

		break;

	}

	case IDC_OUTPUTINFO:
	{
		ofstream myfile("OutputObjectData.xml");
		if (myfile.is_open())
		{
			myfile << "<?xml version=1.0?> \n";
			myfile << "<Universe> \n";
			myfile << "<MaxParticles>" << MAX_PARTICLES;
			myfile << "</MaxParticles>\n";
			for (unsigned int i = 0; i < NUM_PARTICLES; i++)
			{
				myfile << "<object>\n";

				wstring wname=g_pParticleArrayTWO[i].name;
				string name(wname.begin(), wname.end());
				myfile << "<name>" << name;
				myfile << "</name>\n";

				myfile << "<mass>" << g_pParticleArrayTWO[i].mass;
				myfile << "</mass>\n";

				myfile << "<diameter>" << g_pParticleArrayTWO[i].diameter;
				myfile << "</diameter>\n";

				myfile << "<brightness>" << g_pParticleArrayTWO[i].brightness;
				myfile << "</brightness>\n";

				myfile << "<xcoord>" << g_pParticleArray[i].pos.x;
				myfile << "</xcoord>\n";

				myfile << "<ycoord>" << g_pParticleArray[i].pos.y;
				myfile << "</ycoord>\n";

				myfile << "<zcoord>" << g_pParticleArray[i].pos.z;
				myfile << "</zcoord>\n";

				myfile << "</object>\n";
			}
		myfile << "</Universe> \n";
		}

		else cout << "Unable to open output file";
		break;
	}
	case IDC_SUBMITDELETEOBJ:
	{
		LPCWSTR object = g_DeleteObjInBox-> GetText();
		deleteObject(object);
		/*DXUTToggleREF();
		DXUTToggleREF();*/
		auto pd3dImmediateContext = DXUTGetD3D11DeviceContext();
		auto pVertices = updateVertices();
		pd3dImmediateContext->UpdateSubresource(g_pParticleBuffer, 0, 0, pVertices, 0, 0);
		SAFE_DELETE_ARRAY(pVertices);

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
	g_pTxtHelper->DrawTextLine(L"Motion Iterations per Frame:");
	WCHAR iterations[g_cIntStringLength];
	GetWCharFromInt(iterations, g_iterationsPerFrame);
	g_pTxtHelper->DrawTextLine(iterations);
	g_pTxtHelper->DrawTextLine(L"Days:");
	WCHAR currentTime[g_cFloatStringLength];
	GetWCharFromFloat(currentTime, g_systemTime / 24);
	g_pTxtHelper->DrawTextLine(currentTime);
	g_pTxtHelper->End();
}

void renderAddObjectText() {
	g_pTxtHelper->Begin();
	g_pTxtHelper->SetInsertionPos(172, 100);
	g_pTxtHelper->SetForegroundColor(Colors::Yellow);
	g_pTxtHelper->DrawTextLine(L"Name");
	g_pTxtHelper->DrawTextLine(L"\nMass");
	g_pTxtHelper->DrawTextLine(L"\n\nDiameter");
	g_pTxtHelper->DrawTextLine(L"\n\n\nBrightness");
	g_pTxtHelper->DrawTextLine(L"\n\n\n\nX Pos");
	g_pTxtHelper->DrawTextLine(L"\n\n\n\n\nY Pos");
	g_pTxtHelper->DrawTextLine(L"\n\n\n\n\n\nZ Pos");
	g_pTxtHelper->DrawTextLine(L"\n\n\n\n\n\n\nX Vel");
	g_pTxtHelper->DrawTextLine(L"\n\n\n\n\n\n\n\nY Vel");
	g_pTxtHelper->DrawTextLine(L"\n\n\n\n\n\n\n\n\nZ Vel");
	g_pTxtHelper->DrawTextLine(L"\n\n\n\n\n\n\n\n\n\nRed");
	g_pTxtHelper->DrawTextLine(L"\n\n\n\n\n\n\n\n\n\n\nGreen");
	g_pTxtHelper->DrawTextLine(L"\n\n\n\n\n\n\n\n\n\n\n\nBlue");
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
void automatedTelemetry(){

	if (g_frameCounter % 100 != 0) {
		return;
	}

	switch (g_step){
	case 1: {
		//start up time and time interval test data has been gathered by this point
		double startUpTime = getStartTime();

		initializeFile();
		g_dataFile << "Start Time" << "," << startUpTime << endl;
		g_timeTestResults.push_back(startUpTime);
		break;
	}
	case 2:{
		double timeAt1000;
		//gets time for regular simulation run from time=0 to time=100
		timeAt1000 = testRegularSpeed() - g_endStartTime;
		g_dataFile << "Time to run normally to 1000 days " << "," << timeAt1000 << endl;
		g_timeTestResults.push_back(timeAt1000);
		break;
	}
	case 3:{
		double jumpSpeedTime;
		//gets time to jump from time=0 to time=365 days
		jumpSpeedTime = testJumpTimeSpeed();

		g_dataFile << "Time to jump to 365 days:" << "," << jumpSpeedTime << endl;
		g_timeTestResults.push_back(jumpSpeedTime);
		break;
	}
	case 4:{
		double oneIterationPerFrame;
		double hundredIterationPerFrame;
		int initial = g_iterationsPerFrame;
		//gets time for one frame at one iteration per frame
		oneIterationPerFrame = testSpeed1IterationsPerFrame();
		//gets time for one frame at 100 iterations per frame
		hundredIterationPerFrame = testSpeed100IterationsPerFrame();
		g_iterationsPerFrame = initial;

		g_dataFile << "1 Frame @ 1 iteration/frame" << "," << oneIterationPerFrame << endl;
		g_dataFile << "1 Frame @ 100 iteration/frame" << "," << hundredIterationPerFrame << endl;
		g_timeTestResults.push_back(oneIterationPerFrame);
		g_timeTestResults.push_back(hundredIterationPerFrame);
		break;
	}
	case 5: {
		double winToFullTime;
		winToFullTime = getWinToFullTime();
		g_dataFile << "Win to FullScreen" << "," << winToFullTime << endl;
		g_timeTestResults.push_back(winToFullTime);
		break;
	}
	case 6: {
		double pauseFullScreenTime;
		pauseFullScreenTime = getPauseFullScreenTime();
		g_dataFile << "Pause Fullscreen" << "," << pauseFullScreenTime << endl;
		g_timeTestResults.push_back(pauseFullScreenTime);
		break;
	}
	case 7: {
		double unPauseFullScreenTime;
		unPauseFullScreenTime = getUnPauseFullScreenTime();
		g_dataFile << "Unpause Fullscreen" << "," << unPauseFullScreenTime << endl;
		g_timeTestResults.push_back(unPauseFullScreenTime);
		break;
	}
	case 8: {
		double fullToWinTime;
		fullToWinTime = getFullToWinTime();
		g_dataFile << "Full to Windowed" << "," << fullToWinTime << endl;
		g_timeTestResults.push_back(fullToWinTime);
		break;
	}
	case 9: {
		double pauseTime;
		pauseTime = getPauseTime();
		g_dataFile << "Pause" << "," << pauseTime << endl;
		g_timeTestResults.push_back(pauseTime);
		break;
	}
	case 10: {
		OnMouseEvent(true, false, false, false, false, 0, 400.000000, 298.000000, NULL);

		break;
	}
	case 12: {
		OnMouseEvent(true, false, false, false, false, 0, 439.000000, 242.000000, NULL);
		break;
	}
	case 13: {
		OnMouseEvent(true, false, false, false, false, 0, 487.000000, 313.000000, NULL);
		break;
	}
	case 14: {
		OnMouseEvent(true, false, false, false, false, 0, 456.000000, 316.000000, NULL);
		break;
	}
	case 15: {
		OnMouseEvent(true, false, false, false, false, 0, 469.000000, 470.000000, NULL);
		break;
	}
	case 16: {
		OnMouseEvent(true, false, false, false, false, 0, 262.000000, 332.000000, NULL);
		break;
	}
	case 17: {
		//needs to be collected after the call so method can go through onframemove
		double hitTestTime;
		hitTestTime = getHitTestTime();
		g_dataFile << "Mouse Click Hit Test Average" << "," << hitTestTime << endl;
		g_timeTestResults.push_back(hitTestTime);
		break;
	}
	case 18: {
		double unPauseTime;
		unPauseTime = getUnPauseTime();
		g_dataFile << "Unpause" << "," << unPauseTime << endl;
		g_timeTestResults.push_back(unPauseTime);
		break;
	}
	case 19: {
		double ResetParticlesTime = testResetParticles();
		g_dataFile << "Time to reset particles: " << "," << ResetParticlesTime << endl;
		g_timeTestResults.push_back(ResetParticlesTime);
		break;
	}
	case 20: {
		double ResetCameraTime = testResetCamera();
		g_dataFile << "Time to reset camera: " << "," << ResetCameraTime << endl;
		g_timeTestResults.push_back(ResetCameraTime);
		break;
	}
	case 21: {
		double averageFPS;
		if (g_averageFPSCounter != 0) {
			averageFPS = getAverageFPS();
		}

		LPCWSTR deviceStats = DXUTGetDeviceStats();
		wostringstream wss;
		wss << deviceStats;

		g_dataFile << "Average FPS" << "," << averageFPS << endl;
		g_dataFile << wss.str().c_str() << endl;

		getUsername();

		g_dataFile.close();

		system("dxdiag.exe/t dxdiag.txt");

		LPCWSTR grade = totalGrade(averageFPS);

		copyFile(grade, averageFPS);

		exit(0);

	}
	default:{

	}
	}

	g_step++;
	
		
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

	if (g_addingObject) {
		renderAddObjectText();
	}

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


	//runs during telemetry collection versions of application
	//method is called after the time interval test is completed
	//Elapsed time to 1000 days is not being recorded right now, but the value is being collected so the test runs long enough to 
	//collect an accurate FPS
	if (g_isTest && g_elapsedTimeAt1000Days != NULL) {
		automatedTelemetry();
	}


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




