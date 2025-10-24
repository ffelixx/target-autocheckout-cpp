#include "../../include/network/WebhookClient.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <sstream>
#include <chrono>
#include <iomanip>

using json = nlohmann::json;

namespace Network {

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    return size * nmemb;
}

WebhookClient::WebhookClient(const std::string& url) : webhookUrl(url) {}

std::string WebhookClient::buildEmbed(const std::string& title, const std::string& color,
                                      const std::map<std::string, std::string>& fields,
                                      const std::string& thumbnail, const std::string& url) {
    json embed;
    embed["title"] = title;
    embed["url"] = url;
    embed["color"] = std::stoi(color.substr(1), nullptr, 16);

    json fieldsArray = json::array();
    for (const auto& [name, value] : fields) {
        json field;
        field["name"] = name;
        field["value"] = value;
        field["inline"] = true;
        fieldsArray.push_back(field);
    }
    embed["fields"] = fieldsArray;

    if (!thumbnail.empty()) {
        embed["thumbnail"]["url"] = thumbnail;
    }

    embed["footer"]["text"] = "bot aio";

    // Get current timestamp
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss;
    oss << std::put_time(std::gmtime(&now_time_t), "%Y-%m-%dT%H:%M:%S");
    embed["timestamp"] = oss.str();

    json payload;
    payload["embeds"] = json::array({embed});

    return payload.dump();
}

void WebhookClient::send(const std::string& payload) {
    CURL* curl = curl_easy_init();
    if (!curl) return;

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, webhookUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    curl_easy_perform(curl);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
}

void WebhookClient::sendSuccess(const std::string& title, const std::string& image,
                                const std::string& pid, const std::string& price,
                                int quantity, const std::string& profile,
                                const std::string& proxy, const std::string& orderNumber) {
    std::map<std::string, std::string> fields;
    fields["Product"] = title;
    fields["PID"] = pid;
    fields["Store"] = "Target";
    fields["Price"] = "$" + price;
    fields["Speed"] = "1";
    fields["Quantity"] = std::to_string(quantity);
    fields["Profile"] = "||" + profile + "||";
    fields["Proxy"] = "||" + proxy + "||";
    fields["Order #"] = "||" + orderNumber + "||";

    std::string url = "https://www.target.com/p/-/-/A-" + pid;
    std::string payload = buildEmbed("Checkout Successful", "#14dc54", fields, image, url);
    send(payload);
}

void WebhookClient::sendDecline(const std::string& title, const std::string& image,
                                const std::string& pid, const std::string& price,
                                int quantity, const std::string& profile,
                                const std::string& proxy) {
    std::map<std::string, std::string> fields;
    fields["Product"] = title;
    fields["PID"] = pid;
    fields["Store"] = "Target";
    fields["Price"] = "$" + price;
    fields["Speed"] = "1";
    fields["Quantity"] = std::to_string(quantity);
    fields["Profile"] = "||" + profile + "||";
    fields["Proxy"] = "||" + proxy + "||";

    std::string url = "https://www.target.com/p/-/-/A-" + pid;
    std::string payload = buildEmbed("Payment Declined", "#cb260d", fields, image, url);
    send(payload);
}

} // namespace Network
