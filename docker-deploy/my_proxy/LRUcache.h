#ifndef __LRUCACHE__
#define __LRUCACHE__
#include <unordered_map>
#include <list>
#include "datetime.h"
#include "logger.h"

using std::list;
using std::unordered_map;

using std::string;
using std::cout;
using std::endl;
class LRUCache{
public:
    LRUCache(int c) : capacity(c) {}

    bool search(string url, string& content, logger& cachelog) {
        time_t currtime = time(0);
        if (cacheHashMap.find(url) != cacheHashMap.end()) {
            cout << "find element" << endl;
            if (difftime(currtime, cacheHashMap[url]->expirationtime.getTime()) <= 0) {
                cout << "not expired" << endl;
                content = cacheHashMap[url]->content;
                cachelog.writecacheLog(true, true);
                return true;
            }
            else {
                cout << "expired" << endl;
                cachelog.writecacheLog(true, false);
                cachelog.writeLog(string("cached, expires at ") + cacheHashMap[url]->expirationtime.toString());
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

    void update(string url, string content, dateTime expirationtime, logger& cachelog) {
        cout << "update" << url << endl;
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
    struct cacheNode{
        string url;
        string content;
        dateTime expirationtime;

        cacheNode(string u, string c, dateTime e) :url(u), content(c), expirationtime(e) {}
    };

    unordered_map<string, list<cacheNode>::iterator> cacheHashMap;
    list<cacheNode> cachelist;
};

#endif