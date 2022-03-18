#pragma once
#include "Application/ApplicationLayer.h"
#include "json.hpp"

#include "Gameplay/Scene.h"
#include "Gameplay/GameObject.h"

/**
 * This example layer handles creating a default test scene, which we will use 
 * as an entry point for creating a sample scene
 */
class DefaultSceneLayer final : public ApplicationLayer {
public:
	MAKE_PTRS(DefaultSceneLayer)

	DefaultSceneLayer();
	virtual ~DefaultSceneLayer();

	// Inherited from ApplicationLayer

	virtual void OnUpdate() override;
	virtual void OnAppLoad(const nlohmann::json& config) override;
	// Create an empty scene
	

protected:
	void _CreateScene();
	Gameplay::Scene::Sptr _currentScene;

	bool cameraTest = true;
	bool moveLeft = true;
	bool moveRight = true;
	bool moveDown = true;
	bool moveUp = true;
	bool Luted = false;
	bool ambientl = false;
	bool nolight = false;

	float currTime = 0.0f;
	float workload = 0.0f;
	int colourpick = 0;

	float jump1 = 2.0f;
	float jump2 = 0.0f;
	float jump3 = 0.0f;
	float jump4 = 0.0f;
	float jump5 = 0.0f;
	float jump6 = 0.0f;
	float jump7 = 0.0f;
	float jump8 = 0.0f;
	bool jumping1 = false;
	bool jumping2 = false;
	bool jumping3 = false;
	bool jumping4 = false;
	bool jumping5 = false;
	bool jumping6 = false;
	bool jumping7 = false;
	bool jumping8 = false;
	bool jumped1 = false;
	bool jumped2 = false;
	bool jumped3 = false;
	bool jumped4 = false;
	bool jumped5 = false;
	bool jumped6 = false;
	bool jumped7 = false;
	bool jumped8 = false;

	float whomst = 0;

	Gameplay::GameObject::Sptr lightPar;
};