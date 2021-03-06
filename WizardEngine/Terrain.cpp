#include "Terrain.h"

Terrain::Terrain()
{
	m_vertexBuffer = 0;
	m_IndexBuffer = 0;
	m_terrainFile = 0;
	m_HeightMap = 0;
	m_Model = 0;
}

Terrain::Terrain(const Terrain &)
{
}


Terrain::~Terrain()
{
}

bool Terrain::InitialiseTerrain(ID3D11Device* device, char* file)
{
	bool result;

	//Setup the Terrain from the File
	result = LoadSetupFile(file);
	if (!result)
	{
		return false;
	}

	//Initialise the Results
	result = LoadBitmapHeightMap();
	if (!result)
	{
		return false;
	}

	//Setup the Terrain by converting to positions and scaling by the scale factor
	SetTerrainCoordinates();

	//Build the model for the Terrain
	result = BuildTerrain();
	if (!result)
	{
		return false;
	}

	//The heightmap has been turned into a model, so delete it
	ShutdownHeightMap();

	//Load the Rendering buffers with the terrain object
	result = InitialiseBuffer(device);
	if (!result)
	{
		return false;
	}

	//Release the model
	ShutdownTerrain();

	return true;
}

void Terrain::ShutDown()
{

	//Doublecheck that everything has been released

	ShutdownBuffer();

	ShutdownTerrain();

	ShutdownHeightMap();

	return;
}

void Terrain::Render(ID3D11DeviceContext* context)
{
	RenderBuffers(context);
}

int Terrain::GetIndexCount()
{
	return m_nIndexCount;
}

float Terrain::GetHeight(float x, float y)
{
	return Heights[x][y];
}

bool Terrain::InitialiseBuffer(ID3D11Device* device)
{
	

	//Create the list of vertices and indices
	VertexTerrain* vertices;
	UINT* indices;
	D3D11_BUFFER_DESC vertexDesc, indexDesc;
	D3D11_SUBRESOURCE_DATA vertData, indexData;
	HRESULT result;

	int i;
	XMFLOAT4 Colour;

	Colour = XMFLOAT4(1.f, 1.f, 1.f, 1.f);

	//Grab the size in vertices of the terrain and give it to the count
	//The actual size of the terrain * the number of vertices in a single chunk
	m_nVertexCount = (m_nTerrainWidth - 1) * (m_nTerrainHeight - 1) * 6;

	m_nIndexCount = m_nVertexCount;

	//create the vertex array and check that the terrain exists
	vertices = new VertexTerrain[m_nVertexCount];
	if (!vertices) { return false; }

	//create the index array and check that the terrain exists
	indices = new UINT[m_nIndexCount];
	if (!indices) { return false; }

	

	//Load the vertices and indices from the model
	for (i = 0; i < m_nVertexCount; i++)
	{
		vertices[i].Position = XMFLOAT3(m_Model[i].x, m_Model[i].y, m_Model[i].z);

		vertices[i].UV = XMFLOAT2(m_Model[i].x, m_Model[i].z);

		vertices[i].Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);

		indices[i] = i;
	}

	CalculateTangents(vertices, m_nVertexCount, indices, m_nIndexCount);

	//Set the vert Buffer Desc
	vertexDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexDesc.ByteWidth = sizeof(VertexTerrain) * m_nVertexCount;
	vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexDesc.CPUAccessFlags = 0;
	vertexDesc.MiscFlags = 0;
	vertexDesc.StructureByteStride = 0;

	vertData.pSysMem = vertices;
	vertData.SysMemPitch = 0;
	vertData.SysMemSlicePitch = 0;

	//Create the vert Buffer
	result = device->CreateBuffer(&vertexDesc, &vertData, &m_vertexBuffer);
	if (FAILED(result))
	{
		return false;
	}

	//set index description
	indexDesc.Usage = D3D11_USAGE_DEFAULT;
	indexDesc.ByteWidth = sizeof(unsigned long) * m_nIndexCount;
	indexDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexDesc.CPUAccessFlags = 0;
	indexDesc.MiscFlags = 0;
	indexDesc.StructureByteStride = 0;

	indexData.pSysMem = indices;
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	result = device->CreateBuffer(&indexDesc, &indexData, &m_IndexBuffer);
	if (FAILED(result))
	{
		return false;
	}

	delete[] vertices;
	vertices = 0;

	delete[] indices;
	indices = 0;

	return true;

}

