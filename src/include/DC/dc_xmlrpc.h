#pragma once
#include <XmlRpc.h>
#include <QtTypes>
using namespace XmlRpc;

XmlRpcServer *xmlrpc_init(qint32 xmlrpc_port);
