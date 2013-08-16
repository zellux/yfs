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

void
extent_server::save(extent_protocol::extentid_t id,
                    const extent_protocol::attr &a,
                    const std::string &c)
{
    std::ofstream os(local_path(id).c_str(), std::ios::trunc);
    os << a << c;
    os.close();
}

int
extent_server::put(extent_protocol::extentid_t id, std::string buf, int &)
{
    extent_protocol::attr attr;
    printf("put into %llu 0x%016llx\n", id, id);
    time_t current = time(NULL);
    std::ifstream is(local_path(id).c_str(), std::ios::binary);
    if (is.is_open()) {
        is >> attr;
    } else {
        attr.atime = 0;
    }
    attr.mtime = current;
    attr.ctime = current;
    attr.size = buf.size();
    is.close();

    save(id, attr, buf);

    return extent_protocol::OK;
}

int
extent_server::get(extent_protocol::extentid_t id, std::string &buf)
{
    int ret = extent_protocol::OK;
    extent_protocol::attr attr;
    std::ifstream is(local_path(id).c_str());

    printf("get %016llx\n", id);
    if (!is.is_open()) {
        printf("not exist.\n");
        ret = extent_protocol::NOENT;
        goto release;
    }

    is >> attr;
    buf.resize(attr.size);
    is.read(&buf[0], attr.size);
    is.close();

    attr.atime = time(NULL);
    save(id, attr, buf);

 release:
    return ret;
}

int
extent_server::getattr(extent_protocol::extentid_t id, extent_protocol::attr &a)
{
    int ret = extent_protocol::OK;
    std::ifstream is(local_path(id).c_str());
    std::string temp;

    printf("getattr %016llx\n", id);
    if (!is.is_open()) {
        printf("  not exist\n");
        ret = extent_protocol::NOENT;
        goto release;
    }
    is >> a;
    temp.resize(a.size);
    is.read(&temp[0], a.size);
    is.close();

    a.atime = time(NULL);
    save(id, a, temp);

 release:
    return ret;
}

int
extent_server::remove(extent_protocol::extentid_t id, int &)
{
    int ret = extent_protocol::OK;
    std::ifstream is(local_path(id).c_str());

    printf("remove %lld 0x%016llx\n", id, id);
    if (!is.is_open()) {
        printf("  not exist\n");
        ret = extent_protocol::NOENT;
        goto release;
    }
    is.close();
    if (::remove(local_path(id).c_str()))
        ret = extent_protocol::IOERR;

 release:
    return ret;
}

int
extent_server::setsize(extent_protocol::extentid_t id, unsigned int size, int &)
{
    int ret = extent_protocol::OK;
    std::ifstream is(local_path(id).c_str());
    std::string temp;
    extent_protocol::attr attr;

    printf("setsize %016llx\n", id);
    if (!is.is_open()) {
        printf("  not exist\n");
        ret = extent_protocol::NOENT;
        goto release;
    }
    is >> attr;
    attr.size = size;
    attr.mtime = time(NULL);
    attr.ctime = time(NULL);
    temp.resize(attr.size);
    is.read(&temp[0], attr.size);
    is.close();

    save(id, attr, temp);

 release:
    return ret;
}
