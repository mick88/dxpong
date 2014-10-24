#include <Windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <dinput.h>
#include <math.h>
#include <stdio.h>
#include <time.h>
#include "cMesh.h"

#pragma comment (lib, "d3d9.lib")
#pragma comment (lib, "d3dx9.lib")
#pragma comment (lib, "dinput8.lib")
#pragma comment (lib, "dxguid.lib")
#pragma comment (lib, "WINMM.LIB")

#define DESKTOP_WIDTH	1680
#define DESKTOP_HEIGHT	1050

#define SCREEN_WIDTH	1680
#define SCREEN_HEIGHT	1050

#define FIELD_WIDTH		60					//X
#define FIELD_HEIGHT	40					//Y
#define TOP_EDGE		(FIELD_HEIGHT/2) 
#define BOTTOM_EDGE		(-FIELD_HEIGHT/2)
#define LEFT_EDGE		(-FIELD_WIDTH/2)
#define RIGHT_EDGE		(FIELD_WIDTH/2)

#define MAX_BALLS				10
#define BALL_SIZE				1.3f //ball diameter
#define BALL_RADIUS				(BALL_SIZE/2)
#define BALL_SPEED				0.8f
#define INIT_BALLS				3 //initial number of balls

#define PADDLE_WIDTH		8.0f
#define PADDLE_THICKNESS	1.0f
#define PADDLE_HEIGHT		BALL_SIZE
#define LEFT_PADDLE			0
#define RIGHT_PADDLE		1
#define MAX_PADDLE_SPEED	1.7f
#define PADDLE_ACCELERATION	0.17f
#define PADDLE_DECELERATION (PADDLE_ACCELERATION*1.5f)
#define PADDLE_BOUNCE		0.5f	//defines how much paddles bounce off the walls

#define MAX_CAMERA_DIST			350
#define MIN_CAMERA_DIST			50

#define MAX_X_ANGLE				60.0f
#define MIN_X_ANGLE				0.01f
#define MAX_Z_ANGLE				290.0f//300.0f
#define MIN_Z_ANGLE				70.0f//60.0f

#define MAX_BILLBOARD_TEXTURES	7
#define BILLBOARD_WIDTH			5.0f
#define BILLBOARD_HEIGHT		1.25f

#define SCREEN_LEFT		((DESKTOP_WIDTH/2) - (SCREEN_WIDTH/2))
#define SCREEN_TOP		((DESKTOP_HEIGHT/2) - (SCREEN_HEIGHT/2))

#define CUSTOMFVF		(D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1)

LPDIRECT3D9				d3d;		//DirectX
LPDIRECT3DDEVICE9		d3ddev;		//DirectX device
LPDIRECT3DVERTEXBUFFER9 vFieldBuffer;	//Vertex buffer
LPDIRECT3DINDEXBUFFER9	iFieldBuffer;
LPDIRECT3DVERTEXBUFFER9	vPaddleBuffer;
LPDIRECT3DINDEXBUFFER9	iPaddleBuffer;
LPDIRECT3DTEXTURE9		paddleTexture;
LPDIRECT3DTEXTURE9		billboardTexture[MAX_BILLBOARD_TEXTURES];
LPDIRECT3DTEXTURE9		fieldTexture;
LPD3DXMESH				sphereMesh;
LPDIRECTINPUT8			din;
LPDIRECTINPUTDEVICE8	dinKeyboard;
LPDIRECTINPUTDEVICE8	dinMouse;
BYTE					keyState[256];
DIMOUSESTATE 			mouseState;
LPD3DXFONT				HudFont;


float cameraX=50.0f,
		cameraZ=50.0f,
		cameraY=50.0f,
		cameraFOV=30,
		cameraAngle=180.0f,
		cameraAngle2=25.0f,
		cameraDist=115.0f,
		
		ballX=0,
		ballY=0,
		ballZ=0;


struct CUSTOMVERTEX
{
	FLOAT x, y, z;
	D3DVECTOR NORMAL;
	FLOAT U, V;
};

enum CONTROLLER {CPU, HUMAN1, HUMAN2};

struct PADDLE
{
	FLOAT x, y, z,
		velocity;
	int score;
	CONTROLLER controlledBy;
};

enum OBJECT_TYPE {ObjNone, ObjPaddle, ObjBall, ObjField};
enum MOVE_TYPE	{moveCameraPos, moveCameraLookAt};

struct VIEW
{
	//FLOAT			cx, cy, cz;			//camera pos
	FLOAT			lx, ly, lz;			//camera look at - centerpoint
	FLOAT			cAngleX, cAngleZ;	//angle
	FLOAT			distance;			//distance
	//OBJECT_TYPE		cameraMount, cameraFollow;
	//MOVE_TYPE		moveType;
};


struct BILLBOARD
{
	FLOAT x, y, zAngle;
};


