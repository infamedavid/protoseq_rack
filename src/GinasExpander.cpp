#include "plugin.hpp"
#include "GinasExpanderMessage.hpp"
#include "GinasExpanderQuant.hpp"

using namespace protoseq;

struct GinasExpander : Module {
	enum ParamId {
		NUM_PARAMS
	};
	enum InputId {
		SEED_INPUT,
		ALEN_INPUT,
		NUM_INPUTS
	};
	enum OutputId {
		NUM_OUTPUTS
	};
	enum LightId {
		NUM_LIGHTS
	};

	GinasExpander() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configInput(SEED_INPUT, "SEED CV");
		configInput(ALEN_INPUT, "ALEN CV");
	}

	void process(const ProcessArgs& args) override {
		(void) args;
		Module* leftModule = leftExpander.module;
		if (!leftModule || leftModule->model != modelGinaArp) {
			return;
		}
		if (!leftModule->rightExpander.producerMessage) {
			return;
		}

		auto* message = static_cast<GinasExpanderMessage*>(leftModule->rightExpander.producerMessage);
		message->seedActive = inputs[SEED_INPUT].isConnected();
		message->seedBucket = quantizeGinasExpanderSeedCv(inputs[SEED_INPUT].getVoltage());
		message->alenActive = inputs[ALEN_INPUT].isConnected();
		message->alen = quantizeGinasExpanderAlenCv(inputs[ALEN_INPUT].getVoltage());

		leftModule->rightExpander.messageFlipRequested = true;
	}
};

static Vec expanderMockupPx(float x, float y) {
	return Vec(
		x * ((RACK_GRID_WIDTH * 3.0f) / 15.24f),
		y * (RACK_GRID_HEIGHT / 128.5f)
	);
}

struct GinasExpanderWidget : ModuleWidget {
	GinasExpanderWidget(GinasExpander* module) {
		setModule(module);
		box.size = Vec(RACK_GRID_WIDTH * 3, RACK_GRID_HEIGHT);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/gina_expander.svg")));

		addInput(createInputCentered<PJ301MPort>(expanderMockupPx(7.6200f, 92.4772f), module, GinasExpander::SEED_INPUT));
		addInput(createInputCentered<PJ301MPort>(expanderMockupPx(7.6200f, 112.7743f), module, GinasExpander::ALEN_INPUT));
	}
};

Model* modelGinasExpander = createModel<GinasExpander, GinasExpanderWidget>("GinasExpander");
