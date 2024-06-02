#pragma once
#include <stdint.h>
#include "EASTL/string.h"
#include "EASTL/shared_ptr.h"
#include "EASTL/vector.h"

// Used only for OpenGL, deprecated
class RHIFrameBuffer
{
public:
	virtual bool HasDepthAttachment() const = 0;
	virtual bool HasColorAttachment() const = 0;

	eastl::shared_ptr<class RHITexture2D> DepthAttachment;
	eastl::vector<eastl::shared_ptr<class RHITexture2D>> ColorAttachments;
};