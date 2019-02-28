#ifndef __IDMANAGER__
#define __IDMANAGER__
#include <mutex>
class idManager{
public:
    idManager(): idavail(0) {}
    idManager(int base): idavail(base){}

    int getNextAvailableID() {
        std::lock_guard<std::mutex> guard(idlock);
        int id = idavail;
        idavail++;
        return id;
    }
    
private:
    int idavail;
    static std::mutex idlock;

};
#endif