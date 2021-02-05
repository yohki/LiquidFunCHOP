#pragma once

#include "SceneBase.h"

class DamBreak : public SceneBase {
public:
	virtual void setup(b2World* world, b2ParticleSystem* particleSystem, const OP_Inputs* inputs) override {
		b2BodyDef bd;
		b2Body* ground = world->CreateBody(&bd);

		b2ChainShape chain;
		const b2Vec2 vertices[4] = {
			b2Vec2(-2, -2),
			b2Vec2(2, -2),
			b2Vec2(2, 2),
			b2Vec2(-2, 2) };
		chain.CreateLoop(vertices, 4);
		ground->CreateFixture(&chain, 0.0f);


		b2PolygonShape polygon;
		polygon.SetAsBox(0.8f, 1.0f, b2Vec2(-1.2f, -1.01f), 0);
		b2ParticleGroupDef pd;
		int particleType = inputs->getParInt("Particletype");
		if (particleType == 0) {
			pd.groupFlags = b2_solidParticleGroup;
		} else if (particleType == 1) {
			pd.groupFlags = b2_rigidParticleGroup;
		} else if (particleType == 2) {
			pd.flags = b2_elasticParticle;
		}
		pd.shape = &polygon;
		b2ParticleGroup* const group = particleSystem->CreateParticleGroup(pd);
		//if (pd.flags & b2_colorMixingParticle) {
		//	ColorParticleGroup(group, 0);
		//}
	}
};