BILLBOARD billboards[] = 
	{
		//at the back
		{RIGHT_EDGE-(BILLBOARD_WIDTH), TOP_EDGE, 0.0f},
		{RIGHT_EDGE-(BILLBOARD_WIDTH*3), TOP_EDGE, 0.0f},
		{RIGHT_EDGE-(BILLBOARD_WIDTH*5), TOP_EDGE, 0.0f},
		{RIGHT_EDGE-(BILLBOARD_WIDTH*7), TOP_EDGE, 0.0f},
		{RIGHT_EDGE-(BILLBOARD_WIDTH*9), TOP_EDGE, 0.0f},
		{RIGHT_EDGE-(BILLBOARD_WIDTH*11), TOP_EDGE, 0.0f},

		//left side
		{RIGHT_EDGE, BOTTOM_EDGE+(BILLBOARD_WIDTH), 270.0f},
		{RIGHT_EDGE, BOTTOM_EDGE+(BILLBOARD_WIDTH*3), 270.0f},
		{RIGHT_EDGE, BOTTOM_EDGE+(BILLBOARD_WIDTH*5), 270.0f},
		{RIGHT_EDGE, BOTTOM_EDGE+(BILLBOARD_WIDTH*7), 270.0f},

		//right side
		{LEFT_EDGE, BOTTOM_EDGE+(BILLBOARD_WIDTH), 90.0f},
		{LEFT_EDGE, BOTTOM_EDGE+(BILLBOARD_WIDTH*3), 90.0f},
		{LEFT_EDGE, BOTTOM_EDGE+(BILLBOARD_WIDTH*5), 90.0f},
		{LEFT_EDGE, BOTTOM_EDGE+(BILLBOARD_WIDTH*7), 90.0f},

		//at the front
		{RIGHT_EDGE-(BILLBOARD_WIDTH), BOTTOM_EDGE, 180.0f},
		{RIGHT_EDGE-(BILLBOARD_WIDTH*3), BOTTOM_EDGE, 180.0f},
		{RIGHT_EDGE-(BILLBOARD_WIDTH*5), BOTTOM_EDGE, 180.0f},
		{RIGHT_EDGE-(BILLBOARD_WIDTH*7), BOTTOM_EDGE, 180.0f},
		{RIGHT_EDGE-(BILLBOARD_WIDTH*9), BOTTOM_EDGE, 180.0f},
		{RIGHT_EDGE-(BILLBOARD_WIDTH*11), BOTTOM_EDGE, 180.0f},
	};

#define N_OF_BILLBOARDS (sizeof(billboards)/sizeof(BILLBOARD))

struct BALL
{
	FLOAT x, y, z,
			direction, speed;
	bool	spawned,
			created;
};

BALL balls[MAX_BALLS];

PADDLE paddles[2];
cMesh * stadium;

void initD3D(HWND hWnd);
void renderFrame();
void releaseD3D();
void initGraphics();
void initLight();
void initDinput(HINSTANCE hInstance, HWND hWnd);
void detectInput();
void cleanDinput();
void drawText(char*, int*, int *, bool);
void initGame();
void updateGame(float);
float getAngleDifference(float, float, bool);

LRESULT CALLBACK ProcMessages(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);


/********************************************************************************
									Program part
*/
int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hPrevInstance, LPSTR CmdLine, int nShowCmd)
{
	HWND hWnd;
	WNDCLASSEX wc;
	ZeroMemory(&wc, sizeof(WNDCLASSEX));

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = ProcMessages;
	wc.hInstance = hinstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.lpszClassName = "WindowClass1";

	RegisterClassEx(&wc);

	hWnd = CreateWindowEx(NULL, "WindowClass1", "DX Pong", 
		//WS_EX_TOPMOST | WS_POPUP,
		WS_OVERLAPPEDWINDOW, 
		SCREEN_LEFT, SCREEN_TOP, SCREEN_WIDTH, SCREEN_HEIGHT, NULL, NULL, hinstance, NULL);

	ShowWindow(hWnd, nShowCmd);

	srand(time(NULL));
	initD3D(hWnd);	
	initDinput(hinstance, hWnd);
	initLight();
	initGraphics();
	initGame();

	MSG msg;
	float  t = (float)timeGetTime();
	while(true)
	{
		float t2 = (float)timeGetTime();
		while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		if (msg.message == WM_QUIT) break;

		detectInput();
		
		float step = (t2-t) * 0.03f;
		updateGame(step);
		
		renderFrame();

		t = t2;
	}
	releaseD3D();
	return msg.wParam;
}

LRESULT CALLBACK ProcMessages(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
			break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

/*****************************************************************
						DirectX part
*/

void initD3D(HWND hWnd)
{
	d3d = Direct3DCreate9(D3D_SDK_VERSION);

	D3DPRESENT_PARAMETERS d3pp;

	ZeroMemory(&d3pp, sizeof(d3pp));
	d3pp.Windowed = false;
	d3pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3pp.hDeviceWindow = hWnd;
	d3pp.BackBufferFormat = D3DFMT_X8R8G8B8;
	d3pp.BackBufferWidth = SCREEN_WIDTH;
	d3pp.BackBufferHeight = SCREEN_HEIGHT;
	d3pp.EnableAutoDepthStencil = true;
	d3pp.AutoDepthStencilFormat = D3DFMT_D16;
	d3pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

	d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3pp, &d3ddev);
	d3ddev->SetRenderState(D3DRS_LIGHTING, true);
	d3ddev->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
	//d3ddev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	d3ddev->SetRenderState(D3DRS_ZENABLE, true);
	d3ddev->SetRenderState(D3DRS_AMBIENT, 0x404040);
	d3ddev->SetRenderState(D3DRS_NORMALIZENORMALS, true);
}

