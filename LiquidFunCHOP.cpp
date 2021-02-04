
#include "LiquidFunCHOP.h"

#include <stdio.h>
#include <string.h>
#include <cmath>
#include <assert.h>

// These functions are basic C function, which the DLL loader can find
// much easier than finding a C++ Class.
// The DLLEXPORT prefix is needed so the compile exports these functions from the .dll
// you are creating
extern "C" {

	DLLEXPORT void FillCHOPPluginInfo(CHOP_PluginInfo* info) {
		// Always set this to CHOPCPlusPlusAPIVersion.
		info->apiVersion = CHOPCPlusPlusAPIVersion;

		// The opType is the unique name for this CHOP. It must start with a 
		// capital A-Z character, and all the following characters must lower case
		// or numbers (a-z, 0-9)
		info->customOPInfo.opType->setString("Customsignal");

		// The opLabel is the text that will show up in the OP Create Dialog
		info->customOPInfo.opLabel->setString("Custom Signal");

		// Information about the author of this OP
		info->customOPInfo.authorName->setString("Author Name");
		info->customOPInfo.authorEmail->setString("email@email.com");

		// This CHOP can work with 0 inputs
		info->customOPInfo.minInputs = 0;

		// It can accept up to 1 input though, which changes it's behavior
		info->customOPInfo.maxInputs = 1;
	}

	DLLEXPORT CHOP_CPlusPlusBase* CreateCHOPInstance(const OP_NodeInfo* info) {
		// Return a new instance of your class every time this is called.
		// It will be called once per CHOP that is using the .dll
		return new LiquidFunCHOP(info);
	}

	DLLEXPORT void DestroyCHOPInstance(CHOP_CPlusPlusBase* instance) {
		// Delete the instance here, this will be called when
		// Touch is shutting down, when the CHOP using that instance is deleted, or
		// if the CHOP loads a different DLL
		delete (LiquidFunCHOP*)instance;
	}

};


LiquidFunCHOP::LiquidFunCHOP(const OP_NodeInfo* info) : myNodeInfo(info) {

}

LiquidFunCHOP::~LiquidFunCHOP() {
	delete _world;
	_world = NULL;
}

void LiquidFunCHOP::init(const OP_Inputs* inputs) {
	// World
	double gx = 0;
	double gy = 0;
	inputs->getParDouble2("Gravity", gx, gy);

	b2Vec2 gravity;
	gravity.Set(gx, gy);
	_world = new b2World(gravity);

	// Particle System
	double radius = inputs->getParDouble("Particlesize");
	double gravityScale = inputs->getParDouble("Particlegravityscale");
	double density = inputs->getParDouble("Particledensity");
	double damping = inputs->getParDouble("Particledamping");
	const b2ParticleSystemDef particleSystemDef;
	_particleSystem = _world->CreateParticleSystem(&particleSystemDef);
	_particleSystem->SetGravityScale(0.4f);
	_particleSystem->SetDensity(1.2f);
	_particleSystem->SetRadius(radius);
	_particleSystem->SetDamping(damping);

	b2BodyDef bodyDef;
	_groundBody = _world->CreateBody(&bodyDef);

	int scene = inputs->getParInt("Scene");
	if (scene == 0) {
		initDambreak(inputs);
	} else if (scene == 1) {
		initWaterfall(inputs);
	}
	_initialized = true;

}

void LiquidFunCHOP::initDambreak(const OP_Inputs* inputs) {
	b2BodyDef bd;
	b2Body* ground = _world->CreateBody(&bd);

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
	b2ParticleGroup* const group = _particleSystem->CreateParticleGroup(pd);
	//if (pd.flags & b2_colorMixingParticle) {
	//	ColorParticleGroup(group, 0);
	//}

	_initialized = true;
}

