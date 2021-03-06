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
#define SAFE_RELEASE(I)         do { if (I){ I->Release(); } I = NULL; } while(0) 

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
	string name;
	float mass;
	float diameter;
	int brightness;
};


PARTICLE*                           g_pParticleArray = NULL;
PARTICLE_DETAILS*					g_pParticleArrayTWO = NULL;


//will point to arrays that hold info from file until it is put into above two arrays
string* name;
float* mass;
float* diameter;
int* brightness;
float* xcoord;
float* ycoord;
float* zcoord;
const int maxnamesize = 200;



//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
#define IDC_TOGGLEFULLSCREEN    1
#define IDC_TOGGLEREF           3
#define IDC_CHANGEDEVICE        4
#define IDC_RESETPARTICLES      5

//--------------------------------------------------------------------------------------
// Forward declarations 
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext );
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext );
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                         void* pUserContext );
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext );

bool CALLBACK IsD3D11DeviceAcceptable( const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo,
                                      DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext );
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
                                     void* pUserContext );
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
                                         const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext );
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext );
void CALLBACK OnD3D11DestroyDevice( void* pUserContext );
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime,
                                 float fElapsedTime, void* pUserContext );

void InitApp();
void RenderText();
//declare WriteAttributes function here, content is below ParseFile()
HRESULT WriteAttributes(IXmlReader* pReader);

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
//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );
    DXUTSetCallbackMsgProc( MsgProc );
    DXUTSetCallbackFrameMove( OnFrameMove );

    DXUTSetCallbackD3D11DeviceAcceptable( IsD3D11DeviceAcceptable );
    DXUTSetCallbackD3D11DeviceCreated( OnD3D11CreateDevice );
    DXUTSetCallbackD3D11SwapChainResized( OnD3D11ResizedSwapChain );
    DXUTSetCallbackD3D11FrameRender( OnD3D11FrameRender );
    DXUTSetCallbackD3D11SwapChainReleasing( OnD3D11ReleasingSwapChain );
    DXUTSetCallbackD3D11DeviceDestroyed( OnD3D11DestroyDevice );

	ParseFile();

    InitApp();

    DXUTInit( true, true );                 // Use this line instead to try to create a hardware device

    DXUTSetCursorSettings( true, true ); // Show the cursor and clip it when in full screen
    DXUTCreateWindow( L"GalaxySim" );
    DXUTCreateDevice( D3D_FEATURE_LEVEL_10_0, true, 800, 600 );
    DXUTMainLoop();                      // Enter into the DXUT render loop

    return DXUTGetExitCode();
}


//--------------------------------------------------------------------------------------
// Initialize the app 
//--------------------------------------------------------------------------------------
void InitApp()
{
    g_D3DSettingsDlg.Init( &g_DialogResourceManager );
    g_HUD.Init( &g_DialogResourceManager );
    g_SampleUI.Init( &g_DialogResourceManager );

    g_HUD.SetCallback( OnGUIEvent ); int iY = 10;
	g_HUD.AddButton(IDC_TOGGLEFULLSCREEN, L"Full screen (F6)", 0, iY, 170, 23, VK_F6);
    g_HUD.AddButton( IDC_TOGGLEREF, L"Toggle REF (F3)", 0, iY += 26, 170, 23, VK_F3 );
    g_HUD.AddButton( IDC_CHANGEDEVICE, L"Change device (F2)", 0, iY += 26, 170, 23, VK_F2 );
    g_HUD.AddButton( IDC_RESETPARTICLES, L"Reset particles (F4)", 0, iY += 26, 170, 22, VK_F4 );
    g_SampleUI.SetCallback( OnGUIEvent ); 
}