#define PI 3.14f
float toRadians(float degrees)
{
	return degrees * PI / 180;
}

float toDegrees(float radians)
{
	return radians * 180 / PI;
}

int createBall(int n=1)
{
	int id=-1;
	for (int i=0; i < MAX_BALLS; i++)
	{
		if (balls[i].created == false)
		{
			id = i;
			break;
		}
	}
	if (id != -1)
	{
		balls[id].x = 0.0f;
		balls[id].y = 0.0f;
		balls[id].created = true;
		balls[id].speed = BALL_SPEED;
		balls[id].direction = rand() % 360;
	}
	if (--n > 0) createBall(n);
	return id;
}

void renderFrame()
{
	d3ddev->Clear(0, NULL, D3DCLEAR_TARGET, 0x000000, 1.0f, 0);
	d3ddev->Clear(0, NULL, D3DCLEAR_ZBUFFER, 0x000000, 1.0f, 0);
	d3ddev->BeginScene();

		//DRAWING STARTS HERE
		d3ddev->SetFVF(CUSTOMFVF);

		//PIPELINE

		//Transform View
		D3DXMATRIX matCamera;

		cameraZ = cos(toRadians(cameraAngle2)) * cameraDist;
		float cameraZDist = sin(toRadians(cameraAngle2)) * cameraDist;

		cameraX = sin(toRadians(cameraAngle))*cameraZDist;
		cameraY = cos(toRadians(cameraAngle))*cameraZDist;
		D3DXMatrixLookAtLH(&matCamera, 
							&D3DXVECTOR3(cameraX, cameraY, cameraZ),	//camera position
							&D3DXVECTOR3(0, 0, 0),			//camera look at
							&D3DXVECTOR3(0.0f, 0.0f, 1.0f)
						);
		d3ddev->SetTransform(D3DTS_VIEW, &matCamera);

		D3DXMATRIX matView;
		D3DXMatrixPerspectiveFovLH(&matView, D3DXToRadian(cameraFOV), //FOV
					(FLOAT)SCREEN_WIDTH/(FLOAT)SCREEN_HEIGHT, //Aspect ratio
					1.0f, //front plane
					1000.0f//back plane
					);
		d3ddev->SetTransform(D3DTS_PROJECTION, &matView);	

		float angle=0, dist=20;
			
		stadium->renderMesh(0, 0, 0.65, 90, 0, 270);
	/*		DRAWING BACKGROUND - FIELD		*/
	{ 

		D3DXMATRIX matTranslateA;
		D3DXMatrixTranslation(&matTranslateA, 0, 0, 0);

		D3DXMATRIX matRotateXA;
		D3DXMatrixRotationX(&matRotateXA, D3DXToRadian(0));

		D3DXMATRIX matRotateYA;
		D3DXMatrixRotationY(&matRotateYA, D3DXToRadian(0));

		D3DXMATRIX matRotateZA;
		D3DXMatrixRotationZ(&matRotateZA, D3DXToRadian(0));

		D3DXMATRIX matScaleA;
		D3DXMatrixScaling(&matScaleA, FIELD_WIDTH/2, FIELD_HEIGHT/2, 1);

		d3ddev->SetTransform(D3DTS_WORLD, &(matRotateXA * matRotateYA * matRotateZA * matScaleA * matTranslateA));

		d3ddev->SetStreamSource(0, vFieldBuffer, 0, sizeof(CUSTOMVERTEX));
		d3ddev->SetIndices(iFieldBuffer);
		
		d3ddev->SetTexture(0, fieldTexture);
		d3ddev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 4, 0, 2);
	}

	/*		DRAWING BILLBOARDS		*/
	for (int i=0; i < N_OF_BILLBOARDS; i++)
	{
		int texture = i % MAX_BILLBOARD_TEXTURES;		

		D3DXMATRIX matTranslateA;
		D3DXMatrixTranslation(&matTranslateA, billboards[i].x, billboards[i].y, BILLBOARD_HEIGHT);

		D3DXMATRIX matRotateXA;
		D3DXMatrixRotationX(&matRotateXA, D3DXToRadian(90));

		D3DXMATRIX matRotateYA;
		D3DXMatrixRotationY(&matRotateYA, D3DXToRadian(0));

		D3DXMATRIX matRotateZA;
		D3DXMatrixRotationZ(&matRotateZA, D3DXToRadian(billboards[i].zAngle));

		D3DXMATRIX matScaleA;
		D3DXMatrixScaling(&matScaleA, BILLBOARD_WIDTH, 1, BILLBOARD_HEIGHT);

		d3ddev->SetTransform(D3DTS_WORLD, &(matRotateXA * matRotateYA * matScaleA * matRotateZA * matTranslateA));

		d3ddev->SetStreamSource(0, vFieldBuffer, 0, sizeof(CUSTOMVERTEX));
		d3ddev->SetIndices(iFieldBuffer);
		
		d3ddev->SetTexture(0, billboardTexture[texture]);
		d3ddev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 4, 0, 2);
	}
	/*		DRAWING PADDLES		*/
	for (int i=0; i < 2; i++)
	{
		D3DXMATRIX matScaleA;
		D3DXMatrixScaling(&matScaleA, PADDLE_THICKNESS/2, PADDLE_WIDTH/2, PADDLE_HEIGHT/2);

		D3DXMATRIX matTranslateA;
		D3DXMatrixTranslation(&matTranslateA, paddles[i].x, paddles[i].y, paddles[i].z);

		d3ddev->SetTransform(D3DTS_WORLD, &(matScaleA * matTranslateA));
		d3ddev->SetStreamSource(0, vPaddleBuffer, 0, sizeof(CUSTOMVERTEX));
			d3ddev->SetIndices(iPaddleBuffer);
		d3ddev->SetTexture(0, paddleTexture);
		d3ddev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 24, 0, 12);
	}

	/*		DRAWING BALLS		*/
	for (int i=0; i < MAX_BALLS; i++) if (balls[i].spawned == true)
	{

		D3DXMATRIX matTranslateA;
		D3DXMatrixTranslation(&matTranslateA, balls[i].x, balls[i].y, BALL_RADIUS);

		D3DXMATRIX matRotateXA;
		D3DXMatrixRotationX(&matRotateXA, D3DXToRadian(90));

		D3DXMATRIX matRotateYA;
		D3DXMatrixRotationY(&matRotateYA, D3DXToRadian(0));

		D3DXMATRIX matRotateZA;
		D3DXMatrixRotationZ(&matRotateZA, D3DXToRadian(0));

		D3DXMATRIX matScaleA;
		D3DXMatrixScaling(&matScaleA, BALL_SIZE, BALL_SIZE, BALL_SIZE);
		d3ddev->SetTransform(D3DTS_WORLD, &(matRotateXA * matRotateYA * matRotateZA * matScaleA * matTranslateA));
		d3ddev->SetTexture(0, NULL);
		
		sphereMesh->DrawSubset(0);
	}

	int a=2, b=2;
	char string[100];
	sprintf_s(string, "%d - %d", paddles[RIGHT_PADDLE].score, paddles[LEFT_PADDLE].score);
	drawText(string, &a, &b, true);
	sprintf_s(string, "CAM: xAng=%.2f zAng=%.2f dist=%.2f", cameraAngle2, cameraAngle, cameraDist);
	drawText(string, &a, &b, false);
	
	d3ddev->EndScene();
	d3ddev->Present(NULL, NULL, NULL, NULL);
}

