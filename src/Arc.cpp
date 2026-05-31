#include "plugin.hpp"
#include "ArcCore.hpp"
#include <algorithm>
#include <cmath>
#include <functional>

namespace {

std::shared_ptr<Svg> loadArcSvg(const std::string& assetPath) {
	const std::string fullPath = asset::plugin(pluginInstance, assetPath);
	if (!system::exists(fullPath)) {
		return nullptr;
	}
	return APP->window->loadSvg(fullPath);
}

struct ArcLargeKnob : app::SvgKnob {
	ArcLargeKnob() {
		minAngle = -0.83f * M_PI;
		maxAngle = 0.83f * M_PI;
		if (auto svg = loadArcSvg("res/knob_00.svg")) {
			setSvg(svg);
		}
	}
};

struct ArcSmallKnob : app::SvgKnob {
	ArcSmallKnob() {
		minAngle = -0.83f * M_PI;
		maxAngle = 0.83f * M_PI;
		if (auto svg = loadArcSvg("res/knob_01.svg")) {
			setSvg(svg);
		}
	}
};

struct ArcJack : app::SvgPort {
	ArcJack() {
		if (auto svg = loadArcSvg("res/jack.svg")) {
			setSvg(svg);
		}
	}
};

struct ArcPlayButton : SvgSwitch {
	ArcPlayButton() {
		momentary = true;
		addFrame(loadArcSvg("res/play_0.svg"));
		addFrame(loadArcSvg("res/play_1.svg"));
	}
};

struct ArcStopButton : SvgSwitch {
	ArcStopButton() {
		momentary = true;
		addFrame(loadArcSvg("res/stop_0.svg"));
		addFrame(loadArcSvg("res/stop_1.svg"));
	}
};

struct ArcDigitDisplay : TransparentWidget {
	std::vector<std::shared_ptr<Svg>> digitFrames;
	SvgWidget* digitWidgets[3]{};
	std::function<int()> getDisplayValue;
	int lastDisplayValue = -1;

	ArcDigitDisplay() {
		digitFrames.reserve(10);
		for (int digit = 0; digit < 10; ++digit) {
			digitFrames.push_back(loadArcSvg("res/" + std::to_string(digit) + ".svg"));
		}

		for (int i = 0; i < 3; ++i) {
			digitWidgets[i] = new SvgWidget();
			digitWidgets[i]->box.pos = Vec(i * 7.0f, 0.0f);
			addChild(digitWidgets[i]);
		}
	}

	void step() override {
		TransparentWidget::step();
		const int rawValue = getDisplayValue ? getDisplayValue() : 20;
		const int clampedValue = std::min(std::max(rawValue, 20), 350);
		if (clampedValue == lastDisplayValue) {
			return;
		}
		lastDisplayValue = clampedValue;

		const int digits[] = {
			(clampedValue / 100) % 10,
			(clampedValue / 10) % 10,
			clampedValue % 10,
		};
		for (int i = 0; i < 3; ++i) {
			if (digitFrames[digits[i]]) {
				digitWidgets[i]->setSvg(digitFrames[digits[i]]);
			}
		}
	}
};


static constexpr float MIN_BPM = 20.0f;
static constexpr float MAX_BPM = 350.0f;
static constexpr float MAX_MAIN_PULSE_WIDTH = 0.99f;
static constexpr float GATE_HIGH_VOLTAGE = 10.0f;
static constexpr float MAX_ARC_GATE_LENGTH = 0.99f;

static float clamp01(float value) {
	return protoseq::arcClamp01(value);
}

static float bpmFromNormalized(float normalized) {
	return MIN_BPM + clamp01(normalized) * (MAX_BPM - MIN_BPM);
}

static constexpr float ARC_PANEL_MM_W = 60.959999f;
static constexpr float ARC_PANEL_MM_H = 128.5f;

static Vec arcMm(float x, float y) {
	return Vec(
		x * ((RACK_GRID_WIDTH * 12.0f) / ARC_PANEL_MM_W),
		y * (RACK_GRID_HEIGHT / ARC_PANEL_MM_H)
	);
}

} // namespace

struct Arc : Module {
	enum ParamId {
		MAIN_PARAM,
		BAR_PARAM,
		PW_PARAM,
		ARPC_PARAM,
		GLEN_PARAM,
		RLEN_PARAM,
		SWNG_PARAM,
		RSWN_PARAM,
		NRTC_PARAM,
		RRTC_PARAM,
		BRNL_PARAM,
		SEED_PARAM,
		PLAY_PARAM,
		STOP_PARAM,
		NUM_PARAMS
	};
	enum InputId {
		MAIN_CV_INPUT,
		BAR_CV_INPUT,
		PW_CV_INPUT,
		ARPC_CV_INPUT,
		GLEN_CV_INPUT,
		RLEN_CV_INPUT,
		SWNG_CV_INPUT,
		RSWN_CV_INPUT,
		NRTC_CV_INPUT,
		RRTC_CV_INPUT,
		BRNL_CV_INPUT,
		SEED_CV_INPUT,
		PLAY_CV_INPUT,
		STOP_CV_INPUT,
		PLAY_STOP_GATE_INPUT,
		NUM_INPUTS
	};
	enum OutputId {
		MAIN_OUTPUT,
		ARC_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightId {
		NUM_LIGHTS
	};

