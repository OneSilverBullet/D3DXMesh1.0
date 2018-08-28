//////////////////////////////////////////////////////////////////////////////////////////////////
// 
// File: d3dinit.cpp
// 
// Author: Frank Luna (C) All Rights Reserved
//
// System: AMD Athlon 1800+ XP, 512 DDR, Geforce 3, Windows XP, MSVC++ 7.0 
//
// Desc: Demonstrates how to initialize Direct3D, how to use the book's framework
//       functions, and how to clear the screen to black.  Note that the Direct3D
//       initialization code is in the d3dUtility.h/.cpp files.
//          
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "d3dUtility.h"
#include "cube.h"
#include<windows.h>
#include <vector>
#include <fstream>

using namespace std;
//
// Globals
//

IDirect3DDevice9* Device = 0;
const int width = 640;
const int height = 480;

ID3DXMesh* sourceMesh = 0;
ID3DXPMesh* pMesh = 0;
std::vector<D3DMATERIAL9> Mtrls(0);
std::vector<IDirect3DTexture9*> Textures(0);



void dumpVertices(std::ofstream& outFile, ID3DXMesh* mesh);
void dumpIndices(std::ofstream& outFile, ID3DXMesh* mesh);
void dumpAttributeBuffer(std::ofstream& outFile, ID3DXMesh* mesh);
void dumpAdjacencyBuffer(std::ofstream& outFile, ID3DXMesh* mesh);
void dumpAttributeTable(std::ofstream& outFile, ID3DXMesh* mesh);


struct Vertex
{
	Vertex(){}
	Vertex(float x, float y, float z,
		float nx, float ny, float nz,
		float u, float v)
	{
		_x = x;
		_y = y;
		_z = z;
		_nx = nx;
		_ny = ny;
		_nz = nz;
		_u = u;
		_v = v;
	}
	float _x, _y, _z;
	float _nx, _ny, _nz;
	float _u, _v;

	static const DWORD FVF;
};
const DWORD Vertex::FVF = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1;

bool Setup()
{
	//创造了可以存储12个面片，24个顶点的空网格。
	HRESULT hr = 0;

	ID3DXBuffer* adjBuffer = 0;
	ID3DXBuffer* mtrlBuffer = 0;
	DWORD numMtrls = 0;

	hr = D3DXLoadMeshFromX(
		"bigship1.x",
		D3DXMESH_MANAGED,
		Device,
		&adjBuffer,
		&mtrlBuffer,
		0,
		&numMtrls,
		&sourceMesh
	);

	if (FAILED(hr))
	{
		::MessageBox(0, "D3DLOADMESHRFromX is failed", 0, 0);
		return false;
	}

	if (mtrlBuffer!=0&&numMtrls!=0)
	{
		//当前加载材质其中环境光为无，因此在加载的时候进行赋值
		D3DXMATERIAL* mtrl = (D3DXMATERIAL*)mtrlBuffer->GetBufferPointer();
		for (int i =0; i<numMtrls; i++)
		{
			mtrl[i].MatD3D.Ambient = mtrl[i].MatD3D.Diffuse;
			Mtrls.push_back(mtrl[i].MatD3D);
			if (mtrl[i].pTextureFilename!=0)
			{
				IDirect3DTexture9* tex = 0;
				D3DXCreateTextureFromFile(
					Device,
					mtrl[i].pTextureFilename,
					&tex
				);
				Textures.push_back(tex);
			}
			else
			{
				Textures.push_back(0);
			}
		}
		d3d::Release<ID3DXBuffer*>(mtrlBuffer);
	}

	hr = D3DXGeneratePMesh(
		sourceMesh,
		(DWORD*)adjBuffer->GetBufferPointer(),
		0,
		0,
		1,
		D3DXMESHSIMP_FACE,
		&pMesh
	);
	d3d::Release<ID3DXMesh*>(sourceMesh);
	d3d::Release<ID3DXBuffer*>(adjBuffer);

	if (FAILED(hr))
	{
		::MessageBox(0, "D3DXGenerate failed", 0, 0);
		return false;
	}

	//设置采样器参数

	Device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	Device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	Device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);

	//关闭光源
	Device->SetRenderState(D3DRS_LIGHTING, true);

	D3DXVECTOR3 dir(1.0f, -1.0f, 1.0f);
	D3DXCOLOR col(1.0f, 1.0f, 1.0f,1.0f);
	D3DLIGHT9 light = d3d::InitDirectionalLight(&dir, &col);
	Device->SetLight(0, &light);
	Device->LightEnable(0, true);
	Device->SetRenderState(D3DRS_NORMALIZENORMALS, true);
	Device->SetRenderState(D3DRS_SPECULARENABLE, true);

	//设置摄像机视角
	D3DXVECTOR3 pos(0.0f, 0.f,-15.0f);
	D3DXVECTOR3 target(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 up(0.0f, 1.0f, 0.0f);
	D3DXMATRIX V;
	D3DXMatrixLookAtLH(&V, &pos, &target, &up);
	Device->SetTransform(D3DTS_VIEW, &V);

	//设置投影矩阵
	D3DXMATRIX proj;
	D3DXMatrixPerspectiveFovLH(
		&proj,
		D3DX_PI*0.5F,
		(float)width / (float)height,
		1.0f,
		1000.0f);
	Device->SetTransform(D3DTS_PROJECTION, &proj);


	return true;
}

void CleanUp()
{
	d3d::Release<ID3DXMesh*>(sourceMesh);
	d3d::Release<ID3DXPMesh*>(pMesh);


}

