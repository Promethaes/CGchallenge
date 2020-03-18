#include "LightingLayer.h"
#include <florp\game\SceneManager.h>
#include <florp\game\Transform.h>
#include <florp\game\RenderableComponent.h>
#include <ShadowLight.h>
#include "florp/app/Application.h"
#include "FrameState.h"
#include <imgui.h>
#include "PointLightComponent.h"

void LightingLayer::OnWindowResize(uint32_t width, uint32_t height) {
	myAccumulationBuffer->Resize(width, height);
}

void LightingLayer::Initialize() {
	florp::app::Application* app = florp::app::Application::Get();
	
	using namespace florp::graphics;

	isProcessingShadows = isProcessingPointLights = true;

	// We'll set our ambient light to be some very small amount (this will be for the entire scene)
	myAmbientLight = glm::vec3(0.01f);
	
	// The normal shader will handle depth map generation for shadow casting lights (note that we'll just use the default, fallback fragment shader)
	myShader = std::make_shared<Shader>();
	myShader->LoadPart(ShaderStageType::VertexShader, "shaders/simple.vs.glsl");
	myShader->Link(); 

	// The masked shader will be used if we have a light that will mask off areas to not cast shadows (for instance, behind a grate)
	myMaskedShader = std::make_shared<Shader>();
	myMaskedShader->LoadPart(ShaderStageType::VertexShader, "shaders/simple.vs.glsl");  
	myMaskedShader->LoadPart(ShaderStageType::FragmentShader, "shaders/shadow_masked.fs.glsl");  
	myMaskedShader->Link();

	// The shadow composite shader will handle adding shadow casting and projector lights to our accumulation buffer
	myShadowComposite = std::make_shared<Shader>();
	myShadowComposite->LoadPart(ShaderStageType::VertexShader, "shaders/post/post.vs.glsl");
	myShadowComposite->LoadPart(ShaderStageType::FragmentShader, "shaders/post/shadow_post.fs.glsl");
	myShadowComposite->Link();

	myPointLightComposite = std::make_shared<Shader>();
	myPointLightComposite->LoadPart(ShaderStageType::VertexShader, "shaders/post/post.vs.glsl");
	myPointLightComposite->LoadPart(ShaderStageType::FragmentShader, "shaders/post/blinn-phong-post.fs.glsl");
	myPointLightComposite->Link();

	// The final composite shader will handle applying the lighting, and doing our HDR correction for later passes
	myFinalComposite = std::make_shared<Shader>();
	myFinalComposite->LoadPart(ShaderStageType::VertexShader, "shaders/post/post.vs.glsl");
	myFinalComposite->LoadPart(ShaderStageType::FragmentShader, "shaders/post/lighting_composite.fs.glsl");
	myFinalComposite->Link();
	myFinalComposite->SetUniform("a_Exposure", 1.0f);


	// Our accumulation buffer will be a floating-point buffer, so we can do some HDR lighting effects
	RenderBufferDesc mainColor = RenderBufferDesc();
	mainColor.ShaderReadable = true;
	mainColor.Attachment = RenderTargetAttachment::Color0;
	mainColor.Format = RenderTargetType::ColorRgb16F;

	// We'll use one buffer to accumulate all the lighting
	myAccumulationBuffer = std::make_shared<FrameBuffer>(app->GetWindow()->GetWidth(), app->GetWindow()->GetHeight());
	myAccumulationBuffer->AddAttachment(mainColor);
	myAccumulationBuffer->Validate();
	myAccumulationBuffer->SetDebugName("Intermediate");

	// Create our fullscreen quad (just like in PostLayer)
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
}

