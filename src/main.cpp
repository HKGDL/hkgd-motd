#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/binding/LevelInfoLayer.hpp>
#include <Geode/binding/LoadingCircle.hpp>
#include <sstream>

using namespace geode::prelude;

static int parseLevelId(const std::string& raw) {
	auto idPos = raw.find("ID:");
	if (idPos != std::string::npos) {
		try { return std::stoi(raw.substr(idPos + 3)); } catch (...) {}
	}
	// Fallback: raw is a numeric level ID
	try { return std::stoi(raw); } catch (...) { return 0; }
}

static std::string parseCreatorName(const std::string& raw) {
	auto byPos = raw.find("by ");
	if (byPos == std::string::npos) return "";
	auto start = byPos + 3;
	auto end = raw.find('\n', start);
	if (end == std::string::npos) end = raw.find('\r', start);
	if (end == std::string::npos) end = raw.length();
	auto name = raw.substr(start, end - start);
	while (!name.empty() && name.back() == ' ') name.pop_back();
	return name;
}

class MOTDSelectLayer : public CCLayer {
protected:
	std::map<std::string, std::string> m_messages;
	std::function<void(const std::string&)> m_callback;
	CCMenu* m_mainMenu = nullptr;
	CCMenu* m_subMenu = nullptr;
	CCLabelBMFont* m_titleLabel = nullptr;

	bool init(const std::vector<std::string>& types, const std::map<std::string, std::string>& messages, std::function<void(const std::string&)> callback) {
		if (!CCLayer::init()) return false;

		m_messages = messages;
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

		bool hasClassic = std::find(types.begin(), types.end(), "daily") != types.end()
			|| std::find(types.begin(), types.end(), "weekly") != types.end()
			|| std::find(types.begin(), types.end(), "monthly") != types.end();

		if (hasClassic) {
			auto classicBtn = CCMenuItemSpriteExtra::create(
				ButtonSprite::create("CLASSIC"),
				this, menu_selector(MOTDSelectLayer::onClassic)
			);
			classicBtn->setPosition(winSize.width / 2 - 75, winSize.height / 2 + 10);
			m_mainMenu->addChild(classicBtn);
		}

		if (std::find(types.begin(), types.end(), "platformer") != types.end()) {
			auto platBtn = CCMenuItemSpriteExtra::create(
				ButtonSprite::create("PLATFORMER"),
				this, menu_selector(MOTDSelectLayer::onPlatformer)
			);
			platBtn->setPosition(winSize.width / 2 + 75, winSize.height / 2 + 10);
			m_mainMenu->addChild(platBtn);
		}

		if (std::find(types.begin(), types.end(), "curve") != types.end()) {
			auto curveBtn = CCMenuItemSpriteExtra::create(
				ButtonSprite::create("CURVE"),
				this, menu_selector(MOTDSelectLayer::onCurve)
			);
			curveBtn->setPosition(winSize.width / 2, winSize.height / 2 - 45);
			m_mainMenu->addChild(curveBtn);
		}

		addChild(m_mainMenu);

		m_subMenu = CCMenu::create();
		m_subMenu->setPosition(0, 0);
		m_subMenu->setVisible(false);

		if (std::find(types.begin(), types.end(), "daily") != types.end()) {
			auto dayBtn = CCMenuItemSpriteExtra::create(
				ButtonSprite::create("DAY"),
				this, menu_selector(MOTDSelectLayer::onDay)
			);
			dayBtn->setPosition(winSize.width / 2 - 80, winSize.height / 2 + 10);
			m_subMenu->addChild(dayBtn);
		}

		if (std::find(types.begin(), types.end(), "weekly") != types.end()) {
			auto weekBtn = CCMenuItemSpriteExtra::create(
				ButtonSprite::create("WEEK"),
				this, menu_selector(MOTDSelectLayer::onWeek)
			);
			weekBtn->setPosition(winSize.width / 2, winSize.height / 2 + 10);
			m_subMenu->addChild(weekBtn);
		}

		if (std::find(types.begin(), types.end(), "monthly") != types.end()) {
			auto monthBtn = CCMenuItemSpriteExtra::create(
				ButtonSprite::create("MONTH"),
				this, menu_selector(MOTDSelectLayer::onMonth)
			);
			monthBtn->setPosition(winSize.width / 2 + 80, winSize.height / 2 + 10);
			m_subMenu->addChild(monthBtn);
		}

		auto backBtn = CCMenuItemSpriteExtra::create(
			ButtonSprite::create("BACK"),
			this, menu_selector(MOTDSelectLayer::onBack)
		);
		backBtn->setPosition(winSize.width / 2, winSize.height / 2 - 50);
		m_subMenu->addChild(backBtn);

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

	void onClassic(CCObject*) {
		m_mainMenu->setVisible(false);
		m_titleLabel->setString("CLASSIC");
		m_subMenu->setVisible(true);
	}

	void onPlatformer(CCObject*) {
		selectType("platformer");
	}

	void onCurve(CCObject*) {
		selectType("curve");
	}

	void onDay(CCObject*) { selectType("daily"); }
	void onWeek(CCObject*) { selectType("weekly"); }
	void onMonth(CCObject*) { selectType("monthly"); }

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

	void selectType(const std::string& type) {
		auto cb = m_callback;
		removeFromParent();
		if (cb) cb(type);
	}

public:
	static MOTDSelectLayer* create(const std::vector<std::string>& types, const std::map<std::string, std::string>& messages, std::function<void(const std::string&)> callback) {
		auto ret = new MOTDSelectLayer();
		if (ret && ret->init(types, messages, callback)) {
			ret->autorelease();
			return ret;
		}
		CC_SAFE_DELETE(ret);
		return nullptr;
	}
};

class $modify(MyMenuLayer, MenuLayer) {
	struct Fields {
		TaskHolder<web::WebResponse> m_motdListener;
		TaskHolder<web::WebResponse> m_levelListener;
		TaskHolder<web::WebResponse> m_userListener;
		GJGameLevel* m_level = nullptr;
		int m_levelId = 0;
		std::map<std::string, std::string> m_messages;
		LoadingCircle* m_loadingCircle = nullptr;
		CCLabelBMFont* m_loadingLabel = nullptr;
	};

