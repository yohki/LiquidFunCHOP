#pragma once
#include "SceneBase.h"

class WaveMachine : public SceneBase {
public:
	virtual void setup(b2World* world, b2ParticleSystem* particleSystem, const OP_Inputs* inputs) override {
		b2Body* ground = NULL;
		{
			b2BodyDef bd;
			ground = world->CreateBody(&bd);
		}

		{
			b2BodyDef bd;
			bd.type = b2_dynamicBody;
			bd.allowSleep = false;
			bd.position.Set(0.0f, 1.0f);
			b2Body* body = world->CreateBody(&bd);

			b2PolygonShape shape;
			shape.SetAsBox(0.05f, 1.0f, b2Vec2(2.0f, 0.0f), 0.0);
			body->CreateFixture(&shape, 5.0f);
			shape.SetAsBox(0.05f, 1.0f, b2Vec2(-2.0f, 0.0f), 0.0);
			body->CreateFixture(&shape, 5.0f);
			shape.SetAsBox(2.0f, 0.05f, b2Vec2(0.0f, 1.0f), 0.0);
			body->CreateFixture(&shape, 5.0f);
			shape.SetAsBox(2.0f, 0.05f, b2Vec2(0.0f, -1.0f), 0.0);
			body->CreateFixture(&shape, 5.0f);

			b2RevoluteJointDef jd;
			jd.bodyA = ground;
			jd.bodyB = body;
			jd.localAnchorA.Set(0.0f, 1.0f);
			jd.localAnchorB.Set(0.0f, 0.0f);
			jd.referenceAngle = 0.0f;
			jd.motorSpeed = 0.05f * b2_pi;
			jd.maxMotorTorque = 1e7f;
			jd.enableMotor = true;
			_joint = (b2RevoluteJoint*)world->CreateJoint(&jd);
		}

		{
			b2ParticleGroupDef pd;

			b2PolygonShape shape;
			shape.SetAsBox(0.9f, 0.9f, b2Vec2(0.0f, 1.0f), 0.0);

			pd.shape = &shape;
			b2ParticleGroup* const group = particleSystem->CreateParticleGroup(pd);
		}

		_time = 0;
	}

	virtual void update(float dt) {
		_time += dt;
		_joint->SetMotorSpeed(0.05f * cosf(_time) * b2_pi);
	}

private:
	b2RevoluteJoint* _joint;
	float32 _time;
};