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

#ifndef PPG_EVENT_H
#define PPG_EVENT_H

/** @file */

#include "ppg_input.h"
#include "ppg_slots.h"
#include "ppg_time.h"
#include "ppg_layer.h"

enum PPG_Event_Flags
{
	PPG_Event_Flags_Empty = 0,
	
	PPG_Event_Active = (1 << 0),
	PPG_Event_Considered = (PPG_Event_Active << 1),
	PPG_Event_Control_Tag = (PPG_Event_Considered << 1),
	
	PPG_Event_Flags_All = (PPG_Event_Control_Tag << 1) - 1
};

/** @brief Specification of an input event
 */
typedef struct {
	PPG_Input_Id input; ///< The input identifier associated with the input that is pressed or released
	PPG_Time time; ///< The time at which the input event occured
	PPG_Count flags; ///< Whether the input is active
} PPG_Event;

/** @brief A callback function that is used during processing of stored input events
 * 
 * @param event A pointer to a PPG_Event struct that provides information 
 * 						about the input event
 * @param slot_id The slot identifier informs about the slot that triggered input event processing
 * @param user_data Optional user data
 * @returns A boolean value that decides about whether input event processing is continued (true) or aborted (false).
 */
typedef void (*PPG_Event_Processor_Fun)(PPG_Event *event,
														 void *user_data);

/** @brief This is the main entry function for input event processing.
 * 
 * @param event A pointer to an input event to process by papagenop
 * @returns If further input event processing by other input event processors is desired
 */
void ppg_event_process(PPG_Event *event);

/** @brief Call this function to actively flush any input events that are in cache.
 * 
 * This function is called internally when melodies complete, on abort and on timeout.
 * 
 * @param input_processor A custom input processor callback. If NULL the default processor registered by
 *                ppg_global_set_default_event_processor is used.
 * @param user_data Optional user data is passed on to the input processor callback
 */
void ppg_event_buffer_iterate(
								PPG_Event_Processor_Fun input_processor,
								void *user_data);

#endif
