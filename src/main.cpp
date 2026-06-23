#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/binding/LevelListLayer.hpp>
#include <Geode/binding/GJLevelList.hpp>
#include <Geode/binding/GameLevelManager.hpp>
#include <Geode/binding/LoadingCircle.hpp>
#include <algorithm>
#include <functional>

using namespace geode::prelude;

static std::map<std::string, int> LIST_IDS = {
    {"motd", 2584739},
    {"platformer", 2584739},
    {"motw", 2584740},
    {"platformer_motw", 2584821},
    {"motm", 2584818},
    {"curve", 2584819},
};

class MOTDSelectLayer : public CCLayer {
protected:
	std::function<void(const std::string&)> m_callback;
	CCMenu* m_mainMenu = nullptr;
	CCMenu* m_subMenu = nullptr;
	CCLabelBMFont* m_titleLabel = nullptr;
	std::string m_currentCategory;

	bool init(std::function<void(const std::string&)> callback) {
		if (!CCLayer::init()) return false;

		m_callback = callback;

		auto winSize = CCDirector::sharedDirector()->getWinSize();

		auto bg = CCLayerColor::create(ccc4(0, 0, 0, 120));
		addChild(bg, -1);

		auto panel = extension::CCScale9Sprite::create("GJ_square01.png");
		panel->setContentSize({340, 280});
		panel->setPosition(winSize.width / 2, winSize.height / 2);
		addChild(panel);

		m_titleLabel = CCLabelBMFont::create("MOTD S3", "goldFont.fnt");
		m_titleLabel->setPosition(winSize.width / 2, winSize.height / 2 + 100);
		addChild(m_titleLabel);

		m_mainMenu = CCMenu::create();
		m_mainMenu->setPosition(0, 0);

		auto classicBtn = CCMenuItemSpriteExtra::create(
			ButtonSprite::create("CLASSIC"),
			this, menu_selector(MOTDSelectLayer::onClassic)
		);
		classicBtn->setPosition(winSize.width / 2 - 75, winSize.height / 2 + 10);
		m_mainMenu->addChild(classicBtn);

		auto platBtn = CCMenuItemSpriteExtra::create(
			ButtonSprite::create("PLATFORMER"),
			this, menu_selector(MOTDSelectLayer::onPlatformer)
		);
		platBtn->setPosition(winSize.width / 2 + 75, winSize.height / 2 + 10);
		m_mainMenu->addChild(platBtn);

		auto curveBtn = CCMenuItemSpriteExtra::create(
			ButtonSprite::create("CURVE"),
			this, menu_selector(MOTDSelectLayer::onCurve)
		);
		curveBtn->setPosition(winSize.width / 2, winSize.height / 2 - 45);
		m_mainMenu->addChild(curveBtn);

		addChild(m_mainMenu);

		m_subMenu = CCMenu::create();
		m_subMenu->setPosition(0, 0);
		m_subMenu->setVisible(false);
		addChild(m_subMenu);

		auto closeBtn = CCMenuItemSpriteExtra::create(
			CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png"),
			this, menu_selector(MOTDSelectLayer::onClose)
		);
		auto closeMenu = CCMenu::create();
		closeMenu->setPosition(winSize.width / 2 + 150, winSize.height / 2 + 120);
		closeMenu->addChild(closeBtn);
		addChild(closeMenu);

		setTouchEnabled(true);
		setKeypadEnabled(true);

		return true;
	}

	void rebuildSubMenu(const std::string& category) {
		m_subMenu->removeAllChildren();
		m_currentCategory = category;

		auto winSize = CCDirector::sharedDirector()->getWinSize();

		std::vector<std::string> entries;
		if (category == "classic") {
			entries = {"DAY", "WEEK", "MONTH"};
		} else if (category == "platformer") {
			entries = {"DAY", "WEEK"};
		}

		float spacing = 80.f;
		float totalW = spacing * (entries.size() - 1);
		float startX = winSize.width / 2 - totalW / 2;

		for (size_t i = 0; i < entries.size(); i++) {
			auto btn = CCMenuItemSpriteExtra::create(
				ButtonSprite::create(entries[i].c_str()),
				this, menu_selector(MOTDSelectLayer::onSubSelect)
			);
			btn->setPosition(startX + spacing * i, winSize.height / 2 + 10);
			btn->setTag(i);
			m_subMenu->addChild(btn);
		}

		auto backBtn = CCMenuItemSpriteExtra::create(
			ButtonSprite::create("BACK"),
			this, menu_selector(MOTDSelectLayer::onBack)
		);
		backBtn->setPosition(winSize.width / 2, winSize.height / 2 - 50);
		m_subMenu->addChild(backBtn);

		m_subMenu->setVisible(true);
	}

	void onClassic(CCObject*) {
		m_mainMenu->setVisible(false);
		m_titleLabel->setString("CLASSIC");
		rebuildSubMenu("classic");
	}

	void onPlatformer(CCObject*) {
		m_mainMenu->setVisible(false);
		m_titleLabel->setString("PLATFORMER");
		rebuildSubMenu("platformer");
	}

