#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <gtk/gtk.h>
#include "misc.h"
#include "bitmaps.h"
#include "digits.h"

extern void StartTimer ();
extern int StopTimer ();

extern void CreateMenu (GtkWidget *window, GtkWidget *vbox_main);
extern void ShowMessage(char *szTitle, char *szMessage);

void SetStartButtonIcon (gchar **xpm_data);

// set buttons sizes
#define BUTTON_WIDTH 30
#define BUTTON_HEIGHT 30


 
//     Each of the digits showing the number of bombs.
//     Has a colorful bitmap showing the count. 
//     This array makes the looking up of the digits faster.
 
gpointer digits[] = { 
    NULL, 
    xpm_one, 
    xpm_two,
    xpm_three,
    xpm_four,
    xpm_five,
    xpm_six,
    xpm_seven,
    xpm_eight
};




 //  These are the available button states.  
 // 
 //  Unknown - Empty.  Don't know what it could be.
 //  Flagged - User has put a flag on it, suspecting there's a bomb in it.
 //  Question - not implemented.
 //  Down - Button has been pressed.
 
enum {
    BUTTON_UNKNOWN,
    BUTTON_FLAGGED,
    BUTTON_QUESTION,
    BUTTON_DOWN
};

// data structure to keep track of the minesweeper buttons.
typedef struct {
    int       buttonState;  // Column to place the button
    GtkWidget *widget;      // Handle to the button
    int       nBombsNearby; // How many are near this cell?
    int       bHasBomb;     // Does it have a bomb?
    int       nRow;         // Row of button
    int       nCol;         // Column of button
} typeMinesweeperButton;

// Default values for the size of the grid and number of bombs.
static int nRows = 9;
static int nCols = 9;
static int nTotalBombs = 10;


// Use a 2-d array for the grid. Set the max grid sizes.
#define MAX_ROWS 27
#define MAX_COLS 45

#define RIGHT_CLICK 3
#define LEFT_CLICK 1

typeMinesweeperButton mines[MAX_COLS][MAX_ROWS];

// Flags for the game
int bGameOver = FALSE;            
int bResetGame = FALSE;           
int nBombsLeft;

GtkWidget *table = NULL;          /* --- Table with the grid in it. --- */
GtkWidget *button_start;          /* --- Start button --- */
GtkWidget *label_bombs;           /* --- Label showing number of bombs left --- */
GtkWidget *label_time;            /* --- Label showing time passed --- */
GtkWidget *vbox;                  /* --- main window's vbox --- */
GtkWidget *nameLabel;             /* --- user label for highscore --- */
GtkWidget *nameEntry;             /* --- user name input for highscore --- */

void DisplayHiddenInfo (typeMinesweeperButton *mine);
void CreateMinesweeperButtons (GtkWidget *table, int c, int r, int flag);
void FreeChildCallback (GtkWidget *widget);


/*
  Check to see if the game is won.  Game has been won when the 
  number of un-left-clicked squares equals the number of total bombs. 
  Or if all of the flags are put on a mine.
 */
void CheckForWin ()
{
    int i, j;
    int nMines = 0;
    int nflaggedMines = 0;

    // Run through all the cells
    for (i = 0; i <  nCols; i++) {
        for (j = 0; j < nRows; j++) {

            // if cell is unclicked 
            if (mines[i][j].buttonState == BUTTON_UNKNOWN) {
                nMines ++;
            }
            // or has a flag on it
            if (mines[i][j].buttonState == BUTTON_FLAGGED && mines[i][j].bHasBomb) {
                nflaggedMines++;
            }
        }
    }

    // if number of un-left-clicked squares equals the number of total bombs. 
    if (nMines+nflaggedMines == nTotalBombs || nflaggedMines == nTotalBombs) {

        // Game is won!
        int time_passed = StopTimer ();        
        char win_message[100];
        sprintf(win_message, "You won!\nTime spent: %d seconds", time_passed);

        SetStartButtonIcon ((gchar **) xpm_winner);

        ShowMessage("Game Over", win_message);

        bGameOver = TRUE;
    }
}


/*
  Change the image on the button to be that of the xpm.
 
  mine - square on the grid
  xpm - image data
 */
void AddImageToMine (typeMinesweeperButton *mine, char *xpm[])
{
    GtkWidget *widget;

    // Create a widget from the xpm file
    widget = CreateWidgetFromXpm (table, (gchar **) xpm);

    // Put the bitmap in the button
    gtk_container_add (GTK_CONTAINER (mine->widget), widget);
}


/*
  Refresh the seconds display in the toolbar.
 
  nSeconds - how many seconds to display.
 */
void UpdateSeconds(int nSeconds)
{
    char buffer[44];

    // Change the label to show new time
    sprintf (buffer, "Time: %d", nSeconds);
    gtk_label_set (GTK_LABEL (label_time), buffer);
}


