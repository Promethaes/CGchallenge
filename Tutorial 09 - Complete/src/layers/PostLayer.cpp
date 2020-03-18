#include "PostLayer.h"
#include "florp/app/Application.h"
#include "florp/game/SceneManager.h"
#include "FrameState.h"
#include <imgui.h>

PostLayer::PostPass::ShaderParameter PostLayer::__CreateFloatParam(const std::string& name, float defaultValue, float min, float max) {
	PostLayer::PostPass::ShaderParameter result;
	result.Name = name;
	result.Type = florp::graphics::ShaderDataType::Float;
	// For Floats, we'll store a value, a minimum and a maximum in the parameter block
	float bufferData[3] = {
		defaultValue, min, max
	};
	// We copy the buffer into the data block
	memcpy(result.RawDataBuffer, bufferData, 3 * sizeof(float));
	return result;
}

void PostLayer::Initialize() {
	florp::app::Application* app = florp::app::Application::Get();

	// The color buffer should be marked as shader readable, so that we generate a texture for it
	RenderBufferDesc mainColor = RenderBufferDesc();
	mainColor.ShaderReadable = true;
	mainColor.Attachment = RenderTargetAttachment::Color0;
	mainColor.Format = RenderTargetType::ColorRgb8;

	// Create our fullscreen quad mesh
	{
		float vert[] = {
			-1.0f, -1.0f,  0.0f, 0.0f,
			 1.0f, -1.0f,  1.0f, 0.0f,
			-1.0f,  1.0f,  0.0f, 1.0f,
			 1.0f,  1.0f,  1.0f, 1.0f
		};
		uint32_t indices[] = {
			0, 1, 2,
			1, 3, 2
		};
		florp::graphics::BufferLayout layout = {
			{ "inPosition", florp::graphics::ShaderDataType::Float2 },
			{ "inUV",       florp::graphics::ShaderDataType::Float2 }
		};

		myFullscreenQuad = std::make_shared<florp::graphics::Mesh>(vert, 4, layout, indices, 6);
	}

	if (true) {
		auto highlight = __CreatePass("shaders/post/bloom_highlight.fs.glsl");
		highlight->Name = "Bloom Highlight";
		highlight->Shader->SetUniform("a_BloomThreshold", 1.0f);
		highlight->ConfParameters.push_back(__CreateFloatParam("a_BloomThreshold", 1.0f, 0.0f, 2.0f));
		// Add the pass to the post processing stack
		myPasses.push_back(highlight);

		// Horizontal blur pass
		auto hBlur = __CreatePass("shaders/post/blur_gaussian_5.fs.glsl");
		hBlur->Shader->SetUniform("isHorizontal", 1);
		hBlur->Output->SetDebugName("HorizontalBuffer");

		// Vertical blur pass
		auto vBlur = __CreatePass("shaders/post/blur_gaussian_5.fs.glsl");
		vBlur->Shader->SetUniform("isHorizontal", 0);
		vBlur->Output->SetDebugName("VerticalBuffer");

		// Perform 10 passes of blurring
		for (int ix = 0; ix < 10; ix++) {
			myPasses.push_back(hBlur);
			myPasses.push_back(vBlur);
		}

		// Our additive pass will add the color from the scene, and add the blurred highlight to it
		auto additive = __CreatePass("shaders/post/additive_blend.fs.glsl");
		additive->Inputs.push_back({ nullptr });
		additive->Inputs.push_back({ vBlur });
		// Add the pass to the post processing stack
		myPasses.push_back(additive);

		// We will toggle all the bloom stuff with one key
		myToggleInputs[florp::app::Key::B] = { additive, hBlur, vBlur, highlight };
	}

	if (true) {
		// Our additive pass will add the color from the scene, and add the blurred highlight to it
		auto motionBlur = __CreatePass("shaders/post/motion_blur.fs.glsl");
		motionBlur->Inputs.push_back({ nullptr, RenderTargetAttachment::Depth }); // 1 will hold this frame's depth
		// Add the pass to the post processing stack
		myPasses.push_back(motionBlur);

		// We will toggle motion blur on and off with one key
		myToggleInputs[florp::app::Key::M] = { motionBlur };
	}

	// Depth of field effect
	if (true) {

		// Our inputs will be the previous pass, and the depth buffer from the main pass
		auto dof = __CreatePass("shaders/post/depth_of_field.fs.glsl");
		dof->Inputs.push_back({ nullptr, RenderTargetAttachment::Depth }); // 1 will hold this frame's depth
		// Add the pass to the post processing stack 
		myPasses.push_back(dof);

		// We'll set our default values
		dof->Shader->SetUniform("a_FocalDepth", 3.0f);
		dof->Shader->SetUniform("a_LenseDistance", 1.0f);
		dof->Shader->SetUniform("a_Aperture", 20.0f);

		// Set up the stuff for making this pass editable from the GUI
		dof->Name = "Depth of Field";
		dof->ConfParameters.push_back(__CreateFloatParam("a_FocalDepth", 3.0f, 0.1f, 100.0f)); 
		dof->ConfParameters.push_back(__CreateFloatParam("a_LenseDistance", 1.0f, 0.001f, 5.0f));
		dof->ConfParameters.push_back(__CreateFloatParam("a_Aperture", 20.0f, 0.1f, 60.0f));

		// We will toggle DOF on and off with one key
		myToggleInputs[florp::app::Key::T] = { dof };
	}
}