void drawText(char * s, int * x, int * y, bool centered)
{
	RECT r;
	r.left = *x;
	r.right = SCREEN_WIDTH-*x;
	r.top = *y;
	r.bottom = SCREEN_HEIGHT-*y;

	HudFont->DrawText(NULL, s, -1, &r, centered?DT_CENTER:0, 0xFFFFFFFF);
	(*y)+=24;
}

void releaseD3D()
{
	d3d->Release();
	d3ddev->Release();
	vFieldBuffer->Release();

	paddleTexture->Release();
	fieldTexture->Release();

	for (int i=0; i < MAX_BILLBOARD_TEXTURES; i++) if (billboardTexture[i] != NULL) billboardTexture[i]->Release();

	sphereMesh->Release();
	cleanDinput();
	HudFont->Release();
}

void initGraphics()
{
	/*	CREATING FIELD	*/
	CUSTOMVERTEX fieldVertices[] =  //for field and other rectangular objects
	{		
		{1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0, 0},
		{-1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1, 0},
		{1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0, 1},
		{-1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1, 1},
	};

	short fieldIndices[]=
	{
		0, 1, 2,
		2, 1, 3,
	};


	d3ddev->CreateVertexBuffer(sizeof(fieldVertices), 0, CUSTOMFVF, D3DPOOL_MANAGED, &vFieldBuffer, NULL);
	d3ddev->CreateIndexBuffer(sizeof(fieldIndices), 0, D3DFMT_INDEX16, D3DPOOL_MANAGED, &iFieldBuffer, NULL);
	
	VOID * pVoid;

	vFieldBuffer->Lock(0, 0, (void**) &pVoid, 0);
	memcpy(pVoid, fieldVertices, sizeof(fieldVertices));
	vFieldBuffer->Unlock();
	
	iFieldBuffer->Lock(0, 0, (void **) &pVoid, 0);
	memcpy(pVoid, fieldIndices, sizeof(fieldIndices));
	iFieldBuffer->Unlock();

	stadium = new cMesh(d3ddev, "stadium.x", "stadium\\");

	/*	CREATING PADDLES	*/
	CUSTOMVERTEX vPaddle[] =
	{
		{ -1.0f, -1.0f, 1.0f,  0.0f, 0.0f, 1.0f, 1, 0},    // side 1
        { 1.0f, -1.0f, 1.0f,  0.0f, 0.0f, 1.0f, 0, 0},
        { -1.0f, 1.0f, 1.0f,  0.0f, 0.0f, 1.0f, 1, 1},
        { 1.0f, 1.0f, 1.0f,  0.0f, 0.0f, 1.0f, 0, 1},

        { -1.0f, -1.0f, -1.0f,  0.0f, 0.0f, -1.0f, 1, 0},    // side 2
        { -1.0f, 1.0f, -1.0f,  0.0f, 0.0f, -1.0f, 0, 0},
        { 1.0f, -1.0f, -1.0f,  0.0f, 0.0f, -1.0f, 1, 1},
        { 1.0f, 1.0f, -1.0f,  0.0f, 0.0f, -1.0f, 0, 1},

        { -1.0f, 1.0f, -1.0f,  0.0f, 1.0f, 0.0f, 1, 0},    // side 3
        { -1.0f, 1.0f, 1.0f,  0.0f, 1.0f, 0.0f, 0, 0},
        { 1.0f, 1.0f, -1.0f,  0.0f, 1.0f, 0.0f, 1, 1},
        { 1.0f, 1.0f, 1.0f,  0.0f, 1.0f, 0.0f, 0, 1},

        { -1.0f, -1.0f, -1.0f,  0.0f, -1.0f, 0.0f, 1, 0},    // side 4
        { 1.0f, -1.0f, -1.0f,  0.0f, -1.0f, 0.0f, 0, 0},
        { -1.0f, -1.0f, 1.0f,  0.0f, -1.0f, 0.0f, 1, 1},
        { 1.0f, -1.0f, 1.0f,  0.0f, -1.0f, 0.0f, 0, 1},

        { 1.0f, -1.0f, -1.0f,  1.0f, 0.0f, 0.0f, 1, 0},    // side 5
        { 1.0f, 1.0f, -1.0f,  1.0f, 0.0f, 0.0f, 0, 0},
        { 1.0f, -1.0f, 1.0f,  1.0f, 0.0f, 0.0f, 1, 1},
        { 1.0f, 1.0f, 1.0f,  1.0f, 0.0f, 0.0f, 0, 1},

        { -1.0f, -1.0f, -1.0f,  -1.0f, 0.0f, 0.0f, 1, 0},    // side 6
        { -1.0f, -1.0f, 1.0f,  -1.0f, 0.0f, 0.0f, 0, 0},
        { -1.0f, 1.0f, -1.0f,  -1.0f, 0.0f, 0.0f, 1, 1},
        { -1.0f, 1.0f, 1.0f,  -1.0f, 0.0f, 0.0f, 0, 1},
	};

	short iPaddle[] =
	{
        0, 1, 2,    // side 1
        2, 1, 3,
        4, 5, 6,    // side 2
        6, 5, 7,
        8, 9, 10,    // side 3
        10, 9, 11,
        12, 13, 14,    // side 4
        14, 13, 15,
        16, 17, 18,    // side 5
        18, 17, 19,
        20, 21, 22,    // side 6
        22, 21, 23,
	};

	d3ddev->CreateVertexBuffer(sizeof(vPaddle), 0, CUSTOMFVF, D3DPOOL_MANAGED, &vPaddleBuffer, NULL);
	d3ddev->CreateIndexBuffer(sizeof(iPaddle), 0, D3DFMT_INDEX16, D3DPOOL_MANAGED, &iPaddleBuffer, NULL);

	vPaddleBuffer->Lock(0, 0, (void**) &pVoid, 0);
	memcpy(pVoid, vPaddle, sizeof(vPaddle));
	vPaddleBuffer->Unlock();
	
	iPaddleBuffer->Lock(0, 0, (void **) &pVoid, 0);
	memcpy(pVoid, iPaddle, sizeof(iPaddle));
	iPaddleBuffer->Unlock();

	D3DXCreateFont(d3ddev, 24, 0, FW_BOLD, 0, false, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH || FF_DONTCARE,
		TEXT("Courier New"), &HudFont);

	/*	LOADING BALL	*/
	D3DXCreateSphere(d3ddev, 0.5f, 30, 30, &sphereMesh, NULL);
	//D3DXCreateTeapot(d3ddev, &sphereMesh, NULL); /*Teapot LOL*/
	
	/*	LOADING TEXTURES	*/
	D3DXCreateTextureFromFile(d3ddev, "field.jpg", &fieldTexture);			//ground texture
	D3DXCreateTextureFromFile(d3ddev, "aluminium.jpg", &paddleTexture);		//paddle texture
	D3DXCreateTextureFromFile(d3ddev, "banners\\sop.bmp", &billboardTexture[0]);
	D3DXCreateTextureFromFile(d3ddev, "banners\\ctbanner.bmp", &billboardTexture[1]);	
	D3DXCreateTextureFromFile(d3ddev, "banners\\dit.bmp", &billboardTexture[2]);
	D3DXCreateTextureFromFile(d3ddev, "banners\\earth.jpg", &billboardTexture[3]);
	D3DXCreateTextureFromFile(d3ddev, "banners\\ctbanner.bmp", &billboardTexture[4]);	
	D3DXCreateTextureFromFile(d3ddev, "banners\\dollar.BMP", &billboardTexture[5]);
	D3DXCreateTextureFromFile(d3ddev, "banners\\ireland.BMP", &billboardTexture[6]);
	
	//D3DXCreateTextureFromFile(d3ddev, "", &billboardTexture[]);
}

