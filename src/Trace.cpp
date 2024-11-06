#include "DC/Trace.h"
#include <QDebug>

Trace::Trace(QString source)
{
    tracks.push_back(source);
}

Trace::~Trace()
{
    tracks.pop_back();
}

const Trace::tracks_t &Trace::getTracks()
{
    return tracks;
}

void Trace::addTrack(QString source)
{
    tracks.push_back(source);
}

std::ostream &operator<<(std::ostream &out, const QString &str)
{
    out << str.toStdString().c_str();
    return out;
}
