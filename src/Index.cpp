#include "DC/Index.h"
#include "DC/dcstdio.h"
#include <QStringLiteral>
using namespace Qt::StringLiterals;

const QString mob_index_data::indexFilename = u"mobindex"_s;
QString mob_index_data::read_next_file(FILEPtr stream)
{
  return read_next_filename(stream, u"mobs"_s);
}

const QString obj_index_data::indexFilename = u"objectindex"_s;
QString obj_index_data::read_next_file(FILEPtr stream)
{
  return read_next_filename(stream, u"objects"_s);
}
