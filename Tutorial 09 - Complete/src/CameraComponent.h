#pragma once
#include <optional>
#include <memory>
#include <GLM/glm.hpp>
#include "FrameBuffer.h"

/*
 * Stores the information required to render with a camera. Since this is a component of a gameobject,
 * we do not need a view matrix (we can take the inverse of the matrix that it is attached to).
 *
 * Later on, we can add things like render targets for the camera to render to
 */
struct CameraComponent {
	// True if this camera is the main camera for the scene (ie, what gets rendered fullscreen)
	bool              IsMainCamera;
	// The render target that this camera will render to
	FrameBuffer::Sptr BackBuffer;
	// The previous frame this camera rendered
	FrameBuffer::Sptr FrontBuffer;
	// The Color to clear to when using this camera
	glm::vec4         ClearCol = glm::vec4(0, 0, 0, 1);
	
	// The projection matrix for this camera
	glm::mat4         Projection;
};