void initLight()
{
	D3DLIGHT9 light;
	D3DMATERIAL9 material;

	ZeroMemory(&light, sizeof(D3DLIGHT9));
	light.Type = D3DLIGHT_DIRECTIONAL;
	light.Position = D3DXVECTOR3(0.0f, 0.0f, 10.0f);
	light.Direction = D3DXVECTOR3(0.0f, 0.5f, -0.5f);
	light.Diffuse = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
	light.Phi = D3DXToRadian(40.0f);
	light.Theta = D3DXToRadian(20.0f);
	light.Falloff = 1.0f;
	light.Range = 100.0f;
	light.Attenuation0 = 0.0f;
	light.Attenuation1 = 0.125f;
	light.Attenuation2 = 0.0f;

	d3ddev->SetLight(0, &light);
	d3ddev->LightEnable(0, true);

	ZeroMemory(&material, sizeof(D3DMATERIAL9));
	material.Diffuse = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
	material.Ambient = D3DXCOLOR(1.0f,  1.0f, 1.0f, 0.5f);

	d3ddev->SetMaterial(&material);
}

void initDinput(HINSTANCE hInstance, HWND hWnd)
{
	DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&din, NULL);

	din->CreateDevice(GUID_SysKeyboard, &dinKeyboard, NULL);
	dinKeyboard->SetDataFormat(&c_dfDIKeyboard);
	dinKeyboard->SetCooperativeLevel(hWnd, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND | DISCL_NOWINKEY);

	din->CreateDevice(GUID_SysMouse, &dinMouse, NULL);
	dinMouse->SetDataFormat(&c_dfDIMouse);
	dinMouse->SetCooperativeLevel(hWnd, DISCL_EXCLUSIVE | DISCL_FOREGROUND);
}