bool Display(float timedelta)
{
	if (Device)
	{
		int numFaces = pMesh->GetNumFaces();
		if (::GetAsyncKeyState('A')&0x8000f)
		{
			pMesh->SetNumFaces(numFaces + 1);
			if (pMesh->GetNumFaces() == numFaces)
			{
				pMesh->SetNumFaces(numFaces + 2);
			}
		}
		if (::GetAsyncKeyState('D') & 0x8000f)
		{
			pMesh->SetNumFaces(numFaces - 1);
		}
		static float y = 0.0f;
		D3DXMATRIX yRot;
		D3DXMatrixRotationY(&yRot, y);

		y += timedelta;
		if (y >=6.28)
		{
			y = 0.0f;
		}

		D3DXMATRIX world = yRot;
		Device->SetTransform(D3DTS_WORLD, &world);




		Device->Clear(0, 0,
			D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER ,
			0xffffffff, 1.0f, 0L);
		Device->BeginScene();

		for (int i=0; i<Mtrls.size(); i++)
		{
			Device->SetMaterial(&Mtrls[i]);
			Device->SetTexture(0, Textures[i]);
			pMesh->DrawSubset(i);

			Device->SetMaterial(&d3d::YELLOW_MTRL);
			Device->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
			pMesh->DrawSubset(i);
			Device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
		
		}

		Device->EndScene();
		Device->Present(0, 0, 0, 0);
	}
	return true;
}





LRESULT CALLBACK d3d::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_DESTROY:
		::PostQuitMessage(0);
		break;
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
		{
			::DestroyWindow(hwnd);
		}
		break;
	default:
		break;
	}
	return ::DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hinstance,
	HINSTANCE prevInstance,
	PSTR cmdLine,
	int showCmd)
{
	if (!d3d::InitD3D(hinstance,
		width, height, true, D3DDEVTYPE_HAL, &Device))
	{
		::MessageBox(0, "InitD3D() - FAILED", 0, 0);
		return 0;
	}

	if (!Setup())
	{
		::MessageBox(0, "Setup() - FAILED", 0, 0);
		return 0;
	}

	d3d::EnterMsgLoop(Display);

	CleanUp();

	Device->Release();

	return 0;
}

void dumpVertices(std::ofstream& outFile, ID3DXMesh* mesh)
{
	outFile << "Vertices:" << endl;
	outFile << "----------" << endl << endl;
	Vertex* v = 0;
	mesh->LockVertexBuffer(0, (void**)&v);
	for (int i=0; i<mesh->GetNumVertices();i++)
	{
		outFile << "Vertex" << i << ":(";
		outFile << v[i]._x << ", " << v[i]._y << ", " << v[i]._z << ", ";
		outFile << v[i]._nx << ", " << v[i]._ny << ", " << v[i]._nz << ", ";
		outFile << v[i]._u << ", " << v[i]._v << ")" << endl;
	}
	mesh->UnlockVertexBuffer();
	outFile << endl << endl;
}

void dumpIndices(std::ofstream& outFile, ID3DXMesh* mesh)
{
	outFile << "Indices:" << endl;
	outFile << "----------" << endl << endl;
	WORD* v = 0;
	mesh->LockIndexBuffer(0, (void**)&v);
	for (int i = 0; i < mesh->GetNumFaces(); i++)
	{
		outFile << "Triangle" << i << ":";
		outFile << v[i * 3] << " ";
		outFile << v[i * 3 + 1] << " ";
		outFile << v[i * 3 + 2] << endl;
	}
	mesh->UnlockIndexBuffer();
	outFile << endl << endl;
}

void dumpAttributeBuffer(std::ofstream& outFile, ID3DXMesh* mesh)
{
	outFile << "Triangle lives in subset:" << endl;
	outFile << "----------" << endl << endl;
	DWORD* attributeBuffer = 0;
	mesh->LockAttributeBuffer(0, &attributeBuffer);
	for (int i=0;i<mesh->GetNumFaces(); i++)
	{
		outFile << "Triangle lives in subset" << i << ": ";
		outFile << attributeBuffer[i] << endl;
	}
	mesh->UnlockAttributeBuffer();
	outFile << endl << endl;
}

void dumpAdjacencyBuffer(std::ofstream& outFile, ID3DXMesh* mesh)
{
	outFile << "Adjacency Buffer:" << endl;
	outFile << "----------" << endl << endl;

	vector<DWORD> adjacencyBuffer(mesh->GetNumFaces() * 3);

	mesh->GenerateAdjacency(0.0f, &adjacencyBuffer[0]);

	for (int i = 0; i < mesh->GetNumFaces(); i++)
	{
		outFile << "Triangle's adjacent to triangle" << i << ":";
		outFile << adjacencyBuffer[i * 3] << " ";
		outFile << adjacencyBuffer[i * 3 + 1] << " ";
		outFile << adjacencyBuffer[i * 3 + 2] << endl;
	}
	outFile << endl << endl;

}

void dumpAttributeTable(std::ofstream& outFile, ID3DXMesh* mesh)
{
	outFile << "Adjacency Buffer:" << endl;
	outFile << "----------" << endl << endl;

	DWORD numEntries = 0;

	mesh->GetAttributeTable(0, &numEntries);

	vector<D3DXATTRIBUTERANGE> table(numEntries);

	mesh->GetAttributeTable(&table[0], &numEntries);

	for (int i=0; i<numEntries; i++)
	{
		outFile << "Entry " << i << std::endl;
		outFile << "-----------" << std::endl;

		outFile << "Subset ID:    " << table[i].AttribId << std::endl;
		outFile << "Face Start:   " << table[i].FaceStart << std::endl;
		outFile << "Face Count:   " << table[i].FaceCount << std::endl;
		outFile << "Vertex Start: " << table[i].VertexStart << std::endl;
		outFile << "Vertex Count: " << table[i].VertexCount << std::endl;
		outFile << std::endl;
	}
	outFile << endl << endl;
}   