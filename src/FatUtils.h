#ifndef _FATCAT_UTILS_H
#define _FATCAT_UTILS_H

#include <vector>
#include <sstream>
#include <string>
#include <algorithm>
#include <functional>

using namespace std;

// Utils
#define FAT_READ_SHORT(buffer,x) ((buffer[x]&0xff)|((buffer[x+1]&0xff)<<8))
#define FAT_READ_LONG(buffer,x) \
        ((buffer[x]&0xff)|((buffer[x+1]&0xff)<<8))| \
        (((buffer[x+2]&0xff)<<16)|((buffer[x+3]&0xff)<<24))

#define FAT_WRITE_SHORT(buffer,x,s) \
        buffer[x] = (s)&0xff; \
        buffer[x+1] = ((s)>>8)&0xff; \

#define FAT_WRITE_LONG(buffer,x,l) \
        buffer[x] = (l)&0xff; \
        buffer[x+1] = ((l)>>8)&0xff; \
        buffer[x+2] = ((l)>>16)&0xff; \
        buffer[x+3] = ((l)>>24)&0xff;

// trim from start
static inline std::string ltrim(std::string s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
        return s;
}

// trim from end
static inline std::string rtrim(std::string s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
        return s;
}

// trim from both ends
static inline std::string trim(std::string s) {
        return ltrim(rtrim(s));
}

// split a string into vector
static inline vector<string> &split(const string &s, char delim, vector<string> &elems) {
    stringstream ss(s);
    string item;
    while(getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

static char units[] = {'B', 'K', 'M', 'G', 'T', 'P'};

// pretty file size
static inline string prettySize(unsigned long long bytes)
{
    double size = bytes;
    int n = 0;

    while (size >= 1024) {
        size /= 1024;
        n++;
    }

    ostringstream oss;
    oss << size << units[n];

    return oss.str();
}

static inline std::string strtolower(std::string myString)
{
  const int length = myString.length();
  for(int i=0; i!=length; ++i) {
    myString[i] = std::tolower(myString[i]);
  }

  return myString;
}

#endif // _FATCAT_UTILS_H
