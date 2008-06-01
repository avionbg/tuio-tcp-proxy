#include <stdio.h>
#include <gtk/gtk.h>

/* Prompt an about dialog */

static void toxy_tray_icon_prompt_about(GtkMenuItem *menuitem,gpointer user_data){
	GtkWidget *icon = GTK_WIDGET(user_data);
	GtkWidget * dialog = gtk_message_dialog_new(NULL,
		GTK_DIALOG_MODAL,GTK_MESSAGE_INFO,GTK_BUTTONS_OK,
		"ToXy - Tuio sOcket proXy v 1.0\n"
		"by Goran Medakovic @jet-plane\n\n"
		"This software is registered under GPL V2."
		);
	
	g_signal_connect(dialog,"response",G_CALLBACK(gtk_widget_destroy),dialog);
	
	gtk_widget_show (dialog);
}

/* Toggle Tuio service */
static void toxy_tray_icon_tuio_toggled(GtkCheckMenuItem *checkmenuitem,
			gpointer          user_data) {
	printf("%s\n",__func__);
	/* TODO: Add the handler */
}

void toxy_tray_icon_popup_menu(GtkStatusIcon *status_icon,
										guint          button,
										guint          activate_time,
										gpointer       user_data) {

 	GtkWidget *menu = GTK_WIDGET(user_data);
 	
	gtk_menu_popup (GTK_MENU (menu), NULL, NULL,
			gtk_status_icon_position_menu, status_icon,
			button, activate_time);

}

typedef struct {
	GtkStatusIcon *icon;
	GtkWidget *menu;
} ToxyTrayIcon;

ToxyTrayIcon* toxy_tray_icon_new() {
	GtkStatusIcon *icon = gtk_status_icon_new();

 	GtkWidget *menu, *item;
 	
	menu = gtk_menu_new ();

	item = gtk_menu_item_new_with_mnemonic ("_About");
	g_signal_connect_swapped (item, "activate",
				  G_CALLBACK (toxy_tray_icon_prompt_about), NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	
	item = gtk_separator_menu_item_new ();
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	
	item = gtk_check_menu_item_new_with_mnemonic("_Tuio On/Off");
	g_signal_connect_swapped (item, "toggled",G_CALLBACK (toxy_tray_icon_tuio_toggled), NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	
	item = gtk_menu_item_new_with_mnemonic ("_Exit");
	g_signal_connect_swapped (item, "activate",G_CALLBACK (gtk_main_quit), NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	
	gtk_widget_show_all (menu);


	g_signal_connect(G_OBJECT(icon), 
                         "popup-menu",
                         G_CALLBACK(toxy_tray_icon_popup_menu), menu);
                         
	gtk_status_icon_set_from_icon_name(icon, 
                                           GTK_STOCK_NETWORK);
	gtk_status_icon_set_tooltip(icon, 
                                    "ToXy Tuio sOcket proXy tray");
                                    
	gtk_status_icon_set_visible(icon, TRUE);

	ToxyTrayIcon * res = g_new0(ToxyTrayIcon,1);
	res->icon = icon;
	res->menu = menu;
	g_object_ref_sink(res->menu);
	return res;
}

void toxy_tray_icon_destroy(ToxyTrayIcon *icon){
	g_object_unref(icon->icon );
	g_object_unref(icon->menu );
	g_free(icon);
}

int main(int argc, char** argv)
{
	ToxyTrayIcon *tray_icon;
	gtk_init(&argc, &argv);
	
	tray_icon = toxy_tray_icon_new();
	
	gtk_main();
	
	toxy_tray_icon_destroy(tray_icon);
	
	return 0;
}
