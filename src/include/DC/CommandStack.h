/*
 * CommandStack.h
 *
 *  Created on: May 26, 2020
 *      Author: Jared H. Hudson
 */

#pragma once

#include <cstdint>

class CommandStack
{
public:
  CommandStack(void);
  CommandStack(quint32 initial);
  CommandStack(quint32 initial, quint32 max);
  ~CommandStack();
  bool setDepth(quint32 value);
  quint32 getDepth(void);
  bool setMax(quint32 value);
  quint32 getMax(void);
  bool isOverflow(void);
  quint32 getOverflowCount(void);

private:
  // Current depth in Command stack
  static quint32 depth;

  // Maximum depth before it's considered an overflow
  static quint32 max_depth;

  // How many times have we overflowed since last reset
  static quint32 overflow_count;
};