void LiquidFunCHOP::initWaterfall(const OP_Inputs* inputs) {
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

	// Configure particle system parameters.
	_particleSystem->SetRadius(0.035f);
	_particleSystem->SetMaxParticleCount(k_maxParticleCount);
	_particleSystem->SetDestructionByAge(true);

	b2Body* ground = NULL;
	{
		b2BodyDef bd;
		ground = _world->CreateBody(&bd);
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
		const float32 particleDiameter = _particleSystem->GetRadius() * 2.0f;
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
		const float32 faucetLength = _particleSystem->GetRadius() * 2.0f * k_faucetLength;
		_emitter.SetParticleSystem(_particleSystem);
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

void LiquidFunCHOP::restart() {
	delete _world;
	_initialized = false;
}

void LiquidFunCHOP::getGeneralInfo(CHOP_GeneralInfo* ginfo, const OP_Inputs* inputs, void* reserved1) {
	// This will cause the node to cook every frame
	ginfo->cookEveryFrameIfAsked = true;

	// Note: To disable timeslicing you'll need to turn this off, as well as ensure that
	// getOutputInfo() returns true, and likely also set the info->numSamples to how many
	// samples you want to generate for this CHOP. Otherwise it'll take on length of the
	// input CHOP, which may be timesliced.
	ginfo->timeslice = false;

	ginfo->inputMatchIndex = 0;
}

bool LiquidFunCHOP::getOutputInfo(CHOP_OutputInfo* info, const OP_Inputs* inputs, void* reserved1) {
	info->numChannels = 2;
	info->sampleRate = inputs->getParInt("Fps");
	if (!_initialized) {
		init(inputs);
	} 
	info->numSamples = _particleSystem->GetParticleCount();
	return true;
}

void
LiquidFunCHOP::getChannelName(int32_t index, OP_String* name, const OP_Inputs* inputs, void* reserved1) {
	if (index == 0) {
		name->setString("tx");
	} else if (index == 1) {
		name->setString("ty");
	}
}

void LiquidFunCHOP::execute(CHOP_Output* output, const OP_Inputs* inputs, void* reserved) {
	int velocityIter = inputs->getParInt("Velocityiterations");
	int positionIter = inputs->getParInt("Positioniterations");
	int fps = inputs->getParInt("Fps");
	float dt = 1.0 / fps;
	_world->Step(dt, velocityIter, positionIter);

	int scene = inputs->getParInt("Scene");
	if (scene == 1) {
		_emitter.Step(dt, NULL, 0);
	}

	b2Vec2* positions = _particleSystem->GetPositionBuffer();
	int n = _particleSystem->GetParticleCount();

	for (int i = 0; i < n; i++) {
		output->channels[0][i] = positions[i].x;
		output->channels[1][i] = positions[i].y;
	}
}

int32_t LiquidFunCHOP::getNumInfoCHOPChans(void* reserved1) {
	// We return the number of channel we want to output to any Info CHOP
	// connected to the CHOP. In this example we are just going to send one channel.
	return 2;
}

bool LiquidFunCHOP::getInfoDATSize(OP_InfoDATSize* infoSize, void* reserved1) {
	return false;
}

void LiquidFunCHOP::getInfoDATEntries(int32_t index,
	int32_t nEntries,
	OP_InfoDATEntries* entries,
	void* reserved1) {
}

void LiquidFunCHOP::setupParameters(OP_ParameterManager* manager, void* reserved1) {
	// Start
	{
		OP_NumericParameter	np;

		np.name = "Restart";
		np.label = "Restart";

		OP_ParAppendResult res = manager->appendPulse(np);
		assert(res == OP_ParAppendResult::Success);
	}
	// Scene
	{
		OP_StringParameter	sp;

		sp.name = "Scene";
		sp.label = "Scene";

		sp.defaultValue = "Dambreak";

		const char* names[] = { "Dambreak", "Waterfall" };
		const char* labels[] = { "Dam Break", "Waterfall" };

		OP_ParAppendResult res = manager->appendMenu(sp, 2, names, labels);
		assert(res == OP_ParAppendResult::Success);
	}
	// Particle
	{
		OP_StringParameter	sp;

		sp.name = "Particletype";
		sp.label = "Particle Type";

		sp.defaultValue = "Solid";

		const char* names[] = { "Solid", "Rigid", "Elastic" };
		const char* labels[] = { "Solid", "Rigid", "Elastic" };

		OP_ParAppendResult res = manager->appendMenu(sp, 3, names, labels);
		assert(res == OP_ParAppendResult::Success);
	}
	{
		OP_NumericParameter np;
		np.name = "Particlesize";
		np.label = "Particle Size";
		np.defaultValues[0] = 0.02;
		np.minSliders[0] = 0.0;
		np.maxSliders[0] = 0.1;

		OP_ParAppendResult res = manager->appendFloat(np);
	}
	{
		OP_NumericParameter np;
		np.name = "Particledamping";
		np.label = "Particle Damping";
		np.defaultValues[0] = 0.2;
		np.minSliders[0] = 0.0;
		np.maxSliders[0] = 1.0;

		OP_ParAppendResult res = manager->appendFloat(np);
	}
	// Gravity
	{
		OP_NumericParameter	np;

		np.name = "Gravity";
		np.label = "Gravity";
		np.defaultValues[0] = 0.0;
		np.defaultValues[1] = -9.8;
		np.minSliders[0] = -20.0;
		np.maxSliders[0] = 20.0;
		np.minSliders[1] = -20.0;
		np.maxSliders[1] = 20.0;

		OP_ParAppendResult res = manager->appendXY(np);
		assert(res == OP_ParAppendResult::Success);
	}
	// Velocity iterations
	{
		OP_NumericParameter np;
		np.name = "Velocityiterations";
		np.label = "Velocity Iterations";
		np.defaultValues[0] = 6;
		np.minSliders[0] = 0;
		np.maxSliders[0] = 10;

		OP_ParAppendResult res = manager->appendInt(np);
	}
	// Position iterations
	{
		OP_NumericParameter np;
		np.name = "Positioniterations";
		np.label = "Position Iterations";
		np.defaultValues[0] = 2;
		np.minSliders[0] = 0;
		np.maxSliders[0] = 10;

		OP_ParAppendResult res = manager->appendInt(np);
	}
	// fps
	{
		OP_NumericParameter np;
		np.name = "Fps";
		np.label = "fps";
		np.defaultValues[0] = 60;
		np.minSliders[0] = 0;
		np.maxSliders[0] = 240;

		OP_ParAppendResult res = manager->appendInt(np);
	}

}

void LiquidFunCHOP::pulsePressed(const char* name, void* reserved1) {
	if (!strcmp(name, "Restart")) {
		restart();
	}
}

