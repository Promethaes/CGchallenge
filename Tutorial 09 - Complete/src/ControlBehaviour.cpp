#include "ControlBehaviour.h"
#include "florp/app/Window.h"
#include "florp/app/Application.h"
#include "florp/game/Transform.h"
#include "florp/game/SceneManager.h"
#include "florp/app/Timing.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <GLM/gtx/wrap.hpp>

template <typename T>
T wrap(const T& value, const T& min, const T& max) {
	//(((x - x_min) % (x_max - x_min)) + (x_max - x_min)) % (x_max - x_min) + x_min;
	T range = max - min;
	return glm::mod(glm::mod(value - min, range) + range, range )+ min;
}

void ControlBehaviour::Update(entt::entity entity) {
	using namespace florp::app;
	auto& transform = CurrentRegistry().get<florp::game::Transform>(entity);
	Window::Sptr window = Application::Get()->GetWindow();

	glm::vec3 translate = glm::vec3(0.0f);
	glm::vec2 rotation = glm::vec2(0.0f);
	if (window->IsKeyDown(Key::W))
		translate.z -= 1.0f;
	if (window->IsKeyDown(Key::S))
		translate.z += 1.0f;
	if (window->IsKeyDown(Key::A))
		translate.x -= 1.0f;
	if (window->IsKeyDown(Key::D))
		translate.x += 1.0f;
	if (window->IsKeyDown(Key::LeftControl))
		translate.y -= 1.0f;
	if (window->IsKeyDown(Key::Space))
		translate.y += 1.0f;

	//if (window->IsKeyDown(Key::Left))
	//	rotation.x += 1.0f;
	//if (window->IsKeyDown(Key::Right))
	//	rotation.x -= 1.0f;
	//if (window->IsKeyDown(Key::Up))
	//	rotation.y += 1.0f;
	//if (window->IsKeyDown(Key::Down))
	//	rotation.y -= 1.0f;

	translate *= Timing::DeltaTime * mySpeed;
	rotation *= Timing::DeltaTime * 90.0f;

	if (glm::length(translate) > 0) {
		translate = glm::mat3(transform.GetLocalTransform()) * translate;
		translate += transform.GetLocalPosition();
		transform.SetPosition(translate);
	}
	if (glm::length(rotation) > 0) {
		rotation.x *= glm::abs(myYawPitch.y) > 90 ? -1 : 1;
		myYawPitch += rotation;
		myYawPitch = wrap(myYawPitch, glm::vec2(-180.0f), glm::vec2(180.0f));
		glm::quat rot = glm::angleAxis(glm::radians(myYawPitch.x), glm::vec3(0, 1, 0));
		rot = rot * glm::angleAxis(glm::radians(myYawPitch.y), glm::vec3(1, 0, 0));
		transform.SetRotation(rot);
	}
}

void RandomBehaviour::Update(entt::entity entity)
{
	//srand(time(0));
	using namespace florp::app;
	auto& transform = CurrentRegistry().get<florp::game::Transform>(entity);
	Window::Sptr window = Application::Get()->GetWindow();

	glm::vec3 translate = glm::vec3(0.0f);
	glm::vec2 rotation = glm::vec2(0.0f);

	translate.x = (float)(rand() % 10) * (float)(rand() % 2 == 0 ? -1 : 1);
	translate.y = (float)(rand() % 10) * (float)(rand() % 2 == 0 ? -1 : 1);
	translate.z = (float)(rand() % 10) * (float)(rand() % 2 == 0 ? -1 : 1);

	translate *= Timing::DeltaTime;

	if (glm::length(translate) > 0) {
		translate = glm::mat3(transform.GetLocalTransform()) * translate;
		translate += transform.GetLocalPosition();
		transform.SetPosition(translate);
	}
}