//--------------------------------------------------------------------------------------
// Parse File 
//--------------------------------------------------------------------------------------



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
HRESULT CreateParticleBuffer( ID3D11Device* pd3dDevice )
{
    HRESULT hr = S_OK;

    D3D11_BUFFER_DESC vbdesc =
    {
        MAX_PARTICLES * sizeof( PARTICLE_VERTEX ),
        D3D11_USAGE_DEFAULT,
        D3D11_BIND_VERTEX_BUFFER,
        0,
        0
    };
    D3D11_SUBRESOURCE_DATA vbInitData;
    ZeroMemory( &vbInitData, sizeof( D3D11_SUBRESOURCE_DATA ) );

    auto pVertices = new PARTICLE_VERTEX[ MAX_PARTICLES ];
    if( !pVertices )
        return E_OUTOFMEMORY;

    for( UINT i = 0; i < MAX_PARTICLES; i++ )
    {
        pVertices[i].Color = XMFLOAT4( 0.2f, 0.2f, 0.8f, 1 );
    }

    vbInitData.pSysMem = pVertices;
    V_RETURN( pd3dDevice->CreateBuffer( &vbdesc, &vbInitData, &g_pParticleBuffer ) );
    DXUT_SetDebugName( g_pParticleBuffer, "Particles" );
    SAFE_DELETE_ARRAY( pVertices );

    return hr;
}


//--------------------------------------------------------------------------------------
float RPercent()
{
    float ret = ( float )( ( rand() % 10000 ) - 5000 );
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

//--------------------------------------------------------------------------------------
// Functions used to load particles (overarching one is fillParticles2) other ones support it
//--------------------------------------------------------------------------------------

void fillPosVel(PARTICLE particles[], float x[], float y[], float z[], XMFLOAT4 Velocity){
	//method that loads g_pParticleArray with position as a vector and velocity (that is being updated constantly?)
	//create and then call here a getVelocity methods 

	for (int i = 0; i < NUM_PARTICLES; i++){
		particles[i].pos = createPositionFloat(x[i], y[i], z[i]);
		particles[i].velo = Velocity;
	}
}

void fillDetails(PARTICLE_DETAILS particles2[], string nameIn[], float massIn[], float diameterIn[], int brightnessIn[]){
	for (int i = 0; i < NUM_PARTICLES; i++){
		particles2[i].name = nameIn[i];
		particles2[i].mass = massIn[i];
		particles2[i].diameter = diameterIn[i];
		particles2[i].brightness = brightnessIn[i];
	}
}

void fillParticles2(PARTICLE particles[], PARTICLE_DETAILS particles2[], float x[], float y[], float z[], XMFLOAT4 Velocity, string nameIn[], float massIn[], float diameterIn[], int brightnessIn[]){
	fillPosVel(particles, x, y, z, Velocity);
	fillDetails(particles2, nameIn, massIn, diameterIn, brightnessIn);
	SAFE_DELETE_ARRAY(name);
	SAFE_DELETE_ARRAY(mass);
	SAFE_DELETE_ARRAY(diameter);
	SAFE_DELETE_ARRAY(brightness);
	SAFE_DELETE_ARRAY(xcoord);
	SAFE_DELETE_ARRAY(ycoord);
	SAFE_DELETE_ARRAY(zcoord);
}

//--------------------------------------------------------------------------------------
HRESULT CreateParticlePosVeloBuffers( ID3D11Device* pd3dDevice )
{
    HRESULT hr = S_OK;

    D3D11_BUFFER_DESC desc;
    ZeroMemory( &desc, sizeof(desc) );
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

    srand( (unsigned int)GetTickCount64() );   

    // Disk Galaxy Formation
    //float fCenterSpread = g_fSpread * 0.50f;
    /*LoadParticles(g_pParticleArray,
        XMFLOAT3( fCenterSpread, 0, 0 ), XMFLOAT4( 0, 0, 0, 1 ),
        g_fSpread, NUM_PARTICLES );*/

	float xcoord[] = { -200, -100, 0, 100, 200 };
	float ycoord[] = { 100, 200, 300, 400, 500 };
	float zcoord[] = { 100, 200, 300, 400, 500 };

	fillParticles2(g_pParticleArray, g_pParticleArrayTWO, xcoord, ycoord, zcoord, XMFLOAT4(0, 0, 0, 1), name, mass, diameter, brightness);

    D3D11_SUBRESOURCE_DATA InitData;
    InitData.pSysMem = g_pParticleArray;
    V_RETURN( pd3dDevice->CreateBuffer( &desc, &InitData, &g_pParticlePosVelo0 ) );
    V_RETURN( pd3dDevice->CreateBuffer( &desc, &InitData, &g_pParticlePosVelo1 ) );
    DXUT_SetDebugName( g_pParticlePosVelo0, "PosVelo0" );
    DXUT_SetDebugName( g_pParticlePosVelo1, "PosVelo1" );

    D3D11_SHADER_RESOURCE_VIEW_DESC DescRV;
    ZeroMemory( &DescRV, sizeof( DescRV ) );
    DescRV.Format = DXGI_FORMAT_UNKNOWN;
    DescRV.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    DescRV.Buffer.FirstElement = 0;
    DescRV.Buffer.NumElements = desc.ByteWidth / desc.StructureByteStride;
    V_RETURN( pd3dDevice->CreateShaderResourceView( g_pParticlePosVelo0, &DescRV, &g_pParticlePosVeloRV0 ) );
    V_RETURN( pd3dDevice->CreateShaderResourceView( g_pParticlePosVelo1, &DescRV, &g_pParticlePosVeloRV1 ) );
    DXUT_SetDebugName( g_pParticlePosVeloRV0, "PosVelo0 SRV" );
    DXUT_SetDebugName( g_pParticlePosVeloRV1, "PosVelo1 SRV" );

    return hr;
}


//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
    return true;
}


