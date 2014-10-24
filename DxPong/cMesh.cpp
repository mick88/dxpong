#include "cMesh.h"



cMesh::cMesh(LPDIRECT3DDEVICE9 device, char * filename, char * folder)
{
	d3ddev = device;
	scalex = 0.4f*(1.10f);
	scaley = 0.4f;
	scalez = 0.4f;
	nMaterials = 0;
	LoadMesh(filename, folder);
}


cMesh::~cMesh(void)
{
	SAFE_RELEASE(mesh);
	for (DWORD i=0; i < nMaterials; i++) 
	{
		SAFE_RELEASE(textures[i]);
	}
}

void cMesh::LoadMesh(char * filename, char * folder)
{
	LPD3DXBUFFER bMaterial;
	/*
	char * meshpath = new char[strlen(filename)+strlen(folder)+1];

	strcat(meshpath, folder);
	strcat(meshpath, filename);
	*/
	switch (
		D3DXLoadMeshFromX(filename, D3DXMESH_SYSTEMMEM, d3ddev, NULL, &bMaterial, NULL, &nMaterials, &mesh)
		) 
		{
			case D3DERR_INVALIDCALL: MessageBox(NULL, "Invalid call", "ERROR", MB_OK | MB_ICONERROR);
				break;
			case E_OUTOFMEMORY: MessageBox(NULL, "Out of memory", "ERROR", MB_OK | MB_ICONERROR);
				break;
			case D3D_OK:
				break;
			default: 
				MessageBox(NULL, "Unknowon error!", "ERROR", MB_OK | MB_ICONERROR);
		}
	//delete meshpath;

	D3DXMATERIAL * tmpMaterials = (D3DXMATERIAL *) bMaterial->GetBufferPointer();

	materials = new D3DMATERIAL9[nMaterials];
	textures = new LPDIRECT3DTEXTURE9[nMaterials];

	for (DWORD i=0; i < nMaterials; i++)
	{
		materials[i] = tmpMaterials[i].MatD3D;
		materials[i].Ambient = materials[i].Diffuse;
		textures[i] = NULL;
		if (tmpMaterials[i].pTextureFilename)
		{
			D3DXCreateTextureFromFile(d3ddev, tmpMaterials[i].pTextureFilename, &textures[i]);
		}
	}
	bMaterial->Release();
}

void cMesh::renderMesh(float x, float y, float z, float rX, float rY, float rZ)
{
	for (DWORD i=0; i < nMaterials; i++)
	{
		D3DXMATRIX matTranslateA;
		D3DXMatrixTranslation(&matTranslateA, x, y, z);

		D3DXMATRIX matRotateXA;
		D3DXMatrixRotationX(&matRotateXA, D3DXToRadian(rX));

		D3DXMATRIX matRotateYA;
		D3DXMatrixRotationY(&matRotateYA, D3DXToRadian(rY));

		D3DXMATRIX matRotateZA;
		D3DXMatrixRotationZ(&matRotateZA, D3DXToRadian(rZ));

		D3DXMATRIX matScaleA;
		D3DXMatrixScaling(&matScaleA, scalex, scaley, scalez);
		d3ddev->SetTransform(D3DTS_WORLD, &(matRotateXA * matRotateYA * matRotateZA * matScaleA * matTranslateA));
		
		d3ddev->SetTexture(0, textures[i]);
		if (&materials[i] != NULL) d3ddev->SetMaterial(&materials[i]);
		
		mesh->DrawSubset(i);
	}
}