// FPS Optimizer v0.1.0 for Geode (GD 2.208, Windows)
//
// NOTE: this was written without being able to compile against the Geode SDK.
// Hook signatures follow the Geode bindings as I know them; if the compiler
// complains about a signature, fix it to match your SDK's generated headers.

#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/CCParticleSystemQuad.hpp>
#include <Geode/modify/CCMotionStreak.hpp>

#include <algorithm>
#include <cmath>
#include <string>

using namespace geode::prelude;

namespace opt {

	// Largest value a signed 32-bit integer can hold.
	constexpr int64_t kMaxIntFps = 2147483647;
	// At or above this the cap is treated as "no limit". Passing absurd numbers
	// to the frame timer gains nothing, the hardware is the real limit.
	constexpr int64_t kUnlimitedThreshold = 100000;
	constexpr double kUnlimitedInterval = 1.0 / 100000.0;

	// Savings levels the adaptive optimizer moves between.
	// 0 = none, 1 = particles off, 2 = particles + trails off.
	constexpr int kMaxTier = 2;

	struct State {
		int tier = 0;
		double avgFrameTime = 1.0 / 60.0; // smoothed, from the REAL frame time
		float sinceChange = 0.f;
		bool forceActive = false;
	};

	inline State& st() {
		static State s;
		return s;
	}

	inline bool isExtreme() {
		return st().forceActive && Mod::get()->getSettingValue<bool>("extreme-mode");
	}

	// The frame cap actually handed to the engine.
	inline int64_t effectiveMaxFps() {
		if (isExtreme()) return kMaxIntFps;
		return Mod::get()->getSettingValue<int64_t>("max-fps");
	}

	inline void applyFpsCap() {
		int64_t maxFps = effectiveMaxFps();
		double interval = (maxFps <= 0 || maxFps >= kUnlimitedThreshold)
			? kUnlimitedInterval
			: 1.0 / static_cast<double>(maxFps);
		CCDirector::get()->setAnimationInterval(interval);
	}

	inline int activeTier() {
		if (st().forceActive) return kMaxTier;
		return st().tier;
	}

	inline bool particlesOff() {
		return Mod::get()->getSettingValue<bool>("disable-particles") || activeTier() >= 1;
	}

	inline bool trailsOff() {
		return Mod::get()->getSettingValue<bool>("disable-trails") || activeTier() >= 2;
	}

	// Runs once per frame in levels. dt is the real frame time and is the only
	// thing the optimizer ever looks at, never the fake FPS text.
	inline void updateAdaptive(float dt) {
		auto& s = st();
		s.avgFrameTime = s.avgFrameTime * 0.9 + static_cast<double>(dt) * 0.1;
		s.sinceChange += dt;

		auto mod = Mod::get();
		if (!mod->getSettingValue<bool>("adaptive") || s.forceActive) return;

		double fps = 1.0 / std::max(s.avgFrameTime, 1e-6);
		int64_t minFps = mod->getSettingValue<int64_t>("min-fps");
		int64_t targetFps = mod->getSettingValue<int64_t>("target-fps");
		double floorFps = static_cast<double>(minFps > 0 ? minFps : targetFps);

		// Ease off only when there is real headroom. The threshold is also
		// kept under the frame cap, otherwise a capped game could never get
		// above it and the savings would stay on forever.
		double easeAbove = floorFps * 1.3;
		int64_t cap = effectiveMaxFps();
		if (cap > 0 && cap < kUnlimitedThreshold) {
			easeAbove = std::min(easeAbove, static_cast<double>(cap) * 0.95);
		}

		if (fps < floorFps && s.sinceChange > 0.5f && s.tier < kMaxTier) {
			s.tier++;
			s.sinceChange = 0.f;
		} else if (fps > easeAbove && s.sinceChange > 3.0f && s.tier > 0) {
			s.tier--;
			s.sinceChange = 0.f;
		}
	}