//--------------------------------------------------------------------------------------
// This callback function will be called once at the beginning of every frame. This is the
// best location for your application to handle updates to the scene, but is not 
// intended to contain actual rendering calls, which should instead be placed in the 
// OnFrameRender callback.  
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
    if(fElapsedTime < SECONDS_PER_FRAME)
    {
       Sleep(static_cast<DWORD>((SECONDS_PER_FRAME - fElapsedTime) * 1000.0f));
    }

    auto pd3dImmediateContext = DXUTGetD3D11DeviceContext();
    
    D3D11_MAPPED_SUBRESOURCE ms;
    pd3dImmediateContext->Map(g_pParticlePosVelo0, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);

    for (int i = 0; i < NUM_PARTICLES; i++)
    {
        // Insert gravity calculations here
        //g_pParticleArray[i].pos.x -= 2.0f;
		//move each object's button
		
    }

    memcpy(ms.pData, g_pParticleArray, sizeof(PARTICLE) * NUM_PARTICLES);
       
    pd3dImmediateContext->Unmap(g_pParticlePosVelo0, NULL);

    std::swap( g_pParticlePosVelo0, g_pParticlePosVelo1 );
    std::swap( g_pParticlePosVeloRV0, g_pParticlePosVeloRV1 );

    // Update the camera's position based on user input 
    g_Camera.FrameMove( fElapsedTime );
}


//--------------------------------------------------------------------------------------
// Before handling window messages, DXUT passes incoming windows 
// messages to the application through this callback function. If the application sets 
// *pbNoFurtherProcessing to TRUE, then DXUT will not process this message.
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                         void* pUserContext )
{
    // Pass messages to dialog resource manager calls so GUI state is updated correctly
    *pbNoFurtherProcessing = g_DialogResourceManager.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    // Pass messages to settings dialog if its active
    if( g_D3DSettingsDlg.IsActive() )
    {
        g_D3DSettingsDlg.MsgProc( hWnd, uMsg, wParam, lParam );
        return 0;
    }

    // Give the dialogs a chance to handle the message first
    *pbNoFurtherProcessing = g_HUD.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;
    *pbNoFurtherProcessing = g_SampleUI.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    // Pass all windows messages to camera so it can respond to user input
    g_Camera.HandleMessages( hWnd, uMsg, wParam, lParam );

    return 0;
}


