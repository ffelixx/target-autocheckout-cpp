#ifndef WEBHOOKCLIENT_H
#define WEBHOOKCLIENT_H

#include <string>
#include <map>

namespace Network {

class WebhookClient {
private:
    std::string webhookUrl;

public:
    explicit WebhookClient(const std::string& url);

    void sendSuccess(const std::string& title, const std::string& image,
                    const std::string& pid, const std::string& price,
                    int quantity, const std::string& profile,
                    const std::string& proxy, const std::string& orderNumber);

    void sendDecline(const std::string& title, const std::string& image,
                    const std::string& pid, const std::string& price,
                    int quantity, const std::string& profile,
                    const std::string& proxy);

private:
    std::string buildEmbed(const std::string& title, const std::string& color,
                          const std::map<std::string, std::string>& fields,
                          const std::string& thumbnail, const std::string& url);
    void send(const std::string& payload);
};

} // namespace Network

#endif // WEBHOOKCLIENT_H