	inline std::string fakeText() {
		std::string t = Mod::get()->getSettingValue<std::string>("fake-fps-text");
		std::string lower = t;
		std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });
		if (lower == "eps0") return "eps0";
		if (lower == "omega") return "omega";
		if (lower == "knux") return "knux";
		if (lower == "inf") return "inf";
		if (lower == "9001") return "9001";
		if (lower == "nan") return "NaN";
		return t.empty() ? "?" : t;
	}

	inline std::string overlayText() {
		auto mod = Mod::get();
		if (mod->getSettingValue<bool>("fake-fps")) {
			std::string s = fakeText() + " FPS";
			if (mod->getSettingValue<bool>("fake-fps-tag")) s += " (fake)";
			return s;
		}
		double ft = st().avgFrameTime;
		double fps = 1.0 / std::max(ft, 1e-6);
		std::string s = fmt::format("{:.0f} FPS | {:.1f} ms | savings {}", fps, ft * 1000.0, activeTier());
		if (st().forceActive) s += isExtreme() ? " | EXTREME" : " | FORCE";
		return s;
	}

	// Force FPS: five confirmations in a row. Cancelling any of them turns the
	// setting back off.
	inline void showForceWarning(int index) {
		static const char* kTitles[5] = {
			"Force FPS 1/5", "Force FPS 2/5", "Force FPS 3/5", "Force FPS 4/5", "Force FPS 5/5"
		};
		static const char* kBodies[5] = {
			"Force FPS is <cr>experimental</c> and may cause <cr>instability</c>.",
			"It can <cr>crash the game</c>.",
			"It can break visuals and make triggers or objects behave oddly, because updates are skipped to save frames.",
			"<cy>Back up your save data and any editor levels</c> before using it.",
			"Last chance. Do you understand the risks and want to turn Force FPS on?"
		};

		if (index >= 5) {
			st().forceActive = true;
			Mod::get()->setSavedValue<bool>("force-dirty", true);
			applyFpsCap();
			return;
		}

		createQuickPopup(
			kTitles[index],
			kBodies[index],
			"Cancel",
			index == 4 ? "Enable" : "I understand",
			[index](FLAlertLayer*, bool confirmed) {
				if (!confirmed) {
					Mod::get()->setSettingValue<bool>("force-fps", false);
					return;
				}
				showForceWarning(index + 1);
			}
		);
	}

} // namespace opt

// Force FPS and Extreme mode always start the session switched off.
$on_mod(Loaded) {
	auto mod = Mod::get();
	mod->setSavedValue<bool>("force-dirty", false);
	mod->setSettingValue<bool>("force-fps", false);
	mod->setSettingValue<bool>("extreme-mode", false);
}

$execute {
	listenForSettingChanges<int64_t>("max-fps", [](int64_t) {
		opt::applyFpsCap();
	});

	listenForSettingChanges<bool>("extreme-mode", [](bool) {
		opt::applyFpsCap();
	});

	listenForSettingChanges<bool>("force-fps", [](bool on) {
		if (on) {
			if (!opt::st().forceActive) opt::showForceWarning(0);
		} else {
			opt::st().forceActive = false;
			Mod::get()->setSavedValue<bool>("force-dirty", false);
			opt::applyFpsCap();
		}
	});
}

class $modify(OptMenuLayer, MenuLayer) {
	bool init() {
		if (!MenuLayer::init()) return false;
		opt::applyFpsCap();
		return true;
	}
};

class $modify(OptPlayLayer, PlayLayer) {
	struct Fields {
		CCLabelBMFont* label = nullptr;
		float labelTimer = 0.f;
	};

	bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
		if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;

		opt::st().tier = 0;
		opt::st().sinceChange = 0.f;
		opt::applyFpsCap(); // the game resets the frame timer when a level loads

		auto label = CCLabelBMFont::create("", "bigFont.fnt");
		label->setScale(0.35f);
		label->setAnchorPoint({0.f, 1.f});
		auto winSize = CCDirector::get()->getWinSize();
		label->setPosition({4.f, winSize.height - 4.f});
		label->setZOrder(1000);
		label->setOpacity(190);
		m_uiLayer->addChild(label);
		m_fields->label = label;
		return true;
	}

	void postUpdate(float dt) {
		PlayLayer::postUpdate(dt);

		opt::updateAdaptive(dt);

		auto label = m_fields->label;
		if (!label) return;

		bool show = Mod::get()->getSettingValue<bool>("show-overlay");
		label->setVisible(show);
		if (!show) return;

		m_fields->labelTimer += dt;
		if (m_fields->labelTimer >= 0.2f) {
			m_fields->labelTimer = 0.f;
			label->setString(opt::overlayText().c_str());
		}
	}
};

class $modify(OptParticles, CCParticleSystemQuad) {
	void draw() {
		if (opt::particlesOff()) return;
		CCParticleSystemQuad::draw();
	}
};

class $modify(OptStreak, CCMotionStreak) {
	void draw() {
		if (opt::trailsOff()) return;
		CCMotionStreak::draw();
	}
};