	dsp::SchmittTrigger playButtonTrigger;
	dsp::SchmittTrigger stopButtonTrigger;
	dsp::SchmittTrigger playCvTrigger;
	dsp::SchmittTrigger stopCvTrigger;
	dsp::SchmittTrigger playStopGateTrigger;
	bool isPlaying = false;
	bool lastPlayStopGateHigh = false;
	double mainPhase = 0.0;
	double arcPhase = 0.0;
	int displayBpm = 120;
	int barEventIndex = 0;

	Arc() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		configParam(MAIN_PARAM, 20.0f, 350.0f, 120.0f, "MAIN BPM", " BPM");
		configParam(BAR_PARAM, static_cast<float>(protoseq::ARC_BAR_MIN_EVENTS), static_cast<float>(protoseq::ARC_BAR_MAX_EVENTS), 4.0f, "BAR");
		configParam(PW_PARAM, 0.0f, 1.0f, 0.5f, "PW", "%", 0.0f, 100.0f);
		configParam(ARPC_PARAM, static_cast<float>(protoseq::ARC_MULTIPLIER_INDEX_MIN), static_cast<float>(protoseq::ARC_MULTIPLIER_INDEX_MAX), 0.0f, "ARPC");
		configParam(GLEN_PARAM, 0.0f, 1.0f, 0.5f, "GLEN", "%", 0.0f, 100.0f);
		configParam(RLEN_PARAM, 0.0f, 1.0f, 0.0f, "RLEN", "%", 0.0f, 100.0f);
		configParam(SWNG_PARAM, 0.0f, 1.0f, 0.0f, "SWNG", "%", 0.0f, 100.0f);
		configParam(RSWN_PARAM, 0.0f, 1.0f, 0.0f, "RSWN", "%", 0.0f, 100.0f);
		configParam(NRTC_PARAM, 1.0f, 8.0f, 1.0f, "NRTC");
		configParam(RRTC_PARAM, 0.0f, 1.0f, 0.0f, "RRTC", "%", 0.0f, 100.0f);
		configParam(BRNL_PARAM, 0.0f, 1.0f, 0.0f, "BRNL", "%", 0.0f, 100.0f);
		configParam(SEED_PARAM, 0.0f, 1000.0f, 0.0f, "SEED");
		configButton(PLAY_PARAM, "PLAY");
		configButton(STOP_PARAM, "STOP");

		if (ParamQuantity* barQuantity = getParamQuantity(BAR_PARAM)) {
			barQuantity->snapEnabled = true;
		}
		if (ParamQuantity* arpcQuantity = getParamQuantity(ARPC_PARAM)) {
			arpcQuantity->snapEnabled = true;
		}
		if (ParamQuantity* nrtcQuantity = getParamQuantity(NRTC_PARAM)) {
			nrtcQuantity->snapEnabled = true;
		}
		if (ParamQuantity* seedQuantity = getParamQuantity(SEED_PARAM)) {
			seedQuantity->snapEnabled = true;
		}

		configInput(MAIN_CV_INPUT, "MAIN CV IN");
		configInput(BAR_CV_INPUT, "BAR CV IN");
		configInput(PW_CV_INPUT, "PW CV IN");
		configInput(ARPC_CV_INPUT, "ARPC CV IN");
		configInput(GLEN_CV_INPUT, "GLEN CV IN");
		configInput(RLEN_CV_INPUT, "RLEN CV IN");
		configInput(SWNG_CV_INPUT, "SWNG CV IN");
		configInput(RSWN_CV_INPUT, "RSWN CV IN");
		configInput(NRTC_CV_INPUT, "NRTC CV IN");
		configInput(RRTC_CV_INPUT, "RRTC CV IN");
		configInput(BRNL_CV_INPUT, "BRNL CV IN");
		configInput(SEED_CV_INPUT, "SEED CV IN");
		configInput(PLAY_CV_INPUT, "PLAY CV IN");
		configInput(STOP_CV_INPUT, "STOP CV IN");
		configInput(PLAY_STOP_GATE_INPUT, "PLAY/STOP GATE IN");

		configOutput(MAIN_OUTPUT, "MAIN OUT");
		configOutput(ARC_OUTPUT, "ARC OUT");
	}

