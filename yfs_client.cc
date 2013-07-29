// yfs client.  implements FS operations using extent and lock server
#include "yfs_client.h"
#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

yfs_client::yfs_client(std::string extent_dst, std::string lock_dst)
{
    ec = new extent_client(extent_dst);
}

yfs_client::inum
yfs_client::n2i(std::string n)
{
    std::istringstream ist(n);
    unsigned long long finum;
    ist >> finum;
    return finum;
}

std::string
yfs_client::filename(inum inum)
{
    std::ostringstream ost;
    ost << inum;
    return ost.str();
}

bool
yfs_client::isfile(inum inum)
{
    if (inum & YFS_DIR_FLAG)
        return true;
    return false;
}

bool
yfs_client::isdir(inum inum)
{
    return !isfile(inum);
}

int
yfs_client::getfile(inum inum, fileinfo &fin)
{
    int r = OK;

    printf("getfile %016llx\n", inum);
    extent_protocol::attr a;
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }

    fin.atime = a.atime;
    fin.mtime = a.mtime;
    fin.ctime = a.ctime;
    fin.size = a.size;
    printf("getfile %016llx -> sz %llu\n", inum, fin.size);

 release:

    return r;
}

int
yfs_client::getdir(inum inum, dirinfo &din)
{
    int r = OK;

    printf("getdir %016llx\n", inum);
    extent_protocol::attr a;
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }
    din.atime = a.atime;
    din.mtime = a.mtime;
    din.ctime = a.ctime;

 release:
    return r;
}

int
yfs_client::lookup(const char *name, inum parent, inum &inum)
{
    int r = OK;
    std::string content;
    std::vector<dirent> list;
    std::vector<dirent>::iterator iter;

    printf("lookup %s in %016llx\n", name, parent);
    r = readdir(parent, list);
    if (r != OK)
        goto release;

    parse_dir(content, list);

    for (iter = list.begin(); iter != list.end(); iter++) {
        if (iter->name == name) {
            inum = iter->inum;
            goto release;
        }
    }
    r = NOENT;

 release:
    return r;
}

int
yfs_client::readdir(inum inum, std::vector<dirent> &list)
{
    int r = OK;
    std::string content;

    printf("readdir %016llx\n", inum);
    if (ec->get(inum, content) != extent_protocol::OK) {
        r = NOENT;
        goto release;
    }
    parse_dir(content, list);

 release:
    return r;
}

void
yfs_client::parse_dir(const std::string content, std::vector<dirent> &list)
{
    std::istringstream ist(content);
    std::string buffer;

    list.clear();
    while (std::getline(ist, buffer) > 0) {
        dirent entry;
        entry.inum = n2i(std::string(buffer));
        std::getline(ist, entry.name);
        list.push_back(entry);
    }
}

int
yfs_client::create(inum parent, inum &inum, const char *name)
{
    int r = OK;
    std::string content;
    std::ostringstream newcontent;
    std::vector<dirent> list;
    std::vector<dirent>::iterator iter;

    printf("create a new file in %016llx <%s>\n", parent, name);

    if (ec->get(parent, content) != extent_protocol::OK) {
        r = NOENT;
        goto release;
    }

    // filename duplication check
    parse_dir(content, list);
    for (iter = list.begin(); iter != list.end(); iter++) {
        if (iter->name == name) {
            r = EXIST;
            goto release;
        }
    }

    // FIXME: theoratically possible inum collision
    inum = rand() | YFS_DIR_FLAG;
    newcontent << content;
    newcontent << inum << std::endl;
    newcontent << name << std::endl;

    if ((ec->put(parent, newcontent.str()) != extent_protocol::OK) ||
        (ec->put(inum, std::string()) != extent_protocol::OK)) {
        r = IOERR;
        goto release;
    }

 release:
    return r;
}
