#pragma once
#include <iostream>
using std::endl, std::cout, std::cerr;
#include <fstream>
using std::ifstream;
#include <iomanip>
using std::setw;
#include <curl/curl.h>
#include <stdexcept>
using std::runtime_error, std::exception;
#include <string>
using std::string, std::to_string, std::ostringstream;
#include <vector>
using std::vector;
#include <nlohmann/json.hpp>
using json = nlohmann::json;
#include "include/httplib.h"
#include "gmail_browser_interaction_fns.h"


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