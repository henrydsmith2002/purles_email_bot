#include <iostream>
using std::endl, std::cout, std::cerr;
#include <fstream>
using std::ifstream;
#include <curl/curl.h>
#include <stdexcept>
using std::runtime_error, std::exception;
#include <string>
using std::string, std::to_string, std::ostringstream;
#include <vector>
using std::vector;
#include <nlohmann/json.hpp>
using json = nlohmann::json;


static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    string* response = static_cast<string*>(userp);
    response->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

string httpGet (const string& url, const string& accessToken) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        throw runtime_error("Failed to initialize CURL");
    }
    
    string response;
    struct curl_slist* headers = nullptr;

    string authHeader = "Authorization: Bearer " + accessToken;
    headers = curl_slist_append(headers, authHeader.c_str());

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode result = curl_easy_perform(curl);

    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (result != CURLE_OK) {
        throw runtime_error(string("CURL error: ") + curl_easy_strerror(result));
    }

    if (httpCode >= 400) {
        throw runtime_error("HTTP" + to_string(httpCode) + ": " + response);
    }

    return response;
}

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


string urlEncode(const string& value) {
    ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex << std::uppercase;

    for (unsigned char c : value) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else {
            escaped << '%' << std::setw(2) << static_cast<int>(c);
        }
    }

    return escaped.str();
}


json readJsonFile(const string& path) {
    ifstream file(path);

    if (!file.is_open()) {
        throw runtime_error("Could not open file: " + path);
    }

    json data;
    file >> data;
    return data;
}


string buildAuthorizationUrl(const string& clientId) {
    const string authEndpoint = "https://accounts.google.com/o/oauth2/v2/auth";
    const string redirectUri = "http://127.0.0.1:8080/";
    const string scope = "https://www.googleapis.com/auth/gmail.modify";

    ostringstream url;

    url << authEndpoint
        << "?client_id=" << urlEncode(clientId)
        << "&redirect_uri=" << urlEncode(redirectUri)
        << "&response_type=code"
        << "&scope=" << urlEncode(scope)
        << "&access_type=offline"
        << "&prompt=consent"
        << "&login_hint=" << urlEncode("pearlemailbot1433@gmail.com");

    return url.str();
}



int main() {
    try {
        json credentials = readJsonFile("credentials/client_secret_953867945298-3rtu9nrdplc6906j20c3tf1abhmt5bl5.apps.googleusercontent.com.json");

        string clientId =
            credentials.at("installed").at("client_id").get<string>();

        string authUrl = buildAuthorizationUrl(clientId);

        cout << "\nOpen this URL in your browser:\n\n";
        cout << authUrl << "\n\n";

        cout << "After approving, Google should redirect your browser to:\n";
        cout << "http://127.0.0.1:8080/?code=...\n\n";

        cout << "For now, the page may fail to load because we have not built the localhost server yet.\n";
        cout << "That is okay. The important thing is whether the browser URL contains code=...\n";

    } catch (const exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;









    // try {
    //     curl_global_init(CURL_GLOBAL_DEFAULT);
    //     string accessToken = "";
    //     vector<string> ids = listUnreadMessageIds(accessToken);
    //     cout << "Unread messages found: " << ids.size() << "\n";
    //     for (const string& id: ids) {
    //         cout << "Message ID: " << id << "\n";
    //     }
    //     curl_global_cleanup();
    // } catch (const exception& e) {
    //     cerr << "Error: " << e.what() << "\n";
    //     curl_global_cleanup();
    //     return 1;
    // }
    // return 0;
}