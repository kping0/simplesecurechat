
/*
 *  <SimpleSecureChat Client/Server - E2E encrypted messaging application written in C>
 *  Copyright (C) 2017-2018 The SimpleSecureChat Authors. <kping0> 
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "cli.h"

void ssc_cli_init(void){
	debuginfo();
	initscr();
	start_color();
	keypad(stdscr,TRUE);
	noecho();
	init_pair(1,COLOR_CYAN,COLOR_BLACK);
	curs_set(0);
	return;
}

void ssc_cli_end(void){
	debuginfo();
	endwin();
	return;
}

/*
 * create new window structure
 */
WIN* ssc_cli_cust_newwin(int height, int width, int starty, int startx){
	debuginfo();
	WIN* window = calloc(1,sizeof(WIN));
	memset(window,0,sizeof(WIN));
	window->height = height;
	window->width = width;
	window->starty = starty;
	window->startx = startx;
	window->border.ls = '|';
	window->border.rs = '|';
	window->border.ts = '-';
	window->border.bs = '-';
	window->border.tl = '+';
	window->border.tr = '+';
	window->border.bl = '+';	
	window->border.br = '+';
	WPAGE* first_page = calloc(1,sizeof(WPAGE));
	window->current_page = 	first_page;
	memset(window->pages,0,sizeof(WPAGE*) * 1000);
	window->pages[1] = first_page;
	window->page_count++;
	window->page_index++;
	return window;
}

/*
 * display/undisplay the window
 */
void ssc_cli_cust_updwin(WIN* p_win,bool flag){ //flag == TRUE -> createwin / flag == FALSE -> destroywin
	debuginfo();
	int i, j;
	int x, y, w, h;

	x = p_win->startx;
	y = p_win->starty;
	w = p_win->width;
	h = p_win->height;

	if(flag == TRUE)
	{	mvaddch(y, x, p_win->border.tl);
		mvaddch(y, x + w, p_win->border.tr);
		mvaddch(y + h, x, p_win->border.bl);
		mvaddch(y + h, x + w, p_win->border.br);
		mvhline(y, x + 1, p_win->border.ts, w - 1);
		mvhline(y + h, x + 1, p_win->border.bs, w - 1);
		mvvline(y + 1, x, p_win->border.ls, h - 1);
		mvvline(y + 1, x + w, p_win->border.rs, h - 1);
	}
	else
		for(j = y; j <= y + h; ++j)
			for(i = x; i <= x + w; ++i)
				mvaddch(j, i, ' ');
				
	refresh();
	return;
}
void debugstdscr(byte* text){
	debuginfo();
#ifdef DEBUG
	mvwprintw(stdscr,0,0,"[DEBUG] %s",text);
	refresh();
#else
	(void)text;
#endif
	return;
}

/*
 * create a new page and put it in the foreground of the current window
 */
void ssc_cli_add_page(WIN* p_win){ 
	debuginfo();
	WPAGE* new_page = calloc(1,sizeof(WPAGE));	
	new_page->currently_selected = 0;
	new_page->elements = 0;
	memset(new_page->screen_content,0,sizeof(byte*) * 100);
	p_win->current_page = new_page;	
	p_win->page_index++;
	p_win->pages[p_win->page_index] = new_page;
	p_win->page_count++;
	return;
}

/*
 * reload content in window with content stored in memory (p_win->current_page->screen_content)
 */
void ssc_cli_window_reload(WIN* p_win){ 
	debuginfo();
	WPAGE* current_page = p_win->current_page;
	if(!current_page)return;
	int i, j;
	int x, y, w, h;

	x = p_win->startx;
	y = p_win->starty;
	w = p_win->width;
	h = p_win->height;
	int elements = current_page->elements;
	attroff(A_REVERSE);
	for(j = y+1; j <= y + h-1; ++j)
		for(i = x+1; i <= x-1 + w; ++i)
			mvaddch(j, i, ' ');
	for(j = y+1; j <= y + h-1; ++j){
		if(j-y-1 == elements)return;
		byte* line = current_page->screen_content[j-y];
		if(line != NULL)
			mvwprintw(stdscr,j,x+1,"%s",line);	
		else
			mvhline(j,x+1,p_win->border.bs,w - 1);
	}
	refresh();	
	return;	
}

