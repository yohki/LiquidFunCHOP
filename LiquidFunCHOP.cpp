
#include "LiquidFunCHOP.h"

#include <stdio.h>
#include <string.h>
#include <cmath>
#include <assert.h>

#include "DamBreak.h"
#include "Faucet.h"

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
	shared_ptr<SceneBase> damBreak(new DamBreak());
	_scenes.push_back(damBreak);
	shared_ptr<SceneBase> faucet(new Faucet());
	_scenes.push_back(faucet);
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
	//double gravityScale = inputs->getParDouble("Particlegravityscale");
	//double density = inputs->getParDouble("Particledensity");
	double damping = inputs->getParDouble("Particledamping");
	const b2ParticleSystemDef particleSystemDef;
	_particleSystem = _world->CreateParticleSystem(&particleSystemDef);
	_particleSystem->SetGravityScale(0.4f);
	_particleSystem->SetDensity(1.2f);
	_particleSystem->SetRadius(radius);
	_particleSystem->SetDamping(damping);

	b2BodyDef bodyDef;
	_groundBody = _world->CreateBody(&bodyDef);

	int sceneIndex = inputs->getParInt("Sceneindex");
	if (0 <= sceneIndex && sceneIndex < _scenes.size()) {
		auto scene = _scenes[sceneIndex];
		scene->setup(_world, _particleSystem, inputs);
		_initialized = true;
		_sceneIndex = sceneIndex;
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

	if (0 <= _sceneIndex && _sceneIndex < _scenes.size()) {
		auto scene = _scenes[_sceneIndex];
		scene->update(dt);
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
		if (0 < _scenes.size()) {
			OP_NumericParameter	np;

			np.name = "Sceneindex";
			np.label = "Scene Index";

			np.defaultValues[0] = 0;
			np.minSliders[0] = 0;
			np.maxSliders[0] = _scenes.size() - 1;

			manager->appendInt(np);
		}
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

