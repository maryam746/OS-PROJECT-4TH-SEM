#ifndef GUI_H
#define GUI_H

#include "train.h"
#include "raylib.h"

#define SCREEN_WIDTH 1600   
#define SCREEN_HEIGHT 850
#define TRAIN_WIDTH 35
#define TRAIN_HEIGHT 50
#define TOTAL_TRACKS 5
#define TOTAL_JUNCTIONS 8 
#define SAFE_GAP (TRAIN_HEIGHT + 15)


typedef struct {
    int x;         
    int y_start;     
    int y_end;    
    int track_id;    //track no 1-5
} TrackData;

typedef struct {
    int x, y;           //center point
    int from_track;     
    int to_track;     
    int occupied_by;     // train id that is using this junction
} Junction;


typedef struct {
    int train_id;
    int current_track;     //train konse track pr hai
    float y_pos;            // Vertical position on the track
    int moving;             // 1=moving wrna 0
    int using_junction;     // 1=usign wrna 0
    int target_junction;    // junction id jo use kr rha hai
    int junction_progress;  // Progress through junction (0-100)
    int waiting_at_start;   // 1=waiting at top wrna 0
    float speed;            // Movement speed
    int dest_track;         // end kahaan hoga
} VisualTrain;


void* startGUI(void* arg);
void drawTracks();
void drawJunctions();
void drawTrain(int x, int y, int train_id, int is_moving, Color color, int waiting_at_start);
Color getDullColor(Color c);        //for waiting

extern TrackData track_positions[TOTAL_TRACKS];
extern Junction junctions[TOTAL_JUNCTIONS];
extern VisualTrain visual_trains[10];
extern int num_visual_trains;           //current active trains
extern pthread_mutex_t visual_mutex;        //for protecting visual_trains and num_visual_trains
extern Color bright_colors[6];

#endif
