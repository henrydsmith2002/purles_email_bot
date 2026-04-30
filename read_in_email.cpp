#include "gmail_browser_interaction_fns.h"
#include "in_out_helpers.h"


int main() {
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

        string accessToken = tokens.at("access_token").get<string>();

        vector<string> ids = listUnreadMessageIds(accessToken);


    } catch (const exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}









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