/*
  Show number of bombs remaining.
 */
void DisplayBombCount ()
{
    char buffer[33];
    sprintf (buffer, "Bombs: %d", nBombsLeft);
    gtk_label_set (GTK_LABEL (label_bombs), buffer);
}


/*
  Free all the children of the widget
  This is called when the button has to display a new image. 
  The old image is destroyed here.
 */
void FreeChild (GtkWidget *widget) 
{
    gtk_container_foreach (
               GTK_CONTAINER (widget), 
               (GtkCallback) FreeChildCallback,
               NULL);
}

//  The window is closing down, end the gtk loop
void delete_event (GtkWidget *widget, gpointer *data)
{
    gtk_main_quit();
}


/*
  They clicked on a bomb, so now we have to show them were the
  bombs really are.  (At least those not found already.)
  We display the bombs they didn't find as well as the bombs
  they think they found that were not bombs.
 */ 
void ShowBombs (typeMinesweeperButton *minefound)
{
    int i, j;
    typeMinesweeperButton *mine; 

    // Run through all the cells
    for (i = 0; i <  nCols; i++) {
        for (j = 0; j < nRows; j++) {

            mine = &mines[i][j];

            // If there's a button here and there's a bomb inside
            if (mine->buttonState == BUTTON_UNKNOWN &&
                mine->bHasBomb) {

                // Display the bomb.
                DisplayHiddenInfo (mine);

            // If they marked it as a bomb and there is no bomb here
            } else if (mine->buttonState == BUTTON_FLAGGED &&
                       !mine->bHasBomb) {

                // Free the flag
                FreeChild (mine->widget);

                // Show the X at the location
                AddImageToMine (mine, xpm_bigx);
            }
        }
    }
}


/*
   Show all squares around this square.
   col, row - position to open up the square 
   Open all squares near this one - X represents
   the current square.
*/
void OpenNearbySquares (int col, int row)
{
    int i, j;

    // Look one column back and one column ahead
    for (i = MAX (col-1, 0); i <= MIN (col+1, nCols-1); i++) {

        //Check one row back and one row ahead
        for (j = MAX (row-1, 0); j <= MIN (row+1, nRows-1); j++) {

            // Display what's inside
            DisplayHiddenInfo (&mines[i][j]);
        }
    }
}


//  Display what's hidden inside the button.
void DisplayHiddenInfo (typeMinesweeperButton *mine)
{
    // If it's already down, just return
    if (mine->buttonState == BUTTON_DOWN) {
        gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (mine->widget), TRUE);
        return;
    }

    // If the button is flagged, don't fix it for them
    if (mine->buttonState == BUTTON_FLAGGED) {
        gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (mine->widget), FALSE);

    } else {

        // Put the button in the "down" state
        mine->buttonState = BUTTON_DOWN;
        gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (mine->widget), TRUE);

        // If there's a bomb at the location
        if (mine->bHasBomb) {

            // Show the bomb at the location
            AddImageToMine (mine, xpm_bomb);

        // No bombs, but there are bombs near this one
        } else if (mine->nBombsNearby) {

            // Show the count of nearby bombs.
            AddImageToMine (mine, digits[mine->nBombsNearby]);

        } else {

            // Clicked here, but no bombs and no count. Open up all squares near here.
            OpenNearbySquares (mine->nCol, mine->nRow);
        }
    }
}


/*
  Reset the game so it can be replayed.  
  Reset bomb count and create a nice empty field of bombs.
 */
void ResetGame (int nGridColumns, int nGridRows, int nBombs, int bNewButtons)
{

    // Reset the number of bombs in grid 
    nTotalBombs = nBombs;

    // Reset the number of bombs undiscovered
    nBombsLeft = nBombs;

    // Create the Minesweeper buttons
    CreateMinesweeperButtons (table, nGridColumns, nGridRows, bNewButtons);

    // Stop the timer
    StopTimer ();
    UpdateSeconds (0);

    SetStartButtonIcon ((gchar **) xpm_smile);
}


// Free the widget.
void FreeChildCallback (GtkWidget *widget)
{
    gtk_widget_destroy (widget);
}


// Set the start button to have the image based on the data passed in.  
// Usually, this is going to be either the happy face or the frown.
void SetStartButtonIcon (gchar **xpm_data)
{
    GtkWidget *widget;

    // Create a widget from the xpm
    widget = CreateWidgetFromXpm (button_start, xpm_data);

    // Free any children the button has
    FreeChild (button_start);

    // Make this the current image
    gtk_container_add (GTK_CONTAINER (button_start), widget);
}


