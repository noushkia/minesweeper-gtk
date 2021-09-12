#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <gtk/gtk.h>
#include "misc.h"

extern void ShowAbout ();
extern void start_button_clicked (GtkWidget *widget, gpointer *data);
extern void ShowMessage(char *szTitle, char *szMessage);

void SetGrid (int nColumns, int nRows, int nBombs);


GtkWidget           *win_main;
GtkWidget           *toolbar;

// Shows user highscore
void menu_highscore(GtkWidget *widget, gpointer data)
{
    ShowMessage("Highscore", "Your best time is: 1s");
}

// Restarts the game for the user.
void menu_restart(GtkWidget *widget, gpointer data)
{
    start_button_clicked (NULL, NULL);
}

// User Picked the "beginner" difficulty from the menu.
// Create a small grid.
void funcBeginner (GtkWidget *widget, gpointer data)
{
    if (GTK_CHECK_MENU_ITEM (widget)->active) {
        // Set the grid size
        SetGrid (9, 9, 10);
    }
}

// User Picked the "intermediate" difficulty from the menu.
// Creates a medium sized grid.
void funcIntermediate (GtkWidget *widget, gpointer data)
{
    if (GTK_CHECK_MENU_ITEM (widget)->active) {
        SetGrid (18, 18, 40);
    }
}

// User picked the "advanced" difficulty in the menu.
// Make the largest grid with the most bombs.
void funcAdvanced (GtkWidget *widget, gpointer data)
{
    if (GTK_CHECK_MENU_ITEM (widget)->active) {
        SetGrid (27, 45, 100);
    }
}

// Exit the game.
void menu_Quit (GtkWidget *widget, gpointer data)
{
    gtk_main_quit ();
}

// Show information about the game.
void menu_About(GtkWidget *widget, gpointer data)
{
    ShowMessage("About...",
                  "Minesweeper game by Hossein (meyhossi@gmail.com)");
}

// Create the main window and the menu toolbar
void CreateMenu (GtkWidget *window, GtkWidget *vbox_main)
{
    GtkWidget *menubar;
    GtkWidget *menu;
    GtkWidget *menuitem;
    GSList *group = NULL;

    win_main = window;

    // Game menu entry
    menubar = gtk_menu_bar_new ();
    gtk_box_pack_start (GTK_BOX (vbox_main), menubar, FALSE, TRUE, 0);
    gtk_widget_show (menubar);


    menu = CreateBarSubMenu (menubar, "Game");

    menuitem = CreateMenuItem (menu, "High Score", NULL, 
                     GTK_SIGNAL_FUNC (menu_highscore), "highscore");


    menuitem = CreateMenuItem (menu, "Restart", "Restart Game", 
                     GTK_SIGNAL_FUNC (menu_restart), NULL);

    menuitem = CreateMenuItem (menu, NULL, NULL, NULL, NULL);

    menuitem = CreateMenuRadio (menu, "Beginner", &group,
                     GTK_SIGNAL_FUNC (funcBeginner), NULL);

    menuitem = CreateMenuRadio (menu, "Intermediate", &group,
                     GTK_SIGNAL_FUNC (funcIntermediate), NULL);

    menuitem = CreateMenuRadio (menu, "Advanced", &group,
                     GTK_SIGNAL_FUNC (funcAdvanced), NULL);

    menuitem = CreateMenuItem (menu, NULL, NULL, NULL, NULL);

    menuitem = CreateMenuItem (menu, "Quit", "Quit Game", 
                     GTK_SIGNAL_FUNC (menu_Quit), "quit");

    // Help menu entry
    menu = CreateBarSubMenu (menubar, "Help");

    menuitem = CreateMenuItem (menu, "About Minesweeper", 
                     "About the minesweeper game", 
                     GTK_SIGNAL_FUNC (menu_About), "about");
}
