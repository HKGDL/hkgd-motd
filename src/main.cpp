#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/binding/LevelInfoLayer.hpp>
#include <Geode/binding/LoadingCircle.hpp>
#include <sstream>

using namespace geode::prelude;

class $modify(MyMenuLayer, MenuLayer) {
	struct Fields {
		TaskHolder<web::WebResponse> m_motdListener;
		TaskHolder<web::WebResponse> m_levelListener;
		TaskHolder<web::WebResponse> m_userListener;
		GJGameLevel* m_level = nullptr;
		int m_levelId = 0;
		LoadingCircle* m_loadingCircle = nullptr;
		CCLabelBMFont* m_loadingLabel = nullptr;
	};

	bool init() {
		if (!MenuLayer::init()) {
			return false;
		}

		auto myButton = CCMenuItemSpriteExtra::create(
			CCSprite::createWithSpriteFrameName("GJ_likeBtn_001.png"),
			this,
			menu_selector(MyMenuLayer::onMyButton)
		);

		auto menu = this->getChildByID("bottom-menu");
		menu->addChild(myButton);
		myButton->setID("my-button"_spr);
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
				"Loading Today MOTD...", "goldFont.fnt"
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
				if (!res.ok()) {
					hideLoading();
					FLAlertLayer::create("Error", "Failed to fetch MOTD!", "OK")->show();
					return;
				}

				auto json = res.json().unwrapOr(matjson::Value());
				auto raw = json["message"].asString().unwrapOr("");

				if (raw.empty()) {
					hideLoading();
					FLAlertLayer::create("MOTD", "No MOTD set!", "OK")->show();
					return;
				}

				int levelId = 0;
				std::string creatorName;
				// Try Discord format first: "...\nID: 12345"  and "... by CreatorName ..."
				auto idPos = raw.find("ID:");
				if (idPos != std::string::npos) {
					try { levelId = std::stoi(raw.substr(idPos + 3)); } catch (...) {}
					// Parse "by CreatorName" from the message
					auto byPos = raw.find("by ");
					if (byPos != std::string::npos) {
						auto start = byPos + 3;
						auto end = raw.find('\n', start);
						if (end == std::string::npos) end = raw.find('\r', start);
						if (end == std::string::npos) end = raw.length();
						creatorName = raw.substr(start, end - start);
						// Trim trailing spaces
						while (!creatorName.empty() && creatorName.back() == ' ') creatorName.pop_back();
					}
				}
				// Fallback: raw is just a numeric level ID
				if (levelId == 0) {
					try { levelId = std::stoi(raw); } catch (...) {
						hideLoading();
						FLAlertLayer::create("MOTD", raw, "OK")->show();
						return;
					}
				}

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
						level->m_creatorName = creatorName;

						// Store in downloaded levels
						auto dlKey = GameLevelManager::sharedState()->getLevelDownloadKey(levelId, false, 0);
						GameLevelManager::sharedState()->m_downloadedLevels->setObject(level, dlKey);

						// Use saved level if it exists (preserves progress), otherwise save for future tracking
						if (auto saved = GameLevelManager::sharedState()->getSavedLevel(levelId)) {
							level = saved;
							level->m_creatorName = creatorName;
						} else {
							GameLevelManager::sharedState()->saveLevel(level);
						}
						level->retain(); // keep alive during user lookup

						m_fields->m_level = level;
						m_fields->m_levelId = levelId;

						// Look up accountID so creator shows yellow (registered), not green
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
									// Parse accountID (key 16) from "1:name:2:userID:...:16:accountID:..."
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
		);
	}
};
