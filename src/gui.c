
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

#include "gui.h"
void init_gui(struct sscswidgets_gui* data){
	debugprint();
	sqlite3_stmt* stmt;
	sqlite3* db = (data->backend_vars)->db;
	sqlite3_prepare_v2(db,"select username from knownusers where NOT uid=0",-1,&stmt,NULL); //uid(0) is testuser 
	
	assert(stmt);
	while(sqlite3_step(stmt) == SQLITE_ROW){
		add_contact_gui(data->contactslist,data,(byte*)sqlite3_column_text(stmt,0));	
	}
	sqlite3_finalize(stmt);
	return;
}
void clear_messages_gui(struct sscswidgets_gui* data){ //clear sent messages GtkListBox and received_messages
	debugprint();
	GtkContainer* container = (GtkContainer*)data->messagelist;	
	GList *children, *iter;
	children = gtk_container_get_children(GTK_CONTAINER(container));
	for(iter = children; iter != NULL; iter = g_list_next(iter)){
		gtk_widget_destroy(GTK_WIDGET(iter->data));
	}
	g_list_free(children);
	GtkContainer* recvlist = (GtkContainer*)data->recvlist;
	GList* children2 = gtk_container_get_children(GTK_CONTAINER(recvlist));
	for(iter = children2; iter != NULL; iter = g_list_next(iter)){
		gtk_widget_destroy(GTK_WIDGET(iter->data));
	}
	g_list_free(children2);
	
	return;
}

static void internal_create_new_label(GtkWidget* list,byte* item){
	debugprint();
	GtkWidget* label = gtk_label_new(item);
	gtk_container_add((GtkContainer*)list,label);
	gtk_widget_show(label);
	return;
}

static void internal_create_blank_label(GtkWidget* list){
	debugprint();
	GtkWidget* label = gtk_label_new("");
	gtk_container_add((GtkContainer*)list,label);
	gtk_widget_show(label);
	return;
}

void append_list_string_gui(GtkWidget* list,GtkWidget* other_list,byte* item){ //add string to container list and space to other_list
	debugprint();
	int maxperline = 32;
	int rem_l = strlen(item);
	char temp[maxperline+1];
	temp[maxperline] = '\0';
	int index = 0;
	while(rem_l > maxperline){
		memcpy(temp,(item+index),maxperline);
		internal_create_new_label(list,temp);
		internal_create_blank_label(other_list);
		rem_l -= maxperline;		
		index += maxperline;
	}
	internal_create_new_label(list,(item+index));
	internal_create_blank_label(other_list);
	internal_create_blank_label(list);	
	internal_create_blank_label(other_list);
	return;
}
void internal_scroll_window_msg_bottom_gui_2(GtkWidget* unused,void* data){ /* wrapper for internal_scroll_window_msg_bottom_gui() so that it can be used as a Gtk Callback function */
	debugprint();
	(void)unused;
	internal_scroll_window_msg_bottom_gui((struct sscswidgets_gui*)data);
	return;
}
void internal_scroll_window_msg_bottom_gui(struct sscswidgets_gui* widgets){
	debugprint();

	GtkScrolledWindow* scrwin = (GtkScrolledWindow*)widgets->messagescrollwindow;

	gdouble upper = 0;
	gdouble pgsz = 0;
	gdouble lower = 0;
	gdouble adjVal = 0;
	gdouble val = 0;

	GtkAdjustment * vAdj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrwin));

	upper = gtk_adjustment_get_upper(vAdj);
	pgsz = gtk_adjustment_get_page_size(vAdj);

	adjVal = upper - pgsz;
	gtk_adjustment_set_value(vAdj,adjVal); 
	gtk_scrolled_window_set_vadjustment(GTK_SCROLLED_WINDOW(scrwin),vAdj);
	return;
}

void change_current_user_gui(GtkWidget* widget,gpointer data){
	debugprint();
	(void)widget;
	struct sscsbutton_gui* pobj = data;
	clear_messages_gui(pobj->widgets);
	printf("changing current user from %s to %s\n",*((pobj->widgets)->current_username),pobj->item);
	*((pobj->widgets)->current_username) = pobj->item; //Change the pointer in main() to the current_username
	gtk_label_set_text((pobj->widgets)->chatpartnerlabel,pobj->item);
	gtk_widget_show((GtkWidget*)(pobj->widgets)->chatpartnerlabel);

	//get messages from server & and add stored ones to GUI
	getmessages_gui(pobj->widgets);
	return;
}

