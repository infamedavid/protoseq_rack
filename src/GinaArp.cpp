#include "plugin.hpp"

struct GinaArp : Module {
	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		NUM_INPUTS
	};
	enum OutputIds {
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	GinaArp() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
	}

	void process(const ProcessArgs& args) override {
		// Skeleton module: no processing yet.
	}
};

struct GinaArpWidget : ModuleWidget {
	GinaArpWidget(GinaArp* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/GinaArp.svg")));
	}
};

Model* modelGinaArp = createModel<GinaArp, GinaArpWidget>("GinasARP");
