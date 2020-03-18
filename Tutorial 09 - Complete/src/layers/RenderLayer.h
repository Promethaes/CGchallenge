#pragma once
#include "florp/app/ApplicationLayer.h"
#include "FrameBuffer.h"

class RenderLayer : public florp::app::ApplicationLayer
{
public:
	virtual void OnWindowResize(uint32_t width, uint32_t height) override;
	
	virtual void OnSceneEnter() override;
	
	// Render will be where we actually perform our rendering
	virtual void Render() override;
};