// Calculates the tangents of the vertices in a mesh
// Code adapted from: http://www.terathon.com/code/tangent.html
void Terrain::CalculateTangents(Terrain::VertexTerrain* verts, int numVerts, UINT* indices, int numIndices)
{
	// Reset tangents
	for (int i = 0; i < numVerts; i++)
	{
		verts[i].Tangent = DirectX::XMFLOAT3(0, 0, 0);
	}

	// Calculate tangents one whole triangle at a time
	for (int i = 0; i < numVerts;)
	{
		// Grab indices and vertices of first triangle
		int i1 = indices[i++];
		int i2 = indices[i++];
		int i3 = indices[i++];
		VertexTerrain* v1 = &verts[i1];
		VertexTerrain* v2 = &verts[i2];
		VertexTerrain* v3 = &verts[i3];

		// Calculate vectors relative to triangle positions
		float x1 = v2->Position.x - v1->Position.x;
		float y1 = v2->Position.y - v1->Position.y;
		float z1 = v2->Position.z - v1->Position.z;

		float x2 = v3->Position.x - v1->Position.x;
		float y2 = v3->Position.y - v1->Position.y;
		float z2 = v3->Position.z - v1->Position.z;

		// Do the same for vectors relative to triangle uv's
		float s1 = v2->UV.x - v1->UV.x;
		float t1 = v2->UV.y - v1->UV.y;

		float s2 = v3->UV.x - v1->UV.x;
		float t2 = v3->UV.y - v1->UV.y;

		// Create vectors for tangent calculation
		float r = 1.0f / (s1 * t2 - s2 * t1);

		float tx = (t2 * x1 - t1 * x2) * r;
		float ty = (t2 * y1 - t1 * y2) * r;
		float tz = (t2 * z1 - t1 * z2) * r;

		// Adjust tangents of each vert of the triangle
		v1->Tangent.x += tx;
		v1->Tangent.y += ty;
		v1->Tangent.z += tz;

		v2->Tangent.x += tx;
		v2->Tangent.y += ty;
		v2->Tangent.z += tz;

		v3->Tangent.x += tx;
		v3->Tangent.y += ty;
		v3->Tangent.z += tz;
	}

	// Ensure all of the tangents are orthogonal to the normals
	for (int i = 0; i < numVerts; i++)
	{
		// Grab the two vectors
		DirectX::XMVECTOR normal = XMLoadFloat3(&verts[i].Normal);
		DirectX::XMVECTOR tangent = XMLoadFloat3(&verts[i].Tangent);

		// Use Gram-Schmidt orthogonalize
		tangent = DirectX::XMVector3Normalize(
			tangent - normal * DirectX::XMVector3Dot(normal, tangent));

		// Store the tangent
		XMStoreFloat3(&verts[i].Tangent, tangent);
	}
}

void Terrain::ShutdownBuffer()
{
	//handle releases internally
	if (m_IndexBuffer)
	{
		m_IndexBuffer->Release();
		m_IndexBuffer = 0;
	}

	if (m_vertexBuffer)
	{
		m_vertexBuffer->Release();
		m_vertexBuffer = 0;
	}

	return;
}

