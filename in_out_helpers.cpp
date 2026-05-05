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
    cout << endl;
    for (const auto& message : data["messages"]) {
        string unreadMessageId = message.at("id").get<string>();
        ids.push_back(unreadMessageId);
        cout << "unread message Id: " << unreadMessageId << endl;
    }
    return ids;
}

vector<json> getEmailInfo(const string& accessToken, const vector<string>& ids) {
    vector<json> emails_info;
    if (ids.size() == 0) {return emails_info;}
    for (size_t i=0; i<ids.size(); i++) {
        string url = "https://gmail.googleapis.com/gmail/v1/users/me/messages/" + ids[i];
        string body = httpGet(url, accessToken);
        json data = json::parse(body);
        //
        json email_info;
        // get easy values, id, thread id, and snippet, on the top of the data object
        string id = data.value("id", "");
        string threadId = data.value("threadId","");
        string snippet = data.value("snippet","");
        email_info["id"] = id;
        email_info["threadId"] = threadId;
        email_info["snippet"] = snippet;
        // loop through headers to find the other needed values
        const json& headers = data["payload"]["headers"];
        for (const auto& header : headers) {
            string name = header.value("name", "");
            string value = header.value("value", "");
            if (name == "From") {
                email_info["From"] = value;
            }   
            else if (name == "Reply-To") {
                email_info["Reply-To"] = value;
            }
            else if (name == "To") {
                email_info["To"] = value;
            }
            else if (name == "Subject") {
                email_info["Subject"] = value;
            }
            else if (name == "Date") {
                email_info["Date"] = value;
            }
        }
        // push onto vector to return
        emails_info.push_back(email_info);
    }
    return emails_info;
}


void writeJsonFile(const string& path, const json& data) {
    ofstream file(path);
    if (!file.is_open()) {
        throw runtime_error("Could not open file: " + path);
    }
    file << data.dump(2) << '\n';
    if (!file) {
        throw runtime_error("Could not write file: " + path);
    }
}

