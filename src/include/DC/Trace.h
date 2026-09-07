#pragma once
#include <qcontainerfwd.h> // for QVector
#include <qlist.h>         // for QList
#include <qstring.h>       // for QString, operator<<
#include <qtypes.h>        // for qsizetype
#include <QDebug>          // for operator<<
#include <iosfwd>          // for ostream

class Trace
{
public:
  typedef QVector<QString> tracks_t;
  Trace(QString source = "unknown");
  ~Trace();
  const tracks_t &getTracks();
  void addTrack(QString source);

private:
  tracks_t tracks;
};

std::ostream &operator<<(std::ostream &out, const QString &str);

auto &operator<<(auto &out, Trace &t)
{
  qsizetype track_index{};
  for (auto &track : t.getTracks())
  {
    out << track;
    if (++track_index != t.getTracks().length())
    {
      out << "->";
    }
  }
  return out;
}