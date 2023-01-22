#ifndef UTILS_CPP
#define UTILS_CPP
#include <iostream>
#include <fstream>
#include <string>
#include <drogon/drogon.h>
using namespace drogon;
using namespace std;
class Utils
{
public:
    static void readFile(const string& fileName,string &fileData)
    {
        ifstream readFileStream(fileName, ifstream::in);
        string tmp;
        fileData = "";
        while (getline(readFileStream, tmp))
        {
            fileData += tmp + "\n";
        }
    };
    static Cookie createCookieForHTML(const string& key,const string& val)
    {
        Cookie ret(key,val);
        ret.setHttpOnly(false);
        return ret;
    }
};
#endif