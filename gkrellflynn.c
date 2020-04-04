/*
  gkrellflynn.c

  author: Henryk Richter

  last update: Tue May 7 2002 

  purpose: just an addon toy for gkrellm

  changelog:
  0.5 - port to GKrellm 2.0 (thanks to Bill Nalen)
      - port to Win32 (thanks to Bill Nalen)
      - kept the code downwards compatible 

  0.4 - port to GKrellm 1.2.x

  0.3 - stops /proc/stat parsing after the first found cpu line,
        shows better results on SMP machines

  0.2 - minor cleanups, about box

  0.1 - initial version

*/
#ifndef WIN32

/* see makefile */
#ifdef GKRELLM2
#include <gkrellm2/gkrellm.h>
#else  /* GKRELLM2 */
#include <gkrellm/gkrellm.h>
#endif /* GKRELLM2 */

#include <sys/time.h>
#include <sys/wait.h>

#else /* WIN32 */

#include <src/gkrellm.h>
#include <src/win32-plugin.h>

#endif /* WIN32 */


#include <stdlib.h>

#define FLYNN_MAJOR_VERSION 0
#define FLYNN_MINOR_VERSION 5 

#include "flynn.picture_alpha_big.xpm"
#define STYLE_NAME "flynn"

#define HEIGHT		74
#define IMAGE_WIDTH	48
#define IMAGE_HEIGHT	64
#define IMAGE_COUNT	27

/* 3 different directions to look */
#define F_LOOKSTATES	3
/* 2 extra states (strength, grin) */
#define F_EXTRASTATE1   3
#define F_EXTRASTATE2   4
/* 5 torture variants per state */
#define F_TORTURES	5

#define GRIN_TIME	3

#if GKRELLM_VERSION_MAJOR < 2
static Panel		*panel;
static Decal		*flynn = NULL;
#else
static GkrellmPanel	*panel;
static GkrellmDecal	*flynn = NULL;
static GkrellmMonitor	*monitor;
#endif

static gint		style_id;
static GdkPixmap	*flynn_image = NULL;
static GdkBitmap 	*flynn_mask = NULL;
static int              dogrin = 0;

/* config stuff */
#define PLUGIN_CONFIG_KEYWORD "FlynnA"
static GtkWidget *nice_checkbutton;
static int nice_checkdisable = 0;
static GtkWidget *term_checkbutton;
static int term_checkdisable = 0;

static GtkWidget *commandline_entry;
static gchar command_line[256];
static GtkWidget *terminal_entry;
static gchar terminal_command_line[256];
static int child_started = 0;

void flynn_apply_config(void)
{
	nice_checkdisable = GTK_TOGGLE_BUTTON(nice_checkbutton)->active;
	term_checkdisable = GTK_TOGGLE_BUTTON(term_checkbutton)->active;
	strncpy(command_line, gtk_entry_get_text(GTK_ENTRY(commandline_entry)), 255);
	strncpy(terminal_command_line, gtk_entry_get_text(GTK_ENTRY(terminal_entry)), 255);
}

static void flynn_save_config(FILE *f)
{
	fprintf(f, "%s exclude_nice %d\n", PLUGIN_CONFIG_KEYWORD,nice_checkdisable);
	fprintf(f, "%s command_line %s\n", PLUGIN_CONFIG_KEYWORD,command_line);
	fprintf(f, "%s run_in_term %d\n", PLUGIN_CONFIG_KEYWORD,term_checkdisable);
	fprintf(f, "%s terminal_command %s\n", PLUGIN_CONFIG_KEYWORD,terminal_command_line);
}

static void flynn_load_config (gchar *arg)
{
    gchar config[64], item[256];
    gint n;

    n = sscanf(arg, "%s %[^\n]", config, item);
    if (n != 2)
        return;

        if (strcmp(config, "exclude_nice") == 0 )
		sscanf(item, "%d\n", &nice_checkdisable);
	if (strcmp(config, "command_line") == 0 )
		strncpy(command_line,item,255);
	if (strcmp(config, "run_in_term") == 0 )
		sscanf(item, "%d\n", &term_checkdisable);
	if (strcmp(config, "terminal_command") == 0 )
		strncpy(terminal_command_line,item,255);
}

