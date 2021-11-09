#ifndef TRACE_H
#define TRACE_H
#include <string>
#include <vector>
#include <sstream>
#include <fstream>

class Trace
{
    public:
    Trace(std::string source = "unknown");
    ~Trace();
    std::vector<std::string>& getTracks();
    void addTrack(std::string source);

    private:
    std::vector<std::string> tracks;

};

std::ostream& operator<<(std::ostream &out, Trace& t);

#endif