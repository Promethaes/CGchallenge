#pragma once
#include <florp\app\ApplicationLayer.h>
#include <florp\graphics\Shader.h>
#include <florp\graphics\Mesh.h>
#include "FrameBuffer.h"

class LightingLayer : public florp::app::ApplicationLayer {
public:
	// Handles resizing the accumulation buffer
	virtual void OnWindowResize(uint32_t width, uint32_t height) override;
	// Sets up this layer
	virtual void Initialize() override;
	// Pre render will handle generating all the shadow casting light's buffers
	virtual void PreRender() override;
	// Post Render will handle processing the camera's output
	virtual void PostRender() override;

	// Allows us to render some UI to edit our lighting parameters
	virtual void RenderGUI() override;

protected:
	florp::graphics::Mesh::Sptr myFullscreenQuad;        // Used for our post processing passes
	florp::graphics::Shader::Sptr myShader;              // Used to handle depth generation for regular shadow casters
	florp::graphics::Shader::Sptr myMaskedShader;        // Used to handle depth generation for shadow casters that have a mask applied
	florp::graphics::Shader::Sptr myShadowComposite;     // Used to handle adding a shadow cast
	florp::graphics::Shader::Sptr myPointLightComposite; // Used to handle adding a point light
	florp::graphics::Shader::Sptr myFinalComposite;      // Used to perform final compositing of the light buffer and the color buffer 
	FrameBuffer::Sptr myAccumulationBuffer;              // Our buffer for accumulating our lighting factors

	bool isProcessingShadows, isProcessingPointLights;
	
	glm::vec3 myAmbientLight; // Stores our ambient light color

	// Handles post-processing shadows
	void PostProcessShadows();
	// Handles post-processing lights (will come later, dun dun daaaa)
	void PostProcessLights();
};
