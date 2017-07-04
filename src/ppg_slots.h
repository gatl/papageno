/* Copyright 2017 Florian Fleissner
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PPG_SLOTS_H
#define PPG_SLOTS_H

/** @file */

#include <inttypes.h>

/** @brief Input processing slot identifiers
 * 
 * Slot identifiers are used during flushing input events to 
 * distinguish the different reasons for flushing
 */
enum PPG_Slots {
	PPG_On_Abort = 0,
	PPG_On_Timeout,
	PPG_On_Token_Matches,
	PPG_On_Pattern_Matches,
	PPG_On_User
};

/** @brief The data type used to identify slots
 */
typedef uint8_t PPG_Slot_Id;

#endif
