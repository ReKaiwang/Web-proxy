// initialize date time
// compare date time
#ifndef __DATETIME__
#define __DATETIME__
#include <string>
#include <iostream>
#include <ctime>
#include <sstream>
#include <iomanip>
using std::string;
using std::stringstream;
class dateTime{
public:
    dateTime(std::string timestring) {
        strptime(timestring.c_str(),"%a, %d %b %Y %H:%M:%S %Z", &timeDate);
    }

    bool checkIfExpired(dateTime& expiredTime) {
        return difftime(mktime(&timeDate), mktime(&(expiredTime.timeDate))) <= 0;
    }

    time_t getTime() {
        return mktime(&(timeDate));
    }

    string toString() {
        stringstream ss;
        ss << std::put_time(&timeDate, "%a, %d %b %Y %H:%M:%S %Z");
        return ss.str();
    }

private:
    struct tm timeDate;
};
#endif