	bool init() {
		if (!MenuLayer::init()) return false;

		auto yuiSprite = CCSprite::create("yui-token.png"_spr);
		yuiSprite->setScale(0.8f);
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
		showLoading();

		m_fields->m_motdListener.spawn(
			web::WebRequest().get("https://api.hkgdl.dpdns.org/api/motd"),
			[this](web::WebResponse res) {
				hideLoading();

				if (!res.ok()) {
					FLAlertLayer::create("Error", "Failed to fetch MOTD data!", "OK")->show();
					return;
				}

				auto json = res.json().unwrapOr(matjson::Value());
				auto types = json["types"].asArray().unwrapOr(std::vector<matjson::Value>());
				auto messages = json["messages"];

				if (types.empty()) {
					FLAlertLayer::create("MOTD", "No maps available!", "OK")->show();
					return;
				}

				std::vector<std::string> typeStrings;
				std::map<std::string, std::string> messageMap;
				for (auto& t : types) {
					auto key = t.asString().unwrapOr("");
					if (key.empty()) continue;
					typeStrings.push_back(key);
					messageMap[key] = messages[key].asString().unwrapOr("");
				}

				m_fields->m_messages = messageMap;

				auto layer = MOTDSelectLayer::create(typeStrings, messageMap,
					[this](const std::string& type) {
						auto it = m_fields->m_messages.find(type);
						if (it != m_fields->m_messages.end()) {
							openMap(it->second);
						} else {
							FLAlertLayer::create("Error", "Map data not found!", "OK")->show();
						}
					}
				);
				CCDirector::sharedDirector()->getRunningScene()->addChild(layer, 1000);
			}
		);
	}

	void openMap(const std::string& raw) {
		auto levelId = parseLevelId(raw);
		if (levelId == 0) {
			FLAlertLayer::create("Error", "Invalid level ID!", "OK")->show();
			return;
		}

		auto creatorName = parseCreatorName(raw);
		showLoading();
		downloadLevel(levelId, creatorName);
	}

	void downloadLevel(int levelId, const std::string& creatorName) {
		auto body = fmt::format(
			"levelID={}&secret=Wmfd2893gb7", levelId
		);
		m_fields->m_levelListener.spawn(
			web::WebRequest()
				.bodyString(body)
				.header("Content-Type", "application/x-www-form-urlencoded")
				.post("https://www.boomlings.com/database/downloadGJLevel22.php"),
			[this, levelId, creatorName](web::WebResponse res) {
				hideLoading();

				if (!res.ok()) {
					FLAlertLayer::create("Error", "Failed to download level!", "OK")->show();
					return;
				}

				auto text = res.string().unwrapOr("");
				if (text.empty() || text.find("-1") == 0) {
					FLAlertLayer::create("Error", "Level not found!", "OK")->show();
					return;
				}

				auto dict = GameLevelManager::responseToDict(text, false);
				if (!dict) {
					FLAlertLayer::create("Error", "Failed to parse level!", "OK")->show();
					return;
				}

				auto level = GJGameLevel::create(dict, false);
				if (!level) {
					FLAlertLayer::create("Error", "Failed to create level!", "OK")->show();
					return;
				}

				level->m_dontSave = false;
				if (!creatorName.empty()) level->m_creatorName = creatorName;

				auto dlKey = GameLevelManager::sharedState()->getLevelDownloadKey(levelId, false, 0);
				GameLevelManager::sharedState()->m_downloadedLevels->setObject(level, dlKey);

				if (auto saved = GameLevelManager::sharedState()->getSavedLevel(levelId)) {
					level = saved;
					if (!creatorName.empty()) level->m_creatorName = creatorName;
				} else {
					GameLevelManager::sharedState()->saveLevel(level);
				}
				level->retain();

				m_fields->m_level = level;
				m_fields->m_levelId = levelId;

				auto userBody = fmt::format(
					"secret=Wmfd2893gb7&str={}", creatorName
				);
				m_fields->m_userListener.spawn(
					web::WebRequest()
						.bodyString(userBody)
						.header("Content-Type", "application/x-www-form-urlencoded")
						.post("https://www.boomlings.com/database/getGJUsers20.php"),
					[this](web::WebResponse res) {
						if (res.ok()) {
							auto userText = res.string().unwrapOr("");
							auto data = userText.substr(0, userText.find('#'));
							data = data.substr(0, data.find('|'));
							std::istringstream stream(data);
							std::string seg;
							int key = 0, idx = 0;
							while (std::getline(stream, seg, ':')) {
								if (idx % 2 == 0) {
									try { key = std::stoi(seg); } catch (...) { break; }
								} else if (key == 16) {
									try { m_fields->m_level->m_accountID = std::stoi(seg); } catch (...) {}
									break;
								}
								idx++;
							}
						}
						auto scene = LevelInfoLayer::scene(m_fields->m_level, false);
						CCDirector::sharedDirector()->pushScene(CCTransitionFade::create(0.5f, scene));
						m_fields->m_level->release();
					}
				);
			}
		);
	}
};
