#include "gmail_browser_interaction_fns.h"

#include <cctype>
#include <curl/curl.h>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <filesystem>

#include "include/httplib.h"

using std::ifstream;
using std::ostringstream;
using std::runtime_error;
using std::setw;
using std::to_string;

static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    string* response = static_cast<string*>(userp);
    response->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

string httpGet(const string& url, const string& accessToken) {
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

string urlEncode(const string& value) {
    ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex << std::uppercase;

    for (unsigned char c : value) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else {
            escaped << '%' << setw(2) << static_cast<int>(c);
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

string waitForAuthorizationCode() {
    httplib::Server server;
    string authorizationCode;

    server.Get("/", [&](const httplib::Request& req, httplib::Response& res) {
        if (req.has_param("code")) {
            authorizationCode = req.get_param_value("code");

            res.set_content(
                "<html><body>"
                "<h1>Authorization complete</h1>"
                "<p>You can close this tab and return to the terminal.</p>"
                "</body></html>",
                "text/html"
            );

            server.stop();
        } else if (req.has_param("error")) {
            string error = req.get_param_value("error");

            res.set_content(
                "<html><body>"
                "<h1>Authorization failed</h1>"
                "<p>Error: " + error + "</p>"
                "</body></html>",
                "text/html"
            );

            server.stop();
        } else {
            res.set_content(
                "<html><body><h1>No authorization code found.</h1></body></html>",
                "text/html"
            );
        }
    });

    cout << "Waiting for Google OAuth redirect on http://127.0.0.1:8080/\n";

    bool started = server.listen("127.0.0.1", 8080);

    if (!started) {
        throw runtime_error("Could not start local server on 127.0.0.1:8080");
    }

    if (authorizationCode.empty()) {
        throw runtime_error("Did not receive authorization code.");
    }

    return authorizationCode;
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

string httpPostForm(
    const string& url,
    const vector<std::pair<string, string>>& fields
) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        throw runtime_error("Failed to initialize CURL");
    }

    string response;
    string postFields;

    for (size_t i = 0; i < fields.size(); ++i) {
        char* encodedKey = curl_easy_escape(curl, fields[i].first.c_str(), 0);
        char* encodedValue = curl_easy_escape(curl, fields[i].second.c_str(), 0);

        if (i > 0) {
            postFields += "&";
        }

        postFields += encodedKey;
        postFields += "=";
        postFields += encodedValue;

        curl_free(encodedKey);
        curl_free(encodedValue);
    }

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postFields.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode result = curl_easy_perform(curl);

    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (result != CURLE_OK) {
        throw runtime_error(string("curl error: ") + curl_easy_strerror(result));
    }

    if (httpCode >= 400) {
        throw runtime_error("HTTP " + to_string(httpCode) + ": " + response);
    }

    return response;
}

json exchangeCodeForTokens(
    const string& code,
    const string& clientId,
    const string& clientSecret
) {
    string response = httpPostForm(
        "https://oauth2.googleapis.com/token",
        {
            {"code", code},
            {"client_id", clientId},
            {"client_secret", clientSecret},
            {"redirect_uri", "http://127.0.0.1:8080/"},
            {"grant_type", "authorization_code"}
        }
    );

    return json::parse(response);
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




// fn to check for if refresh token exists
bool tokenFileExists(const string& path) {
    return std::filesystem::exists(path);
}


// This function takes in the credentials json and implementents everything to 
// get a valid access token.
string getValidAccessToken(const json& credentials) {
    const string tokenPath = "credentials/token.json";
    if (tokenFileExists(tokenPath)) {
        cout << "Token file exists" << endl;
        
    }
    else {
        cout << "token file does not exist, authorizing through browser" << endl;
    try {
        json credentials = readJsonFile("credentials/client_secret_953867945298-3rtu9nrdplc6906j20c3tf1abhmt5bl5.apps.googleusercontent.com.json");
        string clientId =
            credentials.at("installed").at("client_id").get<string>();
        string authUrl = buildAuthorizationUrl(clientId);
        cout << "\nOpen this URL in your browser:\n\n";
        cout << authUrl << "\n\n";
        string code = waitForAuthorizationCode();
        cout << "\nAuthorization code received:" << endl;
        cout << code << endl;
        string clientSecret = credentials.at("installed").at("client_secret").get<string>();
        json tokens = exchangeCodeForTokens(code, clientId, clientSecret);
        cout << tokens.dump(2) << endl;
        writeJsonFile("credentials/token.json", tokens);
        string accessToken = tokens.at("access_token").get<string>();
        return accessToken;
        } catch (const exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
        }
    }
}