void PostLayer::OnWindowResize(uint32_t width, uint32_t height) {
	//myMainFrameBuffer->Resize(width, height);
	for (auto& pass : myPasses) {
		pass->Output->Resize(
			(uint32_t)(width * pass->ResolutionMultiplier),
			(uint32_t)(height * pass->ResolutionMultiplier)
		);
	}
}

void PostLayer::RenderGUI()
{
	ImGui::Begin("Post Processing");

	// Iterate over all passes
	for (const auto& pass : myPasses) {
		if (!pass->ConfParameters.empty()) {
			// Each pass gets it's own header
			if (ImGui::CollapsingHeader(pass->Name.c_str())) {
				// Iterate over all parameters for the pass
				for (const auto& param : pass->ConfParameters) {
					// Depending on the type, we handle it differently
					switch (param.Type) {
					case florp::graphics::ShaderDataType::Float: {
						float* data = (float*)&param.RawDataBuffer;
						if (ImGui::SliderFloat(param.Name.c_str(), &data[0], data[1], data[2])) {
							pass->Shader->SetUniform(param.Name, data[0]);
						}
						break;
					}
					default:
						break;
					}
				}
			}
		}
	}

	ImGui::End();
}

void PostLayer::PostRender() {
	// We grab the application singleton to get the size of the screen
	florp::app::Application* app = florp::app::Application::Get();

	// We'll get the back buffer from the frame state
	const AppFrameState& state = CurrentRegistry().ctx<AppFrameState>();
	FrameBuffer::Sptr mainBuffer = state.Current.Output;
	
	// Unbind the main framebuffer, so that we can read from it
	//mainBuffer->UnBind();
	glDisable(GL_DEPTH_TEST);

	// The last output will start as the output from the rendering
	FrameBuffer::Sptr lastPass = mainBuffer;

	float m22 = state.Current.Projection[2][2];
	float m32 = state.Current.Projection[3][2];
	float nearPlane = (2.0f * m32) / (2.0f * m22 - 2.0f);
	float farPlane = ((m22 - 1.0f) * nearPlane) / (m22 + 1.0);

	// We'll iterate over all of our render passes
	for (const PostPass::Sptr& pass : myPasses) {
		if (pass->Enabled) {
			// We'll bind our post-processing output as the current render target and clear it
			pass->Output->Bind(RenderTargetBinding::Draw);
			glClear(GL_COLOR_BUFFER_BIT);
			// Set the viewport to be the entire size of the passes output
			glViewport(0, 0, pass->Output->GetWidth(), pass->Output->GetHeight());

			// Use the post processing shader to draw the fullscreen quad
			pass->Shader->Use();
			lastPass->Bind(0);
			pass->Shader->SetUniform("xImage", 0); 

			// Expose camera state to shaders
			pass->Shader->SetUniform("a_View", state.Current.View);
			pass->Shader->SetUniform("a_Projection", state.Current.Projection); 
			pass->Shader->SetUniform("a_ProjectionInv", glm::inverse(state.Current.Projection));
			pass->Shader->SetUniform("a_ViewProjection", state.Current.ViewProjection);
			pass->Shader->SetUniform("a_ViewProjectionInv", glm::inverse(state.Current.ViewProjection));

			pass->Shader->SetUniform("a_PrevView", state.Last.View); 
			pass->Shader->SetUniform("a_PrevProjection", state.Last.Projection);
			pass->Shader->SetUniform("a_PrevProjectionInv", glm::inverse(state.Last.Projection));
			pass->Shader->SetUniform("a_PrevViewProjection", state.Last.ViewProjection);
			pass->Shader->SetUniform("a_PrevViewProjectionInv", glm::inverse(state.Last.ViewProjection));

			pass->Shader->SetUniform("a_NearPlane", nearPlane);
			pass->Shader->SetUniform("a_FarPlane", farPlane);

			// We'll bind all the inputs as textures in the order they were added (starting at index 1)
			for (size_t ix = 0; ix < pass->Inputs.size(); ix++) {
				const auto& input = pass->Inputs[ix];
				if (input.Pass == nullptr) {
					if (input.UsePrevFrame && state.Last.Output != nullptr) {
						state.Last.Output->Bind(ix + 1, input.Attachment);
					}
					else {
						mainBuffer->Bind(ix + 1, input.Attachment);
					}
				}
				else {
					input.Pass->Output->Bind(ix + 1, input.Attachment);
				}
			}
			pass->Shader->SetUniform("xScreenRes", glm::ivec2(pass->Output->GetWidth(), pass->Output->GetHeight()));
			myFullscreenQuad->Draw();

			// Unbind the output pass so that we can read from it
			pass->Output->UnBind();

			// Update the last pass output to be this passes output
			lastPass = pass->Output;
		}
	}
		
	// Bind the last buffer we wrote to as our source for read operations
	lastPass->Bind(RenderTargetBinding::Read);
	// Copies the image from lastPass into the default back buffer
	FrameBuffer::Blit({ 0, 0, lastPass->GetWidth(), lastPass->GetHeight()},
		{0, 0, app->GetWindow()->GetWidth(), app->GetWindow()->GetHeight()}, BufferFlags::All, florp::graphics::MagFilter::Nearest);

	// Unbind the last buffer from read operations, so we can write to it again later
	lastPass->UnBind();
}

