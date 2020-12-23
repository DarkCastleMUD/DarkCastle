/*
 * CommandStack.h
 *
 *  Created on: May 26, 2020
 *      Author: Jared H. Hudson
 */

#ifndef COMMANDSTACK_H_
#define COMMANDSTACK_H_

#include <stdint.h>

class CommandStack
{
public:
	CommandStack(void);
	CommandStack(uint32_t initial);
	CommandStack(uint32_t initial, uint32_t max);
	~CommandStack();
	bool setDepth(uint32_t value);
	uint32_t getDepth(void);
	bool setMax(uint32_t value);
	uint32_t getMax(void);
	bool isOverflow(void);
	uint32_t getOverflowCount(void);

private:
	// Current depth in Command stack
	static uint32_t depth;

	// Maximum depth before it's considered an overflow
	static uint32_t max_depth;

	// How many times have we overflowed since last reset
	static uint32_t overflow_count;
};

#endif /* SRC_COMMANDSTACK_H_ */
