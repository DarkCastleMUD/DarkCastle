#pragma once
#include <QString>
constexpr auto READ_SIZE = 256;
constexpr auto MAX_HELP_KEYWORD_LENGTH = 20;
constexpr auto MAX_HELP_RELATED_LENGTH = 60;
constexpr auto MAX_HELP_LENGTH = 8192;

class help_index_element_new
{
public:
  QString keyword1_;
  QString keyword2_;
  QString keyword3_;
  QString keyword4_;
  QString keyword5_;
  QString entry_;
  QString related_;
  QString min_level_;
};
