#include "BasicShapesData.h"

// Triangle
const float TriangleVertices[] =
{
	-1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.f, 0.f, 0.f, 
	 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.f, 1.f, 0.f,
	 0.0f,  1.0f, 0.0f, 0.0f, 0.0f, 1.f, 0.5f, 1.f
};


const uint32_t TriangleIndices[] =
{
	0, 1, 2
};

const float* BasicShapesData::GetTriangleVertices()
{
	return TriangleVertices;
}

const uint32_t* BasicShapesData::GetTriangleIndices()
{
	return TriangleIndices;
}

int32_t BasicShapesData::GetTriangleVerticesCount()
{
	return sizeof(TriangleVertices) / sizeof(float);
}

int32_t BasicShapesData::GetTriangleIndicesCount()
{
	return sizeof(TriangleIndices) / sizeof(uint32_t);
}

// ~Triangle

// Quad
// To be used for screen quads
const float QuadVertices[] = {
 1.f,  1.f, 0.0f,  1.f, 1.f, // top right
-1.f,  1.f, 0.0f,  0.f, 1.f,  // top left 
 1.f, -1.f, 0.0f,  1.f, 0.f, // bottom right
-1.f, -1.f, 0.0f,  0.f, 0.f, // bottom left
};

const uint32_t QuadIndices[] = {
	0, 1, 2,   // first triangle
	1, 3, 2    // second triangle
};

const float* BasicShapesData::GetQuadVertices()
{
	return QuadVertices;
}

const uint32_t* BasicShapesData::GetQuadIndices()
{
	return QuadIndices;
}

int32_t BasicShapesData::GetQuadVerticesCount()
{
	return sizeof(QuadVertices) / sizeof(float);
}

int32_t BasicShapesData::GetQuadIndicesCount()
{
	return sizeof(QuadIndices) / sizeof(uint32_t);
}

// ~Quad

// TBN Quad
const float TBNQuadVertices[] = {
// Position, Normal, UV, Tangent, BiTangent
 1.f,  1.f, 0.0f,  0.0, 0.0, 1.0, 1.f, 1.f, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, // top right
-1.f,  1.f, 0.0f,  0.0, 0.0, 1.0, 0.f, 1.f, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0,  // top left 
 1.f, -1.f, 0.0f,  0.0, 0.0, 1.0, 1.f, 0.f, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, // bottom right
-1.f, -1.f, 0.0f,  0.0, 0.0, 1.0, 0.f, 0.f, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0 // bottom left
};

const uint32_t TBNQuadIndices[] = {
	0, 1, 2,   // first triangle
	1, 3, 2    // second triangle
};

const float* BasicShapesData::GetTBNQuadVertices()
{
	return TBNQuadVertices;
}

const uint32_t* BasicShapesData::GetTBNQuadIndices()
{
	return TBNQuadIndices;
}

int32_t BasicShapesData::GetTBNQuadVerticesCount()
{
	return sizeof(TBNQuadVertices) / sizeof(float);
}

int32_t BasicShapesData::GetTBNQuadIndicesCount()
{
	return sizeof(TBNQuadIndices) / sizeof(uint32_t);
}

// ~Quad

// Square
// CCW Vertices with secondary diagonal split drawing
// To be used for world space entities
const float SquareVertices[] = {
 1.f,  1.f, 0.0f,  0.0f, 0.0f, 1.f, 1.f, 1.f, // top right
-1.f,  1.f, 0.0f,  0.0f, 0.0f, 1.f, 0.f, 1.f,  // top left 
 1.f, -1.f, 0.0f,  0.0f, 0.0f, 1.f, 1.f, 0.f, // bottom right
-1.f, -1.f, 0.0f,  0.0f, 0.0f, 1.f, 0.f, 0.f, // bottom left
};

const uint32_t SquareIndices[] = {
	0, 1, 2,   // first triangle
	1, 3, 2    // second triangle
};

const float* BasicShapesData::GetSquareVertices()
{
	return SquareVertices;
}

const uint32_t* BasicShapesData::GetSquareIndices()
{
	return SquareIndices;
}

int32_t BasicShapesData::GetSquareVerticesCount()
{
	return sizeof(SquareVertices) / sizeof(float);
}

int32_t BasicShapesData::GetSquareIndicesCount()
{
	return sizeof(SquareIndices) / sizeof(uint32_t);
}

// ~Square

// Cube
 // CCW Cube Vertices
 // z goes from 1 to -1 so they appear at first sight as CW