//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext )
{
    switch( nControlID )
    {
    case IDC_TOGGLEFULLSCREEN:
        DXUTToggleFullScreen(); break;
    case IDC_TOGGLEREF:
        DXUTToggleREF(); break;
    case IDC_CHANGEDEVICE:
        g_D3DSettingsDlg.SetActive( !g_D3DSettingsDlg.IsActive() ); break;

    case IDC_RESETPARTICLES:
        {
            SAFE_RELEASE(g_pParticlePosVelo0);
            SAFE_RELEASE(g_pParticlePosVelo1);
            SAFE_RELEASE(g_pParticlePosVeloRV0);
            SAFE_RELEASE(g_pParticlePosVeloRV1);
            CreateParticlePosVeloBuffers(DXUTGetD3D11Device());
            break;
        }
    }
}


//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D11DeviceAcceptable( const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo,
                                      DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
    // reject any device which doesn't support CS4x
    if ( DeviceInfo->ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x == FALSE )
        return false;
    
    return true;
}


//--------------------------------------------------------------------------------------
// Create any D3D11 resources that aren't dependant on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
                                     void* pUserContext )
{
    HRESULT hr;

    static bool bFirstOnCreateDevice = true;

    // Warn the user that in order to support CS4x, a non-hardware device has been created, continue or quit?
    if ( DXUTGetDeviceSettings().d3d11.DriverType != D3D_DRIVER_TYPE_HARDWARE && bFirstOnCreateDevice )
    {
        if ( MessageBox( 0, L"CS4x capability is missing. "\
                            L"In order to continue, a non-hardware device has been created, "\
                            L"it will be very slow, continue?", L"Warning", MB_ICONEXCLAMATION | MB_YESNO ) != IDYES )
            return E_FAIL;
    }

    CWaitDlg CompilingShadersDlg;
    CompilingShadersDlg.ShowDialog( L"Compiling Shaders..." );

    bFirstOnCreateDevice = false;

    D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS ho;
    V_RETURN( pd3dDevice->CheckFeatureSupport( D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, &ho, sizeof(ho) ) );

    auto pd3dImmediateContext = DXUTGetD3D11DeviceContext();
    V_RETURN( g_DialogResourceManager.OnD3D11CreateDevice( pd3dDevice, pd3dImmediateContext ) );
    V_RETURN( g_D3DSettingsDlg.OnD3D11CreateDevice( pd3dDevice ) );
    g_pTxtHelper = new CDXUTTextHelper( pd3dDevice, pd3dImmediateContext, &g_DialogResourceManager, 15 );

    ID3DBlob* pBlobRenderParticlesVS = nullptr;
    ID3DBlob* pBlobRenderParticlesGS = nullptr;
    ID3DBlob* pBlobRenderParticlesPS = nullptr;

    // Create the shaders
    V_RETURN( DXUTCompileFromFile( L"GalaxyDraw.hlsl", nullptr, "VSGalaxyDraw", "vs_4_0",
                                   D3DCOMPILE_ENABLE_STRICTNESS, 0, &pBlobRenderParticlesVS ) );
    V_RETURN( DXUTCompileFromFile( L"GalaxyDraw.hlsl", nullptr, "GSGalaxyDraw", "gs_4_0",
                                   D3DCOMPILE_ENABLE_STRICTNESS, 0, &pBlobRenderParticlesGS ) );
    V_RETURN( DXUTCompileFromFile( L"GalaxyDraw.hlsl", nullptr, "PSGalaxyDraw", "ps_4_0",
                                   D3DCOMPILE_ENABLE_STRICTNESS, 0, &pBlobRenderParticlesPS ) );

    V_RETURN( pd3dDevice->CreateVertexShader( pBlobRenderParticlesVS->GetBufferPointer(), pBlobRenderParticlesVS->GetBufferSize(), nullptr, &g_pRenderParticlesVS ) );
    DXUT_SetDebugName( g_pRenderParticlesVS, "VSGalaxyDraw" );

    V_RETURN( pd3dDevice->CreateGeometryShader( pBlobRenderParticlesGS->GetBufferPointer(), pBlobRenderParticlesGS->GetBufferSize(), nullptr, &g_pRenderParticlesGS ) );
    DXUT_SetDebugName( g_pRenderParticlesGS, "GSGalaxyDraw" );

    V_RETURN( pd3dDevice->CreatePixelShader( pBlobRenderParticlesPS->GetBufferPointer(), pBlobRenderParticlesPS->GetBufferSize(), nullptr, &g_pRenderParticlesPS ) );
    DXUT_SetDebugName( g_pRenderParticlesPS, "PSGalaxyDraw" );


    // Create our vertex input layout
    const D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    V_RETURN( pd3dDevice->CreateInputLayout( layout, sizeof( layout ) / sizeof( layout[0] ),
        pBlobRenderParticlesVS->GetBufferPointer(), pBlobRenderParticlesVS->GetBufferSize(), &g_pParticleVertexLayout ) );
    DXUT_SetDebugName( g_pParticleVertexLayout, "Particles" );
    
    SAFE_RELEASE( pBlobRenderParticlesVS );
    SAFE_RELEASE( pBlobRenderParticlesGS );
    SAFE_RELEASE( pBlobRenderParticlesPS );

    V_RETURN( CreateParticleBuffer( pd3dDevice ) );
    V_RETURN( CreateParticlePosVeloBuffers( pd3dDevice ) );

    // Setup constant buffer
    D3D11_BUFFER_DESC Desc;
    Desc.Usage = D3D11_USAGE_DYNAMIC;
    Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    Desc.MiscFlags = 0;
    Desc.ByteWidth = sizeof( CB_GS );
    V_RETURN( pd3dDevice->CreateBuffer( &Desc, nullptr, &g_pcbGS ) );
    DXUT_SetDebugName( g_pcbGS, "CB_GS" );

    Desc.ByteWidth = sizeof( CB_CS );
    V_RETURN( pd3dDevice->CreateBuffer( &Desc, nullptr, &g_pcbCS ) );
    DXUT_SetDebugName( g_pcbCS, "CB_CS" );

    // Load the Particle Texture
    V_RETURN( DXUTCreateShaderResourceViewFromFile( pd3dDevice, L"misc\\Particle.dds", &g_pParticleTexRV ) );

    D3D11_SAMPLER_DESC SamplerDesc;
    ZeroMemory( &SamplerDesc, sizeof(SamplerDesc) );
    SamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    SamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    SamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    SamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    V_RETURN( pd3dDevice->CreateSamplerState( &SamplerDesc, &g_pSampleStateLinear ) );
    DXUT_SetDebugName( g_pSampleStateLinear, "Linear" );

    D3D11_BLEND_DESC BlendStateDesc;
    ZeroMemory( &BlendStateDesc, sizeof(BlendStateDesc) );
    BlendStateDesc.RenderTarget[0].BlendEnable = TRUE;
    BlendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    BlendStateDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    BlendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
    BlendStateDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    BlendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
    BlendStateDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    BlendStateDesc.RenderTarget[0].RenderTargetWriteMask = 0x0F;
    V_RETURN( pd3dDevice->CreateBlendState( &BlendStateDesc, &g_pBlendingStateParticle ) );
    DXUT_SetDebugName( g_pBlendingStateParticle, "Blending" );

    D3D11_DEPTH_STENCIL_DESC DepthStencilDesc;
    ZeroMemory( &DepthStencilDesc, sizeof(DepthStencilDesc) );
    DepthStencilDesc.DepthEnable = FALSE;
    DepthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    pd3dDevice->CreateDepthStencilState( &DepthStencilDesc, &g_pDepthStencilState );
    DXUT_SetDebugName( g_pDepthStencilState, "DepthOff" );

    // Setup the camera's view parameters

    XMVECTOR vecEye = XMVectorSet( 0, 0, -g_fSpread * 3, 0.f );
    g_Camera.SetViewParams( vecEye, g_XMZero );

    CompilingShadersDlg.DestroyDialog();

    return S_OK;
}


