//#define DEBUG

#include <stdlib.h>
#include "stage.h"
#include "gui.h"

extern rtk_fig_t* fig_debug;
extern int _stg_quit;

// movies can be saved at these multiples of real time
//const int STG_MOVIE_SPEEDS[] = {1, 2, 5, 10, 20, 50, 100};
//const int STG_MOVIE_SPEED_COUNT = 7; // must match the static array length

// declare callbacks for menu items
void gui_menu_matrix_cb( gpointer data, guint action, GtkWidget* mitem );
void gui_menu_polygons_cb( gpointer data, guint action, GtkWidget* mitem );
void gui_menu_debug_cb( gpointer data, guint action, GtkWidget* mitem );
void gui_menu_file_export_frame_cb( gpointer data, guint action, GtkWidget* mitem );
void gui_menu_file_export_sequence_cb( gpointer data, guint action, GtkWidget* mitem );
void gui_menu_file_save_cb( gpointer data, guint action, GtkWidget* mitem );
void gui_menu_file_exit_cb( void );
void gui_menu_layer_cb( gpointer data, guint action, GtkWidget* mitem );
void gui_menu_clock_pause_cb( gpointer data, guint action, GtkWidget* mitem );

static GtkItemFactoryEntry menu_table[] = {
  { "/_File",         NULL,      NULL, 0, "<Branch>" },
  { "/File/tear1",    NULL,      NULL, 0, "<Tearoff>" },
  { "/File/About", NULL,    NULL, 0, "<Item>" },
  { "/File/_Save", "<CTRL>S", gui_menu_file_save_cb,  1, "<Item>", GTK_STOCK_SAVE },
  { "/File/Export", NULL,   NULL, 1, "<Branch>" },
  { "/File/Export/Single JPEG", "<CTRL>J", gui_menu_file_export_frame_cb, 1, "<Item>" },
  { "/File/Export/Sequence of JPEGs", "<CTRL>K",  gui_menu_file_export_sequence_cb, 1, "<CheckItem>" },
  //{ "/File/Export/Laser", NULL,   NULL, 1, "<RadioItem>" },
  //{ "/File/Export/Ranger", NULL,   NULL, 2, "/File/Export/Laser" },
  //{ "/File/Export/Blob", NULL,   NULL, 3,  "/File/Export/Laser" },
  //{ "/File/Export/Fiducial", NULL,   NULL, 4,  "/File/Export/Laser" },
  { "/File/sep1",     NULL,      NULL, 0, "<Separator>" },
  { "/File/_Quit",    "<CTRL>Q", gui_menu_file_exit_cb, 0, "<StockItem>", GTK_STOCK_QUIT },
  { "/_View",         NULL,      NULL, 0, "<Branch>" },
  { "/View/tear1",    NULL,      NULL, 0, "<Tearoff>" },
  { "/View/Data", NULL,   NULL, 1, "<Branch>" },
  { "/View/Data/Laser", NULL,   gui_menu_layer_cb, STG_LAYER_LASERDATA, "<CheckItem>" },
  { "/View/Data/Ranger", NULL,   gui_menu_layer_cb, STG_LAYER_RANGERDATA, "<CheckItem>" },
  { "/View/Data/Blob", NULL,   gui_menu_layer_cb, STG_LAYER_BLOBDATA,  "<CheckItem>" },
  { "/View/Data/Fiducial", NULL,   gui_menu_layer_cb, STG_LAYER_NEIGHBORDATA, "<CheckItem>" },
  { "/View/Config", NULL,   NULL, 1, "<Branch>" },
  { "/View/Config/Laser", NULL,   gui_menu_layer_cb, STG_LAYER_LASERCONFIG, "<CheckItem>" },
  { "/View/Config/Ranger", NULL,   gui_menu_layer_cb, STG_LAYER_RANGERCONFIG , "<CheckItem>" },  
  { "/View/Config/Blob", NULL,   gui_menu_layer_cb, STG_LAYER_BLOBCONFIG,  "<CheckItem>" },
  { "/View/Config/Fiducial", NULL,   gui_menu_layer_cb, STG_LAYER_NEIGHBORCONFIG,  "<CheckItem>" },

  { "/View/sep1",     NULL,      NULL, 0, "<Separator>" },
  { "/View/Matrix", "<CTRL>M",   gui_menu_matrix_cb, 1, "<CheckItem>" },
  { "/View/Fill polygons", "<CTRL>P", gui_menu_polygons_cb, 1, "<CheckItem>" },
  { "/View/Grid", "<CTRL>G",   gui_menu_layer_cb, STG_LAYER_GRID, "<CheckItem>" },
  { "/View/Debug", "<CTRL>D", gui_menu_debug_cb, 1, "<CheckItem>" },
  { "/_Clock",         NULL,      NULL, 0, "<Branch>" },
  { "/Clock/tear1",    NULL,      NULL, 0, "<Tearoff>" },
  { "/Clock/Pause", NULL, gui_menu_clock_pause_cb, 1, "<CheckItem>" }
};

  static const int menu_table_count = 29;

