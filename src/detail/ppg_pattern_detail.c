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

#include "detail/ppg_pattern_detail.h"
#include "detail/ppg_token_detail.h"
#include "detail/ppg_context_detail.h"
#include "ppg_debug.h"

PPG_Token ppg_pattern_from_list(	
												PPG_Layer layer,
												PPG_Count n_tokens,
												PPG_Token tokens[])
{ 	
	PPG_Token__ *parent_token = &ppg_context->pattern_root;
	
	PPG_PRINTF("   %d members\n", n_tokens);
	
	for (PPG_Count i = 0; i < n_tokens; i++) { 
		
		PPG_Token__ *cur_token = (PPG_Token__*)tokens[i];
		
		PPG_Token__ *equivalent_successor 
			= ppg_token_get_equivalent_successor(parent_token, cur_token);
			
		PPG_PRINTF("   member %d: ", i);
		
		if(	equivalent_successor
			
			/* Only share interior nodes and ...
			 */
			&& equivalent_successor->successors
			/* ... only if the 
			 * newly registered node on the respective level is 
			 * not a leaf node.
			 */ 
			&& (i < (n_tokens - 1))
		) {
			
// 			#if DEBUG_PAPAGENO
// 			if(cur_token->action.type != equivalent_successor->action.type) {
// 				PPG_ERROR("Incompatible action types detected\n");
// 			}
// 			#endif
			
			PPG_PRINTF("already present: 0x%" PRIXPTR "\n", equivalent_successor);
			
			parent_token = equivalent_successor;
			
			if(layer < equivalent_successor->layer) {
				
				equivalent_successor->layer = layer;
			}
			
			/* The successor is already registered in the search tree. Delete the newly created version.
			 */			
			ppg_token_free(cur_token);
		}
		else {
			
			PPG_PRINTF("newly defined: 0x%" PRIXPTR "\n", cur_token);
			
			#if DEBUG_PAPAGENO
			
			/* Detect pattern ambiguities
			 */
			if(equivalent_successor) {
				if(	
					/* Melodies are ambiguous if ...
					 * the conflicting nodes/tokens are both leaf tokens
					 */
						(i == (n_tokens - 1))
					&&	!equivalent_successor->successors
					
					/* And defined for the same layer
					 */
					&& (equivalent_successor->layer == layer)
				) {
					PPG_ERROR("Conflicting melodies detected.\n")
					
					#if PAPAGENO_PRINT_SELF_ENABLED
					PPG_ERROR(
						"The tokens of the conflicting melodies are listed below.\n");
					
					PPG_ERROR("Previously defined:\n");
					ppg_recursively_print_pattern(equivalent_successor);
					
					PPG_ERROR("Conflicting:\n");
					for (PPG_Count i = 0; i < n_tokens; i++) {
						PPG_CALL_VIRT_METHOD(tokens[i], print_self);
					}
					#endif
				}
			}
			#endif /* if DEBUG_PAPAGENO */
					
			cur_token->layer = layer;
			
			ppg_token_add_successor(parent_token, cur_token);
			
			parent_token = cur_token;
		}
	}
	
	/* Return the leaf token 
	 */
	return parent_token;
}