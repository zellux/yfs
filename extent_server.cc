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

int
extent_server::put(extent_protocol::extentid_t id, std::string buf, int &)
{
    extent_protocol::attr attr;
    std::ifstream is(local_path(id).c_str(), std::ios::binary);
    if (is.is_open()) {
        is >> attr.size >> attr.atime >> attr.mtime >> attr.ctime;
    } else {
        time_t current = time(NULL);
        attr.size = buf.size();
        attr.atime = 0;
        attr.mtime = current;
        attr.ctime = current;
    }
    is.close();

    std::ofstream os(local_path(id).c_str(), std::ios::binary);
    os << attr.size << attr.atime <<  attr.mtime << attr.ctime;
    os << buf;
    os.close();

    truncate(local_path(id).c_str(), buf.size() + HEADER_SIZE);
    
    return extent_protocol::OK;
}

int
extent_server::get(extent_protocol::extentid_t id, std::string &buf)
{
    extent_protocol::attr attr;
    std::ifstream is(local_path(id).c_str(), std::ios::binary);
    if (is.is_open())
        return extent_protocol::NOENT;
        
    is >> attr.size >> attr.atime >> attr.mtime >> attr.ctime;
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
    std::ifstream is(local_path(id).c_str(), std::ios::binary);
    if (is.is_open())
        return extent_protocol::NOENT;
    is >> a.size >> a.atime >> a.mtime >> a.ctime;
    is.close();
    
    a.atime = time(NULL);
    std::ofstream os(local_path(id).c_str(), std::ios::binary);
    os.seekp(0);
    os << a.size << a.atime <<  a.mtime << a.ctime;
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
