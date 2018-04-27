#include "gui.h"
void init_gui(struct sscswidgets_gui* data){
	sqlite3_stmt* stmt;
	sqlite3* db = (data->backend_vars)->db;
	sqlite3_prepare_v2(db,"select username from knownusers where NOT uid=0",-1,&stmt,NULL); //uid(0) is testuser 
	
	assert(stmt);
	while(sqlite3_step(stmt) == SQLITE_ROW){
		add_contact_gui(data->contactslist,data,sqlite3_column_text(stmt,0));	
	}
	sqlite3_finalize(stmt);
	return;
}
void clear_messages_gui(struct sscswidgets_gui* data){ //clear sent messages GtkListBox and received_messages
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

void append_list_string_gui(GtkWidget* list,char* item){ //Add Label to Container List
	GtkWidget* label = gtk_label_new(item);
	gtk_container_add((GtkContainer*)list,label);
	gtk_widget_show(label);
	return;
}

void change_current_user_gui(GtkWidget* widget,gpointer data){
	(void)widget;
	struct sscsbutton_gui* pobj = data;
	clear_messages_gui(pobj->widgets);
	printf("changing current user from %s to %s\n",*((pobj->widgets)->current_username),pobj->item);
	*((pobj->widgets)->current_username) = pobj->item; //Change the pointer in main() to the current_username
	gtk_label_set_text((pobj->widgets)->chatpartnerlabel,pobj->item);
	gtk_widget_show((GtkWidget*)(pobj->widgets)->chatpartnerlabel);

	//get messages from server & and add stored ones to GUI
	//getmessages_gui(NULL,pobj);
	return;
}

void add_contact_gui(GtkWidget* contactslist,struct sscswidgets_gui* widgets,char* contactname){
	size_t contactname_len = strlen(contactname);
	if(contactname_len <= 0 || contactname_len >= 100)return;
	struct sscsbutton_gui* passedstruct = malloc(sizeof(struct sscsbutton_gui));
	passedstruct->widgets = widgets;

	char* contactname_alloc = malloc(contactname_len+1);
	memcpy(contactname_alloc,contactname,contactname_len+1);
	passedstruct->item = contactname_alloc;

	GtkWidget* button = gtk_button_new_with_label(contactname);
	g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(change_current_user_gui),passedstruct);
	gtk_container_add((GtkContainer*)contactslist,button);
	gtk_widget_show(button);
	return;
}

void send_message_entry_gui(GtkEntry* entry,gpointer data){
	struct sscswidgets_gui* widgets = data;
	sqlite3* db = (widgets->backend_vars)->db;
	BIO* srvconn = ((widgets->backend_vars)->connection_variables)->bio_obj;		
	EVP_PKEY* priv_evp = (widgets->backend_vars)->privkey;
	char* username = *(widgets->current_username);
	GtkWidget* messagelist = widgets->messagelist;	
	const char* message = gtk_entry_get_text(GTK_ENTRY(entry));
	append_list_string_gui(messagelist,message);
	append_list_string_gui(widgets->recvlist," ");
	printf("Sending Message to %s\n",*(widgets->current_username));
	char* encbuf = (char*)encryptmsg(username,(unsigned char*)message,priv_evp,db); //"user" would be the receiving username
	if(!encbuf)return;
	printf("Encrypted message: %s with length: %d\n",encbuf,(int)strlen(encbuf));
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(db,"insert into messages(msgid,uid,uid2,message)values(NULL,1,?1,?2);",-1,&stmt,NULL);
//	sqlite3_bind_text(stmt,1,username,-1,0);
	sqlite3_bind_int(stmt,1,getUserUID(username,db));
	sqlite3_bind_text(stmt,2,message,-1,0);
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	BIO_write(srvconn,encbuf,strlen(encbuf));
	free(encbuf);
	gtk_entry_set_text(entry,"");
	return;
}

void add_user_entry_gui(GtkEntry* entry,gpointer data){
	struct sscswidgets_gui* widgets = data;
	GtkWidget* contactslist =  widgets->contactslist;
	char* username = (char*)gtk_entry_get_text(GTK_ENTRY(entry));
	addnewuser_gui(widgets,username);
	add_contact_gui(contactslist,widgets,username);	
	gtk_entry_set_text(entry,"");
	return;
}

