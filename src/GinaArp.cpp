#include "plugin.hpp"
#include "GinaArpCore.hpp"
#include <limits>

using namespace protoseq;

namespace {

template <typename T>
T clampValue(T value, T minValue, T maxValue) {
	return value < minValue ? minValue : (value > maxValue ? maxValue : value);
}

template <typename TWidget>
std::shared_ptr<Svg> loadOptionalSvg(const std::string& assetPath) {
	const std::string fullPath = asset::plugin(pluginInstance, assetPath);
	if (!system::exists(fullPath)) {
		return nullptr;
	}
	return APP->window->loadSvg(fullPath);
}

struct GinaLargeKnob : app::SvgKnob {
	GinaLargeKnob() {
		minAngle = -0.83f * M_PI;
		maxAngle = 0.83f * M_PI;
		if (auto svg = loadOptionalSvg<GinaLargeKnob>("res/knob_00.svg")) {
			setSvg(svg);
		}
	}
};

struct GinaSmallKnob : app::SvgKnob {
	GinaSmallKnob() {
		minAngle = -0.83f * M_PI;
		maxAngle = 0.83f * M_PI;
		if (auto svg = loadOptionalSvg<GinaSmallKnob>("res/knob_01.svg")) {
			setSvg(svg);
		}
	}
};

struct GinaJack : app::SvgPort {
	GinaJack() {
		if (auto svg = loadOptionalSvg<GinaJack>("res/jack.svg")) {
			setSvg(svg);
		}
	}
};

struct GinaPivotSwitch : SvgSwitch {
	GinaPivotSwitch() {
		addFrame(loadOptionalSvg<GinaPivotSwitch>("res/switch_0.svg"));
		addFrame(loadOptionalSvg<GinaPivotSwitch>("res/switch_1.svg"));
	}
};

struct GinaMomentaryButton : SvgSwitch {
	GinaMomentaryButton() {
		momentary = true;
		addFrame(loadOptionalSvg<GinaMomentaryButton>("res/momentary_0.svg"));
		addFrame(loadOptionalSvg<GinaMomentaryButton>("res/momentary_1.svg"));
	}
};

} // namespace

struct GinaArp : Module {
	enum ParamId {
		RANGE_PARAM,
		RANGE_ATTEN_PARAM,
		ODTS_PARAM,
		ODTS_ATTEN_PARAM,
		SEED_PARAM,
		ARP_LEN_PARAM,
		KEY_PREV_PARAM,
		KEY_NEXT_PARAM,
		MODE_PREV_PARAM,
		MODE_NEXT_PARAM,
		PIVOT_MODE_PARAM,
		NUM_PARAMS
	};
	enum InputId {
		CLOCK_INPUT,
		GATE_INPUT,
		VOCT_INPUT,
		RANGE_CV_INPUT,
		ODTS_CV_INPUT,
		SEED_CV_INPUT,
		NUM_INPUTS
	};
	enum OutputId {
		VOCT_OUTPUT,
		GATE_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightId {
		NUM_LIGHTS
	};

	GinaArpCore core;
	dsp::SchmittTrigger gateTrigger;
	dsp::SchmittTrigger clockTrigger;
	dsp::SchmittTrigger keyPrevTrigger;
	dsp::SchmittTrigger keyNextTrigger;
	dsp::SchmittTrigger modePrevTrigger;
	dsp::SchmittTrigger modeNextTrigger;
	int keyRootSemitone = 0;
	Mode mode = Mode::Major;
	int noteIndex = 0;
	float heldVolts = 0.0f;
	bool lastGateHigh = false;

	GinaArp() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		configParam(RANGE_PARAM, 0.f, 1.f, 0.25f, "RANGE", "", 0.f, 1.f);
		configParam(RANGE_ATTEN_PARAM, -1.f, 1.f, 0.f, "RANGE attenuverter", "%", 0.f, 100.f);
		configParam(ODTS_PARAM, 0.f, 1.f, 0.f, "ODTS", "", 0.f, 1.f);
		configParam(ODTS_ATTEN_PARAM, -1.f, 1.f, 0.f, "ODTS attenuverter", "%", 0.f, 100.f);
		configParam(SEED_PARAM, 0.f, 1.f, 0.f, "SEED", "", 0.f, 1.f);
		configParam(ARP_LEN_PARAM, 1.f, 13.f, 4.f, "ARP LEN");
		if (ParamQuantity* arpLenQuantity = getParamQuantity(ARP_LEN_PARAM)) {
			arpLenQuantity->snapEnabled = true;
		}
		configButton(KEY_PREV_PARAM, "KEY previous");
		configButton(KEY_NEXT_PARAM, "KEY next");
		configButton(MODE_PREV_PARAM, "MODE previous");
		configButton(MODE_NEXT_PARAM, "MODE next");
		configSwitch(PIVOT_MODE_PARAM, 0.f, 1.f, 0.f, "QNT/RAW pivot mode", {"QNT", "RAW"});

