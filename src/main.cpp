#include <iostream>
#include <vector>
#include <thread>
#include <algorithm>
#include <random>
#include "../include/utils/FileReader.h"
#include "../include/utils/ConfigLoader.h"
#include "../include/utils/Logger.h"
#include "../include/models/Task.h"
#include "../include/network/WebhookClient.h"

struct TaskData {
    std::string pid;
    std::string proxy;
    int quantity;
    std::string profile;
    int delay;
    std::string cookie;
};

std::vector<TaskData> parseTasks(const std::vector<std::string>& taskLines) {
    std::vector<TaskData> tasks;

    for (const auto& line : taskLines) {
        std::istringstream ss(line);
        std::string token;
        std::vector<std::string> tokens;

        while (std::getline(ss, token, '\t')) {
            tokens.push_back(token);
        }

        if (tokens.size() >= 6) {
            TaskData task;
            task.pid = tokens[1];
            task.proxy = tokens[0];
            task.quantity = std::stoi(tokens[2]);
            task.profile = tokens[3];
            task.delay = std::stoi(tokens[4]);
            task.cookie = tokens[5];
            tasks.push_back(task);
        }
    }

    return tasks;
}

Models::Profile* findProfile(std::vector<Models::Profile>& profiles, const std::string& name) {
    for (auto& profile : profiles) {
        if (profile.name == name) {
            return &profile;
        }
    }
    return nullptr;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "       Target Bot - C++ Edition        " << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // Load configuration
    Utils::Logger::info("Loading configuration...");
    std::string webhook = Utils::ConfigLoader::loadWebhook("../config.json");
    std::vector<Models::Profile> profiles = Utils::ConfigLoader::loadProfiles("../profile.json");

    if (webhook.empty()) {
        Utils::Logger::error("Failed to load webhook from config.json");
        return 1;
    }

    if (profiles.empty()) {
        Utils::Logger::error("Failed to load profiles from profile.json");
        return 1;
    }

    Utils::Logger::success("Loaded " + std::to_string(profiles.size()) + " profiles");

    // Load proxies
    Utils::Logger::info("Loading proxies...");
    std::vector<std::string> proxies = Utils::FileReader::readLines("./txt/proxies.txt");

    if (proxies.empty()) {
        Utils::Logger::error("No proxies found in txt/proxies.txt");
        return 1;
    }

    Utils::Logger::success("Loaded " + std::to_string(proxies.size()) + " proxies");

    // Shuffle proxies for random assignment
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(proxies.begin(), proxies.end(), g);

    // Load tasks
    Utils::Logger::info("Loading tasks...");
    std::vector<std::string> taskLines = Utils::FileReader::readLines("./txt/tasks.txt");

    if (taskLines.empty()) {
        Utils::Logger::error("No tasks found in txt/tasks.txt");
        return 1;
    }

    std::vector<TaskData> tasks = parseTasks(taskLines);
    Utils::Logger::success("Loaded " + std::to_string(tasks.size()) + " tasks");

    std::cout << std::endl;
    Utils::Logger::info("Starting tasks...");
    std::cout << std::endl;

    // Create webhook client
    auto webhookClient = std::make_shared<Network::WebhookClient>(webhook);

    // Create and start tasks
    std::vector<std::thread> threads;
    int proxyIndex = 0;

    for (const auto& taskData : tasks) {
        // Get proxy for this task
        std::string proxy;
        if (proxyIndex < proxies.size()) {
            proxy = proxies[proxyIndex++];
        } else {
            proxy = proxies[proxyIndex % proxies.size()];
        }

        // Find profile
        Models::Profile* profile = findProfile(profiles, taskData.profile);
        if (!profile) {
            Utils::Logger::error("Profile not found: " + taskData.profile);
            continue;
        }

        // Create task in new thread
        threads.emplace_back([taskData, proxy, profile, webhookClient]() {
            Models::Task task(
                taskData.pid,
                proxy,
                taskData.quantity,
                *profile,
                taskData.delay,
                taskData.cookie,
                webhookClient
            );
            task.start();
        });

        // Small delay between task starts
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Wait for all tasks to complete
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    Utils::Logger::info("All tasks completed");
    return 0;
}