// Event handler for the clicking of the start button
void start_button_clicked (GtkWidget *widget, gpointer *data)
{
    SetStartButtonIcon ((gchar **) xpm_smile);
    ResetGame (nCols, nRows, nTotalBombs, FALSE);
}


/* 
Event handler for one of the mine buttons being clicked.
   widget - button pressed.
   data - button label.
*/
void button_left_clicked (GtkWidget *widget, gpointer *data)
{
    typeMinesweeperButton *cell;

    cell = (typeMinesweeperButton *) data;

    if (bGameOver) {

        // Do nothing
        gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (widget),
                            (cell->buttonState == BUTTON_DOWN));
        return;
    }

    // If game is being reset
    if (bResetGame) return;

    StartTimer ();

    // If they clicked on a mine button
    if (cell->bHasBomb) {

        // Game is lost!
        bGameOver = TRUE;

        SetStartButtonIcon ((gchar **) xpm_frown);

        int time_passed = StopTimer ();        
        char lose_message[110];
        sprintf(lose_message, "You lost!\nBombs remaining: %d\nTime spent: %d seconds", nBombsLeft, time_passed);
        ShowMessage("Game Over", lose_message);
        printf("%s\n", lose_message);
        // Display all unseen bombs.
        ShowBombs (cell);


    } else {

        // Create a label for the cell and put the number of nearby bombs in it.
        DisplayHiddenInfo (cell);
        CheckForWin ();
    }
}


/*
   The button press could be a right mouse button 
   which needs to be handled by changing the image in the button.
   If it was left click then it must be handled by checking the inside of the cell.
 */
void button_press(GtkWidget *widget, GdkEventButton *event, gpointer *data)
{
    typeMinesweeperButton *mine;

    // Ignore if game is already over
    if (bGameOver) {
        return;
    }

    // Get the clicked mine
    mine = (typeMinesweeperButton *) data;

    // Make sure it's a button event
    if (event->type == GDK_BUTTON_PRESS) {
 
        // Was it the right mouse click?
        if (event->button == RIGHT_CLICK) {

            switch (mine->buttonState) {

                case BUTTON_UNKNOWN:

                    FreeChild (widget);
                    mine->buttonState = BUTTON_FLAGGED;
                    AddImageToMine (mine, xpm_flag);
                    nBombsLeft --;
                    break;

                case BUTTON_FLAGGED:

                    FreeChild (widget);
                    mine->buttonState = BUTTON_UNKNOWN;
                    nBombsLeft ++;
                    break;
            }
            DisplayBombCount ();
            CheckForWin ();
        }
        else if (event->button == LEFT_CLICK) {
            button_left_clicked(widget, data);
        }
    }
}


// Create a button, assign event handlers, and attach the button to the table in the proper place.
GtkWidget *CreateButton (GtkWidget *table, 
                         typeMinesweeperButton *mine, 
                         int row, 
                         int column)
{
    GtkWidget *button;

    // Create the button
    button = gtk_toggle_button_new ();

    // Init the button fields
    mine->buttonState = BUTTON_UNKNOWN;
    mine->nRow = row;
    mine->nCol = column;

    // We care about other mouse events.
    gtk_signal_connect (GTK_OBJECT (button), "button_press_event",
                        GTK_SIGNAL_FUNC (button_press), mine);

    // Put the button in the table in the right place.
    gtk_table_attach (GTK_TABLE (table), button, 
                      column, column+1, 
                      row + 1, row + 2, 
                      GTK_FILL | GTK_EXPAND, 
                      GTK_FILL | GTK_EXPAND, 
                      0, 0);

    // Set the button to a uniform size
    gtk_widget_set_usize (button, BUTTON_WIDTH, BUTTON_HEIGHT);

    // Make the button visible
    gtk_widget_show (button);

    return (button);
}



// Count the number of bombs near this cell.
int CountNearbyBombs (int col, int row)
{
    int i, j;
    int nCount = 0;

    // Every cell that would be at most 1 cell away
    for (i = MAX (col-1, 0); i <= MIN (col+1, nCols-1); i++) {
        for (j = MAX (row-1, 0); j <= MIN (row+1, nRows-1); j++) {

            if (mines[i][j].bHasBomb) {

               nCount++;
            }
        }
    }
    return (nCount);
}


/*
   Create the buttons on the minesweeper from the table we defined at the beginning of this program.  
   The button pointers (handles) are stored back in the table so they can be referenced later.
 */
