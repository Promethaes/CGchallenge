#pragma once
#include "florp/app/ApplicationLayer.h"
#include "florp/app/Window.h"
#include "FrameBuffer.h"
#include "florp/graphics/Mesh.h"
#include "florp/graphics/Shader.h"

class PostLayer : public florp::app::ApplicationLayer
{
public:
	virtual void Initialize() override;
	virtual void OnWindowResize(uint32_t width, uint32_t height) override;
	virtual void RenderGUI() override;
	virtual void PostRender() override;
	virtual void Update() override;

protected:
	florp::graphics::Mesh::Sptr myFullscreenQuad;

	struct PostPass {
		typedef std::shared_ptr<PostPass> Sptr;

		florp::graphics::Shader::Sptr Shader;
		FrameBuffer::Sptr             Output;
		struct Input {
			Sptr                      Pass;
			RenderTargetAttachment    Attachment = RenderTargetAttachment::Color0;
			bool                      UsePrevFrame = false; 
		};
		std::vector<Input>            Inputs;

		std::string                   Name;
		
		// This is kinda a janky way to allow us to edit parameters using the GUI
		struct ShaderParameter {
			std::string                     Name; // Name of shader parameter
			florp::graphics::ShaderDataType Type; // The uniform type
			// A block of data that we'll re-interpret depending on the data type
			// We'll say that it can hold a full mat4, just to be safe
			char                            RawDataBuffer[4 * 4 * sizeof(float)];
		};
		std::vector<ShaderParameter>  ConfParameters;

		float                         ResolutionMultiplier = 1.0f;
		bool                          Enabled = true;
	};
	std::vector<PostPass::Sptr> myPasses;
	std::unordered_map<florp::app::Key, std::vector<PostPass::Sptr>> myToggleInputs;
	
	PostPass::Sptr __CreatePass(const char* fragmentShader, float scale = 1.0f) const;

	PostPass::ShaderParameter __CreateFloatParam(const std::string& name, float defaultVal, float min, float max);

};