float distanceSquared(float x1, float y1, float x2, float y2)
{
	return pow(x1-x2, 2)+pow(y1-y2, 2);
}

int getNearestHeadingBall(int toPaddle)
{
	int nearest=-1;
	float dist=100000;//distanceSquared(balls[0].x, balls[0].y, paddles[toPaddle].x, paddles[toPaddle].y);

	for (int i=0; i<MAX_BALLS; i++) 
		if (balls[i].spawned && getAngleDifference(balls[i].direction, (toPaddle==LEFT_PADDLE)?(270):90, true) < 85)
	{
		int d = distanceSquared(balls[i].x, balls[i].y, paddles[toPaddle].x, paddles[toPaddle].y);
		if (d < dist)
		{
			dist=d;
			nearest=i;
		}
	}

	return nearest;
}

void detectInput()
{
	dinKeyboard->Acquire();
	dinKeyboard->GetDeviceState(256, (LPVOID)keyState);

	dinMouse->Acquire();
	dinMouse->GetDeviceState(sizeof(DIMOUSESTATE), (LPVOID)&mouseState);

	/*
	if (keyState[DIK_A] & 0x80)
	{
		//cameraX -= 1;
		cameraAngle -= 0.5f;
	}
	else if (keyState[DIK_D] & 0x80)
	{
		//cameraX += 1;
		cameraAngle += 0.5f;
	}
	*/
	if (keyState[DIK_ESCAPE] & 0x80)
	{
		PostQuitMessage(0);
	}

	cameraAngle -= mouseState.lX * 0.1f;
	cameraAngle2 -= mouseState.lY * 0.1f;
	cameraDist += mouseState.lZ*0.07f;

	if (cameraDist > MAX_CAMERA_DIST) cameraDist = MAX_CAMERA_DIST;
	else if (cameraDist < MIN_CAMERA_DIST) cameraDist = MIN_CAMERA_DIST;

	if (cameraAngle2 > MAX_X_ANGLE) cameraAngle2=MAX_X_ANGLE;
	else if (cameraAngle2 < MIN_X_ANGLE) cameraAngle2 = MIN_X_ANGLE;

	if (cameraAngle > MAX_Z_ANGLE) cameraAngle = MAX_Z_ANGLE;
	else if (cameraAngle < MIN_Z_ANGLE) cameraAngle = MIN_Z_ANGLE;
}

void cleanDinput()
{
	dinKeyboard->Unacquire();
	din->Release();
	dinMouse->Unacquire();
}

/***********************************************************************
					GAME LOGIC
*/

