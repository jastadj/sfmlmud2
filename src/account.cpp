#include "account.hpp"

#include <sstream>
#include "mud.hpp"
#include "tools.hpp"

bool AccountManager::m_Initialized = false;

AccountManager::AccountManager(sqlite3 *db)
{
    if(m_Initialized)
    {
        std::cout << "Account manager has already been initialized!\n";
        return;
    }
    m_Initialized = true;

    // database reference
    m_DB = db;

    // create new table if new
    if(!tableExists(m_DB, "accounts"))
    {
        std::cout << "Creating new accounts table in database...\n";
        std::stringstream ss;
        char *errormsg = 0;
        ss << "CREATE TABLE accounts( ";
        ss << "account_id INTEGER PRIMARY KEY,";
        ss << "account_name TEXT NOT NULL UNIQUE,";
        ss << "account_password TEXT NOT NULL,";
        ss << "current_room INTEGER";
        ss << ");";

        if(sqlite3_exec(m_DB, ss.str().c_str(), sqlcallback, NULL, &errormsg) != SQLITE_OK)
        {
            std::cout << "Error creating accounts table:" << errormsg << std::endl;
            sqlite3_free(errormsg);
            return;
        }

        // create test account
        if(CREATE_TEST_ACCOUNT)
        {
            if(!createAccount("test", "test") ) std::cout << "Error creating test account.\n";
        }
    }
}

AccountManager::~AccountManager()
{

}

// 0 = successful login, 1 = bad username, 2 = bad password, -1 some error
int AccountManager::loginClient(Client *tclient, std::string username, std::string password)
{
    if(!tclient) return -1;

    std::stringstream ss;
    sqlite3_stmt *stmt;
    int success = -1;

    // format username
    username = formatUsername(username);

    // check database
    ss << "SELECT account_name, account_password, current_room FROM accounts WHERE account_name='" << username << "';";
    // compile sql statement to binary
    int rc = sqlite3_prepare_v2(m_DB, ss.str().c_str(), -1, &stmt, NULL);
    if( rc != SQLITE_OK)
    {
        std::cout << "Error in sql compile:" << sqlite3_errmsg(m_DB) << std::endl;
        sqlite3_finalize(stmt);
        return -1;
    }
    // execute sql statement
    while((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        std::string tuser = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt,0)) );
        std::string tpass = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt,1)) );
        int troom = sqlite3_column_int(stmt, 2);

        if(tuser == username)
        {
            if(tpass == password)
            {
                tclient->m_Username = username;
                tclient->m_CurrentRoom = troom;
                success = 0;
            }
            else success = 2;
        }
        else success = 1;
    }
    if(rc != SQLITE_DONE)
    {
        std::cout << "Error in sql:" << sqlite3_errmsg(m_DB) << std::endl;
    }
    sqlite3_finalize(stmt);

    if(success == 0 ) std::cout << tclient->m_Username << " has logged in.\n";
    return success;
}

bool AccountManager::createAccount(std::string username, std::string password)
{
    std::stringstream ss;
    char *errormsg = 0;
    // valid username?
    if(!stringIsValidUsername(username))
    {
        std::cout << "Unable to create account, username '" << username << "' is invalid.\n";
        return false;
    }

    // does account already exist?
    if(usernameTaken(username))
    {
        std::cout << "Unable to create account, username '" << username << "' already taken.\n";
        return false;
    }

    // format username (only first letter capitalized)
    username = formatUsername(username);

    // create sqlite command
    ss << "INSERT INTO ";
    ss << "accounts(";
    ss << "account_name,";
    ss << "account_password,";
    ss << "current_room) ";
    ss << "VALUES(";
    ss << "'" << username << "',";
    ss << "'" << password << "',";
    ss << "0";
    ss << ");";

    if(sqlite3_exec(m_DB, ss.str().c_str(), sqlcallback, NULL, &errormsg) != SQLITE_OK)
    {
        std::cout << "Error creating accounts table:" << errormsg << std::endl;
        sqlite3_free(errormsg);
        return false;
    }

    return true;
}