	void onCurve(CCObject*) {
		selectList("curve");
	}

	void onSubSelect(CCObject* sender) {
		auto btn = static_cast<CCMenuItemSpriteExtra*>(sender);
		int idx = btn->getTag();

		std::vector<std::string> types;
		if (m_currentCategory == "classic") {
			types = {"motd", "motw", "motm"};
		} else if (m_currentCategory == "platformer") {
			types = {"platformer", "platformer_motw"};
		}

		if (idx >= 0 && idx < (int)types.size()) {
			selectList(types[idx]);
		}
	}

	void onBack(CCObject*) {
		m_subMenu->setVisible(false);
		m_titleLabel->setString("MOTD S3");
		m_mainMenu->setVisible(true);
	}

	void onClose(CCObject*) {
		removeFromParent();
	}

	void keyBackClicked() override {
		removeFromParent();
	}

	void selectList(const std::string& type) {
		auto cb = m_callback;
		removeFromParent();
		if (cb) cb(type);
	}

public:
	static MOTDSelectLayer* create(std::function<void(const std::string&)> callback) {
		auto ret = new MOTDSelectLayer();
		if (ret && ret->init(callback)) {
			ret->autorelease();
			return ret;
		}
		CC_SAFE_DELETE(ret);
		return nullptr;
	}
};

class $modify(MyMenuLayer, MenuLayer) {
	struct Fields {
		TaskHolder<web::WebResponse> m_listDownloader;
		LoadingCircle* m_loadingCircle = nullptr;
		CCLabelBMFont* m_loadingLabel = nullptr;
	};

	bool init() {
		if (!MenuLayer::init()) return false;

		auto yuiSprite = CCSprite::create("yui-token.png"_spr);
		yuiSprite->setScale(2.0f);
		auto myButton = CCMenuItemSpriteExtra::create(
			yuiSprite,
			this,
			menu_selector(MyMenuLayer::onMyButton)
		);

		auto menu = this->getChildByID("bottom-menu");
		menu->addChild(myButton);
		myButton->setID("motd-s3-button"_spr);
		menu->updateLayout();

		return true;
	}

	void showLoading() {
		if (!m_fields->m_loadingCircle) {
			m_fields->m_loadingCircle = LoadingCircle::create();
			m_fields->m_loadingCircle->setParentLayer(this);
			m_fields->m_loadingCircle->setFade(true);
			m_fields->m_loadingCircle->show();

			m_fields->m_loadingLabel = CCLabelBMFont::create(
				"Loading MOTD...", "goldFont.fnt"
			);
			m_fields->m_loadingLabel->setScale(0.6f);
			auto winSize = CCDirector::sharedDirector()->getWinSize();
			m_fields->m_loadingLabel->setPosition(
				winSize.width / 2,
				winSize.height / 2 - 40.f
			);
			m_fields->m_loadingLabel->setZOrder(110);
			this->addChild(m_fields->m_loadingLabel);
		}
	}

	void hideLoading() {
		if (m_fields->m_loadingCircle) {
			m_fields->m_loadingCircle->fadeAndRemove();
			m_fields->m_loadingCircle = nullptr;
		}
		if (m_fields->m_loadingLabel) {
			m_fields->m_loadingLabel->removeFromParent();
			m_fields->m_loadingLabel = nullptr;
		}
	}

	void onMyButton(CCObject*) {
		auto layer = MOTDSelectLayer::create(
			[this](const std::string& type) {
				auto it = LIST_IDS.find(type);
				if (it != LIST_IDS.end()) {
					downloadAndOpenList(it->second);
				} else {
					FLAlertLayer::create("Error", "List not found!", "OK")->show();
				}
			}
		);
		CCDirector::sharedDirector()->getRunningScene()->addChild(layer, 1000);
	}

	void downloadAndOpenList(int listId) {
		showLoading();

		auto body = fmt::format("listID={}&secret=Wmfd2893gb7", listId);
		m_fields->m_listDownloader.spawn(
			web::WebRequest()
				.bodyString(body)
				.header("Content-Type", "application/x-www-form-urlencoded")
				.post("https://www.boomlings.com/database/downloadGJLevelList.php"),
			[this](web::WebResponse res) {
				hideLoading();

				if (!res.ok()) {
					FLAlertLayer::create("Error", "Failed to download list!", "OK")->show();
					return;
				}

				auto text = res.string().unwrapOr("");
				if (text.empty() || text.find("-1") == 0) {
					FLAlertLayer::create("Error", "List not found!", "OK")->show();
					return;
				}

				auto dict = GameLevelManager::responseToDict(text, false);
				if (!dict) {
					FLAlertLayer::create("Error", "Failed to parse list!", "OK")->show();
					return;
				}

				auto list = GJLevelList::create(dict);
				if (!list) {
					FLAlertLayer::create("Error", "Failed to create list!", "OK")->show();
					return;
				}

				auto scene = LevelListLayer::scene(list);
				CCDirector::sharedDirector()->pushScene(CCTransitionFade::create(0.5f, scene));
			}
		);
	}
};