void initGame()
{
	for (int i=0; i<2; i++)
	{
		paddles[i].y = 0;
		paddles[i].z = PADDLE_HEIGHT/2;
		paddles[i].x = i==LEFT_PADDLE?LEFT_EDGE+(PADDLE_THICKNESS/2):RIGHT_EDGE-(PADDLE_THICKNESS/2);
		paddles[i].controlledBy = i==RIGHT_PADDLE? HUMAN1 : CPU ;
		paddles[i].velocity=0.0f;
		paddles[i].score=0;
	}
	createBall(INIT_BALLS);
}

float getAngleDifference(float a1, float a2, bool absolute=false)
{
	int b1=(int)a1, b2=(int)a2;
    float diff = absolute ? abs(b1 - b2)%360 : (b1 - b2)%360;
    if (diff > 180) diff= (360 - diff);
    else if (diff < -180) diff= (-360 + diff);            

    return diff;            
}

void bounceBall(int ball, int angle, bool heading=false)
{
            
	if (heading && getAngleDifference(balls[ball].direction, angle, true) >= 90.0f) 
    {
        return;
    }
             
    angle = (angle - 90) % 360;
    angle -= getAngleDifference(balls[ball].direction, angle);

    if (angle < 0) angle += 360;
    balls[ball].direction = (angle) % 360;
}

float getAngleTo(float fromx, float fromy, float tox, float toy)
{
    float diffx = tox-fromx;
    float diffy = toy-fromy;
	float result =  toDegrees((float)tan(diffx / diffy));
    if (fromx < tox)
    {
        if (fromy > toy) result += 180;
        else result += 0;
    }
    else
    {
        if (fromy > toy) result += 180;
        else result += 360;
    }
    return result;
}

bool detectBallCollisions(int b)
{
	float x = balls[b].x,
			y = balls[b].y;
	//bounce off screen edges
	if (y >= TOP_EDGE-BALL_RADIUS) 
	{
		bounceBall(b, 0, true);
		return true;
	}
	else if (y <= BOTTOM_EDGE+BALL_RADIUS) 
	{
		bounceBall(b, 180, true);
		return true;
	}

	//bounce off the paddles
	//float paddleDist = (PADDLE_THICKNESS/2)+BALL_RADIUS;
	for (int i=0; i<2; i++)
	{	
		float angle = -1.0f;
		if (y >= paddles[i].y-(PADDLE_WIDTH/2) && y <= paddles[i].y+(PADDLE_WIDTH/2)) //check for collision with face
		{
			if (abs(x-paddles[i].x+(PADDLE_THICKNESS/2)) <= BALL_RADIUS) angle=90.0f;
			else if (abs(x-paddles[i].x-(PADDLE_THICKNESS/2)) <= BALL_RADIUS) angle=270.0f;
		}
		else if (x >= paddles[i].x-(PADDLE_THICKNESS/2) && x <= paddles[i].x+(PADDLE_THICKNESS/2)) //check for collision with sides
		{
			if (abs(y-(paddles[i].y+(PADDLE_WIDTH/2))) <= BALL_RADIUS) 
			{
				angle=180.0f;
				//balls[b].y=paddles[i].y+(PADDLE_WIDTH/2);
			}
			else if (abs(y-(paddles[i].y-(PADDLE_WIDTH/2))) <= BALL_RADIUS) 
			{
				angle=0.0f;
				//balls[b].y=paddles[i].y-(PADDLE_WIDTH/2);
			}
			/*else if (y > paddles[i].y-(PADDLE_WIDTH/2) && y < paddles[i].y+(PADDLE_WIDTH/2)) //ball stuck inside
			{
				if (y > paddles[i].y) balls[b].y = paddles[i].y+(PADDLE_WIDTH/2);
				else balls[b].y = paddles[i].y-(PADDLE_WIDTH/2);
			}*/
		}
		else //check corners
		{
			int min = BALL_RADIUS * BALL_RADIUS;
			if (distanceSquared(x, y, paddles[i].x+(PADDLE_THICKNESS/2), paddles[i].y+(PADDLE_WIDTH/2)) <= min)
			{
				angle = getAngleTo(x, y, paddles[i].x+(PADDLE_THICKNESS/2), paddles[i].y+(PADDLE_WIDTH/2));
			}
			else if (distanceSquared(x, y, paddles[i].x+(PADDLE_THICKNESS/2), paddles[i].y-(PADDLE_WIDTH/2)) <= min)
			{
				angle = getAngleTo(x, y, paddles[i].x+(PADDLE_THICKNESS/2), paddles[i].y-(PADDLE_WIDTH/2));
			}
			else if (distanceSquared(x, y, paddles[i].x-(PADDLE_THICKNESS/2), paddles[i].y-(PADDLE_WIDTH/2)) <= min)
			{
				angle = getAngleTo(x, y, paddles[i].x-(PADDLE_THICKNESS/2), paddles[i].y-(PADDLE_WIDTH/2));
			}
			else if (distanceSquared(x, y, paddles[i].x-(PADDLE_THICKNESS/2), paddles[i].y+(PADDLE_WIDTH/2)) <= min)
			{
				angle = getAngleTo(x, y, paddles[i].x-(PADDLE_THICKNESS/2), paddles[i].y+(PADDLE_WIDTH/2));
			}
		}

		if (angle > -1.0f) bounceBall(b, angle, true);
	}

	//with other balls
	float min = BALL_SIZE * BALL_SIZE;	//distance squared
	for (int i=0; i < MAX_BALLS; i++) if (balls[i].spawned && i != b)
	{
		if (distanceSquared(x, y, balls[i].x, balls[i].y) <= min)
		{
			float angle = getAngleTo(x, y, balls[i].x, balls[i].y);
			bounceBall(b, angle, true);
			return true;
		}
	}


}