/* get current cpu usage */
int getcpu( void )
{
	float scale_factor = 1;
#if 0
        FILE * fin;
        char buffer[256];
        char tokens[4] = " \t\n";
#endif

        static long last_user = 0;
        static long last_nice = 0;
        static long last_sys = 0;
        static long last_idle = 0;

        long user=0, nice=0, sys=0, idle=0, total=0;
        long d_user, d_nice, d_sys, d_idle;
        float cpu_use;
        float percent;

	gkrellm_cpu_stats(0, &user, &nice, &sys, &idle);

#if 0
        if ( (fin = fopen("/proc/stat", "r")) == NULL ) {
                return(0);
        }

        while ( fgets(buffer, 256, fin) ) {
                if ( strstr(buffer,"cpu") != NULL ) {
                        strtok(buffer,tokens);
                        user = atol(strtok(NULL,tokens));
                        nice = atol(strtok(NULL,tokens));
                        sys =  atol(strtok(NULL,tokens));
                        idle = atol(strtok(NULL,tokens));

			break;
                }
        }

        fclose (fin);
#endif

        d_user = user - last_user;
        d_nice = nice - last_nice;
        d_sys  = sys  - last_sys;
        d_idle = idle - last_idle;

        last_user = user;
        last_nice = nice;
        last_sys  = sys;
        last_idle = idle;

        total = d_user + d_sys + d_nice + d_idle;
	if( nice_checkdisable == 1 )
		d_idle += d_nice;

        if ( total < 1 )
                total = 1.0;

        cpu_use = 1.0 - ( (float)  d_idle  / (float) total );
#if 0 
        printf("   CPU (/proc/stat): %5.2f  ", cpu_use);
        printf(" (jiffies: user:%ld nice:%ld system:%ld idle:%ld total:%ld)\n",
                        d_user, d_nice, d_sys, d_idle, total  );
#endif
        percent = cpu_use / scale_factor;
        if ( percent > .999999 )
                percent = .999999;

        return ( (int)(percent*100) );
}


static void update_plugin()
{
        static int image_number = 0;
	static int flynn_look = 0;
	int dir;
	int percent;

	/* first check looking direction */
#if GKRELLM_VERSION_MAJOR < 2
	if( GK.second_tick )
#else
	if( gkrellm_ticks()->second_tick )
#endif
	{
	   /* catch exiting childs if any (no semaphore used, gkrellm is synchronous) */
#ifndef WIN32
	   if( child_started > 0 )
	   {
		if( waitpid( -1, NULL, WNOHANG ) > 0 )
			child_started--;
	   }
#endif	  
	   if( dogrin > 0 )
	   {
		dogrin--;
		flynn_look = F_EXTRASTATE2;
	   }
	   else
	   {
		dir = (int)( (float)F_LOOKSTATES * (float)rand() / (RAND_MAX+1.0));
		switch( dir )
		{
			case 0:
				break;
			case 1: 
				flynn_look++;
				break;
			case 2: 
				flynn_look--;
				break;
			default:
				break;
		}
		if( flynn_look < 0 ) flynn_look = 0;
		if( flynn_look >= F_LOOKSTATES ) flynn_look = F_LOOKSTATES-1;
	   }
		percent = getcpu();

		image_number = flynn_look * F_TORTURES + (F_TORTURES * percent / 100);

		/*printf("%d\n",image_number);*/
	}


	/* draw determined frame */
	gkrellm_draw_decal_pixmap(panel, flynn, image_number);
#if GKRELLM_VERSION_MAJOR < 2
	gkrellm_draw_layers(panel);
#else
	gkrellm_draw_panel_layers(panel);
#endif
}

static gint panel_expose_event(GtkWidget *widget, GdkEventExpose *ev)
{
	gdk_draw_pixmap(widget->window,
	                widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
	                panel->pixmap, ev->area.x, ev->area.y, ev->area.x, ev->area.y,
	                ev->area.width, ev->area.height);
    return FALSE;
}

static gint panel_click_event(GtkWidget *widget, GdkEventExpose *ev)
{
	gchar* argv[32];
	gchar localcmd[256];
	int i;
	pid_t pid;
	
	dogrin = GRIN_TIME;

	/* check for command line */
	if (strlen(command_line) == 0) return FALSE;

	child_started++;
	
	/* forkint status */
	pid = fork();
	if (pid == 0)
	{
		/* this is the child process - setup */
		i=0;
		memset(localcmd,0,256);
		/* check for "run in terminal" option */
		if (term_checkdisable)
		{
			strcpy(localcmd,terminal_command_line);
		}
		/* create command line */
		strncat(localcmd,command_line,255);
		/* separate command line into argument array */
		argv[i] = strtok(localcmd," ");
		while (argv[i++] != NULL)
		{
			argv[i] = strtok(NULL," ");
		}
		/* exec child process */
		/* execv(argv[0],argv); */
		execvp(argv[0],argv);
		_exit(EXIT_FAILURE);
	}
	
    return FALSE;
}

