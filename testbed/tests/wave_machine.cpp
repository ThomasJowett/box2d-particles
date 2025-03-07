/*
* Copyright (c) 2013 Google, Inc.
*
* This software is provided 'as-is', without any express or implied
* warranty.  In no event will the authors be held liable for any damages
* arising from the use of this software.
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
*/

#include "test.h"
#include "settings.h"

class WaveMachine : public Test
{
public:

	WaveMachine()
	{

		b2Body* ground = NULL;
		{
			b2BodyDef bd;
			ground = m_world->CreateBody(&bd);
		}

		{
			b2BodyDef bd;
			bd.type = b2_dynamicBody;
			bd.allowSleep = false;
			bd.position.Set(0.0f, 1.0f);
			b2Body* body = m_world->CreateBody(&bd);

			b2PolygonShape shape;
			shape.SetAsBox(0.05f, 1.0f, b2Vec2( 2.0f, 0.0f), 0.0);
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
			m_joint = (b2RevoluteJoint*)m_world->CreateJoint(&jd);
		}

		m_particleSystem->SetRadius(0.025f);
		const uint32 particleType = GetParticleParameterValue();
		m_particleSystem->SetDamping(0.2f);

		{
			b2ParticleGroupDef pd;
			pd.flags = particleType;

			b2PolygonShape shape;
			shape.SetAsBox(0.9f, 0.9f, b2Vec2(0.0f, 1.0f), 0.0);

			pd.shape = &shape;
			b2ParticleGroup * const group = m_particleSystem->CreateParticleGroup(pd);
			if (pd.flags & b2_colorMixingParticle)
			{
				ColorParticleGroup(group, 0);
			}
		}

		m_time = 0;
	}

	void Step(Settings& settings)
	{
		Test::Step(settings);
		if (settings.m_hertz > 0)
		{
			m_time += 1 / settings.m_hertz;
		}
		m_joint->SetMotorSpeed(0.05f * cosf(m_time) * b2_pi);
	}

	float GetDefaultViewZoom() const
	{
		return 0.1f;
	}

	static Test* Create()
	{
		return new WaveMachine;
	}

	b2RevoluteJoint* m_joint;
	float m_time;
};

static int testIndex = RegisterTest("Particles", "Wave machine", WaveMachine::Create);
