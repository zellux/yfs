// yfs client.  implements FS operations using extent and lock server
#include "yfs_client.h"
#include "extent_client.h"
#include "lock_client.h"
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
    // You modify this function for Lab 3
    // - hold and release the file lock
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

    // You modify this function for Lab 3
    // - hold and release the directory lock
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
yfs_client::lookup(inum parent, const char *name, inum &inum)
{
    int r = OK;
    std::string content;
    std::vector<dirent> list;
    std::vector<dirent>::iterator iter;

    printf("lookup %s in %016llx\n", name, parent);
    r = readdir(parent, list);
    if (r != OK)
        goto release;

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
yfs_client::create(inum parent, inum &inum, const char *name, bool is_dir)
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
    if (is_dir)
        inum = rand() & ~YFS_DIR_FLAG;
    else
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

int
yfs_client::setsize(inum inum, unsigned int size)
{
    int r = OK;

    printf("   setsize %016llx\n", inum);
    if (ec->setsize(inum, size) != extent_protocol::OK)
        r = NOENT;

    return r;
}

int
yfs_client::read(inum inum, unsigned offset, unsigned size, std::string &buffer)
{
    int r = OK;
    std::string content;

    printf("   read %016llx\n", inum);
    if (ec->get(inum, content) != extent_protocol::OK) {
        r = NOENT;
        goto release;
    }
    buffer = content.substr(offset, size);

 release:
    return r;
}

int
yfs_client::write(inum inum, unsigned offset, unsigned size, std::string buffer)
{
    int r = OK;
    std::string content;
    std::string head, mid, tail;

    printf("   write %016llx offset %u size %u buffer size %lu\n",
           inum, offset, size, buffer.length());
    if (ec->get(inum, content) != extent_protocol::OK) {
        r = NOENT;
        goto release;
    }
    head = content.substr(0, offset);
    if (offset > content.size())
        head += std::string(offset - content.size(), '\0');
    mid = buffer.substr(0, size);
    if (offset + size < content.size())
        tail = content.substr(offset + size, content.size() - offset - size);
    printf("   head size: %lu\n", head.length());
    printf("   mid  size: %lu\n", mid.length());
    printf("   tail size: %lu\n", tail.length());
    content = head + mid + tail;
    if (ec->put(inum, content) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }

 release:
    return r;
}

int
yfs_client::unlink(inum parent, const char *name)
{
    int r = OK;
    inum inum = 0;
    std::vector<dirent> list;
    std::vector<dirent>::iterator iter;

    printf("   unlink %s from %llu\n", name, parent);
    if ((r = readdir(parent, list)) != OK)
        goto release;
    for (iter = list.begin(); iter != list.end(); iter++) {
        if (!strcmp(iter->name.c_str(), name)) {
            inum = iter->inum;
            list.erase(iter);
            printf("   found item %llu\n", inum);
            break;
        }
    }
    if (inum == 0 || isdir(inum)) {
        r = NOENT;
        goto release;
    }
    if ((r = ec->remove(inum)) != extent_protocol::OK) {
        r = NOENT;
        goto release;
    }
    savedir(parent, list);

 release:
    return r;
}

// Assume lock aquired
int
yfs_client::savedir(inum inum, const std::vector<dirent> &list)
{
    std::ostringstream content;
    std::vector<dirent>::const_iterator iter;
    int r = OK;

    for (iter = list.begin(); iter != list.end(); iter++) {
        content << iter->inum << std::endl;
        content << iter->name << std::endl;
    }
    if (ec->put(inum, content.str()) != extent_protocol::OK)
        r = IOERR;

    return r;
}
