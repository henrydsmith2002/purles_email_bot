#include <iostream>
using std::endl, std::cout;
#include <curl/curl.h>
#include <stdexcept>
using std::runtime_error;
#include <string>
using std::string, std::to_string;
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



int main() {
    cout << "hello world" << endl;
    return 0;
}