void gui_menu_file_save_cb( gpointer data, 
			    guint action, 
			    GtkWidget* mitem )
{
  stg_world_t* world = ((gui_window_t*)data)->world;
  printf( "Saving world \"%s\"\n",  world->token );
  stg_world_save( world );
}

void gui_menu_clock_pause_cb( gpointer data, 
			      guint action, 
			      GtkWidget* mitem )
{
  ((gui_window_t*)data)->world->paused = GTK_CHECK_MENU_ITEM(mitem)->active;
}


void gui_menu_file_exit_cb( void )
{
  PRINT_DEBUG( "Exit menu item" );
  _stg_quit = TRUE;
}

/// save a frame as a jpeg, with incremental index numbers. if series
/// is greater than 0, its value is incorporated into the filename
void export_window( gui_window_t* win  ) //rtk_canvas_t* canvas, int series )
{
  char filename[128];

  win->frame_index++;

  if( win->frame_series > 0 )
    snprintf(filename, sizeof(filename), "stage-%03d-%04d.jpg",
	     win->frame_series, win->frame_index);
  else
    snprintf(filename, sizeof(filename), "stage-%03d.jpg", win->frame_index);

  printf("Stage: saving [%s]\n", filename);
  
  rtk_canvas_export_image( win->canvas, filename, RTK_IMAGE_FORMAT_JPEG);
}

void gui_menu_file_export_frame_cb( gpointer data, 
				    guint action, 
				    GtkWidget* mitem )
{
  PRINT_DEBUG( "File/Image/Save frame menu item" );
  export_window( (gui_window_t*)data );
}

gboolean frame_callback( gpointer data )
{
  export_window( (gui_window_t*)data );
  return TRUE;
}

void gui_menu_file_export_sequence_cb( gpointer data, 
				       guint action, 
				       GtkWidget* mitem )
{
  PRINT_DEBUG( "File/Export/Sequence menu item" );
  
  gui_window_t* win = (gui_window_t*)data;

  if(GTK_CHECK_MENU_ITEM(mitem)->active)
    {
      PRINT_DEBUG( "sequence start" );
      // advance the sequence counter - the first sequence is 1.
      win->frame_series++;
      win->frame_index = 0;
      
      win->frame_callback_tag =
	g_timeout_add( win->frame_interval,
		       frame_callback,
		       (gpointer)win );
    }
  else
    {
      PRINT_DEBUG( "sequence stop" );
      // stop the frame callback
      g_source_remove(win->frame_callback_tag); 
    }
}

void gui_menu_layer_cb( gpointer data, 
			guint action, 
			GtkWidget* mitem )    
{
  // show or hide the layer depending on the state of the menu item
  rtk_canvas_layer_show( ((gui_window_t*)data)->canvas, 
			 action, // action is the layer number
			 GTK_CHECK_MENU_ITEM(mitem)->active );
}

void model_refresh( stg_model_t* mod )
{
  // re-set the current data, config & lines to force redraws
  size_t len = 0;
  void* p = stg_model_get_data( mod, &len );  
  stg_model_set_data( mod, p, len );
  p = stg_model_get_config( mod, &len );
  stg_model_set_config( mod, p, len );  
  stg_model_set_lines( mod, mod->lines, mod->lines_count );
}

void refresh_cb( gpointer key, gpointer value, gpointer user )
{
  model_refresh( (stg_model_t*)value );
}

void gui_menu_polygons_cb( gpointer data, guint action, GtkWidget* mitem )
{
  gui_window_t* win = (gui_window_t*)data;
  win->fill_polygons = GTK_CHECK_MENU_ITEM(mitem)->active;
  // redraw everything to see the polygons change
  g_hash_table_foreach( win->world->models, refresh_cb, NULL ); 
}
  
