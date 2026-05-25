#include "plugin.hpp"

struct ProtoseqBlank : rack::engine::Module {
	ProtoseqBlank() {
		config(0, 0, 0, 0);
	}
};

struct ProtoseqBlankWidget : ModuleWidget {
	ProtoseqBlankWidget(ProtoseqBlank* module) {
		setModule(module);
		box.size = Vec(RACK_GRID_WIDTH * 5, RACK_GRID_HEIGHT);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/protoseq_blank.svg")));
	}
};

Model* modelProtoseqBlank = createModel<ProtoseqBlank, ProtoseqBlankWidget>("ProtoseqBlank");
