/**
 * @file uts.h
 * @brief Unit Test Service — registration macro, types, and API.
 *
 * Declares the descriptor type and `ADD_TEST` registration macro for the
 * self-contained on-target unit-test runner (see doc/SDD.md sec. 5.4). Tests
 * are ordinary `osStatus_t` functions written in the main source file of a unit
 * test project (see app/avrOS_test) and registered into the `TEST_TABLE` linker
 * section. The runner, `utsRun()`, is called directly from that project's
 * `main()` — it does not use the FSM scheduler and never returns.
 *
 * Created: 7/5/2026
 * Author: john anderson
 *
 * Copyright (C) 2021 by John Anderson <racerxr650r@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef UTS_H_
#define UTS_H_

/** @addtogroup unit_test_service
 * @{
 */

// Includes -------------------------------------------------------------------
#include <stdint.h>

// Data Types -----------------------------------------------------------------
/**
 * @brief Unit test function pointer type.
 *
 * A unit test takes no arguments and returns @c OS_OK on success or a negative
 * @ref osStatus_t code on failure. The runner prints the code's short
 * description in the result report.
 */
typedef osStatus_t (*utsTest_t)(void);

/**
 * @brief Flash-resident unit test descriptor (one per ADD_TEST instance).
 *
 * Placed in the `TEST_TABLE` linker section and walked in order by utsRun().
 */
typedef struct TEST_TYPE
{
	const char	*name;	///< Human-readable test name shown in the report.
	utsTest_t	func;	///< The test function to invoke.
} test_t;

// Registration Macro ---------------------------------------------------------
/**
 * @brief Register a unit test with the unit test service.
 *
 * Emits a `const` @ref test_t descriptor into the `TEST_TABLE` linker section
 * (see doc/SDD.md sec. 3.5), following the same pattern as the other `ADD_*`
 * macros. The test function itself is an ordinary function defined in the
 * application's main source file.
 *
 * Usage:
 * @code
 *   osStatus_t testQueuePutGet(void);
 *   ADD_TEST("queue put/get", testQueuePutGet);
 * @endcode
 *
 * @param testName  String literal shown in the report.
 * @param testFunc  The @ref utsTest_t test function.
 */
#define ADD_TEST(testName, testFunc)	\
		osStatus_t testFunc(void); \
		const static test_t SECTION(TEST_TABLE) CONCAT(testFunc,__COUNTER__) = {.name = testName, .func = &testFunc};

// External Functions ---------------------------------------------------------
/**
 * @brief Run all registered unit tests and halt.
 *
 * Called directly from `main()` in a unit test project (see app/avrOS_test) in
 * place of the FSM dispatch loop. Brings up the report UART with blocking,
 * polled output, walks the
 * `TEST_TABLE`, runs and reports each test (pass in green, fail in red, with the
 * returned code's description), prints a `Test Group [PASSED]`/`[FAIL]` summary,
 * sets @ref utsGroupResult, then enters an infinite loop. Never returns.
 */
void utsRun(void) __attribute__((noreturn));

/**
 * @brief Map an osStatus_t value to its short human-readable description.
 *
 * Returns the description text from the @ref osStatus_t typedef comments in
 * avrOS.h (e.g. `OS_OK` → "Success").
 *
 * @param status  The status code.
 * @return Pointer to a static description string.
 */
const char* utsStatusDescription(osStatus_t status);

/**
 * @brief Unit-test group result.
 *
 * Initialized to 0 (tests have not run). Set by utsRun() to 1 if every test
 * returned @c OS_OK, or -1 if any test failed. Exported so a debugger or
 * board-in-the-loop harness can read the outcome at the halt loop.
 */
extern volatile int8_t utsGroupResult;

/** @} */ // end of unit_test_service

#endif /* UTS_H_ */
