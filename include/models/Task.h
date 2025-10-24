#ifndef TASK_H
#define TASK_H

#include <string>
#include <memory>
#include "TaskConfig.h"
#include "Profile.h"
#include "../utils/ProxyParser.h"
#include "../network/HttpClient.h"
#include "../network/WebhookClient.h"

namespace Models {

class Task {
private:
    TaskConfig config;
    Profile profile;
    Utils::Proxy proxy;
    std::unique_ptr<Network::HttpClient> httpClient;
    std::shared_ptr<Network::WebhookClient> webhookClient;

    // Product info
    std::string title;
    std::string image;
    std::string cartId;
    std::string addressId;
    std::string paymentInstructionId;
    std::string price;

public:
    Task(const std::string& pid, const std::string& proxyStr,
         int quantity, const Profile& prof, int delay,
         const std::string& cookie, std::shared_ptr<Network::WebhookClient> webhook);

    void start();

private:
    void getCart();
    void clearItem(const std::string& itemId);
    void monitor();
    void addToCart();
    void loadCheckout();
    void submitShipping();
    void submitPayment();
    void submitOrder();

    std::map<std::string, std::string> getCommonHeaders() const;
    void sleep(int milliseconds);
};

} // namespace Models

#endif // TASK_H
