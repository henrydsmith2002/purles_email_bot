#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

using std::string;
using std::vector;

using json = nlohmann::json;

vector<string> listUnreadMessageIds(const string& accessToken);

vector<json> getEmailInfo(const string& accessToken, const vector<string>& ids);
