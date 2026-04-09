/*
 * CommandStack.cpp
 *
 *  Created on: May 26, 2020
 *      Author: Jared H. Hudson
 */
#include "DC/DC.h"
#include "DC/CommandStack.h"
#include "DC/levels.h"

quint32 CommandStack::depth = {};
quint32 CommandStack::max_depth = 5;
quint32 CommandStack::overflow_count = {};

CommandStack::CommandStack()
{
  depth++;

  if (isOverflow() == true)
  {
    overflow_count++;
  }
}

CommandStack::CommandStack(quint32 initial)
{
  depth = initial;
  overflow_count = {};
  depth++;

  if (isOverflow() == true)
  {
    overflow_count++;
  }
}

CommandStack::CommandStack(quint32 initial, quint32 max)
{
  depth = initial;
  max_depth = max;
  overflow_count = {};
  depth++;

  if (isOverflow() == true)
  {
    overflow_count++;
  }
}

CommandStack::~CommandStack()
{
  if (depth > 0)
  {
    depth--;
  }
  else
  {
    logf(IMMORTAL, DC::LogChannel::LOG_BUG, "Attempt to decrement CommandStack::depth when already 0");
    produce_coredump();
  }
}

// TODO detect invalid values
bool CommandStack::setDepth(quint32 value)
{
  depth = value;
  return true;
}

quint32 CommandStack::getDepth(void)
{
  return depth;
}

// TODO detect invalid values
bool CommandStack::setMax(quint32 value)
{
  max_depth = value;
  return true;
}

quint32 CommandStack::getMax(void)
{
  return max_depth;
}

bool CommandStack::isOverflow(void)
{
  if (depth > max_depth)
  {
    return true;
  }
  else
  {
    return false;
  }
}

quint32 CommandStack::getOverflowCount(void)
{
  return overflow_count;
}
