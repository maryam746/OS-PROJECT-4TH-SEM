#ifndef GUI_H
#define GUI_H

#include "train.h"
#include "raylib.h"

// Constants
#define SCREEN_WIDTH 1600
#define SCREEN_HEIGHT 1000
#define TRAIN_WIDTH 35
#define TRAIN_HEIGHT 50
#define TOTAL_TRACKS 5
#define TOTAL_JUNCTIONS 8  // Increased to 8 junctions


// Track positions structure
typedef struct {
    int x;           // X coordinate of the track
    int y_start;     // Starting Y position
    int y_end;       // Ending Y position
    int track_id;    // Track number (1-5)
} TrackData;

// Junction structure with Y position
typedef struct {
    int x, y;
    int from_track;      // Track numbers (1-5)
    int to_track;        // Track numbers (1-5)
    int occupied_by;     // train id that is using this junction
    int y_position;      // Vertical position of this junction on the track
} Junction;

// Train visual data structure
typedef struct {
    int train_id;
    int current_track;      // Which track the train is on (1-5)
    float y_pos;            // Vertical position on the track
    int moving;             // Is the train moving
    int using_junction;     // Is train currently on a junction
    int target_junction;    // Which junction it's heading to
    int original_track;     // Original track before junction
    int junction_progress;  // Progress through junction (0-100)
    float junction_x;       // X position when on junction
    int waiting_at_start;   // Is train waiting at start of track
    float speed;            // Movement speed
    int dest_track;         // Destination track
    int junctions_used;     // Count of junctions used in current journey
    int max_junctions;      // Max junctions to use (1-3)
} VisualTrain;

// Function declarations
void* startGUI(void* arg);
void drawTracks();
void drawJunctions();
void drawTrain(int x, int y, int train_id, int is_moving, Color color, int waiting_at_start);
Color getDullColor(Color c);

// Global external declarations
extern TrackData track_positions[TOTAL_TRACKS];
extern Junction junctions[TOTAL_JUNCTIONS];
extern VisualTrain visual_trains[10];
extern int num_visual_trains;
extern pthread_mutex_t visual_mutex;
extern Color bright_colors[6];

#endif