/*
 * move cursor to list known position in memory -> useful for resetting the cursor after calling other
 * functions
 */
void ssc_cli_switch_current(WIN* p_win){
	debuginfo();
	WPAGE* current_page = p_win->current_page;
	if(!current_page)return;
	int x, y;
	x = p_win->startx;
	y = p_win->starty;
	int current = current_page->currently_selected;
	if(current != 0)
		move(y+current,x+1);
	else
		move(y+current+1,x+1);
	return;
}

/*
 *  return a ptr to the current screen content 
 */
byte* ssc_cli_currently_selected(WIN* p_win){
	debuginfo();
	WPAGE* current_page = p_win->current_page;
	if(!current_page)return NULL;
	int current = current_page->currently_selected;	
	return (byte*)(current_page->screen_content[current]);
}

/*
 * sync the message tabs to be on the same page with index page_index
 */
void ssc_cli_msg_page_sync(SSCGV* gv){
	debuginfo();
	WIN* follower,*current;
	if(gv->current == gv->sent){
		current = gv->sent;
		follower = gv->received;
	}
	else{
		current = gv->received;	
		follower = gv->sent;
	}
	int index = current->page_index;
	follower->current_page = follower->pages[index]; // adjust the follower to the index of current
	follower->page_index = index;	
	ssc_cli_window_reload(current); // reload windows
	ssc_cli_window_reload(follower);
	ssc_cli_switch_current(current);  
	return;	
}

/*
 * load the next page (if exists) for window p_win 
 */
void ssc_cli_next_page(SSCGV* gv){
	debuginfo();
	WIN* p_win = gv->current;
	if(p_win->page_index == p_win->page_count)return;	
	p_win->page_index++; // increase the page index by 1
	p_win->current_page = p_win->pages[p_win->page_index]; 
	ssc_cli_window_reload(p_win);
	ssc_cli_switch_current(p_win);
	return;
}

/*
 * load the previous page (if not first) for window p_win
 */
void ssc_cli_prev_page(SSCGV* gv){
	debuginfo();
	WIN* p_win = gv->current;
	if(p_win->page_index <= 1)return;		
	p_win->page_index--; // decrease the page index by 1
	p_win->current_page = p_win->pages[p_win->page_index];
	ssc_cli_window_reload(p_win);
	ssc_cli_switch_current(p_win);
	return;
}

/*
 * load the last page for window p_win
 */
void ssc_cli_last_page(SSCGV* gv){
	debuginfo();
	WIN* p_win = gv->current;
	if(p_win->page_index == p_win->page_count)return;
	p_win->page_index = p_win->page_count; // go to the last page that is not empty
	p_win->current_page = p_win->pages[p_win->page_index];
	ssc_cli_window_reload(p_win);
	ssc_cli_switch_current(p_win);
	return;
}

/*
 * append item to current_page
 */
void ssc_cli_add_item(WIN* p_win,byte* block){
	debuginfo();
	WPAGE* current_page = p_win->current_page;
	if(!current_page)return;
	int x,y,w,h,elements;
	x = p_win->startx;
	y = p_win->starty;
	w = p_win->width;
	h = p_win->height;
	elements = current_page->elements;
	if(elements >= h-2){
		ssc_cli_add_page(p_win);
		ssc_cli_add_item(p_win,block);
		ssc_cli_window_reload(p_win);
		ssc_cli_switch_current(p_win);
		return;
	}
	size_t blocksize = strlen(block); 
	byte* saved_block = calloc(1,blocksize);	
	memcpy(saved_block,block,blocksize); // copy the passed block to a seperate address
	current_page->screen_content[elements+1] = saved_block; // add the address to the array for the page
	mvwprintw(stdscr,y+1+elements,x+1,"%s",saved_block); // write the block to screen
	mvhline(y+2+elements,x+1,p_win->border.bs,w - 1); // add space
	current_page->elements+= 2; // increase the elements on the page by 2 (block + space)

	getyx(stdscr,y,x); //reset cursor
	y--;
	move(y,x);
	return;
}