void gui_menu_matrix_cb( gpointer data, guint action, GtkWidget* mitem )
{
  gui_window_t* win = (gui_window_t*)data;
  win->show_matrix = GTK_CHECK_MENU_ITEM(mitem)->active;
  if( win->matrix ) rtk_fig_clear( win->matrix );
}

void gui_menu_debug_cb( gpointer data, guint action, GtkWidget* mitem )
{
  gui_window_t* win = (gui_window_t*)data;

  // fig_debug is global

  if(GTK_CHECK_MENU_ITEM(mitem)->active)
    {
      fig_debug = rtk_fig_create( win->canvas, NULL, STG_LAYER_DEBUG );
      rtk_fig_color_rgb32( fig_debug, stg_lookup_color(STG_DEBUG_COLOR) );
    }
  else if( fig_debug )
    { 
      rtk_fig_destroy( fig_debug );
      fig_debug = NULL;
    }

}


void gui_window_menus_create( gui_window_t* win )
{
  GtkAccelGroup* ag = gtk_accel_group_new();
  
  GtkItemFactory* fac = gtk_item_factory_new( GTK_TYPE_MENU_BAR,
						  "<main>", ag );
  
  gtk_item_factory_create_items( fac, 
				 menu_table_count, 
				 menu_table, 
				 (gpointer)win );

  /* Attach the new accelerator group to the window. */
  gtk_window_add_accel_group (GTK_WINDOW(win->canvas->frame), ag );


  win->canvas->menu_bar = gtk_item_factory_get_widget( fac, "<main>" );
  
  GtkCheckMenuItem* mitem;

  // View
  mitem = GTK_CHECK_MENU_ITEM(gtk_item_factory_get_item(fac, "/View/Grid"));
  gtk_check_menu_item_set_active( mitem, TRUE );

  //mitem = GTK_CHECK_MENU_ITEM(gtk_item_factory_get_item(fac, "/View/Debug"));
  //gtk_check_menu_item_set_active( mitem, FALSE );
  //mitem = GTK_CHECK_MENU_ITEM(gtk_item_factory_get_item(fac, "/View/Matrix"));
  //gtk_check_menu_item_set_active( mitem, FALSE );

  mitem = GTK_CHECK_MENU_ITEM(gtk_item_factory_get_item(fac, "/View/Fill polygons"));
  gtk_check_menu_item_set_active( mitem, TRUE );

  // View/Data
  mitem = GTK_CHECK_MENU_ITEM(gtk_item_factory_get_item(fac, "/View/Data/Laser"));
  gtk_check_menu_item_set_active( mitem, TRUE );
  mitem = GTK_CHECK_MENU_ITEM(gtk_item_factory_get_item(fac, "/View/Data/Ranger"));
  gtk_check_menu_item_set_active( mitem, TRUE );
  mitem = GTK_CHECK_MENU_ITEM(gtk_item_factory_get_item(fac, "/View/Data/Fiducial"));
  gtk_check_menu_item_set_active( mitem, TRUE );
  mitem = GTK_CHECK_MENU_ITEM(gtk_item_factory_get_item(fac, "/View/Data/Blob"));
  gtk_check_menu_item_set_active( mitem, TRUE );

  // View/Config
  //mitem = GTK_CHECK_MENU_ITEM(gtk_item_factory_get_item(fac, "/View/Config/Laser"));
  //gtk_check_menu_item_set_active( mitem, FALSE );
  //mitem = GTK_CHECK_MENU_ITEM(gtk_item_factory_get_item(fac, "/View/Config/Ranger"));
  //gtk_check_menu_item_set_active( mitem, FALSE );
  //mitem = GTK_CHECK_MENU_ITEM(gtk_item_factory_get_item(fac, "/View/Config/Fiducial"));
  //gtk_check_menu_item_set_active( mitem, FALSE );
  //mitem = GTK_CHECK_MENU_ITEM(gtk_item_factory_get_item(fac, "/View/Config/Blob"));
  //gtk_check_menu_item_set_active( mitem, FALSE );
  
  //gtk_container_add( win->canvas->layout, menu_bar );
  gtk_box_pack_start(GTK_BOX(win->canvas->layout), 
		     win->canvas->menu_bar, FALSE, FALSE, 0);
}


void gui_window_menu_destroy( gui_window_t* win )
{
  // TODO - destroy everything
}
