#include "../usb_wrapper.h"
#include "config.h"

// This is for the Xpad
extern unsigned char xpad_button_history[7];

// This is for the IR remote control
extern unsigned int current_remote_key;
extern unsigned int current_remote_keydir;


// This is for the Keyboard
extern unsigned int current_keyboard_key;

int risefall_xpad_BUTTON(unsigned char selected_Button) {
	
      	int xpad_id; 
	int match;
	extern int xpad_num;
	
	// USB keyboard section 
	
	match=0;
	if (current_keyboard_key!=0) {
		switch (selected_Button) {
			case TRIGGER_XPAD_KEY_A :
		   		if (current_keyboard_key == 0x28) match=1;
				break;
			case TRIGGER_XPAD_KEY_B :
		   		if (current_keyboard_key == 0x29) match=1;
				break;
			case TRIGGER_XPAD_PAD_UP :
				if (current_keyboard_key == 0x52) match=1;
				break;
			case TRIGGER_XPAD_PAD_DOWN :
		   		if (current_keyboard_key == 0x51) match=1;
				break;
			case TRIGGER_XPAD_PAD_LEFT :
		   		if (current_keyboard_key == 0x50) match=1;
				break;
			case TRIGGER_XPAD_PAD_RIGHT :
		   		if (current_keyboard_key == 0x4f) match=1;
				break;
		}

		if (match) {
			//A match occurred, so the event has now been processed
			//Clear it, and return success
			current_keyboard_key=0;
			return 1;
		}
	}
	
	// Xbox IR remote section
	
	match=0;
	if ((current_remote_keydir&0x100)) {
	      	//This is a button release event - press events are ignored
		//to avoid duplicates, as a new press event is sent
		//as long as the button is held down.
		
		switch (selected_Button) {
			case TRIGGER_XPAD_KEY_A:
		   		if (current_remote_key == 0x0b) match=1;
				break;
			case TRIGGER_XPAD_PAD_UP:
				if (current_remote_key == 0xa6) match=1;
				break;
			case TRIGGER_XPAD_PAD_DOWN:
				if (current_remote_key == 0xa7) match=1;
				break;
			case TRIGGER_XPAD_PAD_LEFT:
				if (current_remote_key == 0xa9) match=1;
				break;
			case TRIGGER_XPAD_PAD_RIGHT:
				if (current_remote_key == 0xa8) match=1;
				break;
			case TRIGGER_XPAD_KEY_BACK:
				if (current_remote_key == 0xd8) match=1;
				break;
		}
		if (match) {
			//A match occurred, so the event has now been processed
			//Clear it, and return success
			current_remote_key=0;
			current_remote_keydir=0;
			return 1;
		}
	}
       	
	// Xbox controller section
	if (selected_Button < 6) {
       	
       		unsigned char Button;
       	
       		Button = XPAD_current[0].keys[selected_Button];
	
		if ((Button>0x30)&&(xpad_button_history[selected_Button]==0)) {
			// Button Rising Edge
			xpad_button_history[selected_Button] = 1;		
			return 1;
		}	
		
		if ((Button==0x00)&&(xpad_button_history[selected_Button]==1)) {
			// Button Falling Edge
			xpad_button_history[selected_Button] = 0;		
			return -1;
		}	
	}
 	
 	if ((selected_Button > 5) & (selected_Button < 10) ) {
	
		unsigned char Buttonmask;
       	      
		switch (selected_Button) {
			case TRIGGER_XPAD_PAD_UP :
				   Buttonmask = XPAD_PAD_UP; 
				   break;
			case TRIGGER_XPAD_PAD_DOWN :
				   Buttonmask = XPAD_PAD_DOWN;
				   break;
			case TRIGGER_XPAD_PAD_LEFT :
				   Buttonmask = XPAD_PAD_LEFT;
				   break;
			case TRIGGER_XPAD_PAD_RIGHT :
				   Buttonmask = XPAD_PAD_RIGHT;
				   break;
		}		
       	    
		// Rising Edge
		if (((XPAD_current[0].pad&Buttonmask) != 0) & ((xpad_button_history[6]&Buttonmask) == 0)) {
			xpad_button_history[6] ^= Buttonmask;  // Flip the Bit
			return 1;
		}				
		// Falling Edge
		if (((XPAD_current[0].pad&Buttonmask) == 0) & ((xpad_button_history[6]&Buttonmask) != 0)) {
			xpad_button_history[6] ^= Buttonmask;  // Flip the Bit
			return -1;
 		}
	}
	return 0;
}
