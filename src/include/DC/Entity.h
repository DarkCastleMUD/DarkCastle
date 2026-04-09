#pragma once
#include <QObject>

template <typename T>
class DataOperations
{
public:
  T data_;
  DataOperations(T data) : data_(data) {}
  [[nodiscard]] inline operator T() const { return data_; }
};