//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
                                         const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
    HRESULT hr;

    V_RETURN( g_DialogResourceManager.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );
    V_RETURN( g_D3DSettingsDlg.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );

    // Setup the camera's projection parameters
    float fAspectRatio = pBackBufferSurfaceDesc->Width / ( FLOAT )pBackBufferSurfaceDesc->Height;
    g_Camera.SetProjParams( XM_PI / 4, fAspectRatio, 10.0f, 500000.0f );
    g_Camera.SetWindow( pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height );
    g_Camera.SetButtonMasks( 0, MOUSE_WHEEL, MOUSE_LEFT_BUTTON | MOUSE_MIDDLE_BUTTON | MOUSE_RIGHT_BUTTON );

    g_HUD.SetLocation( pBackBufferSurfaceDesc->Width - 170, 0 );
    g_HUD.SetSize( 170, 170 );
    g_SampleUI.SetLocation( pBackBufferSurfaceDesc->Width - 170, pBackBufferSurfaceDesc->Height - 300 );
    g_SampleUI.SetSize( 170, 300 );

    return hr;
}


//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext )
{
    g_DialogResourceManager.OnD3D11ReleasingSwapChain();    
}


//--------------------------------------------------------------------------------------
void RenderText()
{
    g_pTxtHelper->Begin();
    g_pTxtHelper->SetInsertionPos( 2, 0 );
    g_pTxtHelper->SetForegroundColor( Colors::Yellow );
    g_pTxtHelper->DrawTextLine( DXUTGetFrameStats( DXUTIsVsyncEnabled() ) );
    g_pTxtHelper->DrawTextLine( DXUTGetDeviceStats() );
    g_pTxtHelper->End();
}


