#include "DC/connect.h"

QHostAddress Connection::getPeerAddress(void)
{
    return peer_address_;
}
QHostAddress Connection::getPeerOriginalAddress(void)
{
    if (proxy.isActive())
    {
        return proxy.getSourceAddress();
    }
    return getPeerAddress();
}

QString Connection::getPeerFullAddressString(void)
{
    if (proxy.isActive())
    {
        return QStringLiteral("%1 via %2").arg(getPeerOriginalAddress().toString()).arg(getPeerAddress().toString());
    }
    else
    {
        return getPeerOriginalAddress().toString();
    }
}

void Connection::setPeerAddress(QHostAddress address)
{
    peer_address_ = address;
}

void Connection::setPeerPort(uint16_t port)
{
    peer_port_ = port;
}

bool Connection::isEditing(void) const noexcept
{
    return connected == Connection::states::EDITING ||
           connected == Connection::states::EDITING_V2 ||
           connected == Connection::states::WRITE_BOARD ||
           connected == Connection::states::EDIT_MPROG ||
           connected == Connection::states::SEND_MAIL ||
           connected == Connection::states::EXDSCR;
}
bool Connection::isPlaying(void) const noexcept
{
    return connected == Connection::states::PLAYING;
}
