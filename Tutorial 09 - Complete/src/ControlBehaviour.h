#pragma once
#include "florp/game/IBehaviour.h"
#include <GLM/glm.hpp>

class ControlBehaviour : public florp::game::IBehaviour {
public:
	ControlBehaviour(const glm::vec3& speed) : IBehaviour(), mySpeed(speed), myYawPitch(glm::vec2(0.0f)) {};
	virtual ~ControlBehaviour() = default;

	virtual void Update(entt::entity entity) override;

private:
	glm::vec3 mySpeed;
	glm::vec2 myYawPitch;
};

class RandomBehaviour : public florp::game::IBehaviour {
public:
	RandomBehaviour() : IBehaviour() {};
	virtual ~RandomBehaviour() = default;

	virtual void Update(entt::entity entity) override;

private:

};