static void load_images()
{
#if GKRELLM_VERSION_MAJOR < 2
	static GdkImlibImage  *image = NULL;

	gkrellm_load_image(NULL, flynn_xpm, &image, NULL);
	gkrellm_render_to_pixmap(image, &flynn_image, &flynn_mask, 0, 0);
#else
	static GkrellmPiximage *image = NULL;
	gkrellm_load_piximage(NULL, flynn_xpm, &image, NULL);
	gkrellm_scale_piximage_to_pixmap(image, &flynn_image, &flynn_mask, 0, 0);
#endif
}

static void create_plugin(GtkWidget *vbox, gint first_create)
{
#if GKRELLM_VERSION_MAJOR < 2
	Style		*style = NULL;
#else
	GkrellmStyle	*style = NULL;
#endif

	int            image_x_offset;
	int            image_y_offset;

	load_images();

	if (first_create)
	{
		panel = gkrellm_panel_new0();
	}
	else
		gkrellm_destroy_decal_list(panel);

	style = gkrellm_meter_style(style_id);

	image_x_offset = (gkrellm_chart_width() - IMAGE_WIDTH) / 2;
	image_y_offset = (HEIGHT - IMAGE_HEIGHT) / 2;

	flynn = gkrellm_create_decal_pixmap(panel, flynn_image, flynn_mask,
	                                    IMAGE_COUNT,
	                                    style, image_x_offset, image_y_offset);

	panel->textstyle = gkrellm_meter_textstyle(style_id);
	panel->label->h_panel = HEIGHT;

#if GKRELLM_VERSION_MAJOR < 2
	gkrellm_create_panel(vbox, panel, gkrellm_bg_meter_image(style_id));
	gkrellm_monitor_height_adjust(panel->h);
#else
	gkrellm_panel_configure(panel, "", style);
	gkrellm_panel_create(vbox, monitor, panel);
#endif
	
	if (first_create)
	{
		gtk_signal_connect(GTK_OBJECT(panel->drawing_area),
		                   "expose_event", (GtkSignalFunc) panel_expose_event,
		                   NULL);
		gtk_signal_connect(GTK_OBJECT(panel->drawing_area),
		                   "button_press_event", (GtkSignalFunc) panel_click_event,
		                   NULL);
	}

	gkrellm_draw_decal_pixmap(panel, flynn, 1 );

#if GKRELLM_VERSION_MAJOR < 2
	gkrellm_draw_layers(panel);
#else
	gkrellm_draw_panel_layers(panel);	
#endif
	
	
	
}

static void flynn_create_plugin_tab(GtkWidget *tab_vbox)
{
	GtkWidget           *tabs;
	GtkWidget	*vbox,*vbox1,*vbox2;
	GtkWidget       *frame, *commandframe;

	tabs = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(tabs), GTK_POS_TOP);
	gtk_box_pack_start(GTK_BOX(tab_vbox), tabs, TRUE, TRUE, 0);

	/* Config TAB */
	{
		{
#if GKRELLM_VERSION_MAJOR < 2
		 vbox = gkrellm_create_tab(tabs, _("Config"));
#else
		 vbox = gkrellm_gtk_framed_notebook_page(tabs, _("Config"));
#endif

		 frame = gtk_frame_new(_("CPU time Calculations"));
        	 gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 5);
        	 vbox1 = gtk_vbox_new (FALSE, 2);
        	 gtk_container_add(GTK_CONTAINER(frame), vbox1);

		 commandframe = gtk_frame_new(_("Command Line (program executed on click)"));
		 gtk_box_pack_start(GTK_BOX(vbox), commandframe, TRUE, TRUE, 5);
		 vbox2 = gtk_vbox_new (FALSE, 2);
		 gtk_container_add(GTK_CONTAINER(commandframe), vbox2);

#if GKRELLM_VERSION_MAJOR < 2
        	 gkrellm_check_button(vbox1, &nice_checkbutton, nice_checkdisable, FALSE, 0,
                        	      "Exclude Nice time from calculations");
#else
		 {
		  gchar *label = _("Exclude Nice time from calculations");
        	  gkrellm_gtk_check_button_connected(vbox1, &nice_checkbutton, nice_checkdisable, FALSE, 0,0,
			 flynn_apply_config, 
			 NULL, 
			 label);
		 }
#endif
		}

		/* Set up command line text box */
		commandline_entry = gtk_entry_new();
        	gtk_entry_set_text(GTK_ENTRY(commandline_entry),command_line);
        	gtk_box_pack_start (GTK_BOX(vbox2), commandline_entry, FALSE, TRUE, 0);
        	g_signal_connect(G_OBJECT(commandline_entry),"changed",
                        	 G_CALLBACK(flynn_apply_config),NULL);

        	/* Set up run in terminal checkbox */
