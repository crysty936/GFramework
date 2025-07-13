
// Potentially these and their root signature are not needed at all using the global ResourceDescriptorHeap in SM6.6
Texture2D		Tex2DTable[] :		register(t0, space0);
Texture2DArray	Tex2DArrayTable[] : register(t0, space1);
TextureCube		TexCubeTable[] :	register(t0, space2);