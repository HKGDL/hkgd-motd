#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/binding/LevelInfoLayer.hpp>
#include <Geode/binding/LoadingCircle.hpp>

using namespace geode::prelude;

class $modify(MyMenuLayer, MenuLayer) {
	struct Fields {
		TaskHolder<web::WebResponse> m_motdListener;
		TaskHolder<web::WebResponse> m_levelListener;
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
				auto message = json["message"].asString().unwrapOr("");

				if (message.empty()) {
					hideLoading();
					FLAlertLayer::create("MOTD", "No MOTD set!", "OK")->show();
					return;
				}

				int levelId = 0;
				try {
					levelId = std::stoi(message);
				} catch (...) {
					hideLoading();
					FLAlertLayer::create("MOTD", message, "OK")->show();
					return;
				}

				auto body = fmt::format(
					"levelID={}&secret=Wmfd2893gb7", levelId
				);
				m_fields->m_levelListener.spawn(
					web::WebRequest()
						.bodyString(body)
						.header("Content-Type", "application/x-www-form-urlencoded")
						.post("https://www.boomlings.com/database/downloadGJLevel22.php"),
					[this](web::WebResponse res) {
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

						auto scene = LevelInfoLayer::scene(level, false);
						CCDirector::sharedDirector()->pushScene(CCTransitionFade::create(0.5f, scene));
					}
				);
			}
		);
	}
};
