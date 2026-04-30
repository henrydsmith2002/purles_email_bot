#pragma once

#include <exception>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <utility>
#include <vector>

using json = nlohmann::json;
using std::cerr;
using std::cout;
using std::endl;
using std::exception;
using std::string;
using std::vector;

string httpGet(const string& url, const string& accessToken);
string urlEncode(const string& value);
json readJsonFile(const string& path);
string waitForAuthorizationCode();
string buildAuthorizationUrl(const string& clientId);
string httpPostForm(const string& url, const vector<std::pair<string, string>>& fields);
json exchangeCodeForTokens(
    const string& code,
    const string& clientId,
    const string& clientSecret
);
