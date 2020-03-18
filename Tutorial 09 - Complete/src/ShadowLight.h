#pragma once
#include <GLM/glm.hpp>
#include "FrameBuffer.h"

/*
 * Stores the information required to render with a camera. Since this is a component of a gameobject,
 * we do not need a view matrix (we can take the inverse of the matrix that it is attached to).
 *
 * Later on, we can add things like render targets for the camera to render to
 */
struct ShadowLight {
	// The render target that this camera will render to
	FrameBuffer::Sptr                  ShadowBuffer;
	// The mask to use for ignoring sections of this lights view from evaluation
	florp::graphics::Texture2D::Sptr   Mask;
	// An image to be projected onto areas that this light illuminates
	florp::graphics::Texture2D::Sptr   ProjectorImage;
	float                              ProjectorImageIntensity = 0.25f;

	// The projection matrix for this shadow caster
	glm::mat4         Projection;

	glm::vec3 Color;
	float     Attenuation;
};