/*
 * update highlighted object (A_REVERSE) based on cursor position 
 */
void ssc_cli_window_upd_highlight(SSCGV* gv){ 
	debuginfo();
	WIN* p_win = gv->current;
	WPAGE* current_page = p_win->current_page;
	if(!current_page)return;
	int elements = current_page->elements;
	if(elements <= 0)return;
	int c_y,c_x;
	getyx(stdscr,c_y,c_x);
	(void)c_x;
	int index = c_y - p_win->starty;
	byte* highlight_object = current_page->screen_content[index];
	current_page->currently_selected = index;
#ifdef DEBUG
	mvwprintw(stdscr,0,0,"[DEBUG] selected object is %s\n",highlight_object);
#endif
	if(!(index-2 < 0))mvwprintw(stdscr,c_y-2,p_win->startx+1,"%s",current_page->screen_content[index-2]);
	attron(A_REVERSE);
	mvwprintw(stdscr,c_y,p_win->startx+1,"%s",highlight_object);		
	attroff(A_REVERSE);
	if(!(elements-index-1 <= 0)){
		mvwprintw(stdscr,c_y+2,p_win->startx+1,"%s",current_page->screen_content[index+2]);
		wmove(stdscr,c_y,p_win->startx+1);
	}
	if(elements-index <= 0)	wmove(stdscr,c_y-2,p_win->startx+1);
	return;
}

/*
 * get currently selected item
 */
byte* ssc_cli_get_current(SSCGV* gv){
	debuginfo();
	WIN* p_win = gv->current;
	int c_y,c_x;
	getyx(stdscr,c_y,c_x);
	(void)c_x;
	int index = c_y - p_win->starty;
	return (p_win->current_page->screen_content[index]);
}

/*
 * custom move(y,x) -> call ssc_cli_next/prev_page if top/bottom is reached
 */
int ssc_cli_cursor_move(SSCGV* gv,int c_y,int c_x){ 
	debuginfo();
	WIN* p_win = gv->current;
	WPAGE* current_page = p_win->current_page;
	if(!current_page)return -1;
	int y = p_win->starty;
	int elements = current_page->elements;
	if(elements <= 0)return -1;
	int index = c_y - y;	
	if(elements-index < 0){
		ssc_cli_next_page(gv);
		return 1;	
	}
	if(index < 0){
		ssc_cli_prev_page(gv);	
		return -1;
	}
	move(c_y,c_x);
	return 0;
}

/*
 * move for the messages column 
 */
int ssc_cli_msg_cursor_move(SSCGV* gv,int c_y,int c_x){ 
	debuginfo();
	WIN* p_win = gv->current;
	WIN* sync;
	if(gv->current == gv->sent){
		sync = gv->received;
	}
	else{
		sync = gv->sent;
	}

	WPAGE* current_page = p_win->current_page;
	if(!current_page)return -1;
	int y = p_win->starty;
	int elements = current_page->elements;
	if(elements <= 0)return -1;
	int index = c_y - y;	
	if(elements-index < 0){
		ssc_cli_next_page(gv);
		ssc_cli_msg_page_sync(gv);
		return 1;	
	}
	if(index < 0){
		ssc_cli_prev_page(gv);	
		ssc_cli_msg_page_sync(gv);
		return -1;
	}
	move(c_y,c_x);
	return 0;
}

