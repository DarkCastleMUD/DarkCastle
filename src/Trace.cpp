#include "Trace.h"



Trace::Trace(const std::string source)
{
    tracks.push_back(source);
}

Trace::~Trace()
{
    tracks.pop_back();
}

std::vector<std::string>& Trace::getTracks()
{
    return tracks;
}

void Trace::addTrack(std::string source)
{
    tracks.push_back(source);
}

std::ostream& operator<<(std::ostream &out, Trace& t)
{
    std::vector<std::string> tracks = t.getTracks();
    for (auto& track : tracks)
    {
        out << track << " ";
    }

    return out;
}