//--------------------------------------------------------------------------------------
bool RenderParticles( ID3D11DeviceContext* pd3dImmediateContext, CXMMATRIX mView, CXMMATRIX mProj )
{
    ID3D11BlendState *pBlendState0 = nullptr;
    ID3D11DepthStencilState *pDepthStencilState0 = nullptr;
    UINT SampleMask0, StencilRef0;
    XMFLOAT4 BlendFactor0;
    pd3dImmediateContext->OMGetBlendState( &pBlendState0, &BlendFactor0.x, &SampleMask0 );
    pd3dImmediateContext->OMGetDepthStencilState( &pDepthStencilState0, &StencilRef0 );

    pd3dImmediateContext->VSSetShader( g_pRenderParticlesVS, nullptr, 0 );
    pd3dImmediateContext->GSSetShader( g_pRenderParticlesGS, nullptr, 0 );
    pd3dImmediateContext->PSSetShader( g_pRenderParticlesPS, nullptr, 0 );
    
    pd3dImmediateContext->IASetInputLayout( g_pParticleVertexLayout );

    // Set IA parameters
    ID3D11Buffer* pBuffers[1] = { g_pParticleBuffer };
    UINT stride[1] = { sizeof( PARTICLE_VERTEX ) };
    UINT offset[1] = { 0 };
    pd3dImmediateContext->IASetVertexBuffers( 0, 1, pBuffers, stride, offset );
    pd3dImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_POINTLIST );

    ID3D11ShaderResourceView* aRViews[ 1 ] = { g_pParticlePosVeloRV0 };
    pd3dImmediateContext->VSSetShaderResources( 0, 1, aRViews );

    D3D11_MAPPED_SUBRESOURCE MappedResource;
    pd3dImmediateContext->Map( g_pcbGS, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
    auto pCBGS = reinterpret_cast<CB_GS*>( MappedResource.pData );
    XMStoreFloat4x4( &pCBGS->m_WorldViewProj, XMMatrixMultiply( mView, mProj ) ); 
    XMStoreFloat4x4( &pCBGS->m_InvView, XMMatrixInverse( nullptr, mView ) );
    pd3dImmediateContext->Unmap( g_pcbGS, 0 );
    pd3dImmediateContext->GSSetConstantBuffers( 0, 1, &g_pcbGS );

    pd3dImmediateContext->PSSetShaderResources( 0, 1, &g_pParticleTexRV );
    pd3dImmediateContext->PSSetSamplers( 0, 1, &g_pSampleStateLinear );

    float bf[] = { 0.f, 0.f, 0.f, 0.f };
    pd3dImmediateContext->OMSetBlendState( g_pBlendingStateParticle, bf, 0xFFFFFFFF  );
    pd3dImmediateContext->OMSetDepthStencilState( g_pDepthStencilState, 0 );

    pd3dImmediateContext->Draw( NUM_PARTICLES, 0 );

    ID3D11ShaderResourceView* ppSRVNULL[1] = { nullptr };
    pd3dImmediateContext->VSSetShaderResources( 0, 1, ppSRVNULL );
    pd3dImmediateContext->PSSetShaderResources( 0, 1, ppSRVNULL );

    /*ID3D11Buffer* ppBufNULL[1] = { nullptr };
    pd3dImmediateContext->GSSetConstantBuffers( 0, 1, ppBufNULL );*/

    pd3dImmediateContext->GSSetShader( nullptr, nullptr, 0 );
    pd3dImmediateContext->OMSetBlendState( pBlendState0, &BlendFactor0.x, SampleMask0 ); SAFE_RELEASE(pBlendState0);
    pd3dImmediateContext->OMSetDepthStencilState( pDepthStencilState0, StencilRef0 ); SAFE_RELEASE(pDepthStencilState0);

    return true;
}


