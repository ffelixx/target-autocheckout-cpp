#include "../../include/utils/ConfigLoader.h"
#include "../../include/utils/FileReader.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace Utils {

std::string ConfigLoader::loadWebhook(const std::string& configPath) {
    std::string content = FileReader::readFile(configPath);
    if (content.empty()) {
        return "";
    }

    try {
        json config = json::parse(content);
        return config.value("webhook", "");
    } catch (...) {
        return "";
    }
}

std::vector<Models::Profile> ConfigLoader::loadProfiles(const std::string& profilePath) {
    std::vector<Models::Profile> profiles;
    std::string content = FileReader::readFile(profilePath);

    if (content.empty()) {
        return profiles;
    }

    try {
        json profilesJson = json::parse(content);

        for (const auto& profileJson : profilesJson) {
            Models::Profile profile;
            profile.name = profileJson.value("name", "");

            if (profileJson.contains("shippingAddress")) {
                auto addr = profileJson["shippingAddress"];
                profile.shippingAddress.name = addr.value("name", "");
                profile.shippingAddress.line1 = addr.value("line1", "");
                profile.shippingAddress.line2 = addr.value("line2", "");
                profile.shippingAddress.city = addr.value("city", "");
                profile.shippingAddress.state = addr.value("state", "");
                profile.shippingAddress.postCode = addr.value("postCode", "");
                profile.shippingAddress.phone = addr.value("phone", "");
            }

            if (profileJson.contains("paymentDetails")) {
                auto payment = profileJson["paymentDetails"];
                profile.paymentDetails.nameOnCard = payment.value("nameOnCard", "");
                profile.paymentDetails.cardNumber = payment.value("cardNumber", "");
                profile.paymentDetails.cardCvv = payment.value("cardCvv", "");
                profile.paymentDetails.cardExpMonth = payment.value("cardExpMonth", "");
                profile.paymentDetails.cardExpYear = payment.value("cardExpYear", "");
            }

            profiles.push_back(profile);
        }
    } catch (...) {
        return profiles;
    }

    return profiles;
}

} // namespace Utils
