#include "gmail_browser_interaction_fns.h"


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

        writeJsonFile("credentials/token.json", tokens);

        string accessToken = tokens.at("access_token").get<string>();

        vector<string> ids = listUnreadMessageIds(accessToken);

        vector<json> emails_info = getEmailInfo(accessToken, ids);

        for (const json email_info : emails_info) {
            cout << "info: " << endl;
            cout << email_info << endl << endl;
        }


    } catch (const exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}