#ifndef __LRUCACHE__
#define __LRUCACHE__
#include <unordered_map>
#include <list>
#include "datetime.h"
#include "logger.h"
#include <mutex>
class LRUCache{
public:
    LRUCache(int c) : capacity(c) {}

    bool search(string url, dateTime& currtime, string& content, logger& cachelog) {
        std::lock_guard<std::mutex> guard(cachemutex);
        if (cacheHashMap.find(url) != cacheHashMap.end()) {
            if (currtime.checkIfExpired(cacheHashMap[url]->expirationtime)) {
                content = cacheHashMap[url]->content;
                cachelog.writecacheLog(true, true);
                return true;
            }
            else {
                cachelog.writecacheLog(true, false);
                cachelog.writeEvictedCacheLog(url);
                cachelist.erase(cacheHashMap[url]);
                cacheHashMap.erase(url);
            }
        }
        else {
            cachelog.writecacheLog(false, true);
        }
        return false;

    }

    void update(string url, string content, dateTime& expirationtime) {
        std::lock_guard<std::mutex> guard(cachemutex);
        if (cacheHashMap.size() >= capacity) {
            cachelog.writeEvictedCacheLog(cachelist.back().url);
            cacheHashMap.erase(cachelist.back().url);
            cachelist.pop_back();
        }

        cachelist.push_front(cacheNode(url, content, expirationtime));
        cacheHashMap[url] = cachelist.begin();
    }
private:
    int capacity;
    mutex cachemutex;
    struct cacheNode{
        string url;
        string content;
        dateTime expirationtime;

        cacheNode(string u, string c, string e) :u(url), c(content), e(expirationtime) {}
    }

    unordered_map<string, list<cacheNode>::iterator> cacheHashMap;
    list<cacheNode> cachelist;
};

#endif