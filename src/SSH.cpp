#include <iostream>

#include <libssh/callbacks.h>

#include "SSH.h"
#include "utility.h"

using namespace std;

void incoming_connection(ssh_bind_struct *sshbind, void *data)
{
  cerr << "incoming_connection called" << endl;
}

SSH::SSH()
{
  sshbind = ssh_bind_new();
  if (int rc = ssh_bind_options_parse_config(sshbind, NULL) < 0)
  {
    logf(100, LOG_BUG, "ssh_bind_options_parse_config returned %d", rc);
  }

  unsigned int bindport = 6922;
  if (int rc = ssh_bind_options_set(sshbind, ssh_bind_options_e::SSH_BIND_OPTIONS_BINDPORT, &bindport) < 0)
  {
    logf(100, LOG_BUG, "ssh_bind_options_set SSH_BIND_OPTIONS_BINDPORT returned %d", rc);
  }

  ssh_bind_set_blocking(sshbind, true);

  ssh_bind_callbacks_struct bcb = {
      .incoming_connection = incoming_connection};
  ssh_callbacks_init(&bcb);
  if (int rc = ssh_bind_set_callbacks(sshbind, &bcb, nullptr) != SSH_OK)
  {
    logf(100, LOG_BUG, "ssh_bind_set_callbacks returned %d", rc);
  }

  if (int rc = ssh_bind_listen(sshbind) < 0)
  {
    logf(100, LOG_BUG, "ssh_bind_listen returned %d", rc);
  }
}

SSH::~SSH()
{
}