// These are commented as [where they are in the clip cube when looking straigh on][face to be used for]
// NOT [place in used face][face to be used for]
const float CubeVertices[] = {

	// Vertices			 // Normal			  //Tex Coords
	// Back face
	-1.f, -1.f, -1.f,    0.0f,  0.0f, -1.0f,  0.0f, 0.0f, // bottom-left back face		// 0
	-1.f, -1.f, -1.f,   -1.0f,  0.0f,  0.0f,  0.0f, 0.0f, // bottom-left left face
	-1.f, -1.f, -1.f,    0.0f, -1.0f,  0.0f,  1.0f, 0.0f, // bottom-left bottom face

	 1.f,  1.f, -1.f,    0.0f,  0.0f, -1.0f,  1.0f, 1.0f, // top-right back face		// 3
	 1.f,  1.f, -1.f,    1.0f,  0.0f,  0.0f,  1.0f, 1.0f, // top-right right face
	 1.f,  1.f, -1.f,    0.0f,  1.0f,  0.0f,  0.0f, 1.0f, // top-right top face

	 1.f, -1.f, -1.f,    0.0f,  0.0f, -1.0f,  1.0f, 0.0f, // bottom-right back face		// 6
	 1.f, -1.f, -1.f,    1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // bottom-right right face
	 1.f, -1.f, -1.f,    0.0f, -1.0f,  0.0f,  1.0f, 1.0f, // bottom-right bottom face

	-1.f,  1.f, -1.f,    0.0f,  0.0f, -1.0f,  0.0f, 1.0f, // top-left back face			// 9
	-1.f,  1.f, -1.f,   -1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // top-left left face
	-1.f,  1.f, -1.f,    0.0f,  1.0f,  0.0f,  0.0f, 0.0f, // top-left top face

	// Front Face
	-1.f, -1.f,  1.f,    0.0f,  0.0f, 1.0f,   0.0f, 0.0f, // bottom-left front face		// 12
	-1.f, -1.f,  1.f,   -1.0f,  0.0f,  0.0f,  1.0f, 0.0f, // bottom-left left face
	-1.f, -1.f,  1.f,    0.0f, -1.0f,  0.0f,  0.0f, 0.0f, // bottom-left bottom face

	 1.f,  1.f,  1.f,    0.0f,  0.0f, 1.0f,   1.0f, 1.0f, // top-right front face		// 15
	 1.f,  1.f,  1.f,    1.0f,  0.0f,  0.0f,  0.0f, 1.0f, // top-right right face
	 1.f,  1.f,  1.f,    0.0f,  1.0f,  0.0f,  1.0f, 1.0f, // top-right top face

	 1.f, -1.f,  1.f,    0.0f,  0.0f, 1.0f,   1.0f, 0.0f, // bottom-right front face	// 18
	 1.f, -1.f,  1.f,    1.0f,  0.0f,  0.0f,  0.0f, 0.0f, // bottom-right right face
	 1.f, -1.f,  1.f,    0.0f, -1.0f,  0.0f,  0.0f, 1.0f, // bottom-right bottom face

	-1.f,  1.f,  1.f,    0.0f,  0.0f, 1.0f,   0.0f, 1.0f, // top-left front face		// 21
	-1.f,  1.f,  1.f,   -1.0f,  0.0f,  0.0f,  1.0f, 1.0f, // top-left left face
	-1.f,  1.f,  1.f,    0.0f,  1.0f,  0.0f,  1.0f, 0.0f  // top-left top face

};

 // CCW Cube indices
 uint32_t CubeIndices[] =
 {
 	15, 12, 18, // +z
 	15, 21,12,
 
 	9, 6, 0, // -z
 	9, 3, 6,
 
 	22, 1, 13, // -x
 	22, 10, 1,
  
   	4, 19, 7, // +x
   	4, 16, 19,
    
   	8, 14, 2, // -y
   	8, 20, 14,
    
   	17, 11, 23, // +y
   	17, 5, 11,
 };
 
 const float* BasicShapesData::GetCubeVertices()
 {
 	return CubeVertices;
 }
 
 const uint32_t* BasicShapesData::GetCubeIndices()
 {
 	return CubeIndices;
 }
 
 int32_t BasicShapesData::GetCubeVerticesCount()
 {
 	return sizeof(CubeVertices) / sizeof(float);
 }
 
 int32_t BasicShapesData::GetCubeIndicesCount()
 {
 	return sizeof(CubeIndices) / sizeof(uint32_t);
 } 

// ~Cube

// Skybox
constexpr float SkyboxVertices[] = {
	// positions          
	-1.0f,  1.0f, -1.0f,
	-1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,
	 1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,

	-1.0f, -1.0f,  1.0f,
	-1.0f, -1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f,  1.0f,
	-1.0f, -1.0f,  1.0f,

	 1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,

	-1.0f, -1.0f,  1.0f,
	-1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f, -1.0f,  1.0f,
	-1.0f, -1.0f,  1.0f,

	-1.0f,  1.0f, -1.0f,
	 1.0f,  1.0f, -1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	-1.0f,  1.0f,  1.0f,
	-1.0f,  1.0f, -1.0f,

	-1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f,  1.0f,
	 1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f,  1.0f,
	 1.0f, -1.0f,  1.0f
};

const float* BasicShapesData::GetSkyboxVertices()
{
	return SkyboxVertices;
}

int32_t BasicShapesData::GetSkyboxVerticesCount()
{
	return sizeof(SkyboxVertices) / sizeof(float);
}

int32_t BasicShapesData::GetSkyboxIndicesCount()
{
	return 36;

	// TODO: Add the indices of the cube to get rid of Draw Arrays
	return sizeof(SkyboxVertices) / sizeof(uint32_t);
}
// ~Skybox


// const float GLClipCubeVertices[] = {
// 
// 	// Vertices		
// 
// 	// Front face
// 	-1.f, -1.f,  1.f,
// 	-1.f, -1.f, -1.f,
// 	-1.f,  1.f,  1.f,
// 	-1.f,  1.f, -1.f,
// 
// 	// Back face
// 	 1.f, -1.f,  1.f,
// 	 1.f, -1.f, -1.f,
// 	 1.f,  1.f,  1.f,
// 	 1.f,  1.f, -1.f,
// };