		configInput(CLOCK_INPUT, "CLOCK IN (triggers each note while GATE IN is high)");
		configInput(GATE_INPUT, "GATE IN (arpeggio enable; CLOCK ignored while low)");
		configInput(VOCT_INPUT, "V/OCT IN (pivot source: QNT quantizes to KEY+MODE, RAW preserves input)");
		configInput(RANGE_CV_INPUT, "RANGE CV (modulates RANGE through bipolar attenuverter)");
		configInput(ODTS_CV_INPUT, "ODTS CV (modulates ODTS through bipolar attenuverter)");
		configInput(SEED_CV_INPUT, "SEED CV (modulates SEED; 0 keeps mutable if effective seed is 0)");

		configOutput(VOCT_OUTPUT, "V/OCT OUT (generated Gina’s ARP pitch)");
		configOutput(GATE_OUTPUT, "GATE OUT (10V while CLOCK and GATE are high; no internal gate length)");
	}

	void process(const ProcessArgs& args) override {
		if (keyPrevTrigger.process(params[KEY_PREV_PARAM].getValue())) {
			keyRootSemitone = (keyRootSemitone + 11) % 12;
		}
		if (keyNextTrigger.process(params[KEY_NEXT_PARAM].getValue())) {
			keyRootSemitone = (keyRootSemitone + 1) % 12;
		}
		constexpr int modeCount = static_cast<int>(Mode::Hirojoshi) + 1;
		if (modePrevTrigger.process(params[MODE_PREV_PARAM].getValue())) {
			const int current = static_cast<int>(mode);
			mode = static_cast<Mode>((current + modeCount - 1) % modeCount);
		}
		if (modeNextTrigger.process(params[MODE_NEXT_PARAM].getValue())) {
			const int current = static_cast<int>(mode);
			mode = static_cast<Mode>((current + 1) % modeCount);
		}

		const float gateVoltage = inputs[GATE_INPUT].getVoltage();
		const float clockVoltage = inputs[CLOCK_INPUT].getVoltage();

		const bool gateHigh = gateVoltage >= 1.0f;
		const bool clockHigh = clockVoltage >= 1.0f;

		const bool gateRising = gateTrigger.process(gateVoltage);
		const bool gateFalling = lastGateHigh && !gateHigh;
		const bool clockRising = clockTrigger.process(clockVoltage);
		lastGateHigh = gateHigh;

		if (gateRising) {
			noteIndex = 0;
		}

		if (gateFalling) {
			outputs[GATE_OUTPUT].setVoltage(0.0f);
		} else {
			outputs[GATE_OUTPUT].setVoltage((gateHigh && clockHigh) ? 10.0f : 0.0f);
		}

		if (!gateHigh) {
			outputs[VOCT_OUTPUT].setVoltage(heldVolts);
			return;
		}

		if (!clockRising) {
			outputs[VOCT_OUTPUT].setVoltage(heldVolts);
			return;
		}

		const float rangeCv = inputs[RANGE_CV_INPUT].isConnected() ? (inputs[RANGE_CV_INPUT].getVoltage() / 10.0f) * params[RANGE_ATTEN_PARAM].getValue() : 0.0f;
		const float odtsCv = inputs[ODTS_CV_INPUT].isConnected() ? (inputs[ODTS_CV_INPUT].getVoltage() / 10.0f) * params[ODTS_ATTEN_PARAM].getValue() : 0.0f;
		const float seedCv = inputs[SEED_CV_INPUT].isConnected() ? (inputs[SEED_CV_INPUT].getVoltage() / 10.0f) : 0.0f;

		const float effectiveRange = clamp(params[RANGE_PARAM].getValue() + rangeCv, 0.0f, 1.0f);
		const float effectiveODTS = clamp(params[ODTS_PARAM].getValue() + odtsCv, 0.0f, 1.0f);
		const float effectiveSeed = clamp(params[SEED_PARAM].getValue() + seedCv, 0.0f, 1.0f);
		const int arpLen = static_cast<int>(std::round(clamp(params[ARP_LEN_PARAM].getValue(), 1.0f, 13.0f)));

		const PivotInputMode pivotMode = params[PIVOT_MODE_PARAM].getValue() >= 0.5f ? PivotInputMode::Raw : PivotInputMode::Quantized;
		const float vOctIn = inputs[VOCT_INPUT].getVoltage();
		const int pivotMidi = core.resolvePivotMidi(vOctIn, keyRootSemitone, mode, pivotMode);

		const GinaArpContext ctx{
			keyRootSemitone,
			mode,
			pivotMidi,
			effectiveRange,
			effectiveODTS,
			arpLen,
			effectiveSeed,
			noteIndex
		};

		const int midiOut = shouldForcePivot(noteIndex, arpLen) ? pivotMidi : core.generateMidiNote(ctx);
		heldVolts = midiToVoltage(midiOut);
		outputs[VOCT_OUTPUT].setVoltage(heldVolts);
		++noteIndex;
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "keyRootSemitone", json_integer(keyRootSemitone));
		json_object_set_new(rootJ, "mode", json_integer(static_cast<int>(mode)));
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* keyJ = json_object_get(rootJ, "keyRootSemitone");
		if (keyJ) {
			keyRootSemitone = static_cast<int>(json_integer_value(keyJ));
			keyRootSemitone = (keyRootSemitone % 12 + 12) % 12;
		}
		json_t* modeJ = json_object_get(rootJ, "mode");
		if (modeJ) {
			const int rawMode = static_cast<int>(json_integer_value(modeJ));
			const int modeCount = static_cast<int>(Mode::Hirojoshi) + 1;
			const int clamped = clampValue(rawMode, 0, modeCount - 1);
			mode = static_cast<Mode>(clamped);
		}
	}
};

