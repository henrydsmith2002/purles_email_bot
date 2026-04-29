#include <iostream>
using std::endl, std::cout;
#include <curl/curl.h>
#include <stdexcept>
#include <string>
using std::string;
#include <vector>
using std::vector;
#include <nlohmann/json.hpp>
using json = nlohmann::json;


static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    string* response = static_cast<string*>(userp);
    response->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}



int main() {
    cout << "hello world" << endl;
    return 0;
}