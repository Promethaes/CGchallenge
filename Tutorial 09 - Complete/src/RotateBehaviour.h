#pragma once
#include "florp/game/IBehaviour.h"
#include "florp/game/Transform.h"
#include "florp/game/SceneManager.h"
#include "florp/app/Timing.h"

class RotateBehaviour : public florp::game::IBehaviour {
public:
	/*
	 * Creates a new rotate behaviour with the given speed
	 * @param speed The speed to rotate along each axis, in degrees per second
	 */
	RotateBehaviour(const glm::vec3& speed) : IBehaviour(), mySpeed(speed) {};
	virtual ~RotateBehaviour() = default;

	virtual void Update(entt::entity entity) override {
		auto& transform = CurrentRegistry().get<florp::game::Transform>(entity);
		transform.Rotate(mySpeed * florp::app::Timing::DeltaTime);
	}

private:
	glm::vec3 mySpeed;
};

class AxialSpinBehaviour : public florp::game::IBehaviour {
public:
	AxialSpinBehaviour(const glm::vec3& center, const glm::vec3& up, float speed) : IBehaviour(), myCenter(center), myUp(up), mySpeed(speed), myAngle(0.0f) {};
	virtual ~AxialSpinBehaviour() = default;

	virtual void OnLoad(entt::entity entity) override {
		auto& transform = CurrentRegistry().get<florp::game::Transform>(entity);
		myStartingPos = transform.GetLocalPosition();
	}
	
	virtual void Update(entt::entity entity) override {
		auto& transform = CurrentRegistry().get<florp::game::Transform>(entity);
		myAngle += mySpeed * florp::app::Timing::DeltaTime;
		glm::vec3 newPos = glm::translate(glm::mat4_cast(glm::angleAxis(glm::radians(myAngle), myUp)), myCenter) * glm::vec4(myStartingPos, 1.0f);
		transform.SetPosition(newPos);
	}

private:
	glm::vec3 myStartingPos;
	glm::vec3 myCenter;
	glm::vec3 myUp;
	float myAngle;
	float mySpeed;
};