#if GKRELLM_VERSION_MAJOR < 2
        	 gkrellm_check_button(vbox2, &term_checkbutton, term_checkdisable, FALSE, 0,
                        	      "Run in Terminal");
#else
		 {
		  gchar *label = _("Run in Terminal");
        	  gkrellm_gtk_check_button_connected(vbox2, &term_checkbutton, term_checkdisable, FALSE, 0,0,
			flynn_apply_config, 
			NULL, 
			label);
		 }
#endif
		terminal_entry = gtk_entry_new();
        	gtk_entry_set_text(GTK_ENTRY(terminal_entry),terminal_command_line);
        	gtk_box_pack_start (GTK_BOX(vbox2), terminal_entry, FALSE, TRUE, 0);
	       	g_signal_connect(G_OBJECT(terminal_entry),"changed",
                        	 G_CALLBACK(flynn_apply_config),NULL);



	}
	
	/* ABOUT TAB */
        {
            gchar *plugin_about_text;
            GtkWidget *label, *text;

            plugin_about_text = g_strdup_printf(
                "GKrellFlynn %d.%d\n"
                "GKrellM Load Meter Plugin\n\n"
                "(C) 2001 Henryk Richter\n"
                "<buggs@comlabien.net>\n"
		"http://horus.comlabien.net/flynn\n\n"
                "Released under the GNU General Public License",
                FLYNN_MAJOR_VERSION, FLYNN_MINOR_VERSION);

            text = gtk_label_new(plugin_about_text);
            label = gtk_label_new("About");
            gtk_notebook_append_page(GTK_NOTEBOOK(tabs),text,label);
            g_free(plugin_about_text);
        }

}

#if GKRELLM_VERSION_MAJOR < 2
 static Monitor  plugin_mon  =
#else
 static GkrellmMonitor  plugin_mon  =
#endif
         {
         "Flynn",                 /* Name, for config tab.        */
         0,                       /* Id,  0 if a plugin           */
         create_plugin,           /* The create_plugin() function */
         update_plugin,           /* The update_plugin() function */
         flynn_create_plugin_tab, /* The create_plugin_tab() config function */
         flynn_apply_config,      /* The apply_plugin_config() function      */

         flynn_save_config,       /* The save_plugin_config() function  */
         flynn_load_config,       /* The load_plugin_config() function  */
         PLUGIN_CONFIG_KEYWORD,                /* config keyword                     */

         NULL,           /* Undefined 2  */
         NULL,           /* Undefined 1  */
         NULL,           /* Undefined 0  */

         MON_INSERT_AFTER|MON_CLOCK, /* Insert plugin before this monitor.       */
         NULL,           /* Handle if a plugin, filled in by GKrellM */
         NULL            /* path if a plugin, filled in by GKrellM   */
         };

#if GKRELLM_VERSION_MAJOR < 2
Monitor * init_plugin(void)
#else
#ifdef WIN32
__declspec(dllexport) GkrellmMonitor *
gkrellm_init_plugin(win32_plugin_callbacks* calls)
#else
GkrellmMonitor *
gkrellm_init_plugin()
#endif
#endif
{
#if GKRELLM_VERSION_MAJOR < 2
	style_id = gkrellm_add_meter_style(&plugin_mon, STYLE_NAME);
#else
#if defined(WIN32)
	callbacks = calls;
	pwin32GK = calls->GK;
#endif
	style_id = gkrellm_add_meter_style(&plugin_mon, STYLE_NAME);
	monitor = &plugin_mon;
#endif 

	/* set defaults */
	strcpy( terminal_command_line, "/usr/bin/gnome-terminal -x " );

	return &plugin_mon;
}

