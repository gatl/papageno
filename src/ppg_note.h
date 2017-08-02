/* Copyright 2017 Florian Fleissner
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
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
 * @param n_inputs The number of inputs passed as array
 * @param inputs Inputpositions that represent the notes of the single note line
 * @returns The constructed token
 */
PPG_Token ppg_single_note_line(
                     PPG_Layer layer, 
                     PPG_Action action, 
                     PPG_Count n_inputs,
                     PPG_Input_Id inputs[]);

/** An alias for ppg_single_note_line
 */
#define ppg_sequence(...) ppg_single_note_line(__VA_ARGS__)

/** @brief Flags that configure note behavior
 * 
 * Use the functions ppg_token_set_flags and
 * ppg_token_get_flags to manipulated note flags.
 */
enum PPG_Note_Flags {
   PPG_Note_Flag_Match_Activation = (1 << 1), ///< Match activation of corresponding input
   PPG_Note_Flag_Match_Deactivation = (PPG_Note_Flag_Match_Activation << 1), ///< Match deactivation of corresponding input
   PPG_Note_Flags_A_N_D = PPG_Note_Flag_Match_Activation | PPG_Note_Flag_Match_Deactivation ///< Match activation and deactivation of corresponding input (only if compiled in pedantic mode)
}; 

/** @brief Generates a note token.
 *
 * Use this function to generate tokens that are passed to the ppg_pattern function
 * to generate complex patterns. 
 * 
 * @note Notes that are generated by this function must be passed to ppg_pattern
 *       to be effective
 * @note Use setter functions that operate on tokens to change attributes of the generated token 
 * 
 * @param input The input that is represented by the note
 * @param flags The note flags
 * @returns The constructed tokenis
 */
PPG_Token ppg_note_create(PPG_Input_Id input, PPG_Count flags);

/** @brief Generates a note token with standard behavior.
 *
 * Use this function to generate tokens that are passed to the ppg_pattern function
 * to generate complex patterns. 
 * 
 * This is a convenience function that yields the same result as
 * ppg_note_create(input, PPG_Note_Flags_A_N_D)
 * 
 * @note Notes that are generated by this function must be passed to ppg_pattern
 *       to be effective
 * @note Use setter functions that operate on tokens to change attributes of the generated token 
 * 
 * @param input The input that is represented by the note 
 * @returns The constructed tokenis
 */
PPG_Token ppg_note_create_standard(PPG_Input_Id input);

#endif