void LightingLayer::PreRender()
{
	if (isProcessingShadows) {
		using namespace florp::game;
		using namespace florp::graphics;

		auto& ecs = CurrentRegistry();

		// We'll only handle stuff if we actually have a shadow casting light in the scene
		auto view = ecs.view<ShadowLight>();
		if (view.size() > 0) {
			// We'll make sure depth testing and culling are enabled
			glEnable(GL_DEPTH_TEST);
			glEnable(GL_CULL_FACE);
			glCullFace(GL_FRONT); // enable front face culling

			// Iterate over all the shadow casting lights
			Shader::Sptr shader = nullptr;
			ecs.view<ShadowLight>().each([&](auto entity, ShadowLight& light) {
				// Get the light's transform
				const Transform& lightTransform = ecs.get<Transform>(entity);

				// Select which shader to use depending on if the light has a mask or not
				if (light.Mask == nullptr) {
					shader = myShader;
				}
				else {
					shader = myMaskedShader;
					light.Mask->Bind(0);
				}
				// Use the shader, and tell it what our output resolution is
				shader->Use();
				shader->SetUniform("a_OutputResolution", (glm::vec2)light.ShadowBuffer->GetSize());

				// Bind, viewport, and clear
				light.ShadowBuffer->Bind();
				glViewport(0, 0, light.ShadowBuffer->GetWidth(), light.ShadowBuffer->GetHeight());
				glClear(GL_DEPTH_BUFFER_BIT);

				// Determine the position and matrices for the light
				glm::vec3 position = lightTransform.GetLocalPosition();
				glm::mat4 viewMatrix = glm::inverse(lightTransform.GetWorldTransform());
				glm::mat4 viewProjection = light.Projection * viewMatrix;

				// We're going to iterate over every renderable component
				auto view = ecs.view<RenderableComponent>();

				for (const auto& entity : view) {
					// Get our shader
					const RenderableComponent& renderer = ecs.get<RenderableComponent>(entity);

					// Early bail if mesh is invalid (or if if does not cast a shadow)
					if (renderer.Mesh == nullptr || renderer.Material == nullptr || !renderer.Material->IsShadowCaster)
						continue;

					// We'll need some info about the entities position in the world
					const Transform& transform = ecs.get_or_assign<Transform>(entity);

					// Update the MVP using the item's transform
					shader->SetUniform(
						"a_ModelViewProjection",
						viewProjection *
						transform.GetWorldTransform());

					// Draw the item
					renderer.Mesh->Draw();
				}

				// Unbind so that we can use the texture later
				light.ShadowBuffer->UnBind();
				});

			glCullFace(GL_BACK); // enable back face culling
		}
	}
}

