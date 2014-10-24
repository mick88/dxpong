#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include "globals.h"

class cMesh
{
	D3DMATERIAL9*		materials;
	DWORD				nMaterials;
	LPDIRECT3DTEXTURE9*	textures;
	LPDIRECT3DDEVICE9	d3ddev;
	float scalex, scaley, scalez;

public:
	cMesh(LPDIRECT3DDEVICE9 device, char * filename, char * folder);
	~cMesh(void);
	void LoadMesh(char * filename, char * folder);
	void renderMesh(float x, float y, float z, float rX, float rY, float rZ);
	LPD3DXMESH			mesh;
};