/*
 * get 1024 null terminated string from input
 */
byte* _getstr(void){
	debuginfo();
	int row,col;
	getmaxyx(stdscr,row,col);
	mvwprintw(stdscr,row-1,5,"<CMD_MODE>: ");			
	int y,x;
	getyx(stdscr,y,x);
	echo();
	curs_set(1);	
	byte* string = calloc(1,1024);
	int i = 0;
	int ch = getch();	
	while(ch != 27 && ch != '\n' && i+1 < 1024){
		if(ch == KEY_BACKSPACE){
			if(i != 0){
				delch();
				i--;
				string[i] = 0;
			}else{
				move(y,x);	
			}
		}
		else{	
			string[i] = ch;
			i++;
		}
		ch = getch();
	}
	noecho();
	curs_set(0);
	mvhline(y,1,' ',col-1);
	if(ch != 27){
		string[1023] = '\0';
		return string;
	}
	else{
		free(string);
		return NULL;
	}
}
/* TESTING */
byte* _getstr_custom(byte* input_prefix){
	debuginfo();
	int row,col;
	getmaxyx(stdscr,row,col);
	mvwprintw(stdscr,row-1,5,input_prefix);
	int y,x;
	getyx(stdscr,y,x);
	echo();
	curs_set(1);	
	byte* string = calloc(1,1024);
	int i = 0;
	int ch = getch();	
	while(ch != 27 && ch != '\n' && i+1 < 1024){
		if(ch == KEY_BACKSPACE){
			if(i != 0){
				delch();
				i--;
				string[i] = 0;
			}else{
				move(y,x);	
			}
		}
		else{	
			string[i] = ch;
			i++;
		}
		ch = getch();
	}
	noecho();
	curs_set(0);
	mvhline(y,1,' ',col-1);
	if(ch != 27){
		string[1023] = '\0';
		return string;
	}
	else{
		free(string);
		return NULL;
	}

}
/* TESTING */

/*
 * add message to one window, and add blank to the other
 */
void ssc_cli_add_message(WIN* window4msg,WIN* window4space,byte* message){
	debuginfo();
	ssc_cli_add_item(window4msg,message);
	ssc_cli_add_item(window4space,"          ");
	return;
}

/*
 * free the received and send list 
 */
void ssc_cli_msg_clear(SSCGV* gv){
	debuginfo();
	WIN* received = gv->received;
	WIN* sent = gv->sent;
	WPAGE* _page,*new_page;
	byte* element;
	int i,x;
	int page_count = sent->page_count;
	for(i = 0;i<=page_count;i++){
		_page = sent->pages[i];
		if(_page != NULL){
			for(x = 0;x<100;x++){
				element = _page->screen_content[x];
				if(element != NULL)free(element);
			}	
			free(_page);
		}
	}
	memset(sent->pages,0,sizeof(WPAGE*) * 1000); 
	new_page = calloc(1,sizeof(WPAGE));
	sent->pages[1] = new_page;
	sent->current_page = new_page;
	ssc_cli_window_reload(sent);
	page_count = received->page_count;
	for(i = 0;i<page_count;i++){
		_page = received->pages[i];
		if(_page != NULL){
			for(x = 0;x<100;x++){
				element = _page->screen_content[x];
				if(element != NULL)free(element);
			}	
			free(_page);
		}
	}
	memset(received->pages,0,sizeof(WPAGE*) * 1000); 
	new_page = calloc(1,sizeof(WPAGE));
	received->pages[1] = new_page;
	received->current_page = new_page;
	ssc_cli_window_reload(received);
	ssc_cli_switch_current(gv->current);
	return;
}

/*
 * update the messages from the db
 */
