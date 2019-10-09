#ifndef _TOOLS
#define _TOOLS

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "sqlite3.h"

#define PI 3.14159



// STRING TOOLS
std::string toLower(std::string tstring);
std::vector<int> getVectorOfInts(int minval, int maxval);
std::vector<std::string> csvParse(std::string pstring, char delim = ',');
bool isNumber(std::string tstring);
bool isAlpha(std::string tstring);
bool isAlphaNumeric(std::string tstring);
bool hasSpaces(std::string tstring);
bool isIpAddress(std::string tstring);
std::string capitalize(std::string tstring);


// FILE TOOLS
std::vector<std::string> getFiles(std::string directory, std::string extension = "");
bool fileExists(std::string tfile);
std::vector<std::string> getFolders(std::string directory);
bool makeFolder(std::string directory);

// SQLITE TOOLS
//static int sqlcallback(void *data, int argc, char **argv, char **azColName);
int sqlcallback(void *data, int argc, char **argv, char **azColName);
bool tableExists(sqlite3 *db, std::string table_name);

#endif // _TOOLS
