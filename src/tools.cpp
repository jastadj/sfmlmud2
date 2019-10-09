#include "tools.hpp"

#include <iostream>

#include <math.h>
#include <time.h>

#include <dirent.h> // for directory / files
#include <sys/types.h>
#include <sys/stat.h>
#include <sstream>

//////////////////////////////////////////////////////////////////
// STRING TOOLS
std::string toLower(std::string tstring)
{
    std::string lstring;

    for(int i = 0; i < int(tstring.size()); i++)
    {
        char ch = tstring[i];

        if( int(ch) >= int('A') && int(ch <= int('Z')) )
        {
            ch = char( (int(ch) - int('A')) + int('a') );
        }

        lstring.push_back(ch);
    }

    return lstring;
}

std::vector<int> getVectorOfInts(int minval, int maxval)
{
    std::vector<int> ilist;

    if(minval > maxval)
    {
        int t = minval;
        minval = maxval;
        maxval = t;
    }

    for(int i = minval; i <= maxval; i++)
    {
        ilist.push_back(i);
    }

    return ilist;
}

std::vector<std::string> csvParse(std::string pstring, char delim)
{
    std::vector<std::string> parsedStrings;

    //don't use " as delimiter, bad things will happen, detect this and return a blank list
    if(delim == '"') return parsedStrings;

    bool inquotes = false;
    std::string currentstring;

    //walk through each character in string
    for(int i = 0; i < int(pstring.length()); i++)
    {
        //if character is a the delim, stop current string, add to list, and start new string
        if(pstring[i] == delim && !inquotes)
        {
            parsedStrings.push_back(currentstring);

            currentstring = "";
        }
        //are we at the end of the string?
        else if(i == int(pstring.length()-1))
        {
            //if last character is not quote
            if(pstring[i] != '"') currentstring.push_back( pstring[i]);
            parsedStrings.push_back(currentstring);
        }
        //if entering a cell in quotes
        else if(pstring[i] == '"')
        {
            if(!inquotes)
            {
                inquotes = true;
                if(pstring[i+1] == '"') currentstring.push_back('"');
            }
            else
            {
                //if next character is a quote (double quotes) add to string
                if(pstring[i+1] == '"') currentstring.push_back('"');
                //else if next character is the delimiter
                else if(pstring[i+1] == delim)
                {
                    inquotes = false;
                }
            }

        }
        else currentstring.push_back(pstring[i]);

    }


    return parsedStrings;

}

bool isNumber(std::string tstring)
{
    static const std::string num("0123456789");

    if(tstring.empty()) return false;

    for(int i = 0; i < int(tstring.length()); i++)
    {
        if( num.find(tstring[i]) == std::string::npos) return false;
    }

    return true;
}

bool isAlpha(std::string tstring)
{
    for(int i = 0; i < int(tstring.size()); i++)
    {
        int cval = int(tstring[i]);
        if( (cval < int('a') || cval > int('z') ) && (cval < int('A') || cval < int('Z')) ) return false;
    }

    return true;
}

bool isAlphaNumeric(std::string tstring)
{
    static const std::string num("0123456789");

    for(int i = 0; i < int(tstring.size()); i++)
    {
        int cval = int(tstring[i]);
        if( ((cval < int('a') || cval > int('z') ) && (cval < int('A') || cval < int('Z')) )
           || ( num.find(tstring[i]) == std::string::npos ) ) return false;
    }

    return true;
}

bool hasSpaces(std::string tstring)
{
    for(int i = 0; i < int(tstring.size()); i++)
    {
        if(tstring[i] == ' ') return true;
    }

    return false;
}

bool isIpAddress(std::string tstring)
{
    if(tstring.empty()) return false;

    std::vector<std::string> ips = csvParse(tstring, '.');

    if( int(ips.size()) != 4) return false;

    for(int i = 0; i < 4; i++)
    {
        if(isNumber(ips[i]))
        {
            int ip_val = atoi( ips[i].c_str() );
            if( ip_val < 0 || ip_val > 255) return false;
        }
        else return false;
    }

    return true;
}

std::string capitalize(std::string tstring)
{
    for(int i = 0; i < int(tstring.size()); i++)
    {
        int cval = int(tstring[i]);
        if( (cval >= int('a') && cval <= int('z') ) )
        {
            int adelta = cval - int('a');
            tstring[i] = char( int('A') + adelta);
            return tstring;
        }
        else if (cval < int('A') || cval < int('Z'))
        {
            return tstring;
        }
    }
    return tstring;
}

//////////////////////////////////////////////////////////////////
// FILE TOOLS


