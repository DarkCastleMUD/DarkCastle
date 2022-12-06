#ifndef SSH_H
#define SSH_H

#include <libssh/libssh.h>
#include <libssh/server.h>
#include <QObject>
#include <QThread>

// Create RSA host key with: ssh-keygen -t rsa -f ssh_host_rsa_key
// Place resulting two files in lib/

namespace SSH
{

    class SSH : public QObject
    {
        Q_OBJECT
    public:
        explicit SSH(QObject *parent = nullptr);
        void setup(void);
        int poll(void);
        void close(void);
        ~SSH();

    signals:

    public slots:
        void run(void);

    private:
        ssh_bind sshbind = {};
        ssh_session sshsession = {};
        char *data = {};
    };
}

#endif
