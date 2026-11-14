// Press Limit for Geode (GD 2.2081, Windows)
//
// In the level editor a new "Limit" button opens a box. Type how many presses you want
// (a number, or E_0, l_w, a_n, O_w, ^ for Knuth arrows, + * and brackets) and the mod places
// a block plus the triggers that count presses. After that many presses the block turns off.
// Numbers too big for one counter use a chain of counters: when counter 1 reaches the 32-bit
// maximum it resets to 0 and counter 2 goes up by 1, and so on (up to 16 counters).
//
// NOTE: written without a Windows GD to test in. Check the object IDs in plan.hpp in the editor.

#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>

#include "plan.hpp"

using namespace geode::prelude;

class LimitPopup : public geode::Popup {
protected:
	TextInput* m_input = nullptr;
	CCLabelBMFont* m_info = nullptr;
	EditorUI* m_ui = nullptr;

	bool init(EditorUI* ui) {
		if (!Popup::init(340.f, 220.f)) return false;
		m_ui = ui;
		this->setTitle("Press Limit");

		auto hint = CCLabelBMFont::create("Examples: 100  E_0  l_w  a_n  2^^5  3^^^3", "chatFont.fnt");
		hint->setScale(.8f);
		m_mainLayer->addChildAtPosition(hint, Anchor::Center, ccp(0, 55));

		m_input = TextInput::create(260.f, "How many presses?");
		m_input->setMaxCharCount(60);
		m_input->setFilter("0123456789+*^()_ EeLlWwAaNnOo");
		m_mainLayer->addChildAtPosition(m_input, Anchor::Center, ccp(0, 20));

		m_info = CCLabelBMFont::create("", "chatFont.fnt");
		m_info->setScale(.7f);
		m_mainLayer->addChildAtPosition(m_info, Anchor::Center, ccp(0, -20));

		auto spr = ButtonSprite::create("Make it");
		auto btn = CCMenuItemExt::createSpriteExtra(spr, [this](CCObject*) { this->make(); });
		m_buttonMenu->addChildAtPosition(btn, Anchor::Bottom, ccp(0, 35));
		return true;
	}

	void make() {
		std::string text = m_input->getString();
		pl::Parser parser(text);
		pl::Num n;
		std::string err;
		if (!parser.parse(n, err)) {
			m_info->setString(err.c_str());
			return;
		}
		auto digits = pl::toDigits(n);
		pl::Ids ids;
		auto editor = m_ui->m_editorLayer;
		auto layer = editor->m_objectLayer;
		auto win = CCDirector::get()->getWinSize();
		float scale = layer->getScale() != 0.f ? layer->getScale() : 1.f;
		float x = (win.width / 2 - layer->getPositionX()) / scale;
		float y = (win.height / 2 - layer->getPositionY()) / scale;
		auto plan = pl::build(digits, ids, x, y);
		if (!plan.error.empty()) {
			m_info->setString(plan.error.c_str());
			return;
		}
		auto objs = editor->createObjectsFromString(plan.objects, true, true);
		if (objs) m_ui->selectObjects(objs, true);
		if (digits.infinite) {
			FLAlertLayer::create(
				"Too big",
				"That number is bigger than 16 counters can hold, so it can never really be reached. "
				"I made the biggest chain I can: the block will practically never disappear.",
				"OK"
			)->show();
		}
		this->onClose(nullptr);
	}

public:
	static LimitPopup* create(EditorUI* ui) {
		auto r = new LimitPopup();
		if (r->init(ui)) {
			r->autorelease();
			return r;
		}
		delete r;
		return nullptr;
	}
};

class $modify(LimitEditorUI, EditorUI) {
	bool init(LevelEditorLayer* layer) {
		if (!EditorUI::init(layer)) return false;
		auto menu = CCMenu::create();
		menu->setID("press-limit-menu"_spr);
		auto spr = ButtonSprite::create("Limit", 40, true, "bigFont.fnt", "GJ_button_04.png", 30.f, .6f);
		auto btn = CCMenuItemExt::createSpriteExtra(spr, [this](CCObject*) {
			if (auto p = LimitPopup::create(this)) p->show();
		});
		menu->addChild(btn);
		auto win = CCDirector::get()->getWinSize();
		menu->setPosition(ccp(win.width - 50.f, win.height - 60.f));
		this->addChild(menu, 100);
		return true;
	}
};
