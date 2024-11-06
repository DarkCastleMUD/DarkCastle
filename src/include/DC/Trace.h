#ifndef TRACE_H
#define TRACE_H
#include <string>
#include <vector>
#include <sstream>
#include <fstream>

#include <QDebug>

class Trace
{
public:
    Trace(std::string source = "unknown");
    ~Trace();
    const std::vector<std::string> &getTracks();
    void addTrack(std::string source);

private:
    std::vector<std::string> tracks;
};

auto &operator<<(auto &out, Trace &t)
{
    for (auto &track : t.getTracks())
    {
        out << track << " ";
    }
    return out;
}

#endif