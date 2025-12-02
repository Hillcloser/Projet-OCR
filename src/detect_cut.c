#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define BACKGROUND_THRESHOLD 240
#define PROJECTION_THRESHOLD 2

struct Gap 
{
    int start;
    int end;
    int width;
};

static gboolean is_background(guchar r, guchar g, guchar b)
{
    return (r > BACKGROUND_THRESHOLD && g > BACKGROUND_THRESHOLD && b > BACKGROUND_THRESHOLD);
}
static int* calculate_vertical_projection(GdkPixbuf *pixbuf) 
{
    int width = gdk_pixbuf_get_width(pixbuf);
    int height = gdk_pixbuf_get_height(pixbuf);
    int n_channels = gdk_pixbuf_get_n_channels(pixbuf);
    int rowstride = gdk_pixbuf_get_rowstride(pixbuf);
    guchar *pixels = gdk_pixbuf_get_pixels(pixbuf);
    
    int *projection = g_new0(int, width);

    for (int y = 0; y < height; y++) 
    {
        for (int x = 0; x < width; x++) 
	{
            guchar *p = pixels + y * rowstride + x * n_channels;
            if (!is_background(p[0], p[1], p[2]))
                projection[x]++;
        }
    }
    return projection;
}

static int* calculate_horizontal_projection(GdkPixbuf *pixbuf) 
{
    int width = gdk_pixbuf_get_width(pixbuf);
    int height = gdk_pixbuf_get_height(pixbuf);
    int n_channels = gdk_pixbuf_get_n_channels(pixbuf);
    int rowstride = gdk_pixbuf_get_rowstride(pixbuf);
    guchar *pixels = gdk_pixbuf_get_pixels(pixbuf);
    
    int *projection = g_new0(int, height);

    for (int y = 0; y < height; y++) 
    {
        for (int x = 0; x < width; x++) 
	{
            guchar *p = pixels + y * rowstride + x * n_channels;
            if (!is_background(p[0], p[1], p[2]))
                projection[y]++;
        }
    }
    return projection;
}

static struct Gap find_widest_gap(int *projection, int length) 
{
    struct Gap best_gap = {0, 0, 0};
    struct Gap current_gap = {0, 0, 0};
    gboolean in_gap = FALSE;

    for (int i = 0; i < length; i++) 
    {
        if (projection[i] < PROJECTION_THRESHOLD) 
	{
            if (!in_gap) 
	    {
                in_gap = TRUE;
                current_gap.start = i;
                current_gap.width = 0;
            }
            current_gap.width++;
        } 
	else 
	{
            if (in_gap) 
	    {
                in_gap = FALSE;
                current_gap.end = i;
                if (current_gap.width > best_gap.width) 
                    best_gap = current_gap;
            }
        }
    }
    if (in_gap && current_gap.width > best_gap.width) 
    {
        current_gap.end = length;
        best_gap = current_gap;
    }
    return best_gap;
}

static int count_empty_lines(int *h_projection, int height) 
{
    int count = 0;
    for (int y = 0; y < height; y++) 
    {
        if (h_projection[y] < PROJECTION_THRESHOLD)
            count++;
    }
    return count;
}

static GdkPixbuf* trim_whitespace(GdkPixbuf *pixbuf) 
{
    int width = gdk_pixbuf_get_width(pixbuf);
    int height = gdk_pixbuf_get_height(pixbuf);
    int n_channels = gdk_pixbuf_get_n_channels(pixbuf);
    int rowstride = gdk_pixbuf_get_rowstride(pixbuf);
    guchar *pixels = gdk_pixbuf_get_pixels(pixbuf);

    int min_x = width, min_y = height, max_x = 0, max_y = 0;

    for (int y = 0; y < height; y++) 
    {
        for (int x = 0; x < width; x++) 
	{
            guchar *p = pixels + y * rowstride + x * n_channels;
            if (!is_background(p[0], p[1], p[2])) 
	    {
                if (x < min_x) min_x = x;
                if (x > max_x) max_x = x;
                if (y < min_y) min_y = y;
                if (y > max_y) max_y = y;
            }
        }
    }

    if (max_x < min_x || max_y < min_y) 
        return gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, 1, 1);

    int new_width = max_x - min_x + 1;
    int new_height = max_y - min_y + 1;
    GdkPixbuf *sub = gdk_pixbuf_new_subpixbuf(pixbuf, min_x, min_y, new_width, new_height);
    GdkPixbuf *trimmed = gdk_pixbuf_copy(sub);
    g_object_unref(sub);
    return trimmed;
}

static void save_pixbuf(gpointer data, gpointer user_data) 
{
    static int i = 0;
    GdkPixbuf *pixbuf = GDK_PIXBUF(data);
    char *prefix = (char*)user_data;
    GError *error = NULL;

    char filename[256];
    g_snprintf(filename, 256, "%s_%02d.png", prefix, i++);
    gdk_pixbuf_save(pixbuf, filename, "png", &error, NULL);
    
    if (error) 
    {
        g_printerr("Save error %s: %s\n", filename, error->message);
        g_error_free(error);
    } 
    else 
        g_print("Save: %s\n", filename);
}