void ssc_cli_msg_upd(SSCGV* gv,byte* username){
	debuginfo();
/* Slightly modified code from ssc_cli_msg_clear() to save the previous list position START */
	WIN* received = gv->received;
	WIN* sent = gv->sent;
	WPAGE* _page,*new_page;
	byte* element;
	int saved_c_sent,saved_c_recv;
	int i,x;
	int page_count = sent->page_count;
	if(!(sent && sent->current_page))return;
	saved_c_sent = sent->current_page->currently_selected;
	for(i = 0;i<=page_count;i++){
		_page = sent->pages[i];
		if(_page != NULL){
			for(x = 0;x<100;x++){
				element = _page->screen_content[x];
				if(element != NULL)free(element);
			}	
			free(_page);
		}
	}
	memset(sent->pages,0,sizeof(WPAGE*) * 1000); 
	new_page = calloc(1,sizeof(WPAGE));
	sent->pages[1] = new_page;
	sent->current_page = new_page;
	sent->current_page->currently_selected = saved_c_sent; //so we dont lose position in list
	ssc_cli_window_reload(sent);
	page_count = received->page_count;
	saved_c_recv = received->current_page->currently_selected;
	for(i = 0;i<page_count;i++){
		_page = received->pages[i];
		if(_page != NULL){
			for(x = 0;x<100;x++){
				element = _page->screen_content[x];
				if(element != NULL)free(element);
			}	
			free(_page);
		}
	}
	memset(received->pages,0,sizeof(WPAGE*) * 1000); 
	new_page = calloc(1,sizeof(WPAGE));
	received->pages[1] = new_page;
	received->current_page = new_page;
	received->current_page->currently_selected = saved_c_recv;
	ssc_cli_window_reload(received);
	ssc_cli_switch_current(gv->current);
/* END of clear code */

	sqlite3* db = gv->db;
	sqlite3_stmt* stmt;
	BIO* srvconn = gv->conn;
	EVP_PKEY* priv_evp = gv->privkey;
	WIN* recvlist = gv->received;
	WIN* messagelist = gv->sent;
	byte* current_user = username;
	if(!current_user)return;
/*
 * if SSC_UPDATE_THREAD is NOT present, update messages here.
 */
#ifndef SSC_UPDATE_THREAD
	byte* getmsgbuf = (byte*)server_get_messages(db);	//Get buffer to send to server
	if(!getmsgbuf)return;
	byte* decbuf = NULL;
	byte* recvbuf = calloc(1,200000);
	BIO_write(srvconn,getmsgbuf,strlen(getmsgbuf));	//Send buffer to server
	free(getmsgbuf);
	BIO_read(srvconn,recvbuf,199999); //Read response
	cdebug("received message %s",recvbuf);
	if(strcmp(recvbuf,"ERROR") != 0){
	sscsl* list = SSCS_list_open(recvbuf);
	int i = 0;	
	while(1){
			i++;	
			sscsd* prebuf =	SSCS_list_data(list,i);	
			if(!prebuf)break;
			sscso* obj2 = SSCS_open(prebuf->data);
			SSCS_data_release(&prebuf);
			byte* sender = SSCS_object_string(obj2,"sender");
			if(!sender)break;
			decbuf = (byte*)decrypt_msg(obj2->buf_ptr,priv_evp,db);		
			if(!decbuf)break;
			filter_string(decbuf);
			sqlite3_prepare_v2(db,"insert into messages(msgid,uid,uid2,message)values(NULL,?1,1,?2);",-1,&stmt,NULL);	
			sqlite3_bind_int(stmt,1,get_user_uid(sender,db));
			sqlite3_bind_text(stmt,2,decbuf,-1,0);
			sqlite3_step(stmt);
			sqlite3_finalize(stmt);
			stmt = NULL;
			SSCS_release(&obj2);
			free(sender);
			if(decbuf)free(decbuf);
		}
		SSCS_list_release(&list);
	}
	free(recvbuf);
#endif /* !SSC_UPDATE_THREAD */
	int currentuserUID = get_user_uid(current_user,db);
	if(currentuserUID == -1)return;
	sqlite3_prepare_v2(db,"select uid,message from messages where uid=1 AND uid2=?1 OR uid=?2 AND uid2=1",-1,&stmt,NULL);
	sqlite3_bind_int(stmt,1,currentuserUID);
	sqlite3_bind_int(stmt,2,currentuserUID);
	while(sqlite3_step(stmt) == SQLITE_ROW){
		int sqluid = sqlite3_column_int(stmt,0);
		if(sqluid == 1){
			ssc_cli_add_message(messagelist,recvlist,(byte*)sqlite3_column_text(stmt,1));
		}	
		else if(sqluid == currentuserUID){
			ssc_cli_add_message(recvlist,messagelist,(byte*)sqlite3_column_text(stmt,1));
		}
	}
	sqlite3_finalize(stmt);
	ssc_cli_switch_current(gv->current);
	ssc_cli_window_upd_highlight(gv);
	return;
}