struct GinaArpImageDisplay : TransparentWidget {
	std::vector<std::shared_ptr<Svg>> frames;
	std::shared_ptr<Svg> fallbackFrame;
	SvgWidget* svgWidget = nullptr;
	std::function<int()> getIndex;
	int lastIndex = std::numeric_limits<int>::min();
	bool lastUsedFallback = false;
	bool hasLastDisplayState = false;

	GinaArpImageDisplay() {
		svgWidget = new SvgWidget();
		addChild(svgWidget);
	}

	void step() override {
		TransparentWidget::step();
		int idx = getIndex ? getIndex() : 0;
		const bool useFallback = (idx < 0 || idx >= static_cast<int>(frames.size()) || !frames[idx]);

		if (hasLastDisplayState && lastIndex == idx && lastUsedFallback == useFallback) {
			return;
		}

		lastIndex = idx;
		lastUsedFallback = useFallback;
		hasLastDisplayState = true;

		if (useFallback) {
			if (fallbackFrame) {
				svgWidget->setSvg(fallbackFrame);
			}
			return;
		}
		svgWidget->setSvg(frames[idx]);
	}
};

struct GinaArpWidget : ModuleWidget {
	GinaArpWidget(GinaArp* module) {
		setModule(module);
		box.size = Vec(RACK_GRID_WIDTH * 13, RACK_GRID_HEIGHT);
		const std::string panelAsset = system::exists(asset::plugin(pluginInstance, "res/gina.svg"))
			? "res/gina.svg"
			: "res/GinaArp.svg";
		setPanel(createPanel(asset::plugin(pluginInstance, panelAsset)));

			addParam(createParamCentered<GinaLargeKnob>(mm2px(Vec(31.2, 18.9)), module, GinaArp::RANGE_PARAM));
			addParam(createParamCentered<GinaSmallKnob>(mm2px(Vec(31.2, 54.3)), module, GinaArp::RANGE_ATTEN_PARAM));
			addParam(createParamCentered<GinaLargeKnob>(mm2px(Vec(8.58, 35.6)), module, GinaArp::SEED_PARAM));
			addParam(createParamCentered<GinaLargeKnob>(mm2px(Vec(19.89, 43.9)), module, GinaArp::ODTS_PARAM));
			addParam(createParamCentered<GinaSmallKnob>(mm2px(Vec(19.89, 54.2)), module, GinaArp::ODTS_ATTEN_PARAM));
			addParam(createParamCentered<GinaLargeKnob>(mm2px(Vec(42.51, 27.3)), module, GinaArp::ARP_LEN_PARAM));
			addParam(createParamCentered<GinaMomentaryButton>(mm2px(Vec(18.98, 72.7)), module, GinaArp::KEY_PREV_PARAM));
			addParam(createParamCentered<GinaMomentaryButton>(mm2px(Vec(25.48, 72.7)), module, GinaArp::KEY_NEXT_PARAM));
			addParam(createParamCentered<GinaMomentaryButton>(mm2px(Vec(31.85, 72.7)), module, GinaArp::MODE_PREV_PARAM));
			addParam(createParamCentered<GinaMomentaryButton>(mm2px(Vec(38.48, 72.7)), module, GinaArp::MODE_NEXT_PARAM));
			addParam(createParamCentered<GinaPivotSwitch>(mm2px(Vec(8.58, 74.8)), module, GinaArp::PIVOT_MODE_PARAM));

		auto keyDisplay = new GinaArpImageDisplay();
			keyDisplay->box.pos = mm2px(Vec(19.5, 75.7));
		keyDisplay->box.size = mm2px(Vec(10.0, 6.0));
		keyDisplay->fallbackFrame = loadOptionalSvg<GinaArpImageDisplay>("res/c.svg");
		const std::vector<std::string> keyAssets{
			"res/c.svg", "res/c1.svg", "res/d.svg", "res/d1.svg", "res/e.svg", "res/f.svg",
			"res/f1.svg", "res/g.svg", "res/g1.svg", "res/a.svg", "res/a1.svg", "res/b.svg"
		};
		keyDisplay->frames.reserve(keyAssets.size());
		for (const auto& assetPath : keyAssets) {
			if (system::exists(asset::plugin(pluginInstance, assetPath))) keyDisplay->frames.push_back(APP->window->loadSvg(asset::plugin(pluginInstance, assetPath)));
			else keyDisplay->frames.push_back(nullptr);
		}
		keyDisplay->getIndex = [module]() {
			if (!module) return 0;
			return module->keyRootSemitone;
		};
		addChild(keyDisplay);

		auto modeDisplay = new GinaArpImageDisplay();
			modeDisplay->box.pos = mm2px(Vec(28.6, 75.7));
		modeDisplay->box.size = mm2px(Vec(13.0, 6.0));
		modeDisplay->fallbackFrame = loadOptionalSvg<GinaArpImageDisplay>("res/major.svg");
		const std::vector<std::string> modeAssets{
			"res/major.svg", "res/minor.svg", "res/harmonicminor.svg", "res/melodicminor.svg", "res/dorian.svg",
			"res/phrygian.svg", "res/lydian.svg", "res/mixolydian.svg", "res/locrian.svg", "res/majorpentatonic.svg",
			"res/minorpentatonic.svg", "res/blues.svg", "res/wholetone.svg", "res/diminishedhalfwhole.svg",
			"res/diminishedwholehalf.svg", "res/hungarianminor.svg", "res/phrygiandominant.svg", "res/japanesein.svg",
			"res/hirojoshi.svg"
		};
		modeDisplay->frames.reserve(modeAssets.size());
		for (const auto& assetPath : modeAssets) {
			if (system::exists(asset::plugin(pluginInstance, assetPath))) modeDisplay->frames.push_back(APP->window->loadSvg(asset::plugin(pluginInstance, assetPath)));
			else modeDisplay->frames.push_back(nullptr);
		}
		modeDisplay->getIndex = [module]() {
			if (!module) return 0;
			return static_cast<int>(module->mode);
		};
		addChild(modeDisplay);

			addInput(createInputCentered<GinaJack>(mm2px(Vec(8.58, 62.8)), module, GinaArp::CLOCK_INPUT));
			addInput(createInputCentered<GinaJack>(mm2px(Vec(19.89, 62.8)), module, GinaArp::RANGE_CV_INPUT));
			addInput(createInputCentered<GinaJack>(mm2px(Vec(31.2, 62.8)), module, GinaArp::ODTS_CV_INPUT));
			addInput(createInputCentered<GinaJack>(mm2px(Vec(42.51, 62.8)), module, GinaArp::SEED_CV_INPUT));
			addInput(createInputCentered<GinaJack>(mm2px(Vec(8.58, 87.7)), module, GinaArp::VOCT_INPUT));
			addInput(createInputCentered<GinaJack>(mm2px(Vec(19.89, 87.7)), module, GinaArp::GATE_INPUT));

			addOutput(createOutputCentered<GinaJack>(mm2px(Vec(31.2, 87.7)), module, GinaArp::VOCT_OUTPUT));
			addOutput(createOutputCentered<GinaJack>(mm2px(Vec(42.51, 87.7)), module, GinaArp::GATE_OUTPUT));
	}
};

Model* modelGinaArp = createModel<GinaArp, GinaArpWidget>("GinasARP");
