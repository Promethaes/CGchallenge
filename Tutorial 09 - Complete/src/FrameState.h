#pragma once
#include <GLM/glm.hpp>
#include "FrameBuffer.h"

// Represents the state for the previous or current frame
struct FrameState {
	FrameBuffer::Sptr Output;	
	glm::mat4         View;
	glm::mat4         Projection;
	glm::mat4         ViewProjection;
};

// Stores the state of the current and last frames
struct AppFrameState
{
	FrameState Current;
	FrameState Last;
};