void addnewuser_gui(struct sscswidgets_gui* widgets_gui,char* username){
	sqlite3* db = ((widgets_gui->backend_vars)->db);
	char* gtrsa64 = (char*)ServerGetUserRSA(username);		
	BIO_write(((widgets_gui->backend_vars)->connection_variables)->bio_obj,gtrsa64,strlen(gtrsa64));
	free(gtrsa64);
	char* rxbuf = calloc(1,4096);
	BIO_read(((widgets_gui->backend_vars)->connection_variables)->bio_obj,rxbuf,4096);
	if(strcmp(rxbuf,"GETRSA_RSP_ERROR") == 0){
		puts(rxbuf);
	} 
	else{
		sqlite3_stmt* stmt;
		sscso* obj = SSCS_open(rxbuf);
		char* rsapub64 = SSCS_object_string(obj,"b64rsa");
		if(!rsapub64){puts("No supplied public key");return;}
		int rsalen = SSCS_object_int(obj,"rsalen");
		sqlite3_prepare_v2(db,"insert into knownusers(uid,username,rsapub64,rsalen)values(NULL,?1,?2,?3);",-1,&stmt,NULL);
		sqlite3_bind_text(stmt,1,username,-1,0);
		sqlite3_bind_text(stmt,2,(const char*)rsapub64,-1,0);
		sqlite3_bind_int(stmt,3,rsalen);
		sqlite3_step(stmt);
		sqlite3_finalize(stmt);
		free(rsapub64);
		SSCS_release(&obj);
	}
		
	return;
}
void getmessages_gui(GtkWidget* notused,gpointer data){ //get message & add them to db
	(void)notused; //not used.
	sqlite3* db = (((sscvars_gui*)data)->backend_vars)->db;
	sqlite3_stmt* stmt;
	BIO* srvconn = ((((sscvars_gui*)data)->backend_vars)->connection_variables)->bio_obj;		
	EVP_PKEY* priv_evp = (((sscvars_gui*)data)->backend_vars)->privkey;
	GtkWidget* recvlist = ((sscvars_gui*)data)->recvlist;
	char* current_user = *(((sscvars_gui*)data)->current_username);
	if(!current_user)return;
	int i = 0;	
	char* getmsgbuf = (char*)ServerGetMessages(db);		
	if(!getmsgbuf)return;
	char* decbuf = NULL;
	char* recvbuf = malloc(200000);
	BIO_write(srvconn,getmsgbuf,strlen(getmsgbuf));	
	free(getmsgbuf);
	memset(recvbuf,'\0',200000);
	BIO_read(srvconn,recvbuf,199999);	
	
	if(strcmp(recvbuf,"ERROR") != NULL){
	sscsl* list = SSCS_list_open(recvbuf);
	while(1){
		i++;	
		sscsd* prebuf =	SSCS_list_data(list,i);	
		if(!prebuf)break;
		sscso* obj2 = SSCS_open(prebuf->data);
		SSCS_data_release(&prebuf);
		char* sender = SSCS_object_string(obj2,"sender");
		if(!sender)break;
		decbuf = (char*)decryptmsg(obj2->buf_ptr,priv_evp,db);	if(!decbuf)break;
		if(decbuf)printf("Decrypted Message from %s: %s\n",sender,decbuf); 
		sqlite3_prepare_v2(db,"insert into messages(msgid,uid,uid2,message)values(NULL,?1,1,?2);",-1,&stmt,NULL);	
		sqlite3_bind_int(stmt,1,getUserUID(sender,db));
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
	int currentuserUID = getUserUID(current_user,db);
	printf("UID for user %s is %i\n",current_user,currentuserUID);
	if(currentuserUID == -1)return;
	sqlite3_prepare_v2(db,"select message from messages where uid=?1 AND uid2=1",-1,&stmt,NULL);
	sqlite3_bind_int(stmt,1,currentuserUID);	
	while(sqlite3_step(stmt) == SQLITE_ROW){
		append_list_string_gui(recvlist,sqlite3_column_text(stmt,0));
		append_list_string_gui(((sscvars_gui*)data)->messagelist," ");
	}
	sqlite3_finalize(stmt);
	stmt = NULL;	
	sqlite3_prepare_v2(db,"select message from messages where uid=1 AND uid2=?1",-1,&stmt,NULL);
	sqlite3_bind_int(stmt,1,currentuserUID);
	while(sqlite3_step(stmt) == SQLITE_ROW){
		append_list_string_gui(((sscvars_gui*)data)->messagelist,sqlite3_column_text(stmt,0));
		append_list_string_gui(recvlist," ");
	
	}
	sqlite3_finalize(stmt);
	return;
}