bool AccountManager::usernameTaken(std::string username)
{
    std::stringstream ss;
    sqlite3_stmt *stmt;
    bool found_in_db = false;

    username = formatUsername(username);

    // check database
    ss << "SELECT * FROM accounts WHERE account_name='" << username << "' COLLATE NOCASE;";
    // compile sql statement to binary
    int rc = sqlite3_prepare_v2(m_DB, ss.str().c_str(), -1, &stmt, NULL);
    if( rc != SQLITE_OK)
    {
        std::cout << "Error in sql compile:" << sqlite3_errmsg(m_DB) << std::endl;
        sqlite3_finalize(stmt);
        return true;
    }
    // execute sql statement
    while((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        found_in_db = true;
    }
    if(rc != SQLITE_DONE)
    {
        std::cout << "Error in sql:" << sqlite3_errmsg(m_DB) << std::endl;
    }
    sqlite3_finalize(stmt);

    return found_in_db;
}

int AccountManager::loginProcess(Client *tclient)
{
    if(!tclient) return 0;

    Mud *mud = Mud::getInstance();

    // what step is the user in for logging in?
    int login_state = tclient->m_IntRegisters[0];

    // 0 - query - show login prompt
    if(login_state == 0)
    {
        tclient->send("User:\n");
        tclient->clearStorage();
        // check login name
        tclient->m_IntRegisters[0] = 10;
    }
    // 10 - check username
    else if(login_state == 10)
    {
        // store username
        tclient->m_StrRegisters[0] = tclient->m_LastInput;

        // if username is not invalid, give feedback and restart
        if( !AccountManager::stringIsValidUsername(tclient->m_LastInput) )
        {
            tclient->send("Invalid username. Usernames may only contains alpha characters.\n");
            tclient->m_IntRegisters[0] = 0;
            tclient->func(tclient);
        }
        // else, check if username exists or if its new
        else
        {
            // if username is an existing account, goto query password
            if(mud->m_AccountManager->usernameTaken(tclient->m_LastInput))
            {
                tclient->m_IntRegisters[0] = 20;
                tclient->func(tclient);
            }
            // this is username doesnt exist, query user if they want to make a new account
            else
            {
                tclient->m_IntRegisters[0] = 50;
                tclient->func(tclient);
            }
        }
    }
    // query for password
    else if(login_state == 20)
    {
        // check password retry count
        if(tclient->m_IntRegisters[1] >= 3)
        {
            tclient->send("Too many retries.\n");
            tclient->disconnect();
            return 0;
        }

        tclient->send("Password:\n");
        tclient->m_IntRegisters[0] = 21;
    }
    // check entered password
    else if(login_state == 21 )
    {
        // store entered password
        tclient->m_StrRegisters[1] = tclient->m_LastInput;

        // try to login
        int results = mud->m_AccountManager->loginClient(tclient, tclient->m_StrRegisters[0], tclient->m_StrRegisters[1]);
        if(results == 0) tclient->m_IntRegisters[0] = 100;
        // bad password
        else if(results == 2)
        {
            tclient->send("Incorrect password.  Please try again.\n");
            tclient->m_IntRegisters[1]++;
            tclient->m_IntRegisters[0] = 20;
        }
        else tclient->m_IntRegisters[0] = 0;
        tclient->func(tclient);
    }
    // query to make new account
    else if(login_state == 50)
    {
        tclient->send("Create new user '" + tclient->m_StrRegisters[0] + "'?  (y/n)\n");
        tclient->m_IntRegisters[0] = 54;
    }
    // check make new account response
    else if(login_state == 54)
    {
        std::string response = toLower(tclient->m_LastInput);
        // if player wants to make new account, query for new password
        if(response == "y" || response == "yes")
        {
            tclient->m_IntRegisters[0] = 60;
            tclient->m_IntRegisters[1] = 0;
            tclient->func(tclient);
        }
        // user didn't want to make a new account, go back to initial login query
        else
        {
            tclient->m_IntRegisters[0] = 0;
            tclient->func(tclient);
        }
    }
    // query for new password
    else if(login_state == 60)
    {
        tclient->send("Enter new password:\n");
        tclient->m_IntRegisters[0] = 65;
    }
    // store new password, ask to re-enter password
    else if(login_state == 65)
    {
        tclient->m_StrRegisters[1] = tclient->m_LastInput;
        tclient->send("Re-enter password:\n");
        tclient->m_IntRegisters[0] = 70;
    }
    // check that re-entered password patches first password
    else if(login_state == 70)
    {
        // proceed to create account
        if(tclient->m_StrRegisters[1] == tclient->m_LastInput)
        {
            tclient->m_IntRegisters[0] = 90;
            tclient->func(tclient);
        }
        // passwords did not match, increment tries, then start over
        // if too many tries, disconnect
        else
        {
            tclient->m_IntRegisters[1]++;
            if(tclient->m_IntRegisters[1] >= 3) tclient->disconnect();
        }
    }
    // create account and login
    else if(login_state == 90)
    {
        std::string newuser = tclient->m_StrRegisters[0];
        std::string newpass = tclient->m_StrRegisters[1];
        bool success = false;
        // try to create account
        if(mud->m_AccountManager->createAccount(newuser, newpass) )
        {
            // try to log into new account
            if(mud->m_AccountManager->loginClient(tclient, newuser, newpass) == 0)
            {
                tclient->m_IntRegisters[0] = 100;
                success = true;
            }
        }
        // something went very wrong
        if(!success)
        {
            tclient->send("There was an error trying to create new user.\n");
            tclient->m_IntRegisters[0] = 0;
        }
        tclient->func(tclient);
    }
    // finish login
    else if(login_state == 100)
    {
        // clear input storage
        tclient->clearStorage();
        tclient->m_LastInput = "";
        tclient->func = Mud::mainGame;
        tclient->func(tclient);
    }


    return 0;
}

bool AccountManager::stringIsValidUsername(std::string str)
{
    // username cannot be empty
    if(str.empty()) return false;
    // username must contain all alpha characters a-z
    if(!isAlpha(str)) return false;

    // name appears valid
    return true;
}

std::string AccountManager::formatUsername(std::string username)
{
    username = toLower(username);
    username = capitalize(username);
    return username;
}
