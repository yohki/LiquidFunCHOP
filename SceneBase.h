#pragma once
#include "Box2D/Box2D.h"
#include "CHOP_CPlusPlusBase.h"

class SceneBase {

public:
	virtual void setup(b2World* world, b2ParticleSystem* particleSystem, const OP_Inputs* inputs) {}
	virtual void update(float dt) {}
};