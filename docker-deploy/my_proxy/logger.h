#ifndef __LOGGER__
#define __LOGGER__
#include <ios>
#include <fstream>

//using std::mutex;

class logger{
public:
    logger(int i, std::string p) : id(i), path(p) {}

    void writeEvictedCacheLog(std::string url) {
        std::ofstream log(path.c_str(), std::ios_base::app | std::ios_base::out);
        log << "(no-id): NOTE evicted " << url << " from cache\n";
    }

    void writecacheLog(bool inCache, bool notExpired) {
        std::ofstream log(path.c_str(), std::ios_base::app | std::ios_base::out);
        if (!inCache) {
            log << id << ": " << "not in cache\n";
        }
        else {
            if (notExpired) {
                log << id << ": " << "in cache, valid\n";
            }
            else {
                log << id << ": " << "cached, but requires re-validation\n";
            }
        }
    }

    void writeLog(std::string str) {
        std::ofstream log(path.c_str(), std::ios_base::app | std::ios_base::out);
        log << id << ": " << str << "\n";
    }


private:
    int id;
    std::string path;
};
#endif