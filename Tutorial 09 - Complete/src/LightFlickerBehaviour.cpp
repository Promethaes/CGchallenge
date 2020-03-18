#include "LightFlickerBehaviour.h"
#include "PointLightComponent.h"
#include "florp/game/SceneManager.h"
#include "Logging.h"
#include "ShadowLight.h"
#include "florp/app/Timing.h"

LightFlickerBehaviour::LightFlickerBehaviour(float speed, float min, float max) :
	myFlickerSpeed(speed), myMin(min), myMax(max) { }

void LightFlickerBehaviour::OnLoad(entt::entity entity)
{
	auto& ecs = CurrentRegistry();
	if (ecs.has<PointLightComponent>(entity)) {
		PointLightComponent& light = ecs.get<PointLightComponent>(entity);
		myInitialAttenuation = light.Attenuation;
	}
	else if (ecs.has<ShadowLight>(entity)) {
		ShadowLight& light = ecs.get<ShadowLight>(entity);
		myInitialAttenuation = light.Attenuation;
	} else {
		LOG_ASSERT(false, "You must attach a point light or shadow light component before a light flicker behaviour!");
	}

	myTimingOffset = (rand() / (float)RAND_MAX) * myFlickerSpeed;
}

void LightFlickerBehaviour::Update(entt::entity entity) {
	auto& ecs = CurrentRegistry();

	float time = (florp::app::Timing::GameTime + myTimingOffset) * myFlickerSpeed;
	float intensity = glm::sin(time);
	intensity = myMin + glm::fract(intensity) * (myMax - myMin);
	float attenuation = myInitialAttenuation / intensity;
	
	if (ecs.has<PointLightComponent>(entity)) {
		PointLightComponent& light = ecs.get<PointLightComponent>(entity);
		light.Attenuation = attenuation;
	}
	else if (ecs.has<ShadowLight>(entity)) {
		ShadowLight& light = ecs.get<ShadowLight>(entity);
		light.Attenuation = attenuation;
	}
}