void PostLayer::Update() {
	// Get the window to do input
	florp::app::Window::Sptr window = florp::app::Application::Get()->GetWindow();

	// Iterate over all the keys
	for (auto it = myToggleInputs.begin(); it != myToggleInputs.end(); ++it) {
		// If the key is pressed this frame
		if (window->GetKeyState(it->first) == florp::app::ButtonState::Pressed) {
			// Iterate over each pass associated with the key and toggle it
			for (auto& ptr : it->second) {
				// Invert the enabled state
				ptr->Enabled = !ptr->Enabled;
			}
		}
	}
}

PostLayer::PostPass::Sptr PostLayer::__CreatePass(const char* fragmentShader, float scale) const {	
	florp::app::Application* app = florp::app::Application::Get();
	
	RenderBufferDesc mainColor = RenderBufferDesc();
	mainColor.ShaderReadable = true;
	mainColor.Attachment = RenderTargetAttachment::Color0;
	mainColor.Format = RenderTargetType::ColorRgb8;
	
	auto shader = std::make_shared<florp::graphics::Shader>();
	shader->LoadPart(florp::graphics::ShaderStageType::VertexShader, "shaders/post/post.vs.glsl");
	shader->LoadPart(florp::graphics::ShaderStageType::FragmentShader, fragmentShader);
	shader->Link();

	// Each pass gets it's own copy of the frame buffer to avoid pipeline stalls
	auto output = std::make_shared<FrameBuffer>(app->GetWindow()->GetWidth() * scale, app->GetWindow()->GetHeight() * scale);
	output->AddAttachment(mainColor);
	output->Validate();

	// Return the pass that we just created
	auto result = std::make_shared<PostPass>();
	result->Shader = shader;
	result->Output = output;
	return result;
}
