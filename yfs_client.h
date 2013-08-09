#ifndef yfs_client_h
#define yfs_client_h

#include <string>
#include "extent_client.h"
#include <vector>
#include "lock_protocol.h"
#include "lock_client.h"

#define YFS_DIR_FLAG 0x80000000

class yfs_client {
    extent_client *ec;
 public:
    static const int MAX_FILENAME_LENGTH = 256;

    typedef unsigned long long inum;
    enum xxstatus { OK, RPCERR, NOENT, IOERR, EXIST };
    typedef int status;

    struct fileinfo {
        unsigned long long size;
        unsigned long atime;
        unsigned long mtime;
        unsigned long ctime;
    };
    struct dirinfo {
        unsigned long atime;
        unsigned long mtime;
        unsigned long ctime;
    };
    struct dirent {
        std::string name;
        yfs_client::inum inum;
    };

 private:
    static std::string filename(inum);
    static inum n2i(std::string);
    static void parse_dir(const std::string, std::vector<dirent> &);

 public:

    yfs_client(std::string, std::string);

    bool isfile(inum);
    bool isdir(inum);

    int getfile(inum, fileinfo &);
    int getdir(inum, dirinfo &);
    int create(inum, inum &, const char *, bool);
    int readdir(inum, std::vector<dirent> &);
    int lookup(const char *, inum, inum &);
    int setsize(inum, unsigned int);
    int read(inum, unsigned, unsigned, std::string &);
    int write(inum, unsigned, unsigned, std::string);
};

#endif