void LightingLayer::PostRender() {

	// We grab the application singleton to get the size of the screen
	florp::app::Application* app = florp::app::Application::Get();
	auto& ecs = CurrentRegistry();

	// We'll get the back buffer from the frame state
	const AppFrameState& state = ecs.ctx<AppFrameState>();
	FrameBuffer::Sptr mainBuffer = state.Current.Output;
	
	// Bind and clear our lighting accumulation buffer
	myAccumulationBuffer->Bind();
	glClearColor(myAmbientLight.r, myAmbientLight.g, myAmbientLight.b, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	
	// Disable Depth testing, and enable additive blending
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	
	// Do our light post processing
	if (isProcessingShadows) { PostProcessShadows(); }
	if (isProcessingPointLights) { PostProcessLights(); }
	
	// Unbind the accumulation buffer so we can blend it with the main scene
	myAccumulationBuffer->UnBind();

	// Disable blending, we will overwrite the contents now
	glDisable(GL_BLEND);

	// Set the main buffer as the output again
	mainBuffer->Bind();
	// We'll use an additive shader for now, this should be a multiply with the albedo of the scene
	myFinalComposite->Use();
	// We'll combine the GBuffer color and our lighting contributions
	mainBuffer->Bind(1, RenderTargetAttachment::Color0);
	myAccumulationBuffer->Bind(2);
	mainBuffer->Bind(3, RenderTargetAttachment::Color2); // The emissive buffer
	// Render the quad
	myFullscreenQuad->Draw();
	// Unbind main buffer to perform multisample blitting
	mainBuffer->UnBind();   
}

void LightingLayer::RenderGUI()
{
	// We'll put all the lighting stuff into it's own ImGUI window
	ImGui::Begin("Lighting Settings");

	// For now, we'll just have a slider to adjust our exposure
	static float exposure = 1.0f;
	if (ImGui::DragFloat("Exposure", &exposure, 0.1f, 0.1f, 10.0f)) {
		myFinalComposite->SetUniform("a_Exposure", exposure);
	}
	// We'll have a color picker for the ambient light color
	ImGui::ColorEdit3("Ambient", &myAmbientLight.x);

	ImGui::Checkbox("Process Shadows", &isProcessingShadows);
	ImGui::Checkbox("Process Point Lights", &isProcessingPointLights);
	
	ImGui::End();
}

void LightingLayer::PostProcessShadows() {
	// We grab the application singleton to get the size of the screen
	florp::app::Application* app = florp::app::Application::Get();
	auto& ecs = CurrentRegistry();

	// We'll get the back buffer from the frame state
	const AppFrameState& state = ecs.ctx<AppFrameState>();
	FrameBuffer::Sptr mainBuffer = state.Current.Output;

	// We can extract our near and far plane by reversing the projection calculation
	float m22 = state.Current.Projection[2][2];
	float m32 = state.Current.Projection[3][2];
	float nearPlane = (2.0f * m32) / (2.0f * m22 - 2.0f);
	float farPlane = ((m22 - 1.0f) * nearPlane) / (m22 + 1.0);

	// We set up all the camera state once, since we use the same shader for compositing all shadow-casting lights
	myShadowComposite->Use();
	myShadowComposite->SetUniform("a_View", state.Current.View);
	glm::mat4 viewInv = glm::inverse(state.Current.View);
	myShadowComposite->SetUniform("a_ViewInv", viewInv);
	myShadowComposite->SetUniform("a_CameraPos", glm::vec3(viewInv * glm::vec4(0, 0, 0, 1)));
	myShadowComposite->SetUniform("a_ProjectionInv", glm::inverse(state.Current.Projection));
	myShadowComposite->SetUniform("a_ViewProjectionInv", glm::inverse(state.Current.ViewProjection));
	myShadowComposite->SetUniform("a_NearPlane", nearPlane); 
	myShadowComposite->SetUniform("a_FarPlane", farPlane);
	myShadowComposite->SetUniform("a_Bias", 0.000001f);
	myShadowComposite->SetUniform("a_MatShininess", 1.0f); // This should be from the GBuffer

	// Bind our GBuffer textures (note that we skipped 2, since that's the slot for the shadow sampler)
	mainBuffer->Bind(0, RenderTargetAttachment::Color0);
	mainBuffer->Bind(1, RenderTargetAttachment::Depth);
	mainBuffer->Bind(3, RenderTargetAttachment::Color1); // The normal buffer
	
	// Iterate over all the ShadowLights in the scene
	auto view = CurrentRegistry().view<ShadowLight>();
	if (view.size() > 0) {
		view.each([&](auto entity, ShadowLight& light) {
			// Upload light information to the shader
			const florp::game::Transform& transform = ecs.get_or_assign<florp::game::Transform>(entity);

			glm::mat4 lightspaceMatrix = state.Current.View * transform.GetWorldTransform();
			glm::vec3 pos = glm::vec3(lightspaceMatrix * glm::vec4(0, 0, 0, 1)); // Updated

			// If the light has a projector image, we'll treat it as a projector instead
			if (light.ProjectorImage != nullptr) {
				myShadowComposite->SetUniform("b_IsProjector", 1);
				myShadowComposite->SetUniform("a_ProjectorIntensity", light.ProjectorImageIntensity);
				light.ProjectorImage->Bind(4);
			} else { 
				myShadowComposite->SetUniform("b_IsProjector", 0);
			}

			// Upload the light info to the shader
			myShadowComposite->SetUniform("a_LightView", light.Projection * glm::inverse(lightspaceMatrix));
			myShadowComposite->SetUniform("a_LightPos", pos); 
			myShadowComposite->SetUniform("a_LightDir", glm::mat3(lightspaceMatrix) * glm::vec3(0, 0, -1));
			myShadowComposite->SetUniform("a_LightColor", light.Color);
			myShadowComposite->SetUniform("a_LightAttenuation", light.Attenuation); 
			
			// Bind the light's depth and render the quad
			light.ShadowBuffer->Bind(2, RenderTargetAttachment::Depth);
			myFullscreenQuad->Draw();
		});
	}
}

void LightingLayer::PostProcessLights() { 
	// We grab the application singleton to get the size of the screen
	florp::app::Application* app = florp::app::Application::Get(); 
	auto& ecs = CurrentRegistry();

	// We'll get the back buffer from the frame state
	const AppFrameState& state = ecs.ctx<AppFrameState>();
	FrameBuffer::Sptr mainBuffer = state.Current.Output;
	
	// We set up all the camera state once, since we use the same shader for compositing all shadow-casting lights
	myPointLightComposite->Use();
	myPointLightComposite->SetUniform("a_View", state.Current.View);
	myPointLightComposite->SetUniform("a_ProjectionInv", glm::inverse(state.Current.Projection));
	myPointLightComposite->SetUniform("a_ViewProjectionInv", glm::inverse(state.Current.ViewProjection));
	myPointLightComposite->SetUniform("a_MatShininess", 1.0f); // This should be from the GBuffer

	// Bind our G-Buffer to our texture slots
	mainBuffer->Bind(0, RenderTargetAttachment::Color0); // The color buffer
	mainBuffer->Bind(1, RenderTargetAttachment::Depth);
	mainBuffer->Bind(2, RenderTargetAttachment::Color1); // The normal buffer

	// Iterate over all the ShadowLights in the scene
	auto view = CurrentRegistry().view<PointLightComponent>();
	if (view.size() > 0) {
		view.each([&](auto entity, PointLightComponent& light) {
			// Upload light information to the shader
			const florp::game::Transform& transform = ecs.get_or_assign<florp::game::Transform>(entity);
			glm::vec3 pos = glm::vec3(state.Current.View * transform.GetWorldTransform() * glm::vec4(0, 0, 0, 1));
			
			// Upload the light info to the shader
			myPointLightComposite->SetUniform("a_LightPos", pos);
			myPointLightComposite->SetUniform("a_LightColor", light.Color);
			myPointLightComposite->SetUniform("a_LightAttenuation", light.Attenuation);

			myFullscreenQuad->Draw();
		});
	}
}
