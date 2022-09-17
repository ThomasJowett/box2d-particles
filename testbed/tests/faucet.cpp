/*
* Copyright (c) 2014 Google, Inc.
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

#include "particle_emitter.h"
#include "particle_parameter.h"
#include "test.h"
#include "settings.h"

// Faucet test creates a container from boxes and continually spawning
// particles with finite lifetimes that pour into the box.
class Faucet : public Test
{
private:
	// Assigns a random lifetime to each created particle.
	class ParticleLifetimeRandomizer : public EmittedParticleCallback
	{
	public:
		// Initialize the randomizer to set lifetimes between minLifetime to
		// maxLifetime.
		ParticleLifetimeRandomizer(const float minLifetime,
								   const float maxLifetime) :
			m_minLifetime(minLifetime), m_maxLifetime(maxLifetime) { }
		virtual ~ParticleLifetimeRandomizer() { }

		// Called for each created particle.
		virtual void ParticleCreated(b2ParticleSystem * const system,
									 const int32 particleIndex)
		{
			system->SetParticleLifetime(
				particleIndex, ((float)rand() / (float)RAND_MAX) *
					(m_maxLifetime - m_minLifetime) + m_minLifetime);
		}

	private:
		float m_minLifetime;
		float m_maxLifetime;
	};

public:
	// Construct the world.
	Faucet() :
		m_particleColorOffset(0.0f),
		m_lifetimeRandomizer(k_particleLifetimeMin, k_particleLifetimeMax)
	{
		// Configure particle system parameters.
		m_particleSystem->SetRadius(0.035f);
		m_particleSystem->SetMaxParticleCount(k_maxParticleCount);
		m_particleSystem->SetDestructionByAge(true);

		b2Body* ground = NULL;
		{
			b2BodyDef bd;
			ground = m_world->CreateBody(&bd);
		}

		// Create the container / trough style sink.
		{
			b2PolygonShape shape;
			const float height = k_containerHeight + k_containerThickness;
			shape.SetAsBox(k_containerWidth - k_containerThickness,
						   k_containerThickness, b2Vec2(0.0f, 0.0f), 0.0f);
			ground->CreateFixture(&shape, 0.0f);
			shape.SetAsBox(k_containerThickness, height,
						   b2Vec2(-k_containerWidth, k_containerHeight), 0.0f);
			ground->CreateFixture(&shape, 0.0f);
			shape.SetAsBox(k_containerThickness, height,
						   b2Vec2(k_containerWidth, k_containerHeight), 0.0f);
			ground->CreateFixture(&shape, 0.0f);
		}

		// Create ground under the container to catch overflow.
		{
			b2PolygonShape shape;
			shape.SetAsBox(k_containerWidth * 5.0f, k_containerThickness,
						   b2Vec2(0.0f, k_containerThickness * -2.0f), 0.0f);
			ground->CreateFixture(&shape, 0.0f);
		}

		// Create the faucet spout.
		{
			b2PolygonShape shape;
			const float particleDiameter =
				m_particleSystem->GetRadius() * 2.0f;
			const float faucetLength = k_faucetLength * particleDiameter;
			// Dimensions of the faucet in world units.
			const float length = faucetLength * k_spoutLength;
			const float width = k_containerWidth * k_faucetWidth *
				k_spoutWidth;
			// Height from the bottom of the container.
			const float height = (k_containerHeight * k_faucetHeight) +
				(length * 0.5f);

			shape.SetAsBox(particleDiameter, length,
						   b2Vec2(-width, height), 0.0f);
			ground->CreateFixture(&shape, 0.0f);
			shape.SetAsBox(particleDiameter, length,
						   b2Vec2(width, height), 0.0f);
			ground->CreateFixture(&shape, 0.0f);
			shape.SetAsBox(width - particleDiameter, particleDiameter,
						   b2Vec2(0.0f, height + length -
								  particleDiameter), 0.0f);
			ground->CreateFixture(&shape, 0.0f);
		}

		// Initialize the particle emitter.
		{
			const float faucetLength =
				m_particleSystem->GetRadius() * 2.0f * k_faucetLength;
			m_emitter.SetParticleSystem(m_particleSystem);
			m_emitter.SetCallback(&m_lifetimeRandomizer);
			m_emitter.SetPosition(b2Vec2(k_containerWidth * k_faucetWidth,
										 k_containerHeight * k_faucetHeight +
										 (faucetLength * 0.5f)));
			m_emitter.SetVelocity(b2Vec2(0.0f, 0.0f));
			m_emitter.SetSize(b2Vec2(0.0f, faucetLength));
			m_emitter.SetColor(b2ParticleColor(255, 255, 255, 255));
			m_emitter.SetEmitRate(120.0f);
			m_emitter.SetParticleFlags(GetParticleParameterValue());
		}

		// Don't restart the test when changing particle types.
		SetRestartOnParticleParameterChange(false);
		// Limit the set of particle types.
		SetParticleParameters(k_paramDef, k_paramDefCount);
	}

	// Run a simulation step.
	void Step(Settings& settings)
	{
		const float dt = 1.0f / settings.m_hertz;
		Test::Step(settings);
		m_particleColorOffset += dt;
		// Keep m_particleColorOffset in the range 0.0f..k_ParticleColorsCount.
		if (m_particleColorOffset >= (float)k_ParticleColorsCount)
		{
			m_particleColorOffset -= (float)k_ParticleColorsCount;
		}

		// Propagate the currently selected particle flags.
		m_emitter.SetParticleFlags(GetParticleParameterValue());

		// If this is a color mixing particle, add some color.
		b2ParticleColor color(255, 255, 255, 255);
		if (m_emitter.GetParticleFlags() & b2_colorMixingParticle)
		{
			// Each second, select a different color.
			m_emitter.SetColor(k_ParticleColors[(int32)m_particleColorOffset %
												k_ParticleColorsCount]);
		}
		else
		{
			m_emitter.SetColor(b2ParticleColor(255, 255, 255, 255));
		}

		// Create the particles.
		m_emitter.Step(dt, NULL, 0);

		static const char* k_keys[] = {
			"Keys: (w) water, (q) powder",
			"      (t) tensile, (v) viscous",
			"      (c) color mixing, (s) static pressure",
			"      (+) increase flow, (-) decrease flow",
		};
		for (uint32 i = 0; i < sizeof(k_keys) / sizeof(k_keys[0]); ++i)
		{
			g_debugDraw.DrawString(5, m_textLine, k_keys[i]);
			m_textLine += m_textIncrement;
		}
	}

	// Allows you to set particle flags on devices with keyboards
	void Keyboard(int key) override
	{
		uint32 parameter = 0;
		switch (key)
		{
		case GLFW_KEY_W:
			parameter = b2_waterParticle;
			break;
		case GLFW_KEY_Q:
			parameter = b2_powderParticle;
			break;
		case GLFW_KEY_T:
			parameter = b2_tensileParticle;
			break;
		case GLFW_KEY_V:
			parameter = b2_viscousParticle;
			break;
		case GLFW_KEY_C:
			parameter = b2_colorMixingParticle;
			break;
		case GLFW_KEY_S:
			parameter = b2_staticPressureParticle;
			break;
		case GLFW_KEY_EQUAL:
		{
			float emitRate = m_emitter.GetEmitRate();
			emitRate *= k_emitRateChangeFactor;
			emitRate = b2Max(emitRate, k_emitRateMin);
			m_emitter.SetEmitRate(emitRate);
			break;
		}
		case GLFW_KEY_MINUS:
		{
			float emitRate = m_emitter.GetEmitRate();
			emitRate *= 1.0f / k_emitRateChangeFactor;
			emitRate = b2Min(emitRate, k_emitRateMax);
			m_emitter.SetEmitRate(emitRate);
		}
			break;
		default:
			// Nothing.
			return;
		}
		SetParticleParameterValue(parameter);
	}

	float GetDefaultViewZoom() const
	{
		return 0.1f;
	}

	// Create the faucet test.
	static Test* Create()
	{
		return new Faucet;
	}

private:
	// Used to cycle through particle colors.
	float m_particleColorOffset;
	// Particle emitter.
	RadialEmitter m_emitter;
	// Callback which sets the lifetime of emitted particles.
	ParticleLifetimeRandomizer m_lifetimeRandomizer;

private:
	// Minimum lifetime of particles in seconds.
	static const float k_particleLifetimeMin;
	// Maximum lifetime of particles in seconds.
	static const float k_particleLifetimeMax;
	// Height of the container.
	static const float k_containerHeight;
	// Width of the container.
	static const float k_containerWidth;
	// Thickness of the container's walls and bottom.
	static const float k_containerThickness;
	// Width of the faucet relative to the container width.
	static const float k_faucetWidth;
	// Height of the faucet relative to the base as a fraction of the
	// container height.
	static const float k_faucetHeight;
	// Length of the faucet as a fraction of the particle diameter.
	static const float k_faucetLength;
	// Spout height as a fraction of the faucet length.  This should be
	// greater than 1.0f).
	static const float k_spoutLength;
	// Spout width as a fraction of the *faucet* width.  This should be greater
	// than 1.0f).
	static const float k_spoutWidth;
	// Maximum number of particles in the system.
	static const int32 k_maxParticleCount;
	// Factor that is used to increase / decrease the emit rate.
	// This should be greater than 1.0f.
	static const float k_emitRateChangeFactor;
	// Minimum emit rate of the faucet in particles per second.
	static const float k_emitRateMin;
	// Maximum emit rate of the faucet in particles per second.
	static const float k_emitRateMax;

	// Selection of particle types for this test.
	static const ParticleParameter::Value k_paramValues[];
	static const ParticleParameter::Definition k_paramDef[];
	static const uint32 k_paramDefCount;
};

const float Faucet::k_particleLifetimeMin = 30.0f;
const float Faucet::k_particleLifetimeMax = 50.0f;
const float Faucet::k_containerHeight = 0.2f;
const float Faucet::k_containerWidth = 1.0f;
const float Faucet::k_containerThickness = 0.05f;
const float Faucet::k_faucetWidth = 0.1f;
const float Faucet::k_faucetHeight = 15.0f;
const float Faucet::k_faucetLength = 2.0f;
const float Faucet::k_spoutWidth = 1.1f;
const float Faucet::k_spoutLength = 2.0f;
const int32 Faucet::k_maxParticleCount = 1000;
const float Faucet::k_emitRateChangeFactor = 1.05f;
const float Faucet::k_emitRateMin = 1.0f;
const float Faucet::k_emitRateMax = 240.0f;

const ParticleParameter::Value Faucet::k_paramValues[] =
{
	{ b2_waterParticle, ParticleParameter::k_DefaultOptions, "water" },
	{ b2_waterParticle, ParticleParameter::k_DefaultOptions |
				ParticleParameter::OptionStrictContacts, "water (strict)" },
	{ b2_viscousParticle, ParticleParameter::k_DefaultOptions, "viscous" },
	{ b2_powderParticle, ParticleParameter::k_DefaultOptions, "powder" },
	{ b2_tensileParticle, ParticleParameter::k_DefaultOptions, "tensile" },
	{ b2_colorMixingParticle, ParticleParameter::k_DefaultOptions,
		"color mixing" },
	{ b2_staticPressureParticle, ParticleParameter::k_DefaultOptions,
		"static pressure" },
};

const ParticleParameter::Definition Faucet::k_paramDef[] =
{
	{ Faucet::k_paramValues, sizeof(Faucet::k_paramValues) / sizeof(Faucet::k_paramValues[0])},
};

const uint32 Faucet::k_paramDefCount = sizeof(Faucet::k_paramDef) / sizeof(Faucet::k_paramDef[0]);

static int testIndex = RegisterTest("Particles", "Faucet", Faucet::Create);