void CreateMinesweeperButtons (GtkWidget *table, 
                               int nGridColumns, 
                               int nGridRows,
                               int bNewButtons)
{
    int ci;
    int ri;
    int nBombs;
    typeMinesweeperButton *mine; 

    // Update the global variables
    nCols = nGridColumns;
    nRows = nGridRows;

    bGameOver = FALSE;
    bResetGame = TRUE;

    // Update bomb count.
    DisplayBombCount ();

    // Check each button 
    for (ci = 0; ci < nCols; ci++) {
        for (ri = 0; ri < nRows; ri++) {

            // The button has nothing at all.
            mine = &mines[ci][ri];
            mine->bHasBomb = 0;
            mine->buttonState = BUTTON_UNKNOWN;

            // Widget assoc with the mine?
            if (bNewButtons) {
                mine->widget = CreateButton (table, mine, ri, ci);
            } else {

                // Reuse button

                // Free any existing xpm/label
                FreeChild (mine->widget);

                // Put button up.
                gtk_toggle_button_set_state (
                           GTK_TOGGLE_BUTTON (mine->widget), 
                           FALSE);
            }
        }
    }

    // Place the bombs. 
    nBombs = nTotalBombs;

    // While we have bombs to place
    while (nBombs > 0) {

        // get a random position
        ci = rand () % nCols;
        ri = rand () % nRows;

        // If no bomb exists in the position, put one
        if (mines[ci][ri].bHasBomb == 0) {
            mines[ci][ri].bHasBomb = 1;
            nBombs--;
        }
    }

    //Once all bombs have been distributed, calculate how many bombs are adjacent to each button.

    // Check every button
    for (ci = 0; ci < nCols; ci++) {
        for (ri = 0; ri < nRows; ri++) {

            mine = &mines[ci][ri];
           
            // How many bombs are nearby?
            mine->nBombsNearby = CountNearbyBombs (ci, ri);
        }
    }
    bResetGame = FALSE;
}


//  Sets the game grid size and the number of bombs.
void SetGrid (int nGridColumns, int nGridRows, int nBombs)
{
    // If the packing table exists.
    if (table) {
        // Destroy it and all the buttons.
        gtk_widget_destroy (table);
    } 

    // Create a table for the buttons
    table = gtk_table_new (nGridColumns, nGridRows, FALSE); 

    // Add it to the vbox
    gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

    // Show the table.
    gtk_widget_realize (table);

    // Do a game reset with the numbers
    ResetGame (nGridColumns, nGridRows, nBombs, TRUE);

    // Show the table.
    gtk_widget_show (table);
}

static gboolean key_callback(GtkWidget *widget,
                             GdkEventKey *event,
                             GtkIMContext *im_context) {
  return gtk_im_context_filter_keypress(im_context, event);
}

// The app begins:
int main (int argc, char *argv[])
{
    GtkWidget *window;
    GtkWidget *hbox;          

    // GTK initialization
    gtk_init (&argc, &argv);

    // Create the top window
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

    // Insert an icon for the program
    gtk_window_set_icon_from_file(GTK_WINDOW(window),"icon.ico",NULL); 

    // Don't allow window to be resized.
    gtk_window_set_policy (GTK_WINDOW (window), FALSE, TRUE, FALSE);

    // Give the window a title.
    gtk_window_set_title (GTK_WINDOW (window), "Minesweeper");

    //You should always remember to connect the destroy event to the main window.
    gtk_signal_connect (GTK_OBJECT (window), "delete_event",
                        GTK_SIGNAL_FUNC (delete_event), NULL);


    vbox = gtk_vbox_new (FALSE, 1);
    gtk_widget_show (vbox);

    // Create the application menu
    CreateMenu (window, vbox);

    // Horizontal box for score/start button
    hbox = gtk_hbox_new (TRUE, 1);
    gtk_widget_show (hbox);

    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    // Add label with the number of bombs left
    label_bombs = gtk_label_new ("");
    gtk_box_pack_start (GTK_BOX (hbox), label_bombs, FALSE, FALSE, 0);
    gtk_widget_show (label_bombs);

    // Create the start button with smilely face on it 
    button_start = gtk_button_new ();

    gtk_signal_connect (GTK_OBJECT (button_start), 
                        "clicked",
                        GTK_SIGNAL_FUNC (start_button_clicked), 
                        NULL);

    gtk_box_pack_start (GTK_BOX (hbox), button_start, FALSE, FALSE, 0);
    gtk_widget_show (button_start);

    // Add label with the time passed
    label_time = gtk_label_new ("");
    gtk_box_pack_start (GTK_BOX (hbox), label_time, FALSE, FALSE, 0);
    gtk_widget_show (label_time);

    // Make the labels visible
    gtk_widget_show (vbox);

    // Add application vbox to main window
    gtk_container_add (GTK_CONTAINER (window), vbox);
    gtk_widget_show (window);

    // Put on the start button
    SetStartButtonIcon ((gchar **) xpm_smile);

    // Create initial grid
    SetGrid (nRows, nCols, nTotalBombs);

    // Start gtk event processing
    gtk_main ();
    exit (0);
}
