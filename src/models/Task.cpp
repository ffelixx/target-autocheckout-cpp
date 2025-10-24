#include "../../include/models/Task.h"
#include "../../include/utils/Logger.h"
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>
#include <sstream>

using json = nlohmann::json;

namespace Models {

Task::Task(const std::string& pid, const std::string& proxyStr,
           int quantity, const Profile& prof, int delay,
           const std::string& cookie, std::shared_ptr<Network::WebhookClient> webhook)
    : profile(prof), webhookClient(webhook) {

    config.pid = pid;
    config.quantity = quantity;
    config.profile = prof.name;
    config.delay = delay;
    config.cookie = cookie;

    proxy = Utils::ProxyParser::parse(proxyStr);
    httpClient = std::make_unique<Network::HttpClient>(proxy, cookie);

    addressId = "";
    paymentInstructionId = "";
}

void Task::start() {
    getCart();
}

void Task::sleep(int milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

std::map<std::string, std::string> Task::getCommonHeaders() const {
    return {
        {"authority", "carts.target.com"},
        {"accept", "application/json"},
        {"pragma", "no-cache"},
        {"x-application-name", "web"},
        {"content-type", "application/json"},
        {"origin", "https://www.target.com"}
    };
}

void Task::getCart() {
    json body = {
        {"cart_type", "REGULAR"},
        {"channel_id", 10},
        {"shopping_context", "DIGITAL"}
    };

    auto headers = getCommonHeaders();
    headers["referer"] = "https://www.target.com/co-cart";

    auto response = httpClient->put(
        "https://carts.target.com/web_checkouts/v1/cart?field_groups=CART%2CCART_ITEMS%2CSUMMARY%2CPROMOTION_CODES%2CADDRESSES%2CFINANCE_PROVIDERS&key=feaf228eb2777fd3eee0fd5192ae7107d6224b39",
        headers,
        body.dump()
    );

    if (response.statusCode == 200 || response.statusCode == 201) {
        try {
            json responseData = json::parse(response.body);
            if (responseData.contains("cart_items") && responseData["cart_items"].is_array()) {
                for (const auto& item : responseData["cart_items"]) {
                    if (item.contains("cart_item_id")) {
                        clearItem(item["cart_item_id"].get<std::string>());
                    }
                }
                sleep(500);
            }
        } catch (...) {}

        sleep(100);
        monitor();
    } else if (response.statusCode == 401) {
        Utils::Logger::error("Invalid Cookie");
    } else {
        Utils::Logger::error("Error getting cart");
    }
}

void Task::clearItem(const std::string& itemId) {
    auto headers = getCommonHeaders();
    headers["referer"] = "https://www.target.com/co-cart";

    std::string url = "https://carts.target.com/web_checkouts/v1/cart_items/" + itemId +
                      "?cart_type=REGULAR&field_groups=CART%2CCART_ITEMS%2CSUMMARY%2CPROMOTION_CODES%2CADDRESSES%2CFINANCE_PROVIDERS%2CFINANCE_PROVIDERS&key=feaf228eb2777fd3eee0fd5192ae7107d6224b39";

    auto response = httpClient->del(url, headers);

    if (response.statusCode != 200 && response.statusCode != 204) {
        Utils::Logger::error("Error clearing cart");
    }
}

void Task::monitor() {
    auto headers = getCommonHeaders();
    headers["referer"] = "https://www.target.com/p/-/-/A-" + config.pid;
    headers["authority"] = "redsky.target.com";

    std::string url = "https://redsky.target.com/redsky_aggregations/v1/web/pdp_fulfillment_v1?key=ff457966e64d5e877fdbad070f276d18ecec4a01&tcin=" +
                      config.pid +
                      "&store_id=3200&store_positions_store_id=3200&has_store_positions_store_id=true&zip=55414&state=NY&latitude=44.9778&longitude=93.2650&scheduled_delivery_store_id=3200&pricing_store_id=3200&has_pricing_store_id=true";

    auto response = httpClient->get(url, headers);

    if (response.statusCode == 200) {
        try {
            json responseData = json::parse(response.body);
            std::string status = responseData["data"]["product"]["fulfillment"]["shipping_options"]["availability_status"];

            if (status == "IN_STOCK") {
                sleep(250);
                addToCart();
            } else {
                Utils::Logger::warning("Waiting For Restock");
                sleep(config.delay);
                monitor();
            }
        } catch (...) {
            Utils::Logger::warning("Waiting For Product");
            sleep(config.delay);
            monitor();
        }
    } else {
        Utils::Logger::warning("Waiting For Product");
        sleep(config.delay);
        monitor();
    }
}

void Task::addToCart() {
    Utils::Logger::warning("Adding To Cart");

    json body = {
        {"cart_type", "REGULAR"},
        {"channel_id", "90"},
        {"shopping_context", "DIGITAL"},
        {"cart_item", {
            {"tcin", config.pid},
            {"quantity", config.quantity},
            {"item_channel_id", "10"}
        }}
    };

    auto headers = getCommonHeaders();
    headers["referer"] = "https://www.target.com/p/-/-/A-" + config.pid;

    auto response = httpClient->post(
        "https://carts.target.com/web_checkouts/v1/cart_items?field_groups=CART%2CCART_ITEMS%2CSUMMARY%2CFINANCE_PROVIDERS&key=feaf228eb2777fd3eee0fd5192ae7107d6224b39",
        headers,
        body.dump()
    );

    if (response.statusCode == 200 || response.statusCode == 201) {
        try {
            json responseData = json::parse(response.body);
            std::string status = responseData["inventory_info"]["availability_status"];

            if (status == "IN_STOCK") {
                Utils::Logger::success("Added To Cart");
                sleep(500);
                loadCheckout();
            }
        } catch (...) {
            Utils::Logger::error("Error Adding To Cart");
            sleep(config.delay);
            addToCart();
        }
    } else if (response.statusCode == 401) {
        Utils::Logger::error("Expired Cookie");
    } else {
        try {
            json errorData = json::parse(response.body);
            if (errorData.contains("alerts") && errorData["alerts"].is_array() && !errorData["alerts"].empty()) {
                std::string message = errorData["alerts"][0]["message"];
                if (message.find("Maximum purchase limit") != std::string::npos) {
                    Utils::Logger::error("Error Adding To Cart - Max Quantity Exceeded");
                    return;
                }
            }
        } catch (...) {}

        Utils::Logger::error("Error Adding To Cart");
        sleep(config.delay);
        addToCart();
    }
}

void Task::loadCheckout() {
    Utils::Logger::warning("Loading Checkout");

    json body = {
        {"cart_type", "REGULAR"}
    };

    auto headers = getCommonHeaders();
    headers["referer"] = "https://www.target.com/co-review?precheckout=true";

    auto response = httpClient->post(
        "https://carts.target.com/web_checkouts/v1/pre_checkout?field_groups=ADDRESSES%2CCART%2CCART_ITEMS%2CDELIVERY_WINDOWS%2CPAYMENT_INSTRUCTIONS%2CPICKUP_INSTRUCTIONS%2CPROMOTION_CODES%2CSUMMARY%2CFINANCE_PROVIDERS&key=feaf228eb2777fd3eee0fd5192ae7107d6224b39",
        headers,
        body.dump()
    );

    if (response.statusCode == 200 || response.statusCode == 201) {
        try {
            json responseData = json::parse(response.body);

            if (responseData.contains("cart_items") && responseData["cart_items"].is_array() && !responseData["cart_items"].empty()) {
                title = responseData["cart_items"][0]["item_attributes"]["description"];
                image = responseData["cart_items"][0]["item_attributes"]["image_path"];

                if (responseData["cart_items"][0]["fulfillment"].contains("address_id")) {
                    addressId = responseData["cart_items"][0]["fulfillment"]["address_id"];
                }
            }

            if (responseData.contains("cart_id")) {
                cartId = responseData["cart_id"];
            }

            if (responseData.contains("payment_instructions") && responseData["payment_instructions"].is_array() && !responseData["payment_instructions"].empty()) {
                paymentInstructionId = responseData["payment_instructions"][0]["payment_instruction_id"];
            }

            Utils::Logger::success("Loaded Checkout");
            sleep(500);
            submitShipping();
        } catch (...) {
            Utils::Logger::error("Error Loading Checkout");
        }
    } else {
        Utils::Logger::error("Error Loading Checkout");
    }
}

void Task::submitShipping() {
    Utils::Logger::warning("Submitting Shipping");

    std::string firstName, lastName;
    std::istringstream nameStream(profile.shippingAddress.name);
    nameStream >> firstName >> lastName;

    json body = {
        {"cart_type", "REGULAR"},
        {"address", {
            {"address_line1", profile.shippingAddress.line1},
            {"address_line2", profile.shippingAddress.line2},
            {"address_type", "SHIPPING"},
            {"city", profile.shippingAddress.city},
            {"country", "US"},
            {"first_name", firstName},
            {"last_name", lastName},
            {"mobile", profile.shippingAddress.phone},
            {"save_as_default", false},
            {"state", profile.shippingAddress.state},
            {"zip_code", profile.shippingAddress.postCode}
        }},
        {"selected", true},
        {"save_to_profile", false},
        {"skip_verification", true}
    };

    auto headers = getCommonHeaders();
    headers["referer"] = "https://www.target.com/co-delivery";

    std::string url;
    Network::HttpResponse response;

    if (!addressId.empty()) {
        url = "https://carts.target.com/web_checkouts/v1/cart_shipping_addresses/" + addressId +
              "?field_groups=ADDRESSES%2CCART%2CCART_ITEMS%2CPICKUP_INSTRUCTIONS%2CPROMOTION_CODES%2CSUMMARY%2CFINANCE_PROVIDERS%2CFINANCE_PROVIDERS&key=feaf228eb2777fd3eee0fd5192ae7107d6224b39";
        response = httpClient->put(url, headers, body.dump());
    } else {
        url = "https://carts.target.com/web_checkouts/v1/cart_shipping_addresses?field_groups=ADDRESSES%2CCART%2CCART_ITEMS%2CPICKUP_INSTRUCTIONS%2CPROMOTION_CODES%2CSUMMARY%2CFINANCE_PROVIDERS%2CFINANCE_PROVIDERS&key=feaf228eb2777fd3eee0fd5192ae7107d6224b39";
        response = httpClient->post(url, headers, body.dump());
    }

    if (response.statusCode == 200 || response.statusCode == 201) {
        Utils::Logger::success("Submitted Shipping");
        sleep(250);
        submitPayment();
    } else {
        Utils::Logger::error("Error Submitting Shipping");
    }
}

void Task::submitPayment() {
    Utils::Logger::warning("Submitting Payment");

    std::string firstName, lastName;
    std::istringstream nameStream(profile.shippingAddress.name);
    nameStream >> firstName >> lastName;

    json body = {
        {"cart_id", cartId},
        {"wallet_mode", "ADD"},
        {"payment_type", "CARD"},
        {"card_details", {
            {"card_name", profile.paymentDetails.nameOnCard},
            {"card_number", profile.paymentDetails.cardNumber},
            {"cvv", profile.paymentDetails.cardCvv},
            {"expiry_month", profile.paymentDetails.cardExpMonth},
            {"expiry_year", profile.paymentDetails.cardExpYear}
        }},
        {"billing_address", {
            {"address_line1", profile.shippingAddress.line1},
            {"address_line2", profile.shippingAddress.line2},
            {"city", profile.shippingAddress.city},
            {"first_name", firstName},
            {"last_name", lastName},
            {"phone", profile.shippingAddress.phone},
            {"state", profile.shippingAddress.state},
            {"zip_code", profile.shippingAddress.postCode},
            {"country", "US"}
        }},
        {"skip_verification", true}
    };

    auto headers = getCommonHeaders();
    headers["referer"] = "https://www.target.com/co-payment";

    std::string url;
    Network::HttpResponse response;

    if (!paymentInstructionId.empty()) {
        url = "https://carts.target.com/checkout_payments/v1/payment_instructions/" + paymentInstructionId +
              "?key=feaf228eb2777fd3eee0fd5192ae7107d6224b39";
        response = httpClient->put(url, headers, body.dump());
    } else {
        url = "https://carts.target.com/checkout_payments/v1/payment_instructions?key=feaf228eb2777fd3eee0fd5192ae7107d6224b39";
        response = httpClient->post(url, headers, body.dump());
    }

    if (response.statusCode == 200 || response.statusCode == 201) {
        try {
            json responseData = json::parse(response.body);
            price = std::to_string(responseData["payment_instruction_amount"].get<double>());

            Utils::Logger::success("Submitted Payment");
            sleep(250);
            submitOrder();
        } catch (...) {
            Utils::Logger::error("Error Submitting Payment");
        }
    } else {
        Utils::Logger::error("Error Submitting Payment");
    }
}

void Task::submitOrder() {
    Utils::Logger::warning("Submitting Order");

    json body = {
        {"cart_type", "REGULAR"},
        {"channel_id", 10}
    };

    auto headers = getCommonHeaders();
    headers["referer"] = "https://www.target.com/co-review";

    auto response = httpClient->post(
        "https://carts.target.com/web_checkouts/v1/checkout?field_groups=ADDRESSES%2CCART%2CCART_ITEMS%2CDELIVERY_WINDOWS%2CPAYMENT_INSTRUCTIONS%2CPICKUP_INSTRUCTIONS%2CPROMOTION_CODES%2CSUMMARY%2CFINANCE_PROVIDERS%2CFINANCE_PROVIDERS&key=feaf228eb2777fd3eee0fd5192ae7107d6224b39",
        headers,
        body.dump()
    );

    if (response.statusCode == 200 || response.statusCode == 201) {
        try {
            json responseData = json::parse(response.body);

            if (responseData.contains("orders") && responseData["orders"].is_array() && !responseData["orders"].empty()) {
                std::string cartState = responseData["orders"][0]["cart_state"];

                if (cartState == "COMPLETED") {
                    Utils::Logger::success("Checkout Successful");

                    std::string orderNumber = responseData["orders"][0]["parent_cart_number"];
                    std::string proxyStr = proxy.host + ":" + proxy.port;

                    webhookClient->sendSuccess(title, image, config.pid, price,
                                             config.quantity, profile.name, proxyStr, orderNumber);
                }
            }
        } catch (...) {
            Utils::Logger::error("Error Submitting Order");
        }
    } else {
        try {
            json errorData = json::parse(response.body);
            if (errorData.contains("alerts") && errorData["alerts"].is_array() && !errorData["alerts"].empty()) {
                std::string message = errorData["alerts"][0]["message"];

                if (message == "Payment declined") {
                    Utils::Logger::error("Payment Declined");

                    std::string proxyStr = proxy.host + ":" + proxy.port;
                    webhookClient->sendDecline(title, image, config.pid, price,
                                             config.quantity, profile.name, proxyStr);
                    return;
                }
            }

            Utils::Logger::error("Error Submitting Order");
        } catch (...) {
            Utils::Logger::error("Error Submitting Order");
        }
    }
}

} // namespace Models
