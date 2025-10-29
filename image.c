#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	if (argc < 3 || argc > 4)
	{
		g_printerr("Pas assez de parametre\n");
		g_printerr("Utilisation ./image image entree image sortie nb_rotation\n");
		return 1;
	}
	gtk_init(&argc,&argv);
	GError *error = NULL;
	GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(argv[1],&error);
	if (!pixbuf)
	{	
		g_printerr("erreur chargement image erreur : %s\n",error->message);
		g_error_free(error);
		return 1;		
	}

	int width = gdk_pixbuf_get_width(pixbuf);
	int height= gdk_pixbuf_get_height(pixbuf);
	int n_channels = gdk_pixbuf_get_n_channels(pixbuf);
	int rowstride = gdk_pixbuf_get_rowstride(pixbuf);
	guchar *pixels = gdk_pixbuf_get_pixels(pixbuf);

	if (n_channels < 3) {
    	g_printerr("Image non RGB/RGBA : %d canaux\n", n_channels);
    	g_object_unref(pixbuf);
    	return 1;
	}
	for (int y = 0 ; y<height ; y++)
	{	
		guchar *row = pixels + y * rowstride;
		for(int x = 0 ; x<width ; x++)
		{
			guchar *p = row + x*n_channels;
			guchar r = p[0];
			guchar g = p[1];
			guchar b = p[2];
			guchar gray = (guchar)(0.3 * r + 0.59 * g + 0.11 * b);
			p[0] = p[1] = p[2] = gray;
		}
	}
	if (argc > 3)
	{
		for (int i = 0 ; i < atoi(argv[3]) ; i++)
		{
			pixbuf = gdk_pixbuf_rotate_simple(pixbuf,GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE);
		}
	}

	if (!gdk_pixbuf_save(pixbuf, argv[2],"png", &error, NULL))
	{
		g_printerr("Erreur Sauvegarde 🛑: %s\n",error->message);
		g_error_free(error);
	}
	else{
		g_print("conversion réussie ! 👌 : %s\n",argv[2]);
	}


	GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), argv[1]);
	gtk_window_set_default_size(GTK_WINDOW(window),width,height);
	GtkWidget *image = gtk_image_new_from_pixbuf(pixbuf);
	gtk_container_add(GTK_CONTAINER(window), image);


	g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit),NULL);

	gtk_widget_show_all(window);
	gtk_main();
	
	g_object_unref(pixbuf);
	return 0;
}



