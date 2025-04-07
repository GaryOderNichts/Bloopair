#include <fstream>
#include <string>
#include <map>
#include <json/json.h>

class LocalizationManager {
public:
    static LocalizationManager& Instance() {
        static LocalizationManager instance;
        return instance;
    }

    bool LoadLanguage(const std::string& languageCode) {
        std::ifstream file("translations/" + languageCode + ".json");
        if (!file.is_open()) {
            return false;
        }

        Json::Value root;
        file >> root;
        file.close();

        mTranslations.clear();
        for (const auto& key : root.getMemberNames()) {
            mTranslations[key] = root[key].asString();
        }
        return true;
    }

    std::string Translate(const std::string& key) const {
        auto it = mTranslations.find(key);
        if (it != mTranslations.end()) {
            return it->second;
        }
        return key;
    }

private:
    LocalizationManager() = default;
    std::map<std::string, std::string> mTranslations;
};