/*
 * clear contacts and reload them from the db
 */
void ssc_cli_reload_contacts(SSCGV* gv){ 
	debuginfo();
	WIN* contacts = gv->contacts;
	WPAGE* _page;
	sqlite3_stmt* stmt;
	sqlite3* db = gv->db;	
	int i,x;
	int page_count = contacts->page_count;
	byte* element = NULL;

	for(i = 0;i<page_count;i++){
		_page = contacts->pages[i];
		if(_page != NULL){
			for(x = 0;x<100;x++){
				element = _page->screen_content[x];
				if(element != NULL)free(element);
			}	
			free(_page);
		}
	}

	memset(contacts->pages,0,sizeof(WPAGE*) * 1000); 
	WPAGE* new_page = calloc(1,sizeof(WPAGE));
	contacts->pages[1] = new_page;
	contacts->current_page = new_page;

	sqlite3_prepare_v2(db,"select username from knownusers where NOT uid=0",-1,&stmt,NULL);
	assert(stmt);

	while(sqlite3_step(stmt) == SQLITE_ROW){
		ssc_cli_add_item(contacts,(byte*)sqlite3_column_text(stmt,0));
	}	

	sqlite3_finalize(stmt);
	ssc_cli_window_reload(contacts);
	ssc_cli_switch_current(gv->current);
	return;
}

/*
 * parse user commands 
 */
