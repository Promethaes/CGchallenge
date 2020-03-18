#include "AudioLayer.h"
#include "AudioEngine.h"

void AudioLayer::Initialize()
{
	AudioEngine::Init();
	AudioEngine::LoadSound("AH.wav", false, true, true);
	AudioEngine::PlaySound("AH.wav");
}

void AudioLayer::Shutdown()
{
	AudioEngine::Shutdown();
}

void AudioLayer::Update()
{
	AudioEngine::Update();
}
