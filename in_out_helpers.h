#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <nlohmann/json.hpp>

using std::string;
using std::vector;
using std::ofstream;

using json = nlohmann::json;

vector<string> listUnreadMessageIds(const string& accessToken);

vector<json> getEmailInfo(const string& accessToken, const vector<string>& ids);

void writeJsonFile(const string& path, const json& data);
