#include "DC/Trace.h"
#include <QDebug>

Trace::Trace(const std::string source)
{
    tracks.push_back(source);
}

Trace::~Trace()
{
    tracks.pop_back();
}

const std::vector<std::string> &Trace::getTracks()
{
    return tracks;
}

void Trace::addTrack(std::string source)
{
    tracks.push_back(source);
}