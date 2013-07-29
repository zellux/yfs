// the extent server implementation

#include "extent_server.h"
#include <fstream>
#include <utility>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// TODO make all operations atomic

extent_server::extent_server() {}

inline std::ofstream &
operator<<(std::ofstream &m, extent_protocol::attr a)
{
    m << a.atime << std::endl;
    m << a.mtime << std::endl;
    m << a.ctime << std::endl;
    m << a.size << std::endl;
    return m;
}

inline std::ifstream &
operator>>(std::ifstream &m, extent_protocol::attr &a)
{
    std::string endl;
    m >> a.atime;
    m >> a.mtime;
    m >> a.ctime;
    m >> a.size;
    std::getline(m, endl);
    return m;
}

int
extent_server::put(extent_protocol::extentid_t id, std::string buf, int &)
{
    extent_protocol::attr attr;
    printf("put into %016llx\n", id);
    time_t current = time(NULL);
    std::ifstream is(local_path(id).c_str(), std::ios::binary);
    if (is.is_open()) {
        is >> attr;
    } else {
        attr.atime = 0;
        attr.ctime = current;
    }
    attr.mtime = current;
    attr.size = buf.size();
    is.close();

    std::ofstream os(local_path(id).c_str(), std::ios::trunc);
    os << attr << buf;
    os.close();

    // truncate(local_path(id).c_str(), os.tellp());
    
    return extent_protocol::OK;
}

int
extent_server::get(extent_protocol::extentid_t id, std::string &buf)
{
    extent_protocol::attr attr;
    printf("get %016llx\n", id);
    std::ifstream is(local_path(id).c_str());
    if (!is.is_open()) {
        printf("not exist.\n");
        return extent_protocol::NOENT;
    }
        
    is >> attr;
    buf.resize(attr.size);
    is.read(&buf[0], attr.size);
    is.close();
    return extent_protocol::OK;
}

int
extent_server::getattr(extent_protocol::extentid_t id, extent_protocol::attr &a)
{
    // You fill this in for Lab 2.
    // You replace this with a real implementation. We send a phony response
    // for now because it's difficult to get FUSE to do anything (including
    // unmount) if getattr fails.
    std::ifstream is(local_path(id).c_str());
    if (is.is_open())
        return extent_protocol::NOENT;
    is >> a;
    is.close();
    
    a.atime = time(NULL);
    std::ofstream os(local_path(id).c_str(), std::ios::binary);
    os.seekp(0);
    os << a;
    os.close();
    
    return extent_protocol::OK;
}

int
extent_server::remove(extent_protocol::extentid_t id, int &)
{
    std::ifstream is(local_path(id).c_str());
    if (is.is_open())
        return extent_protocol::NOENT;
    is.close();

    if (::remove(local_path(id).c_str()))
        return extent_protocol::IOERR;
    else
        return extent_protocol::OK;
}