void Terrain::RenderBuffers(ID3D11DeviceContext* context)
{
	unsigned int stride;
	unsigned int offset;

	stride = sizeof(VertexTerrain);
	offset = 0;

	context->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
	context->IASetIndexBuffer(m_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	
	return;
}

bool Terrain::LoadSetupFile(char* file)
{
	int stringLength;
	ifstream fin;
	char input;

	stringLength = 256;
	m_terrainFile = new char[stringLength];
	if (!m_terrainFile)
	{
		return false;
	}

	//Open the file
	fin.open(file);
	if (fin.fail())
	{
		return false;
	}

	//Read File
	fin.get(input);
	while (input != ':')
	{
		fin.get(input);
	}
	fin >> m_terrainFile;

	fin.get(input);
	while (input != ':')
	{
		fin.get(input);
	}
	fin >> m_nTerrainHeight;

	fin.get(input);
	while (input != ':')
	{
		fin.get(input);
	}
	fin >> m_nTerrainWidth;

	fin.get(input);
	while (input != ':')
	{
		fin.get(input);
	}
	fin >> m_fHeightScale;

	fin.close();

	return true;
}

bool Terrain::LoadBitmapHeightMap()
{
	int error, imageSize, i, j, k, index;
	FILE* filePtr;
	unsigned long long count;
	BITMAPFILEHEADER bitmapFileHeader;
	BITMAPINFOHEADER bitmapInfoHeader;
	unsigned char* bitmapImage;
	unsigned char height;

	//Create the array to hold the height map data
	m_HeightMap = new HeightMap[m_nTerrainWidth * m_nTerrainHeight];
	if (!m_HeightMap)
	{
		return false;
	}

	//open the bitmap file in binary read
	error = fopen_s(&filePtr, m_terrainFile, "rb");
	if (error != 0)
	{
		return false;
	}

	//Read Bitmap header
	count = fread(&bitmapFileHeader, sizeof(BITMAPFILEHEADER), 1, filePtr);
	if (count != 1)
	{
		return false;
	}

	count = fread(&bitmapInfoHeader, sizeof(BITMAPINFOHEADER), 1, filePtr);
	if (count != 1)
	{
		return false;
	}

	if ((bitmapInfoHeader.biHeight != m_nTerrainHeight) || (bitmapInfoHeader.biWidth != m_nTerrainWidth))
	{
		return false;
	}

	imageSize = m_nTerrainHeight * ((m_nTerrainWidth * 3) + 1);

	bitmapImage = new unsigned char[imageSize];
	if (!bitmapImage)
	{
		return false;
	}

	//start reading the height map
	fseek(filePtr, bitmapFileHeader.bfOffBits, SEEK_SET);
	count = fread(bitmapImage, 1, imageSize, filePtr);
	if (count != imageSize)
	{
		return false;
	}

	//close
	error = fclose(filePtr);
	if (error != 0)
	{
		return false;
	}

	k = 0;

	for (j = 0; j < m_nTerrainHeight; j++)
	{
		for (i = 0; i < m_nTerrainWidth; i++)
		{
			//Bitmap read upside down, so they have to be read from bottom up
			index = (m_nTerrainWidth * (m_nTerrainHeight - 1 - j)) + i;

			//get height
			height = bitmapImage[k];

			//store height
			m_HeightMap[index].y = (float)height;

			k += 3;
		}

		k++;
	}

	//release the bitmap
	delete[] bitmapImage;
	bitmapImage = 0;

	delete[] m_terrainFile;
	m_terrainFile = 0;

	return true;
}

void Terrain::ShutdownHeightMap()
{
	if (m_HeightMap)
	{
		delete[] m_HeightMap;
		m_HeightMap = 0;
	}

	return;
}

void Terrain::SetTerrainCoordinates()
{
	int i, j, index;

	//adjust the heights of the model using the heightmap data
	for (j = 0; j < m_nTerrainWidth; j++)
	{
		for (i = 0; i < m_nTerrainHeight; i++)
		{
			index = (m_nTerrainWidth * j) + i;

			//set X and Z coordinates
			m_HeightMap[index].x = (float)i * m_fHeightScale;
			m_HeightMap[index].z = -(float)j * m_fHeightScale;

			m_HeightMap[index].z += (float)(m_nTerrainHeight - 1);

			m_HeightMap[index].y /= (m_fHeightScale / 8);
		}
	}
	return;
}

bool Terrain::BuildTerrain()
{
	int i, j, index, index1, index2, index3, index4;
	float averageHeight = 0;

	Heights.resize(m_nTerrainHeight);
	for (int k = 0; k < m_nTerrainHeight; k++)
	{
		Heights[k].resize(m_nTerrainWidth);
	}

	//Get the Vertex Count
	m_nVertexCount = (m_nTerrainHeight - 1) * (m_nTerrainWidth - 1) * 6;

	//create the terrain model array
	m_Model = new Model[m_nVertexCount];
	if (!m_Model)
	{
		return false;
	}

	index = 0;

	//Its Triangle Time!
	for (j = 0; j < m_nTerrainHeight - 1; j++)
	{
		for (i = 0; i < m_nTerrainWidth - 1; i++)
		{
			//First, get the indices of the quad
			index1 = (m_nTerrainWidth * j) + i; //Upper Left
			index2 = (m_nTerrainWidth * j) + (i + 1); //Upper Right;
			index3 = (m_nTerrainWidth * (j + 1)) + i; //Bottom Left;
			index4 = (m_nTerrainWidth * (j + 1)) + (i + 1); //Bottom Left;

			//Triangles!
			//Upper Left
			m_Model[index].x = m_HeightMap[index1].x;
			m_Model[index].y = m_HeightMap[index1].y;
			m_Model[index].z = m_HeightMap[index1].z;
			averageHeight += m_Model[index].y;
			index++;

			//Upper Right
			m_Model[index].x = m_HeightMap[index2].x;
			m_Model[index].y = m_HeightMap[index2].y;
			m_Model[index].z = m_HeightMap[index2].z;
			averageHeight += m_Model[index].y;
			index++;

			//Bottom Left
			m_Model[index].x = m_HeightMap[index3].x;
			m_Model[index].y = m_HeightMap[index3].y;
			m_Model[index].z = m_HeightMap[index3].z;
			averageHeight += m_Model[index].y;
			index++;
			
			//Second Triangle

			//Bottom Left
			m_Model[index].x = m_HeightMap[index3].x;
			m_Model[index].y = m_HeightMap[index3].y;
			m_Model[index].z = m_HeightMap[index3].z;
			//averageHeight += m_Model[index].y;
			index++;

			//Upper Right
			m_Model[index].x = m_HeightMap[index2].x;
			m_Model[index].y = m_HeightMap[index2].y;
			m_Model[index].z = m_HeightMap[index2].z;
			//averageHeight += m_Model[index].y;
			index++;

			//Bottom Right
			m_Model[index].x = m_HeightMap[index4].x;
			m_Model[index].y = m_HeightMap[index4].y;
			m_Model[index].z = m_HeightMap[index4].z;
			averageHeight += m_Model[index].y;
			index++;

			Heights[j][i] = (averageHeight / 4);
			averageHeight = 0;
		}
	}

	return true;
}

void Terrain::ShutdownTerrain()
{
	if (m_Model)
	{
		delete[] m_Model;
		m_Model = 0;
	}

	return;
}

void Terrain::GenerateHeights()
{

}