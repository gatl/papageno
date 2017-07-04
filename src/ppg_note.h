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

#ifndef PPG_NOTE_H
#define PPG_NOTE_H

/** @file */

#include "ppg_token.h"
#include "ppg_action.h"
#include "ppg_input.h"
#include "ppg_layer.h"

/** @brief Defines a stand alone pattern that consists of single notes.
 * 
 * @param layer The layer the pattern is associated with
 * @param action The action that is supposed to be carried out if the pattern matches
 * @param inputs Inputpositions that represent the notes of the single note line
 * @returns The constructed token
 */
PPG_Token ppg_single_note_line(
							PPG_Layer layer, 
							PPG_Action action, 
							PPG_Count n_inputs,
							PPG_Input inputs[]);

/** An alias for ppg_single_note_line
 */
#define ppg_sequence(...) ppg_single_note_line(__VA_ARGS__)

/** @brief Generates a note token.
 *
 * Use this function to generate tokens that are passed to the ppg_pattern function
 * to generate complex patterns. 
 * 
 * @note Notes that are generated by this function must be passed to ppg_pattisern
 * 		to be effective
 * @note Use setter functions that operate on tokens to change attributes of the generated token 
 * 
 * @param input The input that is represented by the note
 * @returns The constructed tokenis
 */
PPG_Token ppg_create_note(PPG_Input input);

#endif
