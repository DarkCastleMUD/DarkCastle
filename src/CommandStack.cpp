/*
 * CommandStack.cpp
 *
 *  Created on: May 26, 2020
 *      Author: Jared H. Hudson
 */

#include "DC/CommandStack.h"
#include "DC/player.h"	// LogChannels::LOG_BUG
#include "DC/levels.h"	// IMMORTAL
#include "DC/utility.h" // logf

uint32_t CommandStack::depth = 0;
uint32_t CommandStack::max_depth = 5;
uint32_t CommandStack::overflow_count = 0;

CommandStack::CommandStack()
{
	depth++;

	if (isOverflow() == true)
	{
		overflow_count++;
	}
}

CommandStack::CommandStack(uint32_t initial)
{
	depth = initial;
	overflow_count = 0;
	depth++;

	if (isOverflow() == true)
	{
		overflow_count++;
	}
}

CommandStack::CommandStack(uint32_t initial, uint32_t max)
{
	depth = initial;
	max_depth = max;
	overflow_count = 0;
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
		logf(IMMORTAL, LogChannels::LOG_BUG, "Attempt to decrement CommandStack::depth when already 0");
		produce_coredump();
	}
}

// TODO detect invalid values
bool CommandStack::setDepth(uint32_t value)
{
	depth = value;
	return true;
}

uint32_t CommandStack::getDepth(void)
{
	return depth;
}

// TODO detect invalid values
bool CommandStack::setMax(uint32_t value)
{
	max_depth = value;
	return true;
}

uint32_t CommandStack::getMax(void)
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

uint32_t CommandStack::getOverflowCount(void)
{
	return overflow_count;
}