	float getEffectiveBpm() const {
		if (inputs[MAIN_CV_INPUT].isConnected()) {
			return bpmFromNormalized(inputs[MAIN_CV_INPUT].getVoltage());
		}
		return std::min(std::max(params[MAIN_PARAM].getValue(), MIN_BPM), MAX_BPM);
	}

	float getEffectivePulseWidth() const {
		const float rawPulseWidth = inputs[PW_CV_INPUT].isConnected()
			? inputs[PW_CV_INPUT].getVoltage()
			: params[PW_PARAM].getValue();
		return std::min(clamp01(rawPulseWidth), MAX_MAIN_PULSE_WIDTH);
	}

	protoseq::ArcMultiplier getEffectiveArcMultiplier() const {
		const int index = inputs[ARPC_CV_INPUT].isConnected()
			? protoseq::arcMultiplierIndexFromNormalized(inputs[ARPC_CV_INPUT].getVoltage())
			: protoseq::arcMultiplierIndexFromParam(params[ARPC_PARAM].getValue());
		return protoseq::arcMultiplierForIndex(index);
	}

	float getEffectiveArcGateLength() const {
		const float rawGateLength = inputs[GLEN_CV_INPUT].isConnected()
			? inputs[GLEN_CV_INPUT].getVoltage()
			: params[GLEN_PARAM].getValue();
		return std::min(clamp01(rawGateLength), MAX_ARC_GATE_LENGTH);
	}

	int getEffectiveBarLength() const {
		return inputs[BAR_CV_INPUT].isConnected()
			? protoseq::arcBarLengthFromNormalized(inputs[BAR_CV_INPUT].getVoltage())
			: protoseq::arcBarLengthFromParam(params[BAR_PARAM].getValue());
	}

	void resetPhases() {
		mainPhase = 0.0;
		arcPhase = 0.0;
		barEventIndex = 0;
	}

	void startPlayback(bool resetPhase) {
		if (resetPhase) {
			resetPhases();
		}
		if (!isPlaying) {
			isPlaying = true;
		}
	}

	void stopPlayback(bool resetPhase) {
		if (isPlaying) {
			isPlaying = false;
			if (resetPhase) {
				resetPhases();
			}
		}
		outputs[MAIN_OUTPUT].setVoltage(0.0f);
		outputs[ARC_OUTPUT].setVoltage(0.0f);
	}

	void process(const ProcessArgs& args) override {
		const float effectiveBpm = getEffectiveBpm();
		displayBpm = static_cast<int>(std::round(effectiveBpm));

		if (playButtonTrigger.process(params[PLAY_PARAM].getValue()) || playCvTrigger.process(inputs[PLAY_CV_INPUT].getVoltage())) {
			startPlayback(false);
		}
		if (stopButtonTrigger.process(params[STOP_PARAM].getValue()) || stopCvTrigger.process(inputs[STOP_CV_INPUT].getVoltage())) {
			stopPlayback(true);
		}

		const float playStopGateVoltage = inputs[PLAY_STOP_GATE_INPUT].getVoltage();
		if (playStopGateTrigger.process(playStopGateVoltage)) {
			startPlayback(true);
		}

		const bool playStopGateHigh = playStopGateVoltage >= 1.0f;
		if (lastPlayStopGateHigh && !playStopGateHigh) {
			stopPlayback(false);
		}
		lastPlayStopGateHigh = playStopGateHigh;

		if (!isPlaying) {
			outputs[MAIN_OUTPUT].setVoltage(0.0f);
			outputs[ARC_OUTPUT].setVoltage(0.0f);
			return;
		}

		const float pulseWidth = getEffectivePulseWidth();
		const float arcGateLength = getEffectiveArcGateLength();
		outputs[MAIN_OUTPUT].setVoltage(mainPhase < pulseWidth ? GATE_HIGH_VOLTAGE : 0.0f);
		outputs[ARC_OUTPUT].setVoltage(arcPhase < arcGateLength ? GATE_HIGH_VOLTAGE : 0.0f);

		const double mainPhaseDelta = static_cast<double>(effectiveBpm) * args.sampleTime / 60.0;
		mainPhase += mainPhaseDelta;
		while (mainPhase >= 1.0) {
			mainPhase -= 1.0;
		}

		const protoseq::ArcMultiplier arcMultiplier = getEffectiveArcMultiplier();
		arcPhase += mainPhaseDelta * static_cast<double>(arcMultiplier.numerator) / static_cast<double>(arcMultiplier.denominator);
		const int barLength = getEffectiveBarLength();
		while (arcPhase >= 1.0) {
			arcPhase -= 1.0;
			barEventIndex = (barEventIndex + 1) % barLength;
		}
	}
};