int main(int argc, char **argv) 
{
    if (argc != 2) 
    {
        g_printerr("Arguments error : Should have 1 argument");
        return 1;
    }

    char *input_filename = argv[1];
    GError *error = NULL;
    gtk_init(&argc, &argv);

    // 1.loading
    GdkPixbuf *original_image = gdk_pixbuf_new_from_file(input_filename, &error);
    if (error) 
    {
        g_printerr("Image loading error %s: %s\n", input_filename, error->message);
        g_error_free(error);
        return 1;
    }

    // 2. vertical projection
    int *v_proj = calculate_vertical_projection(original_image);

    // 3. Finding gap
    struct Gap split_gap = find_widest_gap(v_proj, gdk_pixbuf_get_width(original_image));
    
    if (split_gap.width == 0) 
    {
        g_printerr("No gap found\n");
        g_object_unref(original_image);
        g_free(v_proj);
        return 1;
    }

    GdkPixbuf *crop_left = gdk_pixbuf_new_subpixbuf(original_image, 
                                                    0, 
						    0, 
                                                    split_gap.start, 
                                                    gdk_pixbuf_get_height(original_image));
                                                    
    GdkPixbuf *crop_right = gdk_pixbuf_new_subpixbuf(original_image, 
                                                     split_gap.end, 
						     0, 
                                                     gdk_pixbuf_get_width(original_image) - split_gap.end, 
                                                     gdk_pixbuf_get_height(original_image));

    // 4. Identifying list of words
    int *h_proj_left = calculate_horizontal_projection(crop_left);
    int *h_proj_right = calculate_horizontal_projection(crop_right);
    int empty_lines_left = count_empty_lines(h_proj_left, gdk_pixbuf_get_height(crop_left));
    int empty_lines_right = count_empty_lines(h_proj_right, gdk_pixbuf_get_height(crop_right));

    GdkPixbuf *list_area_pixbuf = NULL;
    int *list_h_proj = NULL;
    
    if (empty_lines_left > empty_lines_right && empty_lines_left > 10) 
    { 
        list_area_pixbuf = crop_left;
        list_h_proj = h_proj_left;
	gdk_pixbuf_save(crop_right, "grille.png","png",NULL,NULL);// SAUVEGARDE DE LA GRILLE CROP (reste les marges)
								  
        g_free(h_proj_right); 
        g_object_unref(crop_right); 
    } 
    else 
    {
        list_area_pixbuf = crop_right;
        list_h_proj = h_proj_right;
	gdk_pixbuf_save(crop_left, "grilleleft.png","png",NULL,NULL);// SAUVEGARDE DE LA GRILLE CROP (reste les marges)

        g_free(h_proj_left); 
        g_object_unref(crop_left); 
    }

    // 5. Cutting words in list
    GList *word_images = NULL;
    int list_height = gdk_pixbuf_get_height(list_area_pixbuf);
    int list_width = gdk_pixbuf_get_width(list_area_pixbuf);
    int word_start_y = -1;

    for (int y = 0; y < list_height; y++) 
    {
        gboolean is_text = (list_h_proj[y] >= PROJECTION_THRESHOLD);

        if (is_text && word_start_y == -1)
            word_start_y = y;
	else if (!is_text && word_start_y != -1) 
	{
            int word_height = y - word_start_y;
            if (word_height > 0) 
	    {
                GdkPixbuf *untrimmed_word = gdk_pixbuf_new_subpixbuf(list_area_pixbuf, 
                                                                    0, 
								    word_start_y, 
                                                                    list_width, word_height);
                // 6. Trim
                GdkPixbuf *trimmed_word = trim_whitespace(untrimmed_word);
                word_images = g_list_append(word_images, trimmed_word);
                
                g_object_unref(untrimmed_word); 
            }
            word_start_y = -1;
        }
    }
    
    if (word_start_y != -1) 
    {
        int word_height = list_height - word_start_y;
        GdkPixbuf *untrimmed_word = gdk_pixbuf_new_subpixbuf(list_area_pixbuf, 
                                                             0, 
							     word_start_y, 
                                                             list_width,
							     word_height);
        GdkPixbuf *trimmed_word = trim_whitespace(untrimmed_word);
        word_images = g_list_append(word_images, trimmed_word);
        g_object_unref(untrimmed_word);
    }

    // 7. Saving and free
    g_print("\nSaving image words...\n");
    //mkdir("Words", 0777); //save les mots dans le dossier
    // MANQUE : couper les mots en lettres dans leur fichier
    g_list_foreach(word_images, save_pixbuf, "mot");

    g_free(v_proj);
    g_free(list_h_proj);
    g_list_free_full(word_images, g_object_unref); 
    g_object_unref(list_area_pixbuf); 
    g_object_unref(original_image); 

    g_print("Cutting word list finished\n");
    return 0;
}
