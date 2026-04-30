#include "gmail_browser_interaction_fns.h"

#include <cctype>
#include <curl/curl.h>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

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