void add_contact_gui(GtkWidget* contactslist,struct sscswidgets_gui* widgets,byte* contactname){
	debugprint();
	size_t contactname_len = strlen(contactname);
	if(contactname_len <= 0 || contactname_len >= 100)return;
	struct sscsbutton_gui* passedstruct = malloc(sizeof(struct sscsbutton_gui));
	passedstruct->widgets = widgets;

	byte* contactname_alloc = malloc(contactname_len+1);
	memcpy(contactname_alloc,contactname,contactname_len+1);
	passedstruct->item = contactname_alloc;

	GtkWidget* button = gtk_button_new_with_label(contactname);
	g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(change_current_user_gui),passedstruct);
	gtk_container_add((GtkContainer*)contactslist,button);
	gtk_widget_show(button);
	return;
}

void send_message_entry_gui(GtkEntry* entry,gpointer data){
	debugprint();

	struct sscswidgets_gui* widgets = data;

	sqlite3* db = (widgets->backend_vars)->db;
	BIO* srvconn = ((widgets->backend_vars)->connection_variables)->bio_obj;		
	EVP_PKEY* priv_evp = (widgets->backend_vars)->privkey;
	byte* username = *(widgets->current_username);
	byte* message = (byte*)gtk_entry_get_text(GTK_ENTRY(entry));
	GtkWidget* messagelist = widgets->messagelist;	
	GtkWidget* mainwin = widgets->window;

	if(!username)return; /* saveguard to check if user has selected a user(in the gui) so that the initial user is not NULL (causing this code to SEGFAULT) */
	byte* encbuf = (byte*)encrypt_msg(username,(byte*)message,priv_evp,db); //"user" would be the receiving username
	if(!encbuf){
		cerror("Could not encrypt message");
		return;
	}
	cdebug("Sending Message to %s\n",*(widgets->current_username));
	cdebug("Encrypted message: %s with length: %d\n",encbuf,(int)strlen(encbuf));

	BIO_write(srvconn,encbuf,strlen(encbuf));
	char rsp[3];
	BIO_read(srvconn,rsp,3);
	if(strncmp(rsp,"ACK",3) != 0){
		cerror("Server did not receive message");
		free(encbuf);
		return;
	}

	append_list_string_gui(messagelist,widgets->recvlist,message); /* append item to chat only after server ack */

	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db,"insert into messages(msgid,uid,uid2,message)values(NULL,1,?1,?2);",-1,&stmt,NULL);
	sqlite3_bind_int(stmt,1,get_user_uid(username,db));
	sqlite3_bind_text(stmt,2,message,-1,0);
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	free(encbuf);

	gtk_entry_set_text(entry,"");
	internal_scroll_window_msg_bottom_gui(widgets);
	gtk_widget_show_all(mainwin);
	return;
}

void add_user_entry_gui(GtkEntry* entry,gpointer data){
	debugprint();
	struct sscswidgets_gui* widgets = data;
	byte* username = (byte*)gtk_entry_get_text(GTK_ENTRY(entry));
#ifdef DEBUG
	fprintf(stdout,"trying to add user %s\n",username);
#endif
	addnewuser_gui(widgets,username);
	gtk_entry_set_text(entry,"");
	return;
}

