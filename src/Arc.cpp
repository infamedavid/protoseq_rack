#include "plugin.hpp"
#include "ArcCore.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <random>
#include <vector>

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
static constexpr double ARC_GATE_SAFETY_GAP_SECONDS = 0.001;

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
		PLAY_STOP_TOGGLE_INPUT,
		RESERVED_BOTTOM_ROW_INPUT,
		PLAY_STOP_GATE_INPUT,
		NUM_INPUTS
	};
	enum OutputId {
		MAIN_OUTPUT,
		ARC_OUTPUT,
		BAR_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightId {
		NUM_LIGHTS
	};

	dsp::SchmittTrigger playButtonTrigger;
	dsp::SchmittTrigger stopButtonTrigger;
	dsp::SchmittTrigger playStopToggleTrigger;
	dsp::SchmittTrigger playStopGateTrigger;
	dsp::PulseGenerator barPulseGenerator;
	bool isPlaying = false;
	bool skipBarPulseOnPlay = true;
	bool lastPlayStopGateHigh = false;
	double mainPhase = 0.0;
	double arcPhase = 0.0;
	double currentArcStepStartSeconds = 0.0;
	int displayBpm = 120;
	int arcStepIndex = 0;
	int barStep = 0;
	int seedBucket = 0;
	std::array<std::uint64_t, 4> arcChannelSeeds{};
	std::mt19937_64 arcMutableRandomEngine{std::random_device{}()};
	bool currentArcStepSkipped = false;
	int evaluatedArcStepIndex = -1;
	int evaluatedSeedBucket = -1;
	int evaluatedBarStep = -1;
	int evaluatedBarLength = -1;
	float currentArcGateLengthScale = 1.0f;
	int evaluatedRlenArcStepIndex = -1;
	int evaluatedRlenSeedBucket = -1;
	int evaluatedRlenBarStep = -1;
	int evaluatedRlenBarLength = -1;
	float evaluatedRlenAmount = -1.0f;
	int currentArcRatchetCount = 1;
	int evaluatedRatchetArcStepIndex = -1;
	int evaluatedRatchetSeedBucket = -1;
	int evaluatedRatchetBarStep = -1;
	int evaluatedRatchetBarLength = -1;
	int evaluatedRatchetSelectedCount = -1;
	float evaluatedRatchetProbability = -1.0f;
	std::vector<protoseq::ArcSwingScheduleEvent> arcSwingSchedule;
	int scheduledBarStartArcStepIndex = -1;
	int scheduledBarLength = -1;
	int scheduledSeedBucket = -1;
	float scheduledSwingAmount = -1.0f;
	float scheduledSwingProbability = -1.0f;
	double scheduledArcStepDuration = -1.0;
	double scheduledFirstBaseStartSeconds = 0.0;

	Arc() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		configParam(MAIN_PARAM, 20.0f, 350.0f, 120.0f, "MAIN - Main clock BPM", " BPM");
		configParam(BAR_PARAM, static_cast<float>(protoseq::ARC_BAR_MIN_EVENTS), static_cast<float>(protoseq::ARC_BAR_MAX_EVENTS), 4.0f, "BAR - Seeded pattern cycle length in ARC events");
		configParam(PW_PARAM, 0.0f, 1.0f, 0.5f, "PW - MAIN OUT pulse width; always leaves a low gap", "%", 0.0f, 100.0f);
		configParam(ARPC_PARAM, static_cast<float>(protoseq::ARC_MULTIPLIER_INDEX_MIN), static_cast<float>(protoseq::ARC_MULTIPLIER_INDEX_MAX), 0.0f, "ARPC - ARC clock multiplier index");
		configParam(GLEN_PARAM, 0.0f, 1.0f, 0.5f, "GLEN - Base gate length for ARC OUT", "%", 0.0f, 100.0f);
		configParam(RLEN_PARAM, 0.0f, 1.0f, 0.0f, "RLEN - Seeded random gate shortening amount; only shortens", "%", 0.0f, 100.0f);
		configParam(SWNG_PARAM, 0.0f, 1.0f, 0.0f, "SWNG - Swing delay amount", "%", 0.0f, 100.0f);
		configParam(RSWN_PARAM, 0.0f, 1.0f, 0.0f, "RSWN - Probability that an ARC step receives swing", "%", 0.0f, 100.0f);
		configParam(NRTC_PARAM, static_cast<float>(protoseq::ARC_RATCHET_COUNT_MIN), static_cast<float>(protoseq::ARC_RATCHET_COUNT_MAX), 1.0f, "NRTC - Number of ratchet triggers");
		configParam(RRTC_PARAM, 0.0f, 1.0f, 0.0f, "RRTC - Probability that an ARC step receives ratchet", "%", 0.0f, 100.0f);
		configParam(BRNL_PARAM, 0.0f, 1.0f, 0.0f, "BRNL - Bernoulli skip probability for ARC OUT", "%", 0.0f, 100.0f);
		configParam(SEED_PARAM, 0.0f, 1.0f, 0.0f, "SEED - Global rhythmic seed; 0 is mutable", "", 0.0f, 1000.0f);
		configButton(PLAY_PARAM, "PLAY - Start clock");
		configButton(STOP_PARAM, "STOP - Stop clock and reset internal cycle");

		if (ParamQuantity* barQuantity = getParamQuantity(BAR_PARAM)) {
			barQuantity->snapEnabled = true;
		}
		if (ParamQuantity* arpcQuantity = getParamQuantity(ARPC_PARAM)) {
			arpcQuantity->snapEnabled = true;
		}
		if (ParamQuantity* nrtcQuantity = getParamQuantity(NRTC_PARAM)) {
			nrtcQuantity->snapEnabled = true;
		}
		configInput(MAIN_CV_INPUT, "MAIN CV IN - replaces MAIN knob; 0V=20 BPM, 1V=350 BPM");
		configInput(BAR_CV_INPUT, "BAR CV IN - replaces BAR knob with normalized 0..1V snapped to BAR length");
		configInput(PW_CV_INPUT, "PW CV IN - replaces PW knob with normalized 0..1V; low gap preserved");
		configInput(ARPC_CV_INPUT, "ARPC CV IN - replaces ARPC knob with normalized 0..1V snapped to multiplier table");
		configInput(GLEN_CV_INPUT, "GLEN CV IN - replaces GLEN knob with normalized 0..1V");
		configInput(RLEN_CV_INPUT, "RLEN CV IN - replaces RLEN knob with normalized 0..1V");
		configInput(SWNG_CV_INPUT, "SWNG CV IN - replaces SWNG knob with normalized 0..1V");
		configInput(RSWN_CV_INPUT, "RSWN CV IN - replaces RSWN knob with normalized 0..1V");
		configInput(NRTC_CV_INPUT, "NRTC CV IN - replaces NRTC knob with normalized 0..1V snapped to 1..8");
		configInput(RRTC_CV_INPUT, "RRTC CV IN - replaces RRTC knob with normalized 0..1V");
		configInput(BRNL_CV_INPUT, "BRNL CV IN - replaces BRNL knob with normalized 0..1V skip probability");
		configInput(SEED_CV_INPUT, "SEED CV IN - replaces SEED knob; 0 mutable, >0 fixed buckets 1..1000");
		configInput(PLAY_STOP_TOGGLE_INPUT, "PLAY/STOP TOGGLE IN - rising edge toggles playback; falling edge does nothing");
		configInput(PLAY_STOP_GATE_INPUT, "GATE PLAY/STOP IN - rising edge resets and plays; falling edge stops");

		configOutput(MAIN_OUTPUT, "MAIN OUT - Master clock/gate output");
		configOutput(ARC_OUTPUT, "ARC OUT - Arpeggio clock output");
		configOutput(BAR_OUTPUT, "BAR OUT - 10V trigger at each BAR boundary");
	}

	float getEffectiveBpm() {
		if (inputs[MAIN_CV_INPUT].isConnected()) {
			return bpmFromNormalized(inputs[MAIN_CV_INPUT].getVoltage());
		}
		return std::min(std::max(params[MAIN_PARAM].getValue(), MIN_BPM), MAX_BPM);
	}

	float getEffectivePulseWidth() {
		const float rawPulseWidth = inputs[PW_CV_INPUT].isConnected()
			? inputs[PW_CV_INPUT].getVoltage()
			: params[PW_PARAM].getValue();
		return std::min(clamp01(rawPulseWidth), MAX_MAIN_PULSE_WIDTH);
	}

	protoseq::ArcMultiplier getEffectiveArcMultiplier() {
		const int index = inputs[ARPC_CV_INPUT].isConnected()
			? protoseq::arcMultiplierIndexFromNormalized(inputs[ARPC_CV_INPUT].getVoltage())
			: protoseq::arcMultiplierIndexFromParam(params[ARPC_PARAM].getValue());
		return protoseq::arcMultiplierForIndex(index);
	}

	float getEffectiveArcGateLength() {
		const float rawGateLength = inputs[GLEN_CV_INPUT].isConnected()
			? inputs[GLEN_CV_INPUT].getVoltage()
			: params[GLEN_PARAM].getValue();
		return std::min(clamp01(rawGateLength), MAX_ARC_GATE_LENGTH);
	}

	int getEffectiveBarLength() {
		return inputs[BAR_CV_INPUT].isConnected()
			? protoseq::arcBarLengthFromNormalized(inputs[BAR_CV_INPUT].getVoltage())
			: protoseq::arcBarLengthFromParam(params[BAR_PARAM].getValue());
	}

	int getEffectiveSeedBucket() {
		const float rawSeed = inputs[SEED_CV_INPUT].isConnected()
			? inputs[SEED_CV_INPUT].getVoltage()
			: params[SEED_PARAM].getValue();
		return protoseq::arcSeedBucketFromNormalized(rawSeed);
	}

	float getEffectiveBrnlSkipProbability() {
		const float rawBrnl = inputs[BRNL_CV_INPUT].isConnected()
			? inputs[BRNL_CV_INPUT].getVoltage()
			: params[BRNL_PARAM].getValue();
		return clamp01(rawBrnl);
	}

	float getEffectiveRandomLengthAmount() {
		const float rawRlen = inputs[RLEN_CV_INPUT].isConnected()
			? inputs[RLEN_CV_INPUT].getVoltage()
			: params[RLEN_PARAM].getValue();
		return clamp01(rawRlen);
	}

	int getEffectiveRatchetSelectedCount() {
		return inputs[NRTC_CV_INPUT].isConnected()
			? protoseq::arcRatchetCountFromNormalized(inputs[NRTC_CV_INPUT].getVoltage())
			: protoseq::arcRatchetCountFromParam(params[NRTC_PARAM].getValue());
	}

	float getEffectiveRatchetProbability() {
		const float rawRrtc = inputs[RRTC_CV_INPUT].isConnected()
			? inputs[RRTC_CV_INPUT].getVoltage()
			: params[RRTC_PARAM].getValue();
		return clamp01(rawRrtc);
	}

	float getEffectiveSwingAmount() {
		const float rawSwing = inputs[SWNG_CV_INPUT].isConnected()
			? inputs[SWNG_CV_INPUT].getVoltage()
			: params[SWNG_PARAM].getValue();
		return clamp01(rawSwing);
	}

	float getEffectiveSwingProbability() {
		const float rawSwingProbability = inputs[RSWN_CV_INPUT].isConnected()
			? inputs[RSWN_CV_INPUT].getVoltage()
			: params[RSWN_PARAM].getValue();
		return clamp01(rawSwingProbability);
	}

	void updateArcRandomFoundation(int effectiveBarLen) {
		seedBucket = getEffectiveSeedBucket();
		barStep = protoseq::arcBarStep(arcStepIndex, effectiveBarLen);
		arcChannelSeeds[0] = protoseq::buildArcSeed(seedBucket, barStep, effectiveBarLen, protoseq::ArcRandomChannelId::BRNL);
		arcChannelSeeds[1] = protoseq::buildArcSeed(seedBucket, barStep, effectiveBarLen, protoseq::ArcRandomChannelId::RLEN);
		arcChannelSeeds[2] = protoseq::buildArcSeed(seedBucket, barStep, effectiveBarLen, protoseq::ArcRandomChannelId::RATCH);
		arcChannelSeeds[3] = protoseq::buildArcSeed(seedBucket, barStep, effectiveBarLen, protoseq::ArcRandomChannelId::SWNG);
	}

	bool chooseArcStepSkipped(float skipProbability, int effectiveBarLen) {
		if (skipProbability <= 0.0f) {
			return false;
		}
		if (skipProbability >= 1.0f) {
			return true;
		}
		if (seedBucket == 0) {
			std::uniform_real_distribution<double> distribution(0.0, 1.0);
			return protoseq::arcShouldSkipBernoulli(skipProbability, distribution(arcMutableRandomEngine));
		}
		return protoseq::arcShouldSkipBernoulliDeterministic(seedBucket, barStep, effectiveBarLen, skipProbability, protoseq::ArcRandomChannelId::BRNL);
	}

	float chooseArcGateLengthScale(float randomLengthAmount, int effectiveBarLen) {
		if (randomLengthAmount <= 0.0f) {
			return 1.0f;
		}
		if (seedBucket == 0) {
			std::uniform_real_distribution<double> distribution(0.0, 1.0);
			return static_cast<float>(protoseq::arcRandomLengthScale(randomLengthAmount, distribution(arcMutableRandomEngine)));
		}
		const double randomUnit = protoseq::arcUnitRandomFromSeed(protoseq::buildArcSeed(seedBucket, barStep, effectiveBarLen, protoseq::ArcRandomChannelId::RLEN));
		return static_cast<float>(protoseq::arcRandomLengthScale(randomLengthAmount, randomUnit));
	}

	void updateArcGateLengthScale(float randomLengthAmount, int effectiveBarLen) {
		if (currentArcStepSkipped || randomLengthAmount <= 0.0f) {
			currentArcGateLengthScale = 1.0f;
			evaluatedRlenArcStepIndex = -1;
			return;
		}
		if (evaluatedRlenArcStepIndex == arcStepIndex && evaluatedRlenSeedBucket == seedBucket && evaluatedRlenBarStep == barStep && evaluatedRlenBarLength == effectiveBarLen && evaluatedRlenAmount == randomLengthAmount) {
			return;
		}
		currentArcGateLengthScale = chooseArcGateLengthScale(randomLengthAmount, effectiveBarLen);
		evaluatedRlenArcStepIndex = arcStepIndex;
		evaluatedRlenSeedBucket = seedBucket;
		evaluatedRlenBarStep = barStep;
		evaluatedRlenBarLength = effectiveBarLen;
		evaluatedRlenAmount = randomLengthAmount;
	}

	int chooseArcRatchetCount(float ratchetProbability, int selectedRatchetCount, int effectiveBarLen) {
		const int clampedRatchetCount = protoseq::arcRatchetCountFromParam(static_cast<float>(selectedRatchetCount));
		if (clampedRatchetCount <= 1 || ratchetProbability <= 0.0f) {
			return 1;
		}
		if (ratchetProbability >= 1.0f) {
			return clampedRatchetCount;
		}
		if (seedBucket == 0) {
			std::uniform_real_distribution<double> distribution(0.0, 1.0);
			return protoseq::arcShouldRatchet(ratchetProbability, distribution(arcMutableRandomEngine), clampedRatchetCount) ? clampedRatchetCount : 1;
		}
		return protoseq::arcShouldRatchetDeterministic(seedBucket, barStep, effectiveBarLen, ratchetProbability, clampedRatchetCount, protoseq::ArcRandomChannelId::RATCH) ? clampedRatchetCount : 1;
	}

	bool chooseMutableArcSwingStep(float swingProbability) {
		if (swingProbability <= 0.0f) {
			return false;
		}
		if (swingProbability >= 1.0f) {
			return true;
		}
		std::uniform_real_distribution<double> distribution(0.0, 1.0);
		return protoseq::arcShouldSwing(swingProbability, distribution(arcMutableRandomEngine));
	}

	void invalidateArcSwingSchedule() {
		arcSwingSchedule.clear();
		scheduledBarStartArcStepIndex = -1;
		scheduledBarLength = -1;
		scheduledSeedBucket = -1;
		scheduledSwingAmount = -1.0f;
		scheduledSwingProbability = -1.0f;
		scheduledArcStepDuration = -1.0;
		scheduledFirstBaseStartSeconds = 0.0;
	}

	void updateArcSwingSchedule(int effectiveBarLen, double arcStepDuration, float swingAmount, float swingProbability) {
		const int clampedBarLen = protoseq::arcBarLengthFromParam(static_cast<float>(effectiveBarLen));
		const int currentBarStep = protoseq::arcBarStep(arcStepIndex, clampedBarLen);
		const int barStartArcStepIndex = arcStepIndex - currentBarStep;
		const double firstBaseStartSeconds = currentArcStepStartSeconds - static_cast<double>(currentBarStep) * arcStepDuration;
		const bool cacheMatches = scheduledBarStartArcStepIndex == barStartArcStepIndex
			&& scheduledBarLength == clampedBarLen
			&& scheduledSeedBucket == seedBucket
			&& scheduledSwingAmount == swingAmount
			&& scheduledSwingProbability == swingProbability
			&& std::fabs(scheduledArcStepDuration - arcStepDuration) < 1e-12
			&& std::fabs(scheduledFirstBaseStartSeconds - firstBaseStartSeconds) < 1e-12
			&& static_cast<int>(arcSwingSchedule.size()) == clampedBarLen + 1;
		if (cacheMatches) {
			return;
		}

		if (seedBucket > 0) {
			arcSwingSchedule = protoseq::arcBuildSwingSchedule(clampedBarLen, firstBaseStartSeconds, arcStepDuration, swingAmount, swingProbability, seedBucket);
		}
		else {
			arcSwingSchedule.clear();
			arcSwingSchedule.reserve(static_cast<std::size_t>(clampedBarLen + 1));
			const bool swingActive = swingAmount > 0.0f && swingProbability > 0.0f;
			for (int i = 0; i <= clampedBarLen; ++i) {
				const int step = i % clampedBarLen;
				const double baseStart = firstBaseStartSeconds + static_cast<double>(i) * arcStepDuration;
				const bool swung = swingActive && chooseMutableArcSwingStep(swingProbability);
				arcSwingSchedule.push_back({step, baseStart, protoseq::arcScheduledEventStartSeconds(baseStart, arcStepDuration, swingAmount, swung), swung});
			}
		}

		scheduledBarStartArcStepIndex = barStartArcStepIndex;
		scheduledBarLength = clampedBarLen;
		scheduledSeedBucket = seedBucket;
		scheduledSwingAmount = swingAmount;
		scheduledSwingProbability = swingProbability;
		scheduledArcStepDuration = arcStepDuration;
		scheduledFirstBaseStartSeconds = firstBaseStartSeconds;
	}

	void updateArcRatchetCount(float ratchetProbability, int selectedRatchetCount, int effectiveBarLen) {
		const int clampedRatchetCount = protoseq::arcRatchetCountFromParam(static_cast<float>(selectedRatchetCount));
		if (currentArcStepSkipped || clampedRatchetCount <= 1 || ratchetProbability <= 0.0f) {
			currentArcRatchetCount = 1;
			evaluatedRatchetArcStepIndex = -1;
			return;
		}
		if (ratchetProbability >= 1.0f) {
			currentArcRatchetCount = clampedRatchetCount;
			evaluatedRatchetArcStepIndex = -1;
			return;
		}
		if (evaluatedRatchetArcStepIndex == arcStepIndex && evaluatedRatchetSeedBucket == seedBucket && evaluatedRatchetBarStep == barStep && evaluatedRatchetBarLength == effectiveBarLen && evaluatedRatchetSelectedCount == clampedRatchetCount && evaluatedRatchetProbability == ratchetProbability) {
			return;
		}
		currentArcRatchetCount = chooseArcRatchetCount(ratchetProbability, clampedRatchetCount, effectiveBarLen);
		evaluatedRatchetArcStepIndex = arcStepIndex;
		evaluatedRatchetSeedBucket = seedBucket;
		evaluatedRatchetBarStep = barStep;
		evaluatedRatchetBarLength = effectiveBarLen;
		evaluatedRatchetSelectedCount = clampedRatchetCount;
		evaluatedRatchetProbability = ratchetProbability;
	}

	void updateArcStepSkipDecision(float skipProbability, int effectiveBarLen) {
		if (skipProbability <= 0.0f) {
			currentArcStepSkipped = false;
			evaluatedArcStepIndex = -1;
			return;
		}
		if (skipProbability >= 1.0f) {
			currentArcStepSkipped = true;
			evaluatedArcStepIndex = -1;
			return;
		}
		if (evaluatedArcStepIndex == arcStepIndex && evaluatedSeedBucket == seedBucket && evaluatedBarStep == barStep && evaluatedBarLength == effectiveBarLen) {
			return;
		}
		currentArcStepSkipped = chooseArcStepSkipped(skipProbability, effectiveBarLen);
		evaluatedArcStepIndex = arcStepIndex;
		evaluatedSeedBucket = seedBucket;
		evaluatedBarStep = barStep;
		evaluatedBarLength = effectiveBarLen;
	}

	void resetPhases() {
		mainPhase = 0.0;
		arcPhase = 0.0;
		currentArcStepStartSeconds = 0.0;
		arcStepIndex = 0;
		barStep = 0;
		currentArcStepSkipped = false;
		evaluatedArcStepIndex = -1;
		evaluatedSeedBucket = -1;
		evaluatedBarStep = -1;
		evaluatedBarLength = -1;
		currentArcGateLengthScale = 1.0f;
		evaluatedRlenArcStepIndex = -1;
		evaluatedRlenSeedBucket = -1;
		evaluatedRlenBarStep = -1;
		evaluatedRlenBarLength = -1;
		evaluatedRlenAmount = -1.0f;
		currentArcRatchetCount = 1;
		evaluatedRatchetArcStepIndex = -1;
		evaluatedRatchetSeedBucket = -1;
		evaluatedRatchetBarStep = -1;
		evaluatedRatchetBarLength = -1;
		evaluatedRatchetSelectedCount = -1;
		evaluatedRatchetProbability = -1.0f;
		invalidateArcSwingSchedule();
		barPulseGenerator.process(1.0f);
	}

	void triggerBarPulse() {
		barPulseGenerator.trigger(0.001f);
	}

	void startPlayback(bool resetPhase) {
		const bool wasPlaying = isPlaying;
		if (resetPhase) {
			resetPhases();
		}
		if (!wasPlaying || resetPhase) {
			isPlaying = true;
			if (!skipBarPulseOnPlay) {
				triggerBarPulse();
			}
		}
	}

	void stopPlayback(bool resetPhase) {
		if (isPlaying) {
			isPlaying = false;
			if (resetPhase) {
				resetPhases();
			}
		}
		barPulseGenerator.process(1.0f);
		outputs[MAIN_OUTPUT].setVoltage(0.0f);
		outputs[ARC_OUTPUT].setVoltage(0.0f);
		outputs[BAR_OUTPUT].setVoltage(0.0f);
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "skipBarPulseOnPlay", json_boolean(skipBarPulseOnPlay));
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		skipBarPulseOnPlay = true;
		json_t* skipBarPulseOnPlayJ = json_object_get(rootJ, "skipBarPulseOnPlay");
		if (skipBarPulseOnPlayJ) {
			skipBarPulseOnPlay = json_boolean_value(skipBarPulseOnPlayJ) != 0;
		}
	}

	void process(const ProcessArgs& args) override {
		const float effectiveBpm = getEffectiveBpm();
		displayBpm = static_cast<int>(std::round(effectiveBpm));

		if (playButtonTrigger.process(params[PLAY_PARAM].getValue())) {
			startPlayback(false);
		}
		if (playStopToggleTrigger.process(inputs[PLAY_STOP_TOGGLE_INPUT].getVoltage())) {
			if (isPlaying) {
				stopPlayback(false);
			}
			else {
				startPlayback(false);
			}
		}
		if (stopButtonTrigger.process(params[STOP_PARAM].getValue())) {
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
			outputs[BAR_OUTPUT].setVoltage(0.0f);
			return;
		}

		const int barLength = getEffectiveBarLength();
		updateArcRandomFoundation(barLength);
		const protoseq::ArcMultiplier arcMultiplier = getEffectiveArcMultiplier();
		const double arcStepDuration = protoseq::arcStepDurationSeconds(effectiveBpm, arcMultiplier);
		const float swingAmount = getEffectiveSwingAmount();
		const float swingProbability = getEffectiveSwingProbability();
		updateArcSwingSchedule(barLength, arcStepDuration, swingAmount, swingProbability);

		const float brnlSkipProbability = getEffectiveBrnlSkipProbability();
		updateArcStepSkipDecision(brnlSkipProbability, barLength);
		const float randomLengthAmount = getEffectiveRandomLengthAmount();
		updateArcGateLengthScale(randomLengthAmount, barLength);
		const int selectedRatchetCount = getEffectiveRatchetSelectedCount();
		const float ratchetProbability = getEffectiveRatchetProbability();
		updateArcRatchetCount(ratchetProbability, selectedRatchetCount, barLength);

		const int scheduleIndex = std::min(std::max(barStep, 0), static_cast<int>(arcSwingSchedule.size()) - 2);
		const protoseq::ArcSwingScheduleEvent& currentArcEvent = arcSwingSchedule[scheduleIndex];
		const protoseq::ArcSwingScheduleEvent& nextArcEvent = arcSwingSchedule[scheduleIndex + 1];
		const double currentArcStepStart = currentArcEvent.start;
		const double nextArcStepStart = nextArcEvent.start;
		const double currentArcTimelineSeconds = currentArcStepStartSeconds + arcPhase * arcStepDuration;
		const double currentArcEventPhase = (currentArcTimelineSeconds - currentArcStepStart) / arcStepDuration;

		const float pulseWidth = getEffectivePulseWidth();
		const float arcGateLength = getEffectiveArcGateLength();
		const float shortenedArcGateLength = std::min(arcGateLength, arcGateLength * currentArcGateLengthScale);
		const double desiredArcGateDuration = static_cast<double>(shortenedArcGateLength) * arcStepDuration;
		const double effectiveArcGateDuration = protoseq::arcEffectiveGateDurationSeconds(currentArcStepStart, nextArcStepStart, desiredArcGateDuration, ARC_GATE_SAFETY_GAP_SECONDS);
		const double effectiveArcGatePhase = effectiveArcGateDuration / arcStepDuration;
		const double ratchetSafetyGapPhase = ARC_GATE_SAFETY_GAP_SECONDS / arcStepDuration;
		const bool arcGateActive = protoseq::arcRatchetGateActive(currentArcEventPhase, effectiveArcGatePhase, currentArcRatchetCount, ratchetSafetyGapPhase);
		outputs[MAIN_OUTPUT].setVoltage(mainPhase < pulseWidth ? GATE_HIGH_VOLTAGE : 0.0f);
		outputs[ARC_OUTPUT].setVoltage((!currentArcStepSkipped && arcGateActive) ? GATE_HIGH_VOLTAGE : 0.0f);

		const double mainPhaseDelta = static_cast<double>(effectiveBpm) * args.sampleTime / 60.0;
		mainPhase += mainPhaseDelta;
		while (mainPhase >= 1.0) {
			mainPhase -= 1.0;
		}

		arcPhase += args.sampleTime / arcStepDuration;
		while (arcPhase >= 1.0) {
			arcPhase -= 1.0;
			currentArcStepStartSeconds += arcStepDuration;
			++arcStepIndex;
			barStep = protoseq::arcBarStep(arcStepIndex, barLength);
			if (barStep == 0) {
				triggerBarPulse();
			}
			evaluatedArcStepIndex = -1;
			evaluatedRlenArcStepIndex = -1;
			evaluatedRatchetArcStepIndex = -1;
		}
		outputs[BAR_OUTPUT].setVoltage(barPulseGenerator.process(args.sampleTime) ? GATE_HIGH_VOLTAGE : 0.0f);
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

		addInput(createInputCentered<ArcJack>(arcMm(8.88f, 112.82f), module, Arc::PLAY_STOP_TOGGLE_INPUT));
		addInput(createInputCentered<ArcJack>(arcMm(19.67f, 112.82f), module, Arc::PLAY_STOP_GATE_INPUT));

		addOutput(createOutputCentered<ArcJack>(arcMm(30.46f, 112.82f), module, Arc::BAR_OUTPUT));
		addOutput(createOutputCentered<ArcJack>(arcMm(41.25f, 112.82f), module, Arc::MAIN_OUTPUT));
		addOutput(createOutputCentered<ArcJack>(arcMm(52.04f, 112.82f), module, Arc::ARC_OUTPUT));
	}

	void appendContextMenu(Menu* menu) override {
		if (!menu) return;
		Arc* arcModule = dynamic_cast<Arc*>(module);
		menu->addChild(new MenuSeparator());
		menu->addChild(createCheckMenuItem("Skip BAR pulse on PLAY", "",
			[arcModule]() { return arcModule && arcModule->skipBarPulseOnPlay; },
			[arcModule]() { if (arcModule) arcModule->skipBarPulseOnPlay = !arcModule->skipBarPulseOnPlay; }));
	}
};

Model* modelArc = createModel<Arc, ArcWidget>("Arc");
