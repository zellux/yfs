// the lock server implementation

#include "lock_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "jsl_log.h"

lock_server::lock_server():
    nacquire (0)
{
    pthread_mutex_init(&mutex, NULL);
}

lock_protocol::status
lock_server::stat(int clt, lock_protocol::lockid_t lid, int &r)
{
    lock_protocol::status ret = lock_protocol::OK;
    printf("stat request from clt %d\n", clt);
    r = nacquire;
    return ret;
}

lock_protocol::status
lock_server::acquire(int clt, lock_protocol::lockid_t lid, int &r)
{
    lock_protocol::status ret;

    pthread_mutex_lock(&mutex);
    std::map<lock_protocol::lockid_t, int>::iterator it = locks.find(lid);
    if (it == locks.end()) {
        locks[lid] = 1;
        conditions[lid] = new pthread_cond_t;
        pthread_cond_init(conditions[lid], NULL);
        ret = lock_protocol::OK;
    } else if (it->second == 0) {
        locks[lid] = 1;
        ret = lock_protocol::OK;
    } else {
        while (1) {
            pthread_cond_wait(conditions[lid], &mutex);
            if (it->second == 0)
                break;
        }
        it->second = 1;
    }
    pthread_mutex_unlock(&mutex);
    
    r = 0;
    return ret;
}

lock_protocol::status
lock_server::release(int clt, lock_protocol::lockid_t lid, int &r)
{
    lock_protocol::status ret = lock_protocol::OK;

    jsl_log(JSL_DBG_2, "lock_server::release called with lockid %llu\n", lid);

    pthread_mutex_lock(&mutex);
    jsl_log(JSL_DBG_2, "lock_server::release (%llu) grant mutex\n", lid);
    locks[lid] = 0;
    pthread_cond_signal(conditions[lid]);
    
    pthread_mutex_unlock(&mutex);
    jsl_log(JSL_DBG_2, "lock_server::release (%llu) returns\n", lid);

    r = 0;
    return ret;

}