void addnewuser_gui(struct sscswidgets_gui* widgets_gui,byte* username){
	debugprint();
	GtkWidget* contactslist =  widgets_gui->contactslist;

	sqlite3* db = ((widgets_gui->backend_vars)->db);
	byte* gtrsa64 = (byte*)server_get_user_rsa(username);		
	BIO_write(((widgets_gui->backend_vars)->connection_variables)->bio_obj,gtrsa64,strlen(gtrsa64));
	free(gtrsa64);
	byte* rxbuf = calloc(1,4096);
	BIO_read(((widgets_gui->backend_vars)->connection_variables)->bio_obj,rxbuf,4095);
	rxbuf[4095] = '\0'; /* add NULL terminator */
#ifdef DEBUG
	fprintf(stdout,"DEBUG: got usersaobj %s from srv\n",rxbuf);
#endif
	if(strcmp(rxbuf,"ERR") != 0){
		sqlite3_stmt* stmt;
		sscso* obj = SSCS_open(rxbuf);
		byte* rsapub64 = SSCS_object_string(obj,"b64rsa");
		if(!rsapub64){
			cerror("Encoded object did not contain a public key!");	
			free(rxbuf);
			return;
		}
		int rsalen = SSCS_object_int(obj,"rsalen");
		sqlite3_prepare_v2(db,"insert into knownusers(uid,username,rsapub64,rsalen)values(NULL,?1,?2,?3);",-1,&stmt,NULL);
		sqlite3_bind_text(stmt,1,username,-1,0);
		sqlite3_bind_text(stmt,2,(byte*)rsapub64,-1,0);
		sqlite3_bind_int(stmt,3,rsalen);
		sqlite3_step(stmt);
		sqlite3_finalize(stmt);
		free(rsapub64);
		SSCS_release(&obj);
		add_contact_gui(contactslist,widgets_gui,username);	
	}
	else{
		cinfo("Could not add user, server side error (maybe user doesnt exist?) ");
	}
	free(rxbuf);
	return;
}
gboolean getmessages_gui(void* data){ //get message & add them to db
	debugprint();
	//Get Variables from passed structure
	sqlite3* db = (((sscvars_gui*)data)->backend_vars)->db;
	sqlite3_stmt* stmt;
	BIO* srvconn = ((((sscvars_gui*)data)->backend_vars)->connection_variables)->bio_obj;		
	EVP_PKEY* priv_evp = (((sscvars_gui*)data)->backend_vars)->privkey;
	GtkWidget* recvlist = ((sscvars_gui*)data)->recvlist;
	GtkWidget* messagelist = ((sscvars_gui*)data)->messagelist;
	byte* current_user = *(((sscvars_gui*)data)->current_username);
	if(!current_user)return 0;
	byte* getmsgbuf = (byte*)server_get_messages(db);	//Get buffer to send to server
	if(!getmsgbuf)return 0;

	byte* decbuf = NULL;
	byte* recvbuf = calloc(1,200000);
	BIO_write(srvconn,getmsgbuf,strlen(getmsgbuf));	//Send buffer to server
	free(getmsgbuf);
	BIO_read(srvconn,recvbuf,199999); //Read response
	recvbuf[199999] = 0x0;
	if(strncmp(recvbuf,"ERROR",5) != 0){
#ifndef RELEASE_IMAGE
	cdebug("received response from server -- %s",recvbuf);	
#endif
	sscsl* list = SSCS_list_open(recvbuf);
	if(list == NULL){
		cerror("Could not open list");
	}
	int i = 0;	
	while(1){
			i++;	
			sscsd* prebuf =	SSCS_list_data(list,i);	
			if(!prebuf){
				cdebug("Could not retrieve item %d from list",i);	
				break;
			}
			sscso* obj2 = SSCS_open(prebuf->data);
			SSCS_data_release(&prebuf);
			byte* sender = SSCS_object_string(obj2,"sender");
			if(!sender){
				cerror("Could not retrieve sender from message.");
				break;
			}
			decbuf = (byte*)decrypt_msg(obj2->buf_ptr,priv_evp,db);	
			if(!decbuf)break;
			filter_string(decbuf); /* filter down to small subset of ASCII */
			cdebug("Decrypted Message from %s: %s\n",sender,decbuf); 
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
	int currentuserUID = get_user_uid(current_user,db);
	if(currentuserUID == -1)return 0;

	sqlite3_prepare_v2(db,"select uid,message from messages where uid=?1 AND uid2=1 OR uid=1 AND uid2=?2",-1,&stmt,NULL);
	sqlite3_bind_int(stmt,1,currentuserUID);
	sqlite3_bind_int(stmt,2,currentuserUID);
	while(sqlite3_step(stmt) == SQLITE_ROW){
		int sqluid = sqlite3_column_int(stmt,0);
		if(sqluid == 1){
			append_list_string_gui(messagelist,recvlist,(byte*)sqlite3_column_text(stmt,1));
		}	
		else if(sqluid == currentuserUID){
			append_list_string_gui(recvlist,messagelist,(byte*)sqlite3_column_text(stmt,1));
		}
	}
	sqlite3_finalize(stmt);
	return 0;
}