void ssc_cli_cmd_parser(SSCGV* gv,byte* userinput){ 
	debuginfo();
	if(!userinput)return;
	WIN* contacts = gv->contacts;
	sqlite3* db = gv->db;
	BIO* conn = gv->conn;
	EVP_PKEY* privkey = gv->privkey;
	int userinputl = strlen(userinput);
	if(userinputl < 3){
		debugstdscr("length of command too short.");	
	}
	else if(strncmp(userinput,"send",4) == 0){
		byte* tmp = userinput+5;
		byte* usrname = strtok(tmp," ");	
		size_t usrnamel = strlen(tmp);
		char getmsg_prebuf[strlen(usrname) + 20];
		sprintf(getmsg_prebuf," Message[%s]: ",usrname);
		byte* message = _getstr_custom(getmsg_prebuf);
		if(!message)return;
		byte* encbuf = encrypt_msg(usrname,message,privkey,db); 
		if(!encbuf)return;
		sqlite3_stmt* stmt;
		sqlite3_prepare_v2(db,"insert into messages(msgid,uid,uid2,message)values(NULL,1,?1,?2);",-1,&stmt,NULL);
		sqlite3_bind_int(stmt,1,get_user_uid(usrname,db));
		sqlite3_bind_text(stmt,2,message,-1,0);
		sqlite3_step(stmt);
		sqlite3_finalize(stmt);
		BIO_write(conn,encbuf,strlen(encbuf));
		free(encbuf);
		free(message);
		ssc_cli_msg_upd(gv,usrname);
	}
	else if(strncmp(userinput,"add",3) == 0){
		byte* usraddbuf = (byte*)server_get_user_rsa(userinput+4);
		BIO_write(conn,usraddbuf,strlen(usraddbuf));
		free(usraddbuf);
		byte* rxbuf = calloc(1,4096);
		BIO_read(conn,rxbuf,4096);
		rxbuf[4095] = '\0';
		if(strcmp(rxbuf,"GETRSA_RSP_ERROR") == 0){
			cerror("could not retrieve user from server (GETRSA_RSP_ERROR)");
		}
		else{
			sqlite3_stmt* stmt;
			sscso* obj = SSCS_open(rxbuf);
			byte* rsapub64 = SSCS_object_string(obj,"b64rsa");
			if(!rsapub64){
				cerror("recv packet to add a user did not contain a public key!");
				return;
			}
			int rsalen = SSCS_object_int(obj,"rsalen");
			sqlite3_prepare_v2(db,"insert into knownusers(uid,username,rsapub64,rsalen)values(NULL,?1,?2,?3);",-1,&stmt,NULL);
			sqlite3_bind_text(stmt,1,userinput+4,-1,0);
			sqlite3_bind_text(stmt,2,(byte*)rsapub64,-1,0);
			sqlite3_bind_int(stmt,3,rsalen);
			sqlite3_step(stmt);
			sqlite3_finalize(stmt);
			free(rsapub64);
			SSCS_release(&obj);
			ssc_cli_add_item(contacts,userinput+4);
		}
		free(rxbuf);
		return;
	}
	else if(strncmp(userinput,"switch",6) == 0){
		byte* usrname = userinput+7;
		byte* contact_temp = NULL;
		unsigned int usrname_l = strlen(usrname);
		WPAGE* current_page_contacts = gv->contacts->current_page;

		for(int i = 0;i<=current_page_contacts->elements;i++){
			contact_temp = current_page_contacts->screen_content[i];
			if(contact_temp != NULL){
				if(usrname_l == strlen(contact_temp)){
					if(strncmp(usrname,contact_temp,usrname_l) == 0){
						current_page_contacts->currently_selected = i;
						break;
					}
				}
			}
		}
		ssc_cli_msg_clear(gv);
		ssc_cli_msg_upd(gv,usrname);
	}
	else if(strncmp(userinput,"delete",6) == 0){
		debugstdscr("delete");	
		sqlite3_stmt* stmt;
		sqlite3_prepare_v2(db,"delete from KNOWNUSERS WHERE username = ?1;",-1,&stmt,NULL);
		sqlite3_bind_text(stmt,1,userinput+7,-1,0);
		sqlite3_step(stmt);
		sqlite3_finalize(stmt);
	}
	else{
		debugstdscr("Command not recognized.");
	}
		debugstdscr(userinput);
		ssc_cli_switch_current(gv->current);
	return;
}

void ssc_cli_reload_all(SSCGV* gv){
	clear(); /* clear screen */
	
	attron(COLOR_PAIR(2));
	ssc_cli_cust_updwin(gv->contacts,TRUE);
	ssc_cli_cust_updwin(gv->sent,TRUE);
	ssc_cli_cust_updwin(gv->received,TRUE);
	attroff(COLOR_PAIR(2));
	
	int col,row;
	getmaxyx(stdscr,row,col);
	
	attron(COLOR_PAIR(1));
	attron(A_REVERSE);

	mvwprintw(stdscr,1,(col * 0.02 + 2)," Contacts ");
	mvwprintw(stdscr,1,(gv->received->startx + 2)," Received ");
	mvwprintw(stdscr,1,(gv->sent->startx + 2)," Sent ");

	attroff(A_REVERSE);

	ssc_cli_reload_contacts(gv);
	ssc_cli_window_reload(gv->sent);
	ssc_cli_window_reload(gv->received);

	ssc_cli_switch_current(gv->current);
	return;
}
