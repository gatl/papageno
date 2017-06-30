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

#include "papageno.h"

#include <stdlib.h>
#include <stddef.h>
#include <inttypes.h>
#include <stdarg.h>

#define PPG_MAX_KEYCHANGES 100

#ifdef DEBUG_PAPAGENO

#if PAPAGENO_PRINT_SELF_ENABLED
#define PAPAGENO_PRINT_SELF_ENABLED 1
#endif

#ifdef __AVR_ATmega32U4__ 
#include "debug.h"
#define PPG_PRINTF(...) \
	uprintf(__VA_ARGS__);
#define PPG_ERROR(...) PPG_PRINTF("*** Error: " __VA_ARGS__)
#else
#define PPG_PRINTF(...)
#define PPG_ERROR(...)
#endif
	
#define PPG_ASSERT(...) \
	if(!(__VA_ARGS__)) { \
		PPG_ERROR("Assertion failed: " #__VA_ARGS__ "\n"); \
	}

#else //DEBUG_PAPAGENO

#ifndef PAPAGENO_PRINT_SELF_ENABLED
#define PAPAGENO_PRINT_SELF_ENABLED 0
#endif

#define PPG_PRINTF(...)
#define PPG_ERROR(...)

#define PPG_ASSERT(...)

#endif //DEBUG_PAPAGENO

#define PPG_CALL_VIRT_METHOD(THIS, METHOD, ...) \
	THIS->vtable->METHOD(THIS, ##__VA_ARGS__);

static bool ppg_key_id_simple_equal(PPG_Key_Id kp1, PPG_Key_Id kp2)
{
	return 	kp1 == kp2;
}

struct PPG_PhraseStruct;

typedef uint8_t (*PPG_Phrase_Consider_Keychange_Fun)(	
														struct PPG_PhraseStruct *a_This, 
														PPG_Key_Event *key_event,
														uint8_t cur_layer
																	);

typedef uint8_t (*PPG_Phrase_Successor_Consider_Keychange_Fun)(
														struct PPG_PhraseStruct *a_This, 
														PPG_Key_Event *key_event
 													);

typedef void (*PPG_Phrase_Reset_Fun)(	struct PPG_PhraseStruct *a_This);

typedef bool (*PPG_Phrase_Trigger_Action_Fun)(	struct PPG_PhraseStruct *a_This, uint8_t slot_id);

typedef struct PPG_PhraseStruct * (*PPG_Phrase_Destroy_Fun)(struct PPG_PhraseStruct *a_This);

typedef bool (*PPG_Phrase_Equals_Fun)(struct PPG_PhraseStruct *p1, struct PPG_PhraseStruct *p2);

#if PAPAGENO_PRINT_SELF_ENABLED
typedef void (*PPG_Phrase_Print_Self_Fun)(struct PPG_PhraseStruct *p);
#endif

typedef struct {
	
	PPG_Phrase_Consider_Keychange_Fun 
									consider_key_event;
									
	PPG_Phrase_Successor_Consider_Keychange_Fun 
									successor_consider_key_event;
									
	PPG_Phrase_Reset_Fun
									reset;
									
	PPG_Phrase_Trigger_Action_Fun
									trigger_action;
									
	PPG_Phrase_Destroy_Fun	
									destroy;
			
	PPG_Phrase_Equals_Fun
									equals;
									
	#if PAPAGENO_PRINT_SELF_ENABLED
	PPG_Phrase_Print_Self_Fun
									print_self;
	#endif								
} PPG_Phrase_Vtable;

typedef struct PPG_PhraseStruct {

	PPG_Phrase_Vtable *vtable;
	
	struct PPG_PhraseStruct *parent;
	
	struct PPG_PhraseStruct **successors;
	
	uint8_t n_allocated_successors;
	uint8_t n_successors;
	
	PPG_Action action;
	
	uint8_t state;
	
	uint8_t layer;
	 
} PPG_Phrase__;

typedef struct
{
	uint16_t n_key_events;
	PPG_Key_Event key_events[PPG_MAX_KEYCHANGES];

	PPG_Phrase__ melody_root;
	bool melody_root_initialized;
	bool process_actions_if_aborted;

	PPG_Phrase__ *current_phrase;

	bool papageno_enabled;
	bool papageno_temporarily_disabled;

	PPG_Key abort_key;

	PPG_Time time_last_keypress;

	PPG_Time keypress_timeout;
	
	PPG_Key_Id_Equal_Fun key_id_equal;
	
	PPG_Key_Event_Processor_Fun key_processor;
	
	PPG_Time_Fun time;
	PPG_Time_Difference_Fun time_difference;
	PPG_Time_Comparison_Fun time_comparison;
  
} PPG_Context;

static void ppg_phrase_free_successors(PPG_Phrase__ *a_This);
static bool ppg_recurse_and_process_actions(uint8_t slot_id);

static void ppg_default_time(PPG_Time *time)
{
	*time = 0;
}

static void ppg_default_time_difference(PPG_Time time1, PPG_Time time2, PPG_Time *delta)
{
	*delta = 0;
}

int8_t ppg_default_time_comparison(
								PPG_Time time1,
								PPG_Time time2)
{
	return 0;
}

static void ppg_init_key(PPG_Key *key)
{
	key->key_id = (PPG_Key_Id)((uint16_t)-1);
	key->check_active = NULL;
}

static void ppg_initialize_context(PPG_Context *context) {
	
	PPG_PRINTF("Initializing context\n");
	
	context->n_key_events = 0;
	context->melody_root_initialized = false;
	context->process_actions_if_aborted = false;
	context->current_phrase = NULL;
	context->papageno_enabled = true;
	context->papageno_temporarily_disabled = false;
	ppg_init_key(&context->abort_key);
	context->keypress_timeout = NULL;
	context->key_id_equal = (PPG_Key_Id_Equal_Fun)ppg_key_id_simple_equal;
	context->time = ppg_default_time;
	context->time_difference = ppg_default_time_difference;
	context->time_comparison = ppg_default_time_comparison;
};

static PPG_Context *ppg_context = NULL;

void* ppg_create_context(void)
{
	PPG_PRINTF("Creating new context\n");
	
	PPG_Context *context = (PPG_Context *)malloc(sizeof(PPG_Context));
	
	ppg_initialize_context(context);
	
	return context;
}

#ifndef PAPAGENO_DISABLE_CONTEXT_SWITCHING

void ppg_destroy_context(void *context_void)
{
	PPG_Context *context = (PPG_Context *)context_void;
	
	ppg_phrase_free_successors(&context->melody_root);
	
	free(context);
}

void* ppg_set_context(void *context_void)
{
	void *old_context = ppg_context;
	
	ppg_context = (PPG_Context *)context_void;
	
	return old_context;
}

void* ppg_get_current_context(void)
{
	return ppg_context;
}

#endif

enum {
	PPG_Phrase_In_Progress = 0,
	PPG_Phrase_Completed,
	PPG_Phrase_Invalid,
	PPG_Melody_Completed
};

static void ppg_store_key_event(PPG_Key_Event *key_event)
{
	PPG_ASSERT(ppg_context->n_key_events < PPG_MAX_KEYCHANGES);
	
	ppg_context->key_events[ppg_context->n_key_events] = *key_event;
	
	++ppg_context->n_key_events;
}

static void ppg_phrase_store_action(PPG_Phrase__ *phrase, 
											  PPG_Action action)
{
	phrase->action = action; 
	
	PPG_PRINTF("Action of phrase 0x%" PRIXPTR ": %u\n",
				  phrase, (uint16_t)phrase->action.user_callback.user_data);
}

void ppg_flush_stored_key_events(
								uint8_t slot_id, 
								PPG_Key_Event_Processor_Fun key_processor,
								void *user_data)
{
	if(ppg_context->n_key_events == 0) { return; }
	
	PPG_Key_Event_Processor_Fun kp	=
		(key_processor) ? key_processor : ppg_context->key_processor;
	
	if(!kp) { return; }
	
	/* Make sure that no recursion is possible if the key processor 
	 * invokes ppg_process_key_event 
	 */
	ppg_context->papageno_temporarily_disabled = true;
	
	for(uint16_t i = 0; i < ppg_context->n_key_events; ++i) {
		
		if(!kp(&ppg_context->key_events[i], slot_id, user_data)) { 
			break;
		}
	}
	
	ppg_context->papageno_temporarily_disabled = false;
}

static void ppg_delete_stored_key_events(void)
{
	ppg_context->n_key_events = 0;
}

static void ppg_phrase_reset(	PPG_Phrase__ *phrase)
{
	phrase->state = PPG_Phrase_In_Progress;
}

/* Phrases have states. This method resets the states
 * after processing a melody. On our way through the
 * melody tree we only need to reset thoses phrases
 * that were candidates.
 */
static void ppg_phrase_reset_successors(PPG_Phrase__ *phrase)
{
	/* Reset all successor phrases if there are any
	 */
	for(uint8_t i = 0; i < phrase->n_successors; ++i) {
		PPG_CALL_VIRT_METHOD(phrase->successors[i], reset);
	}
}
	
void ppg_abort_magic_melody(void)
{		
	if(!ppg_context->current_phrase) { return; }
	
// 	PPG_PRINTF("Aborting magic melody\n");
	
	/* The frase could not be parsed. Reset any successor phrases.
	*/
	ppg_phrase_reset_successors(ppg_context->current_phrase);
	
	if(ppg_context->process_actions_if_aborted) {
		ppg_recurse_and_process_actions(PPG_On_Abort);
	}
	
	/* Cleanup and issue all keypresses as if they happened without parsing a melody
	*/
	ppg_flush_stored_key_events(	PPG_On_Abort, 
										NULL /* key_processor */, 
										NULL /* user data */);
	
	ppg_delete_stored_key_events();
	
	ppg_context->current_phrase = NULL;
}

// Returns true if an action was triggered
//
static bool ppg_phrase_trigger_action(PPG_Phrase__ *phrase, uint8_t slot_id) {
	
	/* Actions of completed subphrases have already been triggered during 
	 * melody processing.
	 */
	if(	(slot_id != PPG_On_Subphrase_Completed)
		&&	(phrase->action.flags & PPG_Action_Immediate)) {
		return false;
	}
			
	if(phrase->action.user_callback.func) {

// 		PPG_PRINTF("Action of phrase 0x%" PRIXPTR ": %u\n",
// 				  phrase, (uint16_t)phrase->action.user_callback.user_data);
		
		PPG_PRINTF("*\n");
	
		phrase->action.user_callback.func(
			slot_id,
			phrase->action.user_callback.user_data);
		
		return true;
	}
	
	return false;
}

/* Returns if an action has been triggered.
 */
static bool ppg_recurse_and_process_actions(uint8_t slot_id)
{			
	if(!ppg_context->current_phrase) { return false; }
	
// 	PPG_PRINTF("Triggering action of most recent phrase\n");
	
	PPG_Phrase__ *cur_phrase = ppg_context->current_phrase;
	
	while(cur_phrase) {
		
		bool action_present = ppg_phrase_trigger_action(cur_phrase, slot_id);
		
		if(action_present) {
			if(cur_phrase->action.flags &= PPG_Action_Fall_Through) {
				cur_phrase = cur_phrase->parent;
			}
			else {
				return true;
			}
		}
		else {
			if(cur_phrase->action.flags &= PPG_Action_Fall_Back) {
				cur_phrase = cur_phrase->parent;
			}
			else {
				return false;
			}
		}
	}
	
	PPG_PRINTF("Done\n");
	
	return false;
}

static void ppg_on_timeout(void)
{
	if(!ppg_context->current_phrase) { return; }
	
	/* The frase could not be parsed. Reset any previous phrases.
	*/
	ppg_phrase_reset_successors(ppg_context->current_phrase);
	
	/* It timeout was hit, we either trigger the most recent action
	 * (e.g. necessary for tap dances) or flush the keyevents
	 * that happend until this point
	 */
	
	bool action_triggered 
		= ppg_recurse_and_process_actions(PPG_On_Timeout);
	
	/* Cleanup and issue all keypresses as if they happened without parsing a melody
	*/
	if(!action_triggered) {
		ppg_flush_stored_key_events(	PPG_On_Timeout, 
											NULL /* key_processor */, 
											NULL /* user data */);
	}
	
	ppg_delete_stored_key_events();
	
	ppg_context->current_phrase = NULL;
}

static uint8_t ppg_phrase_consider_key_event(	
												PPG_Phrase__ **current_phrase,
												PPG_Key_Event *key_event,
												uint8_t cur_layer
 														) 
{
	/* Loop over all phrases and inform them about the 
	 * key_event 
	 */
	bool all_successors_invalid = true;
	
	PPG_Phrase__ *a_current_phrase = *current_phrase;
	
	ppg_store_key_event(key_event);
	
// 	PPG_PRINTF("Processing key\n");
	
// 	#if DEBUG_PAPAGENO
// 	if(key_event->pressed) {
// 		PPG_PRINTF("v");
// 	}
// 	else {
// 		PPG_PRINTF("^");
// 	}
// 	#endif
	
	bool any_phrase_completed = false;
		
	/* Pass the keypress to the phrases and let them decide it they 
	 * can use it. If a phrase cannot it becomes invalid and is not
	 * processed further on this node level. This speeds up processing.
	 */
	for(uint8_t i = 0; i < a_current_phrase->n_successors; ++i) {
		
		// PPG_CALL_VIRT_METHOD(a_current_phrase->successors[i], print_self);
		
		/* Accept only paths through the search tree whose
		 * nodes' cur_layer tags are lower or equal the current cur_layer
		 */
		if(a_current_phrase->successors[i]->layer > cur_layer) { continue; }
		
		if(a_current_phrase->successors[i]->state == PPG_Phrase_Invalid) {
			continue;
		}
		
		uint8_t successor_process_result 
			= a_current_phrase->successors[i]
					->vtable->successor_consider_key_event(	
								a_current_phrase->successors[i], 
								key_event
						);
								
		switch(successor_process_result) {
			
			case PPG_Phrase_In_Progress:
				all_successors_invalid = false;
				
				break;
				
			case PPG_Phrase_Completed:
				
				all_successors_invalid = false;
				any_phrase_completed = true;

				break;
			case PPG_Phrase_Invalid:
// 				PPG_PRINTF("Phrase invalid");
				break;
		}
	}
	
	/* If all successors are invalid, the keystroke chain does not
	 * match any defined melody.
	 */
	if(all_successors_invalid) {
					
		return PPG_Phrase_Invalid;
	}
	
	/* If any phrase completed we have to find the most suitable one
	 * and either terminate melody processing if the matching successor
	 * is a leaf node, or replace the current phrase to await further 
	 * keystrokes.
	 */
	else if(any_phrase_completed) {
		
		int8_t highest_layer = -1;
		int8_t match_id = -1;
		
		/* Find the most suitable phrase with respect to the current cur_layer.
		 */
		for(uint8_t i = 0; i < a_current_phrase->n_successors; ++i) {
		
// 			PPG_CALL_VIRT_METHOD(a_current_phrase->successors[i], print_self);
		
			/* Accept only paths through the search tree whose
			* nodes' cur_layer tags are lower or equal the current cur_layer
			*/
			if(a_current_phrase->successors[i]->layer > cur_layer) { continue; }
			
			if(a_current_phrase->successors[i]->state != PPG_Phrase_Completed) {
				continue;
			}
			
			if(a_current_phrase->successors[i]->layer > highest_layer) {
				highest_layer = a_current_phrase->successors[i]->layer;
				match_id = i;
			}
		}
		
		PPG_ASSERT(match_id >= 0);
				
		/* Cleanup successors of the current node for further melody processing.
		 */
		ppg_phrase_reset_successors(a_current_phrase);
		
		/* Replace the current phrase. From this point on
		 * a_current_phrase references the child phrase
		*/
		*current_phrase = a_current_phrase->successors[match_id];
		a_current_phrase = *current_phrase;
		
		if(a_current_phrase->action.flags & PPG_Action_Immediate) {
			ppg_phrase_trigger_action(a_current_phrase, PPG_On_Subphrase_Completed);
		}
			
		if(0 == a_current_phrase->n_successors) {
			
			return PPG_Melody_Completed;
		}
		else {
			
			/* The melody is still in progress. We continue with the 
			 * new current node as soon as the next keystroke happens.
			 */
			return PPG_Phrase_In_Progress;
		}
	}
	
	return PPG_Phrase_In_Progress;
}

static void ppg_phrase_allocate_successors(PPG_Phrase__ *a_This, uint8_t n_successors) {

	 a_This->successors 
		= (struct PPG_PhraseStruct **)malloc(n_successors*sizeof(struct PPG_PhraseStruct*));
	 a_This->n_allocated_successors = n_successors;
}

static void ppg_phrase_grow_successors(PPG_Phrase__ *a_This) {

	if(a_This->n_allocated_successors == 0) {
		
		ppg_phrase_allocate_successors(a_This, 1);
	}
	else {
		PPG_Phrase__ **oldSucessors = a_This->successors;
		
		ppg_phrase_allocate_successors(a_This, 2*a_This->n_allocated_successors);
			
		for(uint8_t i = 0; i < a_This->n_successors; ++i) {
			a_This->successors[i] = oldSucessors[i];
		}
		
		free(oldSucessors); 
	}
}

static void ppg_phrase_add_successor(PPG_Phrase__ *phrase, PPG_Phrase__ *successor) {
	
	if(phrase->n_allocated_successors == phrase->n_successors) {
		ppg_phrase_grow_successors(phrase);
	}
	
	phrase->successors[phrase->n_successors] = successor;
	
	successor->parent = phrase;
	
	++phrase->n_successors;
}

static void ppg_phrase_free(PPG_Phrase__ *phrase);

static void ppg_phrase_free_successors(PPG_Phrase__ *phrase)
{
	if(!phrase->successors) { return; }
	
	for(uint8_t i = 0; i < phrase->n_allocated_successors; ++i) {
		
		ppg_phrase_free(phrase->successors[i]);
	}
	
	free(phrase->successors);
	
	phrase->successors = NULL;
	phrase->n_allocated_successors = 0;
}

static PPG_Phrase__* ppg_phrase_destroy(PPG_Phrase__ *phrase) {

	ppg_phrase_free_successors(phrase);
	
	return phrase;
}

static bool ppg_phrase_equals(PPG_Phrase__ *p1, PPG_Phrase__ *p2) 
{
	if(p1->vtable != p2->vtable) { return false; }
	
	return p1->vtable->equals(p1, p2);
}

static void ppg_phrase_free(PPG_Phrase__ *phrase) {
	
	PPG_CALL_VIRT_METHOD(phrase, destroy);

	free(phrase);
}

static PPG_Phrase__* ppg_phrase_get_equivalent_successor(
														PPG_Phrase__ *parent_phrase,
														PPG_Phrase__ *sample) {
	
	if(parent_phrase->n_successors == 0) { return NULL; }
	
	for(uint8_t i = 0; i < parent_phrase->n_successors; ++i) {
		if(ppg_phrase_equals(
											parent_phrase->successors[i], 
											sample)
		  ) {
			return parent_phrase->successors[i];
		}
	}
	
	return NULL;
}

#if PAPAGENO_PRINT_SELF_ENABLED
static void ppg_phrase_print_self(PPG_Phrase__ *p)
{
	PPG_PRINTF("phrase (0x%" PRIXPTR ")\n", (void*)p);
	PPG_PRINTF("   parent: 0x%" PRIXPTR "\n", (void*)&p->parent);
	PPG_PRINTF("   successors: 0x%" PRIXPTR "\n", (void*)&p->successors);
	PPG_PRINTF("   n_allocated_successors: %d\n", p->n_allocated_successors);
	PPG_PRINTF("   n_successors: %d\n", p->n_successors);
	PPG_PRINTF("   action.flags: %d\n", p->action.flags);
	PPG_PRINTF("   action_user_func: %0x%" PRIXPTR "\n", p->action.user_callback.func);
	PPG_PRINTF("   action_user_data: %0x%" PRIXPTR "\n", p->action.user_callback.user_data);
	PPG_PRINTF("   state: %d\n", p->state);
	PPG_PRINTF("   layer: %d\n", p->layer);
}
#endif

static PPG_Phrase_Vtable ppg_phrase_vtable =
{
	.consider_key_event 
		= (PPG_Phrase_Consider_Keychange_Fun) ppg_phrase_consider_key_event,
	.successor_consider_key_event 
		= NULL,
	.reset 
		= (PPG_Phrase_Reset_Fun) ppg_phrase_reset,
	.trigger_action 
		= (PPG_Phrase_Trigger_Action_Fun) ppg_phrase_trigger_action,
	.destroy 
		= (PPG_Phrase_Destroy_Fun) ppg_phrase_destroy,
	.equals
		= NULL
	
	#if PAPAGENO_PRINT_SELF_ENABLED
	,
	.print_self
		= (PPG_Phrase_Print_Self_Fun) ppg_phrase_print_self,
	#endif
};

static PPG_Phrase__ *ppg_phrase_new(PPG_Phrase__ *phrase) {
	
    phrase->vtable = &ppg_phrase_vtable;
	 
    phrase->parent = NULL;
    phrase->successors = NULL;
	 phrase->n_allocated_successors = 0;
    phrase->n_successors = 0;
    phrase->action.flags = PPG_Action_Default;
    phrase->action.user_callback.func = NULL;
    phrase->action.user_callback.user_data = NULL;
    phrase->state = PPG_Phrase_In_Progress;
    phrase->layer = 0;
	 
    return phrase;
}

/*****************************************************************************
 Notes 
*****************************************************************************/

typedef struct {
	
	PPG_Phrase__ phrase_inventory;
	 
	PPG_Key key;
	
	bool active;
	 
} PPG_Note;

static uint8_t ppg_note_successor_consider_key_event(	
											PPG_Note *note,
											PPG_Key_Event *key_event) 
{	
	PPG_ASSERT(note->phrase_inventory.state == PPG_Phrase_In_Progress);
	
	/* Set state appropriately 
	 */
	if(ppg_context->key_id_equal(note->key.key_id, key_event->key_id)) {
		
		bool key_active 
			= note->key.check_active(note->key.key_id, key_event->state);
			
		if(key_active) {
			note->active = true;
			note->phrase_inventory.state = PPG_Phrase_In_Progress;
		}
		else {
			if(note->active) {
	// 		PPG_PRINTF("note successfully finished\n");
				PPG_PRINTF("N");
				note->phrase_inventory.state = PPG_Phrase_Completed;
			}
		}
	}
	else {
// 		PPG_PRINTF("note invalid\n");
		note->phrase_inventory.state = PPG_Phrase_Invalid;
	}
	
	return note->phrase_inventory.state;
}

static void ppg_note_reset(PPG_Note *note) 
{
	ppg_phrase_reset((PPG_Phrase__*)note);
	
	note->active = false;
}

static PPG_Note *ppg_note_alloc(void) {
    return (PPG_Note*)malloc(sizeof(PPG_Note));
}

static bool ppg_note_equals(PPG_Note *n1, PPG_Note *n2) 
{
	return ppg_context->key_id_equal(n1->key.key_id, n2->key.key_id);
}

#if PAPAGENO_PRINT_SELF_ENABLED
static void ppg_note_print_self(PPG_Note *p)
{
	ppg_phrase_print_self((PPG_Phrase__*)p);
	
	PPG_PRINTF("note\n");
	PPG_PRINTF("   key_id: %d\n", p->key.key_id);
	PPG_PRINTF("   active: %d\n", p->active);
}
#endif

static PPG_Phrase_Vtable ppg_note_vtable =
{
	.consider_key_event 
		= (PPG_Phrase_Consider_Keychange_Fun) ppg_phrase_consider_key_event,
	.successor_consider_key_event 
		= (PPG_Phrase_Successor_Consider_Keychange_Fun) ppg_note_successor_consider_key_event,
	.reset 
		= (PPG_Phrase_Reset_Fun) ppg_note_reset,
	.trigger_action 
		= (PPG_Phrase_Trigger_Action_Fun) ppg_phrase_trigger_action,
	.destroy 
		= (PPG_Phrase_Destroy_Fun) ppg_phrase_destroy,
	.equals
		= (PPG_Phrase_Equals_Fun) ppg_note_equals
		
	#if PAPAGENO_PRINT_SELF_ENABLED
	,
	.print_self
		= (PPG_Phrase_Print_Self_Fun) ppg_note_print_self
	#endif
};

static PPG_Note *ppg_note_new(PPG_Note *note)
{
    /* Explict call to parent constructor 
	  */
    ppg_phrase_new((PPG_Phrase__*)note);

    note->phrase_inventory.vtable = &ppg_note_vtable;
	 
	 ppg_init_key(&note->key);
	 
    return note;
}

/*****************************************************************************
 Chords 
*****************************************************************************/

typedef struct {
	
	PPG_Phrase__ phrase_inventory;
	 
	uint8_t n_members;
	PPG_Key *keys;
	bool *member_active;
	uint8_t n_keys_active;
	 
} PPG_Aggregate;

typedef PPG_Aggregate PPG_Chord;

static uint8_t ppg_chord_successor_consider_key_event(	
											PPG_Chord *chord,
											PPG_Key_Event *key_event) 
{
	bool key_part_of_chord = false;
	
	PPG_ASSERT(chord->n_members != 0);
	
	PPG_ASSERT(chord->phrase_inventory.state == PPG_Phrase_In_Progress);
	
	/* Check it the key is part of the current chord 
	 */
	for(uint8_t i = 0; i < chord->n_members; ++i) {
		
		if(ppg_context->key_id_equal(chord->keys[i].key_id, key_event->key_id)) {
			
			key_part_of_chord = true;
			
			bool key_active 
				= chord->keys[i].check_active(chord->keys[i].key_id, key_event->state);
			
			if(key_active) {
				if(!chord->member_active[i]) {
					chord->member_active[i] = true;
					++chord->n_keys_active;
				}
			}
			else {
				if(chord->member_active[i]) {
					chord->member_active[i] = false;
					--chord->n_keys_active;
				}
			}
			break;
		}
	}
	
	if(!key_part_of_chord) {
// 		if(key_event->pressed) {
			chord->phrase_inventory.state = PPG_Phrase_Invalid;
			return chord->phrase_inventory.state;
// 		}
	}
	
	if(chord->n_keys_active == chord->n_members) {
		
		/* Chord completed
		 */
		chord->phrase_inventory.state = PPG_Phrase_Completed;
 		PPG_PRINTF("C");
	}
	
	return chord->phrase_inventory.state;
}

static void ppg_aggregate_reset(PPG_Aggregate *aggregate) 
{
	ppg_phrase_reset((PPG_Phrase__*)aggregate);
	
	aggregate->n_keys_active = 0;
	
	for(uint8_t i = 0; i < aggregate->n_members; ++i) {
		aggregate->member_active[i] = false;
	}
}

static void ppg_aggregate_deallocate_member_storage(PPG_Aggregate *aggregate) {	
	
	if(aggregate->keys) {
		free(aggregate->keys);
		aggregate->keys = NULL;
	}
	if(aggregate->member_active) {
		free(aggregate->member_active);
		aggregate->member_active = NULL;
	}
}

static void ppg_aggregate_resize(PPG_Aggregate *aggregate, 
							uint8_t n_members)
{
	ppg_aggregate_deallocate_member_storage(aggregate);
	
	aggregate->n_members = n_members;
	aggregate->keys = (PPG_Key *)malloc(n_members*sizeof(PPG_Key));
	aggregate->member_active = (bool *)malloc(n_members*sizeof(bool));
	aggregate->n_keys_active = 0;
	
	for(uint8_t i = 0; i < n_members; ++i) {
		aggregate->member_active[i] = false;
		ppg_init_key(&aggregate->keys[i]);
	}
}

static PPG_Aggregate* ppg_aggregate_destroy(PPG_Aggregate *aggregate) {
	
	ppg_aggregate_deallocate_member_storage(aggregate);

	return aggregate;
}

static PPG_Aggregate *ppg_aggregate_alloc(void){
    return (PPG_Aggregate*)malloc(sizeof(PPG_Aggregate));
}

static bool ppg_aggregates_equal(PPG_Aggregate *c1, PPG_Aggregate *c2) 
{
	if(c1->n_members != c2->n_members) { return false; }
	
	uint8_t n_equalities = 0;
	
	for(uint8_t i = 0; i < c1->n_members; ++i) {
		for(uint8_t j = 0; j < c1->n_members; ++j) {
			if(ppg_context->key_id_equal(c1->keys[i].key_id, c2->keys[j].key_id)) { 
				++n_equalities;
				break;
			}
		}
	}
	
	return n_equalities == c1->n_members;
}

#if PAPAGENO_PRINT_SELF_ENABLED
static void ppg_chord_print_self(PPG_Chord *c)
{
	ppg_phrase_print_self((PPG_Phrase__*)c);
	
	PPG_PRINTF("chord\n");
	PPG_PRINTF("   n_members: %d\n", c->n_members);
	PPG_PRINTF("   n_keys_active: %d\n", c->n_keys_active);
	
	for(uint8_t i = 0; i < c->n_members; ++i) {
		PPG_PRINTF("      key_id: %d, active: %d\n", 
				  c->keys[i].key_id, c->member_active[i]);
	}
}
#endif

static PPG_Phrase_Vtable ppg_chord_vtable =
{
	.consider_key_event 
		= (PPG_Phrase_Consider_Keychange_Fun) ppg_phrase_consider_key_event,
	.successor_consider_key_event 
		= (PPG_Phrase_Successor_Consider_Keychange_Fun) ppg_chord_successor_consider_key_event,
	.reset 
		= (PPG_Phrase_Reset_Fun) ppg_aggregate_reset,
	.trigger_action 
		= (PPG_Phrase_Trigger_Action_Fun) ppg_phrase_trigger_action,
	.destroy 
		= (PPG_Phrase_Destroy_Fun) ppg_aggregate_destroy,
	.equals
		= (PPG_Phrase_Equals_Fun) ppg_aggregates_equal
	#if PAPAGENO_PRINT_SELF_ENABLED
	,
	.print_self
		= (PPG_Phrase_Print_Self_Fun) ppg_chord_print_self
	#endif
};

static void *ppg_aggregate_new(void *aggregate) {
    
	/* Explict call to parent constructor
	 */
	ppg_phrase_new((PPG_Phrase__*)aggregate);
	
	PPG_Chord *chord = (PPG_Chord*)aggregate;
		
	/* Initialize the aggregate
	 */
	chord->n_members = 0;
	chord->keys = NULL;
	chord->member_active = NULL;
	chord->n_keys_active = 0;

	return aggregate;
}

/*****************************************************************************
 Clusters 
*****************************************************************************/

// typedef struct {
// 	
// 	// Important: Keep clusters and chords binary compatible to safe 
// 	//					program space
// 	
// 	PPG_Phrase__ phrase_inventory;
// 	 
// 	uint8_t n_members;
// 	PPG_Key *keys;
// 	bool *member_active;
// 	uint8_t n_keys_active;
// 	 
// } PPG_Cluster;

typedef PPG_Aggregate PPG_Cluster;

static uint8_t ppg_cluster_successor_consider_key_event(	
											PPG_Cluster *cluster,
											PPG_Key_Event *key_event) 
{
	bool key_part_of_cluster = false;
	
	PPG_ASSERT(cluster->n_members != 0);
	
	PPG_ASSERT(cluster->phrase_inventory.state == PPG_Phrase_In_Progress);
	
	/* Check it the key is part of the current chord 
	 */
	for(uint8_t i = 0; i < cluster->n_members; ++i) {
		
		if(ppg_context->key_id_equal(cluster->keys[i].key_id, key_event->key_id)) {
			
			key_part_of_cluster = true;
			
			bool key_active 
				= cluster->keys[i].check_active(cluster->keys[i].key_id, key_event->state);
			
			if(key_active) {
				if(!cluster->member_active[i]) {
					cluster->member_active[i] = true;
					++cluster->n_keys_active;
				}
			}
			/* Note: We do not care for released keys here. Every cluster member must be pressed only once
			 */

			break;
		}
	}
	
	if(!key_part_of_cluster) {
// 		if(key_event->pressed) {
			cluster->phrase_inventory.state = PPG_Phrase_Invalid;
			return cluster->phrase_inventory.state;
// 		}
	}
	
	if(cluster->n_keys_active == cluster->n_members) {
		
		/* Cluster completed
		 */
		cluster->phrase_inventory.state = PPG_Phrase_Completed;
 		PPG_PRINTF("O");
	}
	
	return cluster->phrase_inventory.state;
}

// static void ppg_cluster_reset(PPG_Cluster *cluster) 
// {
// 	ppg_phrase_reset((PPG_Phrase__*)cluster);
// 	
// 	cluster->n_keys_active = 0;
// 	
// 	for(uint8_t i = 0; i < cluster->n_members; ++i) {
// 		cluster->member_active[i] = false;
// 	}
// }

// static void ppg_cluster_deallocate_member_storage(PPG_Cluster *cluster) {	
// 	
// 	if(cluster->keys) {
// 		free(cluster->keys);
// 		cluster->keys = NULL;
// 	}
// 	if(cluster->member_active) {
// 		free(cluster->member_active);
// 		cluster->member_active = NULL;
// 	}
// }

// static void ppg_cluster_resize(PPG_Cluster *cluster, 
// 							uint8_t n_members)
// {
// 	ppg_aggregate_deallocate_member_storage(cluster);
// 	
// 	cluster->n_members = n_members;
// 	cluster->keys = (PPG_Key *)malloc(n_members*sizeof(PPG_Key));
// 	cluster->member_active = (bool *)malloc(n_members*sizeof(bool));
// 	cluster->n_keys_active = 0;
// 	
// 	for(uint8_t i = 0; i < n_members; ++i) {
// 		cluster->member_active[i] = false;
// 		ppg_init_key(&cluster->keys[i]);
// 	}
// }

// static bool ppg_is_cluster_member(PPG_Cluster *c, PPG_Key_Id key)
// {
// 	for(uint8_t i = 0; i < c->n_members; ++i) {
// 		if(ppg_context->key_id_equal(c->keys[i].key_id, key)) {
// 			return true;
// 		}
// 	}
// 	
// 	return false;
// }

// static bool ppg_cluster_equals(PPG_Cluster *c1, PPG_Cluster *c2) 
// {
// 	if(c1->n_members != c2->n_members) { return false; }
// 	
// 	uint8_t n_equalities = 0;
// 	
// 	for(uint8_t i = 0; i < c1->n_members; ++i) {
// 		for(uint8_t j = 0; j < c1->n_members; ++j) {
// 			if(ppg_context->key_id_equal(c1->keys[i].key_id, c2->keys[j].key_id)) { 
// 				++n_equalities;
// 				break;
// 			}
// 		}
// 	}
// 	
// 	return n_equalities == c1->n_members;
// }

// static PPG_Cluster* ppg_cluster_destroy(PPG_Cluster *cluster) {
// 	
// 	ppg_aggregate_deallocate_member_storage(cluster);
// 
//     return cluster;
// }

// static PPG_Aggregate *ppg_aggregate_alloc(void){
//     return (PPG_Aggregate*)malloc(sizeof(PPG_Aggregate));
// } 

#if PAPAGENO_PRINT_SELF_ENABLED
static void ppg_cluster_print_self(PPG_Cluster *c)
{
	ppg_phrase_print_self((PPG_Phrase__*)c);
	
	PPG_PRINTF("cluster\n");
	PPG_PRINTF("   n_members: %d\n", c->n_members);
	PPG_PRINTF("   n_keys_active: %d\n", c->n_keys_active);
	
	for(uint8_t i = 0; i < c->n_members; ++i) {
		PPG_PRINTF("      key_id: %d, active: %d\n", 
				  c->keys[i].key_id, c->member_active[i]);
	}
}
#endif

static PPG_Phrase_Vtable ppg_cluster_vtable =
{
	.consider_key_event 
		= (PPG_Phrase_Consider_Keychange_Fun) ppg_phrase_consider_key_event,
	.successor_consider_key_event 
		= (PPG_Phrase_Successor_Consider_Keychange_Fun) ppg_cluster_successor_consider_key_event,
	.reset 
		= (PPG_Phrase_Reset_Fun) ppg_aggregate_reset,
	.trigger_action 
		= (PPG_Phrase_Trigger_Action_Fun) ppg_phrase_trigger_action,
	.destroy 
		= (PPG_Phrase_Destroy_Fun) ppg_aggregate_destroy,
	.equals
		= (PPG_Phrase_Equals_Fun) ppg_aggregates_equal
	
	#if PAPAGENO_PRINT_SELF_ENABLED
	,
	.print_self
		= (PPG_Phrase_Print_Self_Fun) ppg_cluster_print_self
	#endif
};

// static PPG_Cluster *ppg_cluster_new(PPG_Cluster *a_cluster) {
// 	
// 	/*Explict call to parent constructor
// 	*/
// 	ppg_phrase_new((PPG_Phrase__*)a_cluster);
// 
// 	a_cluster->phrase_inventory.vtable = &ppg_cluster_vtable;
// 	
// 	a_cluster->n_members = 0;
// 	a_cluster->keys = NULL;
// 	a_cluster->member_active = NULL;
// 	a_cluster->n_keys_active = 0;
// 	
// 	return a_cluster;
// }

/*****************************************************************************
 * Public access functions for creation and destruction of the melody tree
 *****************************************************************************/

void ppg_init(void) {

	if(!ppg_context) {
		
		ppg_context = (PPG_Context *)ppg_create_context();
	}
	
	if(!ppg_context->melody_root_initialized) {
		
		/* Initialize the melody root
		 */
		ppg_phrase_new(&ppg_context->melody_root);
		
		ppg_context->melody_root_initialized = true;
	}
}

void ppg_finalize(void) {
	
	if(!ppg_context) { return; }
	
	ppg_phrase_free_successors(&ppg_context->melody_root);
	
	free(ppg_context);
	
	ppg_context = NULL;
}

void ppg_reset(void)
{
	ppg_finalize();
	
	ppg_context = (PPG_Context *)ppg_create_context();
}

PPG_Phrase ppg_create_note(PPG_Key key)
{
    PPG_Note *note = (PPG_Note*)ppg_note_new(ppg_note_alloc());
	 
	 note->key = key;
	 
	 ppg_context->key_id_equal(key.key_id, note->key.key_id);
	 
	 /* Return the new end of the melody */
	 return note;
}
	
static PPG_Phrase ppg_initialize_aggregate(	
								PPG_Aggregate *aggregate,
								uint8_t n_keys,
								PPG_Key keys[])
{
	ppg_aggregate_resize(aggregate, n_keys);
	 
	for(uint8_t i = 0; i < n_keys; ++i) {
		aggregate->keys[i] = keys[i];
	}
	 
	/* Return the new end of the melody 
	 */
	return aggregate;
}

PPG_Phrase ppg_create_chord(	
								uint8_t n_keys,
								PPG_Key keys[])
{
	PPG_Chord *chord = (PPG_Chord*)ppg_aggregate_new(ppg_aggregate_alloc());
	
	chord->phrase_inventory.vtable = &ppg_chord_vtable;
	
	return ppg_initialize_aggregate(chord, n_keys, keys);
}
	
PPG_Phrase ppg_create_cluster(
								uint8_t n_keys,
									PPG_Key keys[])
{
	PPG_Cluster *cluster = (PPG_Cluster*)ppg_aggregate_new(ppg_aggregate_alloc());
	 
	cluster->phrase_inventory.vtable = &ppg_cluster_vtable;
	
	return ppg_initialize_aggregate(cluster, n_keys, keys);
}

#if PAPAGENO_PRINT_SELF_ENABLED
static void ppg_recursively_print_melody(PPG_Phrase__ *p)
{
	if(	p->parent
		&& p->parent != &ppg_context->melody_root) {
		
		ppg_recursively_print_melody(p->parent);
	}
	
	PPG_CALL_VIRT_METHOD(p, print_self);
}
#endif

static PPG_Phrase ppg_melody_from_list(	
												uint8_t layer,
												uint8_t n_phrases,
												PPG_Phrase phrases[])
{ 	
	PPG_Phrase__ *parent_phrase = &ppg_context->melody_root;
	
	PPG_PRINTF("   %d members\n", n_phrases);
	
	for (int i = 0; i < n_phrases; i++) { 
		
		PPG_Phrase__ *cur_phrase = (PPG_Phrase__*)phrases[i];
		
		PPG_Phrase__ *equivalent_successor 
			= ppg_phrase_get_equivalent_successor(parent_phrase, cur_phrase);
			
		PPG_PRINTF("   member %d: ", i);
		
		if(	equivalent_successor
			
			/* Only share interior nodes and ...
			 */
			&& equivalent_successor->successors
			/* ... only if the 
			 * newly registered node on the respective level is 
			 * not a leaf node.
			 */ 
			&& (i < (n_phrases - 1))
		) {
			
// 			#if DEBUG_PAPAGENO
// 			if(cur_phrase->action.type != equivalent_successor->action.type) {
// 				PPG_ERROR("Incompatible action types detected\n");
// 			}
// 			#endif
			
			PPG_PRINTF("already present: 0x%" PRIXPTR "\n", equivalent_successor);
			
			parent_phrase = equivalent_successor;
			
			if(layer < equivalent_successor->layer) {
				
				equivalent_successor->layer = layer;
			}
			
			/* The successor is already registered in the search tree. Delete the newly created version.
			 */			
			ppg_phrase_free(cur_phrase);
		}
		else {
			
			PPG_PRINTF("newly defined: 0x%" PRIXPTR "\n", cur_phrase);
			
			#if DEBUG_PAPAGENO
			
			/* Detect melody ambiguities
			 */
			if(equivalent_successor) {
				if(	
					/* Melodies are ambiguous if ...
					 * the conflicting nodes/phrases are both leaf phrases
					 */
						(i == (n_phrases - 1))
					&&	!equivalent_successor->successors
					
					/* And defined for the same layer
					 */
					&& (equivalent_successor->layer == layer)
				) {
					PPG_ERROR("Conflicting melodies detected.\n")
					
					#if PAPAGENO_PRINT_SELF_ENABLED
					PPG_ERROR(
						"The phrases of the conflicting melodies are listed below.\n");
					
					PPG_ERROR("Previously defined:\n");
					ppg_recursively_print_melody(equivalent_successor);
					
					PPG_ERROR("Conflicting:\n");
					for (int i = 0; i < n_phrases; i++) {
						PPG_CALL_VIRT_METHOD(phrases[i], print_self);
					}
					#endif
				}
			}
			#endif /* if DEBUG_PAPAGENO */
					
			cur_phrase->layer = layer;
			
			ppg_phrase_add_successor(parent_phrase, cur_phrase);
			
			parent_phrase = cur_phrase;
		}
	}
	
	/* Return the leaf phrase 
	 */
	return parent_phrase;
}

PPG_Phrase ppg_melody(		
							uint8_t layer, 
							uint8_t n_phrases,
							PPG_Phrase phrases[])
{ 
	PPG_PRINTF("Adding magic melody\n");
	
	ppg_init();
		
	return ppg_melody_from_list(layer, n_phrases, phrases);
}

PPG_Phrase ppg_chord(		
							uint8_t layer, 
							PPG_Action action, 
							uint8_t n_keys,
							PPG_Key keys[])
{   	
	PPG_PRINTF("Adding chord\n");
	
	PPG_Phrase__ *phrase = 
		(PPG_Phrase__ *)ppg_create_chord(n_keys, keys);
		
	phrase->action = action;
		
	PPG_Phrase phrases[1] = { phrase };
	
	PPG_Phrase__ *leaf_phrase 
		= ppg_melody_from_list(layer, 1, phrases);
		
	return leaf_phrase;
}

PPG_Phrase ppg_cluster(		
							uint8_t layer, 
							PPG_Action action, 
							uint8_t n_keys,
							PPG_Key keys[])
{   	
	PPG_PRINTF("Adding cluster\n");
	
	PPG_Phrase__ *phrase = 
		(PPG_Phrase__ *)ppg_create_cluster(n_keys, keys);
		
	phrase->action = action;
	
	PPG_Phrase phrases[1] = { phrase };
	
	PPG_Phrase__ *leaf_phrase 
		= ppg_melody_from_list(layer, 1, phrases);
		
	return leaf_phrase;
}

PPG_Phrase ppg_single_note_line(	
							uint8_t layer,
							PPG_Action action,
							uint8_t n_keys,
							PPG_Key keys[])
{
	PPG_PRINTF("Adding single note line\n");
  
	PPG_Phrase phrases[n_keys];
		
	for (int i = 0; i < n_keys; i++) {

		phrases[i] = ppg_create_note(keys[i]);
	}
	
	ppg_phrase_store_action(phrases[n_keys - 1], action);
	
	PPG_Phrase__ *leaf_phrase 
		= ppg_melody_from_list(layer, n_keys, phrases);
  
	return leaf_phrase;
}

PPG_Phrase ppg_tap_dance(	
							uint8_t layer,
							PPG_Key key,
							uint8_t default_action_flags,
							uint8_t n_tap_definitions,
							PPG_Tap_Definition tap_definitions[])
{
	PPG_PRINTF("Adding tap dance\n");
	
	ppg_init();
	
	int n_taps = 0;
	for (uint8_t i = 0; i < n_tap_definitions; i++) { 
		
		if(tap_definitions[i].tap_count > n_taps) {
			n_taps = tap_definitions[i].tap_count;
		}
	}
	
	if(n_taps == 0) { return NULL; }
		
	PPG_Phrase phrases[n_taps];
	
	for (int i = 0; i < n_taps; i++) {
		
		PPG_Phrase__ *new_note = (PPG_Phrase__*)ppg_create_note(key);
			new_note->action.flags = default_action_flags;
		
		phrases[i] = new_note;
	}
	
	for (uint8_t i = 0; i < n_tap_definitions; i++) {
		
		ppg_phrase_store_action(
					phrases[tap_definitions[i].tap_count - 1], 
					tap_definitions[i].action);
	}
			
	PPG_Phrase__ *leafPhrase 
		= ppg_melody_from_list(layer, n_taps, phrases);
	
	return leafPhrase;
}

PPG_Phrase ppg_phrase_set_action(	
							PPG_Phrase phrase__,
							PPG_Action action)
{
	PPG_Phrase__ *phrase = (PPG_Phrase__ *)phrase__;
	
	ppg_phrase_store_action(phrase, action);
	
	return phrase__;
}

PPG_Action ppg_phrase_get_action(PPG_Phrase phrase__)
{
	PPG_Phrase__ *phrase = (PPG_Phrase__ *)phrase__;
	
	return phrase->action;
}

PPG_Phrase ppg_phrase_set_action_flags(
									PPG_Phrase phrase__,
									uint8_t action_flags)
{
	PPG_Phrase__ *phrase = (PPG_Phrase__ *)phrase__;
	
	phrase->action.flags = action_flags;
	
	return phrase;
}

uint8_t ppg_phrase_get_action_flags(PPG_Phrase phrase__)
{
	PPG_Phrase__ *phrase = (PPG_Phrase__ *)phrase__;
	
	return phrase->action.flags;
}

bool ppg_check_timeout(void)
{
// 	PPG_PRINTF("Checking timeout\n");
	
	#ifdef DEBUG_PAPAGENO
	if(!ppg_context->time) {
		PPG_ERROR("Time function undefined\n");
	}
	if(!ppg_context->time_difference) {
		PPG_ERROR("Time difference function undefined\n");
	}
	if(!ppg_context->time_comparison) {
		PPG_ERROR("Time comparison function undefined\n");
	}
	#endif
	
	PPG_Time cur_time;
	
	ppg_context->time(&cur_time);
	
	PPG_Time delta;
	ppg_context->time_difference(
					ppg_context->time_last_keypress, 
					cur_time, 
					&delta);
	
	if(ppg_context->current_phrase
		&& (ppg_context->time_comparison(
					delta,
					ppg_context->keypress_timeout
			) > 0)
	  ) {
		
		PPG_PRINTF("Magic melody timeout hit\n");
	
		/* Too late...
			*/
		ppg_on_timeout();
	
		return true;
	}
	
	return false;
}

bool ppg_process_key_event(PPG_Key_Event *key_event,
								  uint8_t cur_layer)
{ 
	if(!ppg_context->papageno_enabled) {
		return true;
	}
	
	/* When a melody could not be finished, all keystrokes are
	 * processed through the process_record_quantum method.
	 * To prevent infinite recursion, we have to temporarily disable 
	 * processing magic melodies.
	 */
	if(ppg_context->papageno_temporarily_disabled) { return true; }
	
	/* Early exit if no melody was registered 
	 */
	if(!ppg_context->melody_root_initialized) { return true; }
	
	/* Check if the melody is being aborted
	 */
	if(ppg_context->key_id_equal(ppg_context->abort_key.key_id, key_event->key_id)) {
		
		/* If a melody is in progress, we abort it and consume the abort key.
		 */
		if(ppg_context->current_phrase) {
			PPG_PRINTF("Processing melodies interrupted by user\n");
			ppg_abort_magic_melody();
			return false;
		}
	
		return false;
	}
	
// 	PPG_PRINTF("Starting keyprocessing\n");
	
	if(!ppg_context->current_phrase) {
		
// 		PPG_PRINTF("New melody \n");
		
		/* Start of melody processing
		 */
		ppg_context->n_key_events = 0;
		ppg_context->current_phrase = &ppg_context->melody_root;
		
		ppg_context->time(&ppg_context->time_last_keypress);
	}
	else {
		
		if(ppg_check_timeout()) {
			
			/* Timeout hit. Cleanup already done.
			 */
			return false;
		}
		else {
			ppg_context->time(&ppg_context->time_last_keypress);
		}
	}
	
	uint8_t result 
		= ppg_phrase_consider_key_event(	
										&ppg_context->current_phrase,
										key_event,
										cur_layer
								);
		
	switch(result) {
		
		case PPG_Phrase_In_Progress:
			
			return false;
			
		case PPG_Melody_Completed:
			
			ppg_recurse_and_process_actions(PPG_On_Melody_Completed);
			
			ppg_context->current_phrase = NULL;
			
			ppg_delete_stored_key_events();
											  
			return false;
			
		case PPG_Phrase_Invalid:
			
			PPG_PRINTF("-\n");
		
			ppg_abort_magic_melody();
			
			return false; /* The key(s) have been already processed */
			//return true; // Why does this require true to work and 
			// why is t not written?
	}
	
	return true;
}

/* Use this function to define a key_id that always aborts a magic melody
 */
PPG_Key ppg_set_abort_key(PPG_Key key)
{
	PPG_Key old_key = ppg_context->abort_key;
	
	ppg_context->abort_key = key;
	
	return old_key;
}

PPG_Key ppg_get_abort_key(void)
{
	return ppg_context->abort_key;
}

bool ppg_set_process_actions_if_aborted(bool state)
{
	bool old_value = ppg_context->process_actions_if_aborted;
	
	ppg_context->process_actions_if_aborted = state;
	
	return old_value;
}

bool ppg_get_process_actions_if_aborted(void)
{
	return ppg_context->process_actions_if_aborted;
}

PPG_Time ppg_set_timeout(PPG_Time timeout)
{
	PPG_Time old_value = ppg_context->keypress_timeout;
	
	ppg_context->keypress_timeout = timeout;
	
	return old_value;
}

PPG_Time ppg_get_timeout(void)
{
	return ppg_context->keypress_timeout;
}

void ppg_set_key_id_equal_function(PPG_Key_Id_Equal_Fun fun)
{
	ppg_context->key_id_equal = fun;
}

PPG_Key_Event_Processor_Fun ppg_set_default_key_processor(PPG_Key_Event_Processor_Fun key_processor)
{
	PPG_Key_Event_Processor_Fun old_key_processor = ppg_context->key_processor;
	
	ppg_context->key_processor = key_processor;
	
	return old_key_processor;
}

bool ppg_set_enabled(bool state)
{
	bool old_state = ppg_context->papageno_enabled;
	
	ppg_context->papageno_enabled = state;
	
	return old_state;
}

PPG_Time_Fun ppg_set_time_function(PPG_Time_Fun fun)
{
	PPG_Time_Fun old_fun = fun;
	
	ppg_context->time = fun;
	
	return old_fun;
}

PPG_Time_Difference_Fun ppg_set_time_difference_function(PPG_Time_Difference_Fun fun)
{
	PPG_Time_Difference_Fun old_fun = ppg_context->time_difference;
	
	ppg_context->time_difference = fun;
	
	return old_fun;
}

PPG_Time_Comparison_Fun ppg_set_time_comparison_function(PPG_Time_Comparison_Fun fun)
{
	PPG_Time_Comparison_Fun old_fun = ppg_context->time_comparison;
	
	ppg_context->time_comparison = fun;
	
	return old_fun;
}
