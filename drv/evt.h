/**
 * @file evt.h
 * @brief Event system driver — inline accessors for the AVR-Dx EVSYS.
 *
 * Provides a thin, zero-overhead set of `static inline` functions that
 * manipulate the registers of the peripheral Event System (EVSYS). The AVR-Dx
 * has a single event system, so these functions operate directly on the global
 * `EVSYS` register set rather than on a caller-supplied pointer.
 *
 * @note This is the *hardware* event system, which routes events between
 *       peripherals without CPU involvement. It is unrelated to the software
 *       event manager in sys/event.h used to wake state machines.
 *
 * The EVSYS connects event *generators* to event *users* through eight
 * multiplexer *channels*:
 *  - A **channel** selects one generator source (a peripheral output, port pin,
 *    software event, ...). Configure with `evtSetChannelGenerator()` using the
 *    device's `EVSYS_CHANNELn_*_gc` constants for the matching channel.
 *  - A **user** is a peripheral event input. Each user register selects which
 *    channel drives it. Connect with `evtSetUser()`, passing the address of the
 *    desired `EVSYS.USER*` register.
 *  - `evtSoftwareEvent()` strobes a software event onto a channel whose
 *    generator is left off (or onto any channel) for CPU-initiated events.
 *
 * @note The per-channel generator constants are typed per channel
 *       (`EVSYS_CHANNEL0_t`, `EVSYS_CHANNEL1_t`, ...): even and odd channels
 *       expose slightly different source options (e.g. PIT divisions and the
 *       routable port). `evtSetChannelGenerator()` takes the raw `uint8_t`
 *       group code so any channel can be configured through one function; use
 *       the constant whose name matches the target channel.
 *
 * ### Interrupt safety
 *
 * All EVSYS registers are 8-bit and each holds a single field, so the channel,
 * user, and software-event accessors are plain full-register writes — not
 * read-modify-write — and carry no byte-pair (`TEMP`) hazard. The event system
 * is normally configured once at start-up. `evtSoftwareEvent()` is a strobe and
 * is safe to call at any time.
 *
 * The bit-mask (`_bm`) and group-code (`_gc`) symbols referenced here are
 * supplied by `<avr/io.h>` for the selected device.
 *
 * Created: 6/27/2026
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

#ifndef EVT_H_
#define EVT_H_

/** @addtogroup evt_driver
 * @{
 */

// Includes -------------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>

// Constants ------------------------------------------------------------------
#define EVT_CHANNEL_COUNT	8	///< Number of event multiplexer channels.

/**
 * @brief Generator group code that turns a channel off.
 *
 * Pass to `evtSetChannelGenerator()` (equivalent to `evtDisableChannel()`).
 */
#define EVT_GENERATOR_OFF	0x00

// Data Types -----------------------------------------------------------------
/**
 * @brief Event multiplexer channel selector.
 *
 * The AVR-Dx event system has eight channels; each routes one generator source
 * to any number of users.
 */
typedef enum
{
	EVT_CHANNEL0 = 0,	///< Event channel 0
	EVT_CHANNEL1 = 1,	///< Event channel 1
	EVT_CHANNEL2 = 2,	///< Event channel 2
	EVT_CHANNEL3 = 3,	///< Event channel 3
	EVT_CHANNEL4 = 4,	///< Event channel 4
	EVT_CHANNEL5 = 5,	///< Event channel 5
	EVT_CHANNEL6 = 6,	///< Event channel 6
	EVT_CHANNEL7 = 7	///< Event channel 7
} evtChannel_t;

// Inline Functions -----------------------------------------------------------
// Channel (generator) configuration ------------------------------------------
/**
 * @brief Route a generator source onto an event channel.
 *
 * Writes the selected channel's multiplexer register with a generator group
 * code. Use the `EVSYS_CHANNELn_*_gc` constant whose channel number matches
 * @p channel, since the available sources differ between even and odd channels.
 *
 * @param channel   Event channel to configure (`evtChannel_t`).
 * @param generator Generator group code (`EVSYS_CHANNELn_*_gc`), or
 *                  `EVT_GENERATOR_OFF`.
 */
static inline void evtSetChannelGenerator(evtChannel_t channel, uint8_t generator)
{
	(&EVSYS.CHANNEL0)[channel] = generator;
}

/**
 * @brief Read the generator currently routed onto a channel.
 *
 * @param channel Event channel to query (`evtChannel_t`).
 * @return The generator group code in the channel's multiplexer register.
 */
static inline uint8_t evtGetChannelGenerator(const evtChannel_t channel)
{
	return (&EVSYS.CHANNEL0)[channel];
}

/**
 * @brief Turn an event channel off.
 *
 * Writes the channel's multiplexer register to OFF so it produces no events.
 *
 * @param channel Event channel to disable (`evtChannel_t`).
 */
static inline void evtDisableChannel(evtChannel_t channel)
{
	(&EVSYS.CHANNEL0)[channel] = EVT_GENERATOR_OFF;
}

// User (consumer) configuration ----------------------------------------------
/**
 * @brief Connect an event user to a channel.
 *
 * Writes the user's select register so the peripheral input listens to
 * @p channel. The register value written is the channel number plus one, which
 * matches the device's `EVSYS_USER_CHANNELn_gc` encoding.
 *
 * @param user    Address of the user's select register (e.g.
 *                `&EVSYS.USERTCB0CAPT`).
 * @param channel Event channel to connect the user to (`evtChannel_t`).
 */
static inline void evtSetUser(volatile register8_t *user, evtChannel_t channel)
{
	*user = (uint8_t)(channel + 1);
}

/**
 * @brief Disconnect an event user from any channel.
 *
 * Writes the user's select register to OFF so the peripheral input receives no
 * events.
 *
 * @param user Address of the user's select register (e.g.
 *             `&EVSYS.USERTCB0CAPT`).
 */
static inline void evtClearUser(volatile register8_t *user)
{
	*user = EVSYS_USER_OFF_gc;
}

/**
 * @brief Read which channel an event user is connected to.
 *
 * Returns the raw user select-register value: `EVSYS_USER_OFF_gc` (0) when not
 * connected, otherwise the channel number plus one. Subtract one to recover the
 * `evtChannel_t` when the result is non-zero.
 *
 * @param user Address of the user's select register (e.g.
 *             `&EVSYS.USERTCB0CAPT`).
 * @return The user select-register value.
 */
static inline uint8_t evtGetUser(const volatile register8_t *user)
{
	return *user;
}

// Software events ------------------------------------------------------------
/**
 * @brief Strobe a software event onto a channel.
 *
 * Writes the SWEVENTA register to fire a one-shot software event on @p channel.
 * Any user connected to that channel receives the event. The bit self-clears
 * after the strobe.
 *
 * @param channel Event channel to strobe (`evtChannel_t`).
 */
static inline void evtSoftwareEvent(evtChannel_t channel)
{
	EVSYS.SWEVENTA = (uint8_t)(1 << channel);
}

/** @} */ // end of evt_driver

#endif /* EVT_H_ */
