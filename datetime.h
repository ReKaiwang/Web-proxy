// initialize date time
// compare date time
#ifndef __DATETIME__
#define __DATETIME__
class dateTime{
public:
    dateTime(string timestring) {
        strptime(timestring.c_str(),"%a, %d %b %Y %H:%M:%S %Z", &timeDate);
    }

    bool checkIfExpired(dateTime& expiredTime) {
        return difftime(mktime(&timeDate), mktime(&(expiredTime.timeDate))) <= 0;
    }

private:
    tm timeDate;
};
#endif