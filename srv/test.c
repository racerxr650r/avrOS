/**
 * @brief Test service
 * @details This file implements the test manager CLI command and API functions
 *
 * @date 6/23/2024
 * @author john anderson
 * @copyright Copyright (C) 2024 by John Anderson <racerxr650r@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
// Includes -------------------------------------------------------------------
#include "../avrOS.h"

// Private functions prototypes ----------------------------------------------

// Private Globals ------------------------------------------------------------

// Start and end of the linker assembled array of cliCommand_t structures
extern void *__start_TEST_TABLE;
extern void *__stop_TEST_TABLE;

// CLI Commands ---------------------------------------------------------------
#ifdef TEST_CLI
// Command test
ADD_COMMAND("test",testRun);
#endif // TEST_CLI

// Implement the test CLI command
int testRun(int argC, char *argV[])
{
	testUnit_t *test = (testUnit_t *)&__start_TEST_TABLE;

	// If no group or test name provided...
	if(argC == 1)
	{
		for(; test < (testUnit_t *)&__stop_TEST_TABLE; ++test)
	}
	// Else if a group or test name provided...
	else if(argC > 1)
	{

	}

	return(0);
}

void testPrintResult(FILE *filePtr)
{
	static char resultString =
}