#include "plugin.hpp"
#include "GinaArpCore.hpp"

using namespace protoseq;

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
	int keyRootSemitone = 0;
	Mode mode = Mode::Major;
	int noteIndex = 0;
	float heldVolts = 0.0f;
	bool lastGateHigh = false;

	GinaArp() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		configParam(RANGE_PARAM, 0.f, 1.f, 0.25f, "Range");
		configParam(RANGE_ATTEN_PARAM, -1.f, 1.f, 0.f, "Range CV attenuverter");
		configParam(ODTS_PARAM, 0.f, 1.f, 0.f, "ODTS");
		configParam(ODTS_ATTEN_PARAM, -1.f, 1.f, 0.f, "ODTS CV attenuverter");
		configParam(SEED_PARAM, 0.f, 1.f, 0.f, "Seed");
		configParam(ARP_LEN_PARAM, 1.f, 13.f, 4.f, "ARP LEN");
		configButton(KEY_PREV_PARAM, "Key previous");
		configButton(KEY_NEXT_PARAM, "Key next");
		configButton(MODE_PREV_PARAM, "Mode previous");
		configButton(MODE_NEXT_PARAM, "Mode next");
		configSwitch(PIVOT_MODE_PARAM, 0.f, 1.f, 0.f, "Pivot input mode", {"Quantized", "Raw"});

		configInput(CLOCK_INPUT, "Clock");
		configInput(GATE_INPUT, "Gate");
		configInput(VOCT_INPUT, "V/Oct");
		configInput(RANGE_CV_INPUT, "Range CV");
		configInput(ODTS_CV_INPUT, "ODTS CV");
		configInput(SEED_CV_INPUT, "Seed CV");

		configOutput(VOCT_OUTPUT, "V/Oct");
		configOutput(GATE_OUTPUT, "Gate");
	}

	void process(const ProcessArgs& args) override {
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
};

struct GinaArpWidget : ModuleWidget {
	GinaArpWidget(GinaArp* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/GinaArp.svg")));

		addParam(createParamCentered<RoundLargeBlackKnob>(mm2px(Vec(15.0, 20.0)), module, GinaArp::RANGE_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(24.0, 20.0)), module, GinaArp::RANGE_ATTEN_PARAM));
		addParam(createParamCentered<RoundLargeBlackKnob>(mm2px(Vec(15.0, 35.0)), module, GinaArp::ODTS_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(24.0, 35.0)), module, GinaArp::ODTS_ATTEN_PARAM));
		addParam(createParamCentered<RoundLargeBlackKnob>(mm2px(Vec(15.0, 50.0)), module, GinaArp::SEED_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(15.0, 63.0)), module, GinaArp::ARP_LEN_PARAM));
		addParam(createParamCentered<LEDButton>(mm2px(Vec(6.0, 76.0)), module, GinaArp::KEY_PREV_PARAM));
		addParam(createParamCentered<LEDButton>(mm2px(Vec(12.0, 76.0)), module, GinaArp::KEY_NEXT_PARAM));
		addParam(createParamCentered<LEDButton>(mm2px(Vec(18.0, 76.0)), module, GinaArp::MODE_PREV_PARAM));
		addParam(createParamCentered<LEDButton>(mm2px(Vec(24.0, 76.0)), module, GinaArp::MODE_NEXT_PARAM));
		addParam(createParamCentered<CKSS>(mm2px(Vec(15.0, 86.0)), module, GinaArp::PIVOT_MODE_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.0, 100.0)), module, GinaArp::CLOCK_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(23.0, 100.0)), module, GinaArp::GATE_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.0, 112.0)), module, GinaArp::VOCT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(23.0, 112.0)), module, GinaArp::RANGE_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.0, 124.0)), module, GinaArp::ODTS_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(23.0, 124.0)), module, GinaArp::SEED_CV_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.0, 136.0)), module, GinaArp::VOCT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(23.0, 136.0)), module, GinaArp::GATE_OUTPUT));
	}
};

Model* modelGinaArp = createModel<GinaArp, GinaArpWidget>("GinasARP");