//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime,
                                 float fElapsedTime, void* pUserContext )
{
    // If the settings dialog is being shown, then render it instead of rendering the app's scene
    if( g_D3DSettingsDlg.IsActive() )
    {
        g_D3DSettingsDlg.OnRender( fElapsedTime );
        return;
    }
    
    ID3D11RenderTargetView* pRTV = DXUTGetD3D11RenderTargetView();
    pd3dImmediateContext->ClearRenderTargetView( pRTV, Colors::Black );
    ID3D11DepthStencilView* pDSV = DXUTGetD3D11DepthStencilView();
    pd3dImmediateContext->ClearDepthStencilView( pDSV, D3D11_CLEAR_DEPTH, 1.0, 0 );

    // Get the projection & view matrix from the camera class
    XMMATRIX mProj = g_Camera.GetProjMatrix();
    XMMATRIX mView = g_Camera.GetViewMatrix();

    // Render the particles
    RenderParticles( pd3dImmediateContext, mView, mProj );

    DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR, L"HUD / Stats" );
    g_HUD.OnRender( fElapsedTime );
    g_SampleUI.OnRender( fElapsedTime );
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
void CALLBACK OnD3D11DestroyDevice( void* pUserContext )
{
    g_DialogResourceManager.OnD3D11DestroyDevice();
    g_D3DSettingsDlg.OnD3D11DestroyDevice();
    DXUTGetGlobalResourceCache().OnDestroyDevice();
    SAFE_DELETE( g_pTxtHelper );
    SAFE_DELETE_ARRAY( g_pParticleArray );
	SAFE_DELETE_ARRAY(g_pParticleArrayTWO);

    SAFE_RELEASE( g_pParticleBuffer ); 
    SAFE_RELEASE( g_pParticleVertexLayout );

    SAFE_RELEASE( g_pParticlePosVelo0 );
    SAFE_RELEASE( g_pParticlePosVelo1 );
    SAFE_RELEASE( g_pParticlePosVeloRV0 );
    SAFE_RELEASE( g_pParticlePosVeloRV1 );

    SAFE_RELEASE( g_pcbGS );
    SAFE_RELEASE( g_pcbCS );

    SAFE_RELEASE( g_pParticleTexRV );

    SAFE_RELEASE( g_pRenderParticlesVS );
    SAFE_RELEASE( g_pRenderParticlesGS );
    SAFE_RELEASE( g_pRenderParticlesPS );
    SAFE_RELEASE( g_pSampleStateLinear );
    SAFE_RELEASE( g_pBlendingStateParticle );
    SAFE_RELEASE( g_pDepthStencilState );
}
