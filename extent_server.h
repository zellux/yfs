// this is the extent server

#ifndef extent_server_h
#define extent_server_h

#include <string>
#include <map>
#include <sstream>
#include "extent_protocol.h"

#define EXTENT_SERVER_ROOT "objects/"
class extent_server {

 public:
    extent_server();

    int put(extent_protocol::extentid_t id, std::string, int &);
    int get(extent_protocol::extentid_t id, std::string &);
    int getattr(extent_protocol::extentid_t id, extent_protocol::attr &);
    int remove(extent_protocol::extentid_t id, int &);

 private:
    inline const std::string local_path(extent_protocol::extentid_t id);
    void save(extent_protocol::extentid_t, const extent_protocol::attr &, const std::string &);
    static const int HEADER_SIZE = sizeof(((extent_protocol::attr *) 0)->size) +
        sizeof(((extent_protocol::attr *) 0)->atime) +
        sizeof(((extent_protocol::attr *) 0)->mtime) +
        sizeof(((extent_protocol::attr *) 0)->ctime);
};

inline const std::string
extent_server::local_path(extent_protocol::extentid_t id) {
    std::ostringstream ost;
    ost << EXTENT_SERVER_ROOT;
    ost << id;
    return ost.str();
}

#endif
