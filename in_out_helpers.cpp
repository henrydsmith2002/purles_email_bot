#include "in_out_helpers.h"

#include <nlohmann/json.hpp>

#include "gmail_browser_interaction_fns.h"

using json = nlohmann::json;

vector<string> listUnreadMessageIds(const string& accessToken) {
    string url =
        "https://gmail.googleapis.com/gmail/v1/users/me/messages"
        "?q=is:unread&maxResults=10";

    string body = httpGet(url, accessToken);
    json data = json::parse(body);

    vector<string> ids;

    if (!data.contains("messages")) {
        return ids;
    }

    for (const auto& message : data["messages"]) {
        ids.push_back(message.at("id").get<string>());
    }

    return ids;
}
