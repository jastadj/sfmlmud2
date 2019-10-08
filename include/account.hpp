#ifndef CLASS_ACCOUNT
#define CLASS_ACCOUNT

#include "sqlite3.h"
#include "client.hpp"

#define CREATE_TEST_ACCOUNT 1

class AccountManager
{
private:
    static bool m_Initialized;
    AccountManager(sqlite3 *db);
    ~AccountManager();

    // database reference
    sqlite3 *m_DB;

public:

    int loginClient(Client *tclient, std::string username, std::string password);

    static int loginProcess(Client *tclient);
    static bool stringIsValidUsername(std::string str);
    static std::string formatUsername(std::string username);

    bool createAccount(std::string username, std::string password);
    bool usernameTaken(std::string username);

    friend class Mud;
};


#endif // CLASS_ACCOUNT
