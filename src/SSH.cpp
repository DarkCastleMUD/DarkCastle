#include <iostream>

#include <libssh/callbacks.h>
#include <QDebug>

#include "SSH.h"
#include "utility.h"

namespace SSH
{

  

  SSH::SSH(QObject *parent) : QObject(parent)
  {
  }

  void SSH::setup(void)
  {
    char buffer[NAME_MAX] = {};
    getcwd(buffer, NAME_MAX - 1);
    logf(0, LogChannels::LOG_BUG, "cwd is %s", buffer);
    chdir("../lib");

    ssh_init();
    sshbind = ssh_bind_new();
    if (int rc = ssh_bind_options_parse_config(sshbind, nullptr) < 0)
    {
      logf(0, LogChannels::LOG_BUG, "ssh_bind_options_parse_config returned %d", rc);
    }

    int loglevel = SSH_LOG_TRACE;
    if (int rc = ssh_bind_options_set(sshbind, ssh_bind_options_e::SSH_BIND_OPTIONS_LOG_VERBOSITY, &loglevel) < 0)
    {
      logf(0, LogChannels::LOG_BUG, "ssh_bind_options_set SSH_BIND_OPTIONS_BINDPORT returned %d", rc);
    }

    const char rsahostkeyfilename[] = "ssh_host_rsa_key";
    if (int rc = ssh_bind_options_set(sshbind, ssh_bind_options_e::SSH_BIND_OPTIONS_RSAKEY, &rsahostkeyfilename) < 0)
    {
      logf(0, LogChannels::LOG_BUG, "ssh_bind_options_set SSH_BIND_OPTIONS_BINDPORT returned %d", rc);
    }

    unsigned int bindport = 6922;
    if (int rc = ssh_bind_options_set(sshbind, ssh_bind_options_e::SSH_BIND_OPTIONS_BINDPORT, &bindport) < 0)
    {
      logf(0, LogChannels::LOG_BUG, "ssh_bind_options_set SSH_BIND_OPTIONS_BINDPORT returned %d", rc);
    }

    ssh_bind_set_blocking(sshbind, false);

    if (int rc = ssh_bind_listen(sshbind) < 0)
    {
      logf(0, LogChannels::LOG_BUG, "ssh_bind_listen returned %d", rc);
    }
  }

  int SSH::poll(void)
  {
    sshsession = ssh_new();
    auto rc = ssh_bind_accept(sshbind, sshsession);
    return rc;
  }

  void SSH::close(void)
  {
    if (sshsession)
    {
      ssh_disconnect(sshsession);
      ssh_free(sshsession);
    }

    if (sshbind)
    {
      ssh_bind_free(sshbind);
    }
  }

  void SSH::run(void)
  {
  }

  SSH::~SSH()
  {
    delete data;
  }
}