std::vector<std::string> getFiles(std::string directory, std::string extension)
{
    //create file list vector
    std::vector<std::string> fileList;

    //create directory pointer and directory entry struct
    DIR *dir;
    struct dirent *ent;
    struct stat st;

    if(directory.back() != '\\' || directory.back() != '/') directory.push_back('/');

    //if able to open directory
    if ( (dir = opendir (directory.c_str() ) ) != NULL)
    {
      //go through each file in directory
      while ((ent = readdir (dir)) != NULL)
      {
            //convert read in file to a string
            std::string filename(ent->d_name);

            if(filename != "." && filename != "..")
            {

                //check that entry is a directory
                stat(std::string(directory+filename).c_str(), &st);
                if(!S_ISDIR(st.st_mode))
                {
                    //check that file matches given extension parameter
                    //if an extension is provided, make sure it matches
                    if(extension != "")
                    {
                        //find location of extension identifier '.'
                        size_t trim = filename.find_last_of('.');
                        if(trim > 200) std::cout << "ERROR:getFiles extension is invalid\n";
                        else
                        {
                            std::string target_extension = filename.substr(trim);

                            if(target_extension == extension) fileList.push_back(filename);
                            //else std::cout << "Filename:" << filename << " does not match extension parameter - ignoring\n";
                        }
                    }
                    else fileList.push_back(filename);
                }

            }

      }

      closedir (dir);
    }
    //else failed to open directory
    else
    {
      std::cout << "Failed to open directory:" << directory << std::endl;
      //return empty file list
      return fileList;
    }

    return fileList;
}

bool fileExists(std::string tfile)
{
    //file is found flag
    bool fileFound = false;

    //strip directory from target file and path
    size_t split = tfile.find_last_of('\\');
    //if could not find directory terminator, set directory to current dir
    if(split == std::string::npos)
    {
        split = tfile.find_last_of('/');
        if(split == std::string::npos) split = 0;
    }
    //std::cout << "SPLIT=" << split << std::endl;
    std::string directory =tfile.substr(0, split);
    if(directory.empty()) directory = "./";
    //std::cout << "DIRECTORY:" << directory << std::endl;
    //if(directory.empty()) std::cout << "directory is EMPTY\n";

    //strip filename from target file and path
    std::string tfilename;
    if(split) tfilename = tfile.substr(split+1);
    else tfilename = tfile;
    //std::cout << "FILENAME:" << tfilename << std::endl;

    //string

    //create directory pointer and directory entry struct
    DIR *dir;
    struct dirent *ent;
    //struct stat st;

    //if able to open directory
    if ( (dir = opendir (directory.c_str() ) ) != NULL)
    {
      //go through each file in directory
      while ((ent = readdir (dir)) != NULL)
      {
            //convert read in file to a string
            std::string filename(ent->d_name);

            // debug
            //std::cout << "target:" << tfilename << " - " << filename << std::endl;

            //if extension matches, set found flag to true
            if(!filename.compare(tfilename) ) fileFound = true;

      }

        //done reading files, close directory for reading
        closedir (dir);
    }
    //else failed to open directory
    else
    {
      std::cout << "Failed to open directory:" << directory << std::endl;
      return false;
    }


    //return if file was found or not
    if(fileFound) return true;
    else return false;
}

std::vector<std::string> getFolders(std::string directory)
{
    //create file list vector
    std::vector<std::string> fileList;

    //create directory pointer and directory entry struct
    DIR *dir;
    struct dirent *ent;
    struct stat st;

    //if able to open directory
    if ( (dir = opendir (directory.c_str() ) ) != NULL)
    {
      //go through each file in directory
      while ((ent = readdir (dir)) != NULL)
      {
            //convert read in file to a string
            std::string filename = (ent->d_name);

            //check that entry is a directory, if so, add it to the list
            stat(std::string(directory + filename).c_str(), &st);
            if(S_ISDIR(st.st_mode)) fileList.push_back(filename);

      }

      closedir (dir);
    }
    //else failed to open directory
    else
    {
      std::cout << "Failed to open directory:" << directory << std::endl;
      //return empty file list
      return fileList;
    }

    return fileList;
}

bool makeFolder(std::string directory)
{
    int results = mkdir(directory.c_str());

    if(results == 0 || results == -1) return true;

    return false;
}

//////////////////////////////////////////////////////////////////
// SQLITE TOOLS

int sqlcallback(void *data, int argc, char **argv, char **azColName)
{
    int i;
    for(i = 0; i < argc; i++)
    {
        std::cout << azColName[i] << " = ";
        if(argv[i]) std::cout << argv[i];
        else std::cout << "NULL";
        std::cout << "\n";
    }
    return 0;
}

bool tableExists(sqlite3 *db, std::string table_name)
{
    sqlite3_stmt *stmt;
    std::stringstream ss;

    // construct SQL statement to look for table
    ss << "SELECT name FROM sqlite_master WHERE type = 'table' AND name='" << table_name << "';";

    // compile sql statement to binary
    if( sqlite3_prepare_v2(db, ss.str().c_str(), -1, &stmt, NULL) != SQLITE_OK)
    {
        std::cout << "Error in tableExists while compiling sql:" << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }

    // execute sql statement
    int ret_code = 0;
    bool found_table = false;
    while((ret_code = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        found_table = true;
        //sqlite3_column_text(stmt, 0)) ) == table_name) found_table = true;
        //std::cout << "TEST: ID = " << sqlite3_column_int(stmt, 0) << std::endl;
        //std::cout << "TEST2 = " << sqlite3_column_name(stmt, 0) << std::endl;
        //std::cout << "TEST3 = " << sqlite3_column_text(stmt, 0) << std::endl;
    }
    if(ret_code != SQLITE_DONE)
    {
        std::cout << "Error in tableExists while performing sql:" << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_finalize(stmt);
    return found_table;
}
