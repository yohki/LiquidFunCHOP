#pragma once
#include "SceneBase.h"
#include "Testbed/Framework/ParticleEmitter.h"

class Faucet : public SceneBase {

public:
	const int k_maxParticleCount = 1000;
	const float k_particleLifetimeMin = 30.0f;
	const float k_particleLifetimeMax = 50.0f;
	const float k_containerHeight = 0.2f;
	const float k_containerWidth = 1.0f;
	const float k_containerThickness = 0.05f;
	const float k_faucetWidth = 0.1f;
	const float k_faucetHeight = 15.0f;
	const float k_faucetLength = 2.0f;
	const float k_spoutWidth = 1.1f;
	const float k_spoutLength = 2.0f;
	const float k_emitRateChangeFactor = 1.05f;
	const float k_emitRateMin = 1.0f;
	const float k_emitRateMax = 240.0f;

	virtual void setup(b2World* world, b2ParticleSystem* particleSystem, const OP_Inputs* inputs) override {
		
		// Configure particle system parameters.
		particleSystem->SetRadius(0.035f);
		particleSystem->SetMaxParticleCount(k_maxParticleCount);
		particleSystem->SetDestructionByAge(true);

		b2Body* ground = NULL;
		{
			b2BodyDef bd;
			ground = world->CreateBody(&bd);
		}

		// Create the container / trough style sink.
		{
			b2PolygonShape shape;
			const float height = k_containerHeight + k_containerThickness;
			shape.SetAsBox(k_containerWidth - k_containerThickness, k_containerThickness, b2Vec2(0.0f, 0.0f), 0.0f);
			ground->CreateFixture(&shape, 0.0f);
			shape.SetAsBox(k_containerThickness, height, b2Vec2(-k_containerWidth, k_containerHeight), 0.0f);
			ground->CreateFixture(&shape, 0.0f);
			shape.SetAsBox(k_containerThickness, height, b2Vec2(k_containerWidth, k_containerHeight), 0.0f);
			ground->CreateFixture(&shape, 0.0f);
		}

		// Create ground under the container to catch overflow.
		{
			b2PolygonShape shape;
			shape.SetAsBox(k_containerWidth * 5.0f, k_containerThickness, b2Vec2(0.0f, k_containerThickness * -2.0f), 0.0f);
			ground->CreateFixture(&shape, 0.0f);
		}

		// Create the faucet spout.
		{
			b2PolygonShape shape;
			const float32 particleDiameter = particleSystem->GetRadius() * 2.0f;
			const float32 faucetLength = k_faucetLength * particleDiameter;
			// Dimensions of the faucet in world units.
			const float32 length = faucetLength * k_spoutLength;
			const float32 width = k_containerWidth * k_faucetWidth * k_spoutWidth;
			// Height from the bottom of the container.
			const float32 height = (k_containerHeight * k_faucetHeight) + (length * 0.5f);

			shape.SetAsBox(particleDiameter, length, b2Vec2(-width, height), 0.0f);
			ground->CreateFixture(&shape, 0.0f);
			shape.SetAsBox(particleDiameter, length, b2Vec2(width, height), 0.0f);
			ground->CreateFixture(&shape, 0.0f);
			shape.SetAsBox(width - particleDiameter, particleDiameter, b2Vec2(0.0f, height + length - particleDiameter), 0.0f);
			ground->CreateFixture(&shape, 0.0f);
		}

		// Initialize the particle emitter.
		{
			const float32 faucetLength = particleSystem->GetRadius() * 2.0f * k_faucetLength;
			_emitter.SetParticleSystem(particleSystem);
			//_emitter.SetCallback(&m_lifetimeRandomizer);
			_emitter.SetPosition(b2Vec2(k_containerWidth * k_faucetWidth, k_containerHeight * k_faucetHeight +
				(faucetLength * 0.5f)));
			_emitter.SetVelocity(b2Vec2(0.0f, 0.0f));
			_emitter.SetSize(b2Vec2(0.0f, faucetLength));
			_emitter.SetColor(b2ParticleColor(255, 255, 255, 255));
			_emitter.SetEmitRate(120.0f);
			//_emitter.SetParticleFlags(TestMain::GetParticleParameterValue());
		}
	}

	virtual void update(float dt) {
		_emitter.Step(dt, NULL, 0);
	}
private:
	RadialEmitter _emitter;
};