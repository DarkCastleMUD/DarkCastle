#ifndef DCASTLE_SSH_H
#define DCASTLE_SSH_H

#include <libssh/libssh.h>
#include <libssh/server.h>

class SSH
{
public:
    SSH();
    ~SSH();

private:
    ssh_bind sshbind;
};

#endif
