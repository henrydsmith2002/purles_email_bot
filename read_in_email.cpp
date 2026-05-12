#include "gmail_browser_interaction_fns.h"


int main() {
    try {
        string accessToken = getValidAccessToken("credentials/client_secret_953867945298-3rtu9nrdplc6906j20c3tf1abhmt5bl5.apps.googleusercontent.com.json");

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