struct ArcWidget : ModuleWidget {
	ArcWidget(Arc* module) {
		setModule(module);
		box.size = Vec(RACK_GRID_WIDTH * 12, RACK_GRID_HEIGHT);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/ARC.svg")));

		addParam(createParamCentered<ArcLargeKnob>(arcMm(8.88f, 25.22f), module, Arc::MAIN_PARAM));
		addParam(createParamCentered<ArcSmallKnob>(arcMm(19.67f, 25.76f), module, Arc::BAR_PARAM));
		addParam(createParamCentered<ArcSmallKnob>(arcMm(30.46f, 25.76f), module, Arc::PW_PARAM));

		addParam(createParamCentered<ArcLargeKnob>(arcMm(8.88f, 57.07f), module, Arc::ARPC_PARAM));
		addParam(createParamCentered<ArcSmallKnob>(arcMm(19.67f, 57.62f), module, Arc::GLEN_PARAM));
		addParam(createParamCentered<ArcSmallKnob>(arcMm(30.46f, 57.62f), module, Arc::RLEN_PARAM));
		addParam(createParamCentered<ArcSmallKnob>(arcMm(40.99f, 57.62f), module, Arc::SWNG_PARAM));
		addParam(createParamCentered<ArcSmallKnob>(arcMm(51.78f, 57.62f), module, Arc::RSWN_PARAM));

		addParam(createParamCentered<ArcSmallKnob>(arcMm(8.88f, 81.62f), module, Arc::NRTC_PARAM));
		addParam(createParamCentered<ArcSmallKnob>(arcMm(19.67f, 81.62f), module, Arc::RRTC_PARAM));
		addParam(createParamCentered<ArcSmallKnob>(arcMm(30.46f, 81.62f), module, Arc::BRNL_PARAM));
		addParam(createParamCentered<ArcSmallKnob>(arcMm(52.04f, 81.62f), module, Arc::SEED_PARAM));

		addParam(createParamCentered<ArcPlayButton>(arcMm(8.88f, 106.15f), module, Arc::PLAY_PARAM));
		addParam(createParamCentered<ArcStopButton>(arcMm(19.58f, 106.15f), module, Arc::STOP_PARAM));

		auto bpmDisplay = new ArcDigitDisplay();
		bpmDisplay->getDisplayValue = [module]() {
			return module ? module->displayBpm : 120;
		};
		bpmDisplay->box.pos = arcMm(48.90f, 23.25f);
		bpmDisplay->box.size = arcMm(6.95f, 3.88f);
		addChild(bpmDisplay);

		addInput(createInputCentered<ArcJack>(arcMm(8.88f, 36.15f), module, Arc::MAIN_CV_INPUT));
		addInput(createInputCentered<ArcJack>(arcMm(19.67f, 36.15f), module, Arc::BAR_CV_INPUT));
		addInput(createInputCentered<ArcJack>(arcMm(30.46f, 36.15f), module, Arc::PW_CV_INPUT));

		addInput(createInputCentered<ArcJack>(arcMm(8.88f, 68.00f), module, Arc::ARPC_CV_INPUT));
		addInput(createInputCentered<ArcJack>(arcMm(19.67f, 68.00f), module, Arc::GLEN_CV_INPUT));
		addInput(createInputCentered<ArcJack>(arcMm(30.46f, 68.00f), module, Arc::RLEN_CV_INPUT));
		addInput(createInputCentered<ArcJack>(arcMm(40.99f, 68.00f), module, Arc::SWNG_CV_INPUT));
		addInput(createInputCentered<ArcJack>(arcMm(51.78f, 68.00f), module, Arc::RSWN_CV_INPUT));

		addInput(createInputCentered<ArcJack>(arcMm(8.88f, 92.00f), module, Arc::NRTC_CV_INPUT));
		addInput(createInputCentered<ArcJack>(arcMm(19.67f, 92.00f), module, Arc::RRTC_CV_INPUT));
		addInput(createInputCentered<ArcJack>(arcMm(30.46f, 92.00f), module, Arc::BRNL_CV_INPUT));
		addInput(createInputCentered<ArcJack>(arcMm(52.04f, 92.00f), module, Arc::SEED_CV_INPUT));

		addInput(createInputCentered<ArcJack>(arcMm(8.88f, 112.82f), module, Arc::PLAY_CV_INPUT));
		addInput(createInputCentered<ArcJack>(arcMm(19.67f, 112.82f), module, Arc::STOP_CV_INPUT));
		addInput(createInputCentered<ArcJack>(arcMm(30.46f, 112.82f), module, Arc::PLAY_STOP_GATE_INPUT));

		addOutput(createOutputCentered<ArcJack>(arcMm(41.25f, 112.82f), module, Arc::MAIN_OUTPUT));
		addOutput(createOutputCentered<ArcJack>(arcMm(52.04f, 112.82f), module, Arc::ARC_OUTPUT));
	}
};

Model* modelArc = createModel<Arc, ArcWidget>("Arc");