bool findBallInRange(float x, float y, float range)
{
	range *= range;
	for (int i=0; i < MAX_BALLS; i++) if (balls[i].spawned == true)
	{
		if (distanceSquared(x, y, balls[i].x, balls[i].y) < range) return true;
	}
	return false;
}

void fclamp(float * val, float min, float max)
{
	if (*val > max) *val = max;
	else if (*val < min) *val = min;
}

void updateGame(float t)
{
	/*		SIMULATE BALLS		*/
	for(int i=0; i<MAX_BALLS; i++) if (balls[i].created)
	{
		if (balls[i].spawned == false && !findBallInRange(balls[i].x, balls[i].y, BALL_RADIUS*2))
		{
			balls[i].x = 0;
			balls[i].y = 0;
			balls[i].direction = rand() % 360;
			balls[i].spawned = true;
		}

		if (balls[i].spawned == true)
		{
			float x = balls[i].x + (sin(toRadians(balls[i].direction)) * balls[i].speed * t);
			float y = balls[i].y + (cos(toRadians(balls[i].direction)) * balls[i].speed * t);

			{
				balls[i].x = x;
				balls[i].y = y;
			}

			detectBallCollisions(i);
			
			//remove ball if its outside of the screen and add score
			if (balls[i].y > TOP_EDGE+BALL_RADIUS || balls[i].y < BOTTOM_EDGE-BALL_RADIUS || balls[i].x > RIGHT_EDGE+BALL_RADIUS || balls[i].x < LEFT_EDGE-BALL_RADIUS) 
			{
				if (balls[i].x > RIGHT_EDGE) paddles[LEFT_PADDLE].score++;
				if (balls[i].x < LEFT_EDGE) paddles[RIGHT_PADDLE].score++;
				balls[i].spawned=false;
			}
		}
	}

	/*		DETECT PLAYER INPUT & MOVE PADDLES		*/
	for (int i=0; i < 2; i++)
	{
		float go=0;

		switch(paddles[i].controlledBy)
		{
			case HUMAN1:
			{
				if ((keyState[DIK_W] & 0x80) || (keyState[DIK_A] & 0x80)) go=1;
				else if ((keyState[DIK_S] & 0x80) || (keyState[DIK_D] & 0x80)) go=-1;
				break;
			}
			case HUMAN2:
			{
				if ((keyState[DIK_UP] & 0x80) || (keyState[DIK_RIGHT] & 0x80)) go=1;
				else if ((keyState[DIK_DOWN] & 0x80) || (keyState[DIK_LEFT] & 0x80)) go=-1;
				break;
			}
			case CPU:
			{
				int ball = getNearestHeadingBall(i);

				if (ball == -1) go=0;
				else if (balls[ball].y > paddles[i].y) go=1;
				else if (balls[ball].y < paddles[i].y) go=-1;
				//else go = (balls[ball].y-paddles[i].y);
				break;
			}
			
		}

		if (go != 0 && (paddles[i].velocity==0 || (paddles[i].velocity < 0 && go < 0) || (paddles[i].velocity > 0 && go > 0))) 
		{
			paddles[i].velocity += PADDLE_ACCELERATION * go * t;
			if (paddles[i].velocity > MAX_PADDLE_SPEED) paddles[i].velocity = MAX_PADDLE_SPEED;
			else if (paddles[i].velocity < -MAX_PADDLE_SPEED) paddles[i].velocity = -MAX_PADDLE_SPEED;
		}
		else //go==0
		{
			if (paddles[i].velocity > PADDLE_DECELERATION) paddles[i].velocity -= PADDLE_DECELERATION * t;
			else if (paddles[i].velocity < -PADDLE_DECELERATION) paddles[i].velocity += PADDLE_DECELERATION * t;
			else paddles[i].velocity = 0;
		}
	}

	//move paddles
	for (int i=0; i<2; i++)
	{
		paddles[i].y += paddles[i].velocity*t;
		if (paddles[i].y > TOP_EDGE-(PADDLE_WIDTH/2)) 
		{
			paddles[i].y = TOP_EDGE-(PADDLE_WIDTH/2);
			paddles[i].velocity *= -PADDLE_BOUNCE;
		}
		else if (paddles[i].y < BOTTOM_EDGE+(PADDLE_WIDTH/2)) 
		{
			paddles[i].y = BOTTOM_EDGE+(PADDLE_WIDTH/2);
			paddles[i].velocity *= -PADDLE_BOUNCE;
		}
	}
}