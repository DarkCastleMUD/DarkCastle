#include "Trace.h"

using namespace std;

Trace::Trace(const string source)
{
    tracks.push_back(source);
}

Trace::~Trace()
{
    tracks.pop_back();
}

vector<string>& Trace::getTracks()
{
    return tracks;
}

void Trace::addTrack(string source)
{
    tracks.push_back(source);
}

std::ostream& operator<<(std::ostream &out, Trace& t)
{
    vector<string> tracks = t.getTracks();
    for (auto& track : tracks)
    {
        out << track << " ";
    }

    return out;
}