#include "Confirm.h"
#include "TextMenu.h"

int Confirm_Result;
static void Confirm_Yes(void*);
static void Confirm_No(void*);

int Confirm(char *message, char *yesText, char *noText, int defaultItem) {
	TEXTMENU ConfirmMenu;
	TEXTMENUITEM YesItem, NoItem;

	memset(&ConfirmMenu, 0x00, sizeof(TEXTMENU));
	memset(&YesItem, 0x00, sizeof(TEXTMENUITEM));
	memset(&NoItem, 0x00, sizeof(TEXTMENUITEM));
	
	//Title the menu
	strncpy(ConfirmMenu.szCaption, message, MENUCAPTIONSIZE);
	
	//Set up the yes/no items
	strncpy(YesItem.szCaption, yesText, MENUCAPTIONSIZE);
	strncpy(NoItem.szCaption, noText, MENUCAPTIONSIZE);
	YesItem.functionPtr=Confirm_Yes;
	NoItem.functionPtr=Confirm_No;
	//Add them to the menu	
	TextMenuAddItem(&ConfirmMenu, &YesItem);	
	TextMenuAddItem(&ConfirmMenu, &NoItem);	
	
	//Draw the menu
	TextMenu(&ConfirmMenu, defaultItem?&YesItem:&NoItem);
	
	return Confirm_Result;
}

static void Confirm_Yes(void *nothing) {
	Confirm_Result=1;
	breakOutOfMenu=1;
}

static void Confirm_No(void *nothing) {
	Confirm_Result=0;
	breakOutOfMenu=1;
}
