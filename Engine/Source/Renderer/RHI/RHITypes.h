#pragma once
#include "EASTL/shared_ptr.h"
#include "EASTL/string.h"
#include "EASTL/vector.h"
#include "Renderer/RenderingPrimitives.h"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_float4.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "Resources/RHITexture.h"

enum class ERasterizerState : uint8_t
{
	Disabled,
	BackFaceCull,
	FrontFaceCull,
	Count
};

enum class EBlendState : uint8_t
{
	Disabled,
	Count
};

enum class EDepthState : uint8_t
{
	Disabled,
	WriteEnabled,
	Count
};

enum EShaderType : uint8_t
{
	Sh_Vertex = 1 << 0,
	Sh_Fragment = 1 << 1,
	Sh_Geometry = 1 << 2,

	Sh_Universal = Sh_Vertex | Sh_Fragment | Sh_Geometry
};

enum class ERasterizerFront
{
	CW,
	CCW
};

enum class ECullFace
{
	Front,
	Back
};

enum class EDepthOp
{
	Never,
	Less,
	Equal,
	LessOrEqual,
	Greater,
	NotEqual,
	GreaterOrEqual,
	Always
};

struct ShaderSourceInput
{
	/** Path or code */
	eastl::string ShaderSource;

	/** How it should be compiled */
	EShaderType ShaderType;
};

enum class EBlendFunc
{
	Zero,
	One,
	Src_Alpha,
	One_Minus_Src_Alpha
	// To add more..
};

enum class EBlendEquation
{
	Add,
	Subtract,
	Reverse_Subtract,
	Min,
	Max
};

struct BlendState
{
	EBlendFunc SrcColorBlendFunc = EBlendFunc::Src_Alpha;
	EBlendFunc DestColorBlendFunc = EBlendFunc::One_Minus_Src_Alpha;
	EBlendFunc SrcAlphaBlendFunc = EBlendFunc::One;
	EBlendFunc DestAlphaBlendFunc = EBlendFunc::Zero;

	EBlendEquation ColorBlendEq = EBlendEquation::Add;
	EBlendEquation AlphaBlendEq = EBlendEquation::Add;
};

enum class EStencilFunc
{
	Never,		// Always fails
	Less,		// Passes if (ref & mask) < (stencil & mask)
	LEqual,		// Passes if (ref & mask) <= (stencil & mask)
	Greater,	// Passes if (ref & mask) > (stencil & mask)
	GEqual,		// Passes if (ref & mask) >= (stencil & mask)
	NotEqual,	// Passes if (ref & mask) != (stencil & mask)
	Equal,		// Passes if (ref & mask) == (stencil & mask)
	Always		// Always passes
};

enum class EStencilOp
{
	Keep,		//Keeps the current value
	Zero,		// Sets stencil buffer value to 0
	Replace,	//Sets the stencil buffer value to Ref
	Incr,		// Increment stencil buffer value, clamps to maximum representable value
	Incr_Wrap,	// Increment and wrap to 0 when exceeding maximum representable value
	Decr,		// Decrement stencil buffer value, clamps to 0
	Decr_Wrap,	// Decrement stencil buffer value, wrap to maximum representable value when going under 0
	Invert		// Bitwise invert current stencil value
};

struct SideStencilFunc
{
	EStencilFunc StencilFunction = EStencilFunc::Always;
	uint32_t StencilRef = 0;
	uint32_t StencilFuncMask = 0xFF; // Mask used on both sides in stencil comparison, default removes its effect
};

struct SideStencilOp
{
	EStencilOp StencilOpStencilFail = EStencilOp::Keep;
	EStencilOp StencilOpZFail = EStencilOp::Keep;
	EStencilOp StencilOpZPass = EStencilOp::Keep; // Happens if both stencil and Depth are passed or if Stencil is passed and depth test disabled
};

struct DepthStencilState
{
	EDepthOp DepthOperation = EDepthOp::Less;

	uint32_t FrontStencilMask = 0xFF;
	uint32_t BackStencilMask = 0xFF;

	SideStencilFunc FrontStencilFunc;
	SideStencilFunc BackStencilFunc;

	SideStencilOp FrontStencilOp;
	SideStencilOp BackStencilOp;
};

enum class EFaceCullMode
{
	Front,
	Back
};

enum class EDrawType
{
	DrawElements,
	DrawArrays,
	DrawInstanced
};

struct PointLightData
{
	float Linear;
	float Quadratic;
};

enum class ELightType
{
	Directional,
	Point
};



