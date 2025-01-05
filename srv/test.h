/**
 * @brief Test service
 * @details This header file implements the test manager macros and datatypes
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
#ifndef __TEST_H
#define __TEST_H
// Includes -------------------------------------------------------------------
#include "../avrOS.h"

// Types ----------------------------------------------------------------------
typedef char (*testHandler_t)();

typedef enum
{
    UNTESTED,
    PASS,
    FAIL,
    ERROR
}testResult_t;

typedef struct
{
    char            *name, *group;
    testResult_t    *result;
    testHandler_t   testFuncPtr;
}testUnit_t;

// Macros ---------------------------------------------------------------------
// This macro adds a new unit test
#define ADD_TEST(testFunc) \
        testResult_t CONCAT(testFunc,_result);
        const static SECTION(TEST_TABLE) CONCAT(testFunc,__COUNTER__) = { .name = #testFunc, .group = #__FILE__ , .result = &CONCAT(testFunc,_result), .funcPtr = &testFunc};

// This macro implements a test assert
#define testAssert(expr, descr) \
        if (expr) \
            {} \
        else \
            testFailed(descr)

// External Functions ---------------------------------------------------------
external int testRun(int argC, char *argV[]);

#endif // __TEST_H