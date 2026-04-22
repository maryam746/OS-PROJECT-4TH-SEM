#include "gui.h"
#include "raylib.h"
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include "signal.h"

// Declare external variables (defined in train.c)
extern Train* global_trains;
extern int global_train_count;
extern int track_status[];
extern FILE* log_file;
extern volatile int simulation_running;

// Define global GUI variables
TrackData track_positions[TOTAL_TRACKS] = {
    {300, 120, 720, 1},   // Track 1 (y_start: 120, y_end: 720, height: 600px)
    {520, 120, 720, 2},   // Track 2
    {740, 120, 720, 3},   // Track 3
    {960, 120, 720, 4},   // Track 4
    {1180, 120, 720, 5}   // Track 5
};

Junction junctions[TOTAL_JUNCTIONS] = {
    // Lower junctions (closer to start)
    {410, 250, 1, 2, -1, 250},   // Junction 1: y 250
    {630, 250, 2, 3, -1, 250},   // Junction 2: y 250
    
    // Middle junctions
    {410, 420, 1, 2, -1, 420},   // Junction 3: y 420
    {630, 420, 2, 3, -1, 420},   // Junction 4: y 420
    {850, 420, 3, 4, -1, 420},   // Junction 5: y 420
    {1070, 420, 4, 5, -1, 420},  // Junction 6: y 420
    
    // Upper junctions (near bottom)
    {850, 590, 3, 4, -1, 590},   // Junction 7: y 590
    {1070, 590, 4, 5, -1, 590}   // Junction 8: y 590
};

VisualTrain visual_trains[10];
int num_visual_trains = 0;
pthread_mutex_t visual_mutex = PTHREAD_MUTEX_INITIALIZER;

// Colors for trains
Color bright_colors[6] = {
    (Color){100, 150, 255, 255},  // Light Blue
    (Color){255, 150, 100, 255},  // Orange
    (Color){200, 100, 255, 255},  // Purple
    (Color){100, 255, 200, 255},  // Cyan
    (Color){255, 100, 200, 255},  // Pink
    (Color){150, 150, 255, 255}   // Periwinkle
};

// Calculate proper speed based on track height and travel time
// Track height = 650 pixels (700-50), Travel time = 3 seconds, Frame rate = 60 fps
const float BASE_SPEED = 650.0f / (1.2f * 60.0f); // 3.61 pixels per frame

Color getDullColor(Color c) {
    Color dull;
    dull.r = c.r / 2;
    dull.g = c.g / 2;
    dull.b = c.b / 2;
    dull.a = 200;
    return dull;
}

void drawTracks() {
    for(int i = 0; i < TOTAL_TRACKS; i++) {
        // Draw the track line
        DrawLine(track_positions[i].x, track_positions[i].y_start, 
                track_positions[i].x, track_positions[i].y_end, (Color){180, 180, 220, 255});

        // Draw track sleepers
        for(int y = track_positions[i].y_start; y < track_positions[i].y_end; y += 20) {
            DrawLine(track_positions[i].x - 8, y, track_positions[i].x + 8, y, (Color){120, 120, 150, 255});
        }

        // Draw track bed
        Color track_bed = {60, 50, 80, 80};
        DrawRectangle(track_positions[i].x - 12, track_positions[i].y_start, 
                    24, track_positions[i].y_end - track_positions[i].y_start, track_bed);
        
        // Draw track label
        char label[20];
        sprintf(label, "Track %d", track_positions[i].track_id);
        DrawText(label, track_positions[i].x - 15, track_positions[i].y_start - 60, 12, (Color){220, 220, 255, 255});
        
       
        // Draw track end marker
        DrawLine(track_positions[i].x - 12, track_positions[i].y_end, 
                 track_positions[i].x + 12, track_positions[i].y_end, RED);
        DrawText("END", track_positions[i].x - 10, track_positions[i].y_end + 5, 8, RED);
    }
}

void drawJunctions() {
    for(int i = 0; i < TOTAL_JUNCTIONS; i++) {
        int from_idx = junctions[i].from_track - 1;
        int to_idx = junctions[i].to_track - 1;
        int junction_y = junctions[i].y;
        
        // Draw the static connection curve (the dotted line)
        for(float t = 0; t <= 1; t += 0.05) {
            float cur_x = track_positions[from_idx].x + (track_positions[to_idx].x - track_positions[from_idx].x) * t;
            float cur_y = junction_y - 15 + (30 * sinf(t * 3.14159f));
            DrawCircle(cur_x, cur_y, 1.5f, (Color){150, 150, 200, 150});
        }
        
        // Draw the Junction status circle (J1, J2, etc)
        Color junction_color = (junctions[i].occupied_by == -1) ? GREEN : RED;
        DrawCircle(junctions[i].x, junctions[i].y, 10, junction_color);
        DrawCircle(junctions[i].x, junctions[i].y, 6, (Color){20, 20, 40, 255}); // Dark center
        
        char label[10];
        sprintf(label, "J%d", i + 1);
        DrawText(label, junctions[i].x - 8, junctions[i].y - 22, 10, (Color){255, 200, 100, 255});
    }
}

void drawTrain(int x, int y, int train_id, int is_moving, Color color, int waiting_at_start) {
    Color train_color;
    if(is_moving) {
        train_color = color;
    } else {
        train_color = getDullColor(color);
    }
    
    // Draw train body - using TRAIN_HEIGHT from gui.h
    DrawRectangle(x - TRAIN_WIDTH/2, y - TRAIN_HEIGHT/2, TRAIN_WIDTH, TRAIN_HEIGHT, train_color);
    DrawRectangleLines(x - TRAIN_WIDTH/2, y - TRAIN_HEIGHT/2, TRAIN_WIDTH, TRAIN_HEIGHT, BLACK);
    
    // Draw windows - positioned for taller train
    DrawCircle(x - 8, y + TRAIN_HEIGHT/8, 4, WHITE);
    DrawCircle(x + 8, y + TRAIN_HEIGHT/8, 4, WHITE);
    
    // Draw train number
    char num[5];
    sprintf(num, "%d", train_id);
    DrawText(num, x - 5, y - 8, 18, BLACK);
    
    // Draw wheels - positioned at bottom
    DrawCircle(x - 12, y + TRAIN_HEIGHT/2 - 5, 4, DARKGRAY);
    DrawCircle(x + 12, y + TRAIN_HEIGHT/2 - 5, 4, DARKGRAY);
    DrawCircle(x - 12, y + TRAIN_HEIGHT/2 - 13, 4, DARKGRAY);
    DrawCircle(x + 12, y + TRAIN_HEIGHT/2 - 13, 4, DARKGRAY);
    
    // Draw waiting indicator
    if(waiting_at_start && !is_moving) {
        DrawCircle(x, y - TRAIN_HEIGHT/2 - 8, 8, YELLOW);
        DrawText("!", x - 3, y - TRAIN_HEIGHT/2 - 12, 12, BLACK);
    }
}
void* startGUI(void* arg) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Railway Signal Coordination System");
    SetTargetFPS(60);
    
    // Initialize visual trains
    for(int i = 0; i < 10; i++) {
        visual_trains[i].train_id = -1;
        visual_trains[i].using_junction = 0;
        visual_trains[i].waiting_at_start = 0;
        visual_trains[i].speed = BASE_SPEED;
        visual_trains[i].current_track = 1;
        visual_trains[i].y_pos = 50;
    }
    
    while(!WindowShouldClose() && !IsKeyPressed(KEY_ESCAPE) && simulation_running) {
        // Update visual trains based on actual train states
        pthread_mutex_lock(&visual_mutex);
        
        // Add new trains if they appear
        for(int i = 0; i < global_train_count; i++) {
            Train* t = &global_trains[i];
            int found = 0;
            
            for(int j = 0; j < num_visual_trains; j++) {
                if(visual_trains[j].train_id == t->id) {
                    found = 1;
                    break;
                }
            }
            
            // Add train if not found and not finished
            if(!found && t->state != FINISHED && num_visual_trains < 10) {
                visual_trains[num_visual_trains].train_id = t->id;
                visual_trains[num_visual_trains].current_track = t->track1;
                visual_trains[num_visual_trains].dest_track = t->track2;
                visual_trains[num_visual_trains].y_pos = track_positions[t->track1-1].y_start;
                visual_trains[num_visual_trains].moving = (t->state == MOVING);
                visual_trains[num_visual_trains].using_junction = 0;
                visual_trains[num_visual_trains].waiting_at_start = (t->state != MOVING);
                visual_trains[num_visual_trains].junction_progress = 0;
                visual_trains[num_visual_trains].speed = BASE_SPEED;
                visual_trains[num_visual_trains].junctions_used = 0;
                visual_trains[num_visual_trains].max_junctions = abs(t->track2 - t->track1); // Number of track jumps needed
                num_visual_trains++;
                printf("Added train %d on track %d, dest track %d, state=%d, y_pos=%.1f\n", 
                       t->id, t->track1, t->track2, t->state, visual_trains[num_visual_trains-1].y_pos);
            }
        }
        
        // Update positions and states of visual trains
        for(int i = 0; i < num_visual_trains; i++) {
            // Find corresponding actual train
            Train* actual_train = NULL;
            for(int j = 0; j < global_train_count; j++) {
                if(global_trains[j].id == visual_trains[i].train_id) {
                    actual_train = &global_trains[j];
                    break;
                }
            }
            
            if(actual_train && actual_train->state != FINISHED) {
                // Update based on actual train state
                visual_trains[i].moving = (actual_train->state == MOVING);
                visual_trains[i].dest_track = actual_train->track2;
                
                // Handle waiting state
                // Inside the for loop for num_visual_trains in gui.c
            // Inside the visual trains loop in gui.c
            if (actual_train->state == WAITING_SIGNAL || actual_train->state == WAITING_TRACK) {
                visual_trains[i].waiting_at_start = 1;
                visual_trains[i].moving = 0;
                
                // ONLY reset the position if the train isn't already "at the top" 
                // of its current track. This prevents jumping.
                if (visual_trains[i].y_pos > track_positions[actual_train->track1-1].y_start + 10) {
                    // This means it just finished a loop and is teleporting to the top
                    visual_trains[i].y_pos = track_positions[actual_train->track1-1].y_start;
                    visual_trains[i].current_track = actual_train->track1;
                }
                
                visual_trains[i].using_junction = 0;
                visual_trains[i].junction_progress = 0;
                
            } else if (actual_train->state == MOVING) {
                visual_trains[i].waiting_at_start = 0;
            }
                
                // Check if tracks are adjacent
                // In the visual train update loop, replace the junction detection code:

// Check if tracks are adjacent and train can use a junction at this Y position
int tracks_adjacent = (abs(visual_trains[i].current_track - visual_trains[i].dest_track) == 1);

// Move if moving and not on junction
if(visual_trains[i].moving && !visual_trains[i].using_junction) {
    // Move down
    visual_trains[i].y_pos += visual_trains[i].speed;
    
    // Check for any available junction at current Y position
    if(tracks_adjacent && !visual_trains[i].using_junction) {
        // Find all junctions that connect current track to destination at appropriate Y level
        int available_junctions[TOTAL_JUNCTIONS];
        int junction_count = 0;
        
        for(int j = 0; j < TOTAL_JUNCTIONS; j++) {
            // Check if junction connects current track to destination track
            if((junctions[j].from_track == visual_trains[i].current_track && 
                junctions[j].to_track == visual_trains[i].dest_track) ||
               (junctions[j].from_track == visual_trains[i].dest_track && 
                junctions[j].to_track == visual_trains[i].current_track)) {
                
                // Check if train is at the correct Y position for this junction
                if(visual_trains[i].y_pos >= junctions[j].y - 15 &&
                   visual_trains[i].y_pos <= junctions[j].y + 15) {
                    available_junctions[junction_count++] = j;
                }
            }
        }
        
        // If there's an available junction at this Y position
        if(junction_count > 0) {
            // Randomly decide whether to use a junction (70% chance if multiple junctions exist)
            int use_junction = (rand() % 100 < 70);
            
            if(use_junction) {
                // Pick a random available junction
                int target_junction = available_junctions[rand() % junction_count];
                
                // Check if junction is free
                if(junctions[target_junction].occupied_by == -1) {
                    visual_trains[i].using_junction = 1;
                    visual_trains[i].target_junction = target_junction;
                    visual_trains[i].junction_progress = 0;
                    junctions[target_junction].occupied_by = actual_train->id;
                    visual_trains[i].junctions_used++;
                    printf("Train %d entering junction %d at Y=%d (junction %d of %d)\n", 
                           actual_train->id, target_junction, junctions[target_junction].y,
                           visual_trains[i].junctions_used, visual_trains[i].max_junctions);
                } else {
                    // Junction occupied, stop moving temporarily
                    visual_trains[i].moving = 0;
                    visual_trains[i].y_pos = junctions[target_junction].y - 20;
                    printf("Train %d waiting at junction %d\n", actual_train->id, target_junction);
                }
            }
        }
    }
}
                else if(visual_trains[i].moving && visual_trains[i].using_junction) {
                    // Move through junction
                    visual_trains[i].junction_progress += 4;
                    
                    if(visual_trains[i].junction_progress >= 100) {
                        visual_trains[i].current_track = visual_trains[i].dest_track;
                        visual_trains[i].using_junction = 0;
                        junctions[visual_trains[i].target_junction].occupied_by = -1;
                        visual_trains[i].y_pos = junctions[visual_trains[i].target_junction].y + 5;
                        printf("Train %d finished junction, now on track %d\n", 
                               actual_train->id, visual_trains[i].current_track);
                    }
                }
                
                /// In the movement section, add a safety timeout counter
// static int movement_timeout[TOTAL_TRACKS] = {0};

// Check if reached bottom
if(!visual_trains[i].using_junction) {
    int track_idx = visual_trains[i].current_track - 1;
    int bottom_y = track_positions[track_idx].y_end;
    
    if(visual_trains[i].y_pos >= bottom_y) {
        if(actual_train && !actual_train->reached_bottom) {
            actual_train->reached_bottom = 1;
            printf("Train %d reached bottom\n", actual_train->id);
        }
        // Reset to top for next loop
        visual_trains[i].y_pos = track_positions[track_idx].y_start;
        visual_trains[i].waiting_at_start = 1;
        visual_trains[i].moving = 0;
        visual_trains[i].using_junction = 0;
        // movement_timeout[i] = 0;
    }
}
                
                // Keep within bounds
                int track_idx = visual_trains[i].current_track - 1;
                if(visual_trains[i].y_pos < track_positions[track_idx].y_start) {
                    visual_trains[i].y_pos = track_positions[track_idx].y_start;
                }
                if(visual_trains[i].y_pos > track_positions[track_idx].y_end) {
                    visual_trains[i].y_pos = track_positions[track_idx].y_end;
                }
            } else if(actual_train && actual_train->state == FINISHED) {
                // Remove finished train
                for(int j = i; j < num_visual_trains - 1; j++) {
                    visual_trains[j] = visual_trains[j + 1];
                }
                num_visual_trains--;
                i--;
                printf("Removed finished train\n");
            }
        }
        
        pthread_mutex_unlock(&visual_mutex);
        
        // Drawing section
        BeginDrawing();
        ClearBackground((Color){20, 15, 35, 255});
        
        // Draw title
        DrawText("RAILWAY SIGNAL COORDINATION SYSTEM", SCREEN_WIDTH/2 - 300, 10, 20, (Color){255, 200, 100, 255});
        
        // Draw tracks and junctions
        drawTracks();
        drawJunctions();
        
        // Draw all trains
        pthread_mutex_lock(&visual_mutex);
        for(int i = 0; i < num_visual_trains; i++) {
            if(visual_trains[i].train_id != -1) {
                int x, y;
                
                if(visual_trains[i].using_junction) {
    int j = visual_trains[i].target_junction;
    float t = visual_trains[i].junction_progress / 100.0f;
    int start_x = track_positions[visual_trains[i].current_track - 1].x;
    int end_x = track_positions[visual_trains[i].dest_track - 1].x;
    
    // Train curve position
    x = start_x + (end_x - start_x) * t;
    y = junctions[j].y - 15 + (30 * sinf(t * 3.14159f));

    // --- TEXT-BASED PULSING ARROW & LABEL ---
    int direction = (end_x > start_x) ? 1 : -1; // 1 = Right, -1 = Left
    int arrow_y = junctions[j].y + 22;
    int arrow_x = junctions[j].x;
    
    // Pulse calculation for a "breathing" effect
    float pulse = sinf(GetTime() * 10.0f) * 5.0f; 

    if (direction > 0) {
        // RIGHT: Draw line and '>'
        DrawLine(arrow_x - 10, arrow_y, arrow_x + (int)pulse, arrow_y, YELLOW);
        DrawText(">", arrow_x + (int)pulse + 2, arrow_y - 5, 15, YELLOW);
        DrawText("RIGHT", arrow_x - 15, arrow_y + 12, 10, YELLOW);
    } else {
        // LEFT: Draw line and '<'
        DrawLine(arrow_x + 10, arrow_y, arrow_x - (int)pulse, arrow_y, YELLOW);
        DrawText("<", arrow_x - (int)pulse - 12, arrow_y - 5, 15, YELLOW);
        DrawText("LEFT", arrow_x - 12, arrow_y + 12, 10, YELLOW);
    }
} else {
            // Normal track position (existing code)
            int track_idx = visual_trains[i].current_track - 1;
            x = track_positions[track_idx].x;
            y = visual_trains[i].y_pos;
        }
                
                Color train_color = bright_colors[visual_trains[i].train_id % 6];
                drawTrain(x, y, visual_trains[i].train_id, visual_trains[i].moving, 
                         train_color, visual_trains[i].waiting_at_start);
                
                // Draw status
                char status[50];
                if(visual_trains[i].moving) {
                    if(visual_trains[i].using_junction) {
                        sprintf(status, "Train %d: JUNCTION", visual_trains[i].train_id);
                    } else {
                        sprintf(status, "Train %d: MOVING", visual_trains[i].train_id);
                    }
                    DrawText(status, x - 35, y - 25, 10, GREEN);
                } else {
                    sprintf(status, "Train %d: WAITING", visual_trains[i].train_id);
                    DrawText(status, x - 35, y - 25, 10, ORANGE);
                }
            }
        }
        pthread_mutex_unlock(&visual_mutex);
        
        // Schedule panel
        DrawRectangle(SCREEN_WIDTH - 280, 10, 270, 130, (Color){30, 25, 45, 230});
        DrawRectangleLines(SCREEN_WIDTH - 280, 10, 270, 130, (Color){150, 130, 200, 255});
        DrawText("SCHEDULE", SCREEN_WIDTH - 280 + 135 - MeasureText("SCHEDULE", 15)/2, 15, 15, (Color){255, 200, 100, 255});
        
        pthread_mutex_lock(&visual_mutex);
        int y_offset = 40;
        
        for(int i = 0; i < global_train_count && i < 5; i++) {
            char schedule_text[100];
            Color text_color;
            
            if(global_trains[i].state == FINISHED) {
                sprintf(schedule_text, "Train %d: JOURNEY COMPLETE", global_trains[i].id);
                text_color = GRAY;
            } 
            else if(global_trains[i].waiting_at_bottom) {
                sprintf(schedule_text, "Train %d: ARRIVED AT STATION", global_trains[i].id);
                text_color = SKYBLUE;
            }
            else if(global_trains[i].state == WAITING_SIGNAL) {
                sprintf(schedule_text, "Train %d: PLANNING ROUTE", global_trains[i].id);
                text_color = ORANGE;
            }
            else if(global_trains[i].state == WAITING_TRACK) {
                sprintf(schedule_text, "Train %d: AWAITING DEPARTURE", global_trains[i].id);
                text_color = YELLOW;
            }
            else if(global_trains[i].state == MOVING) {
                sprintf(schedule_text, "Train %d: DEPARTED", global_trains[i].id);
                    text_color = GREEN;
            }
            else if(global_trains[i].state == ARRIVING) {
    sprintf(schedule_text, "Train %d: ARRIVING AT STATION", global_trains[i].id);
    text_color = SKYBLUE;
}
            
            int text_width = MeasureText(schedule_text, 10);
            DrawText(schedule_text, SCREEN_WIDTH - 280 + (270 - text_width)/2, y_offset, 10, text_color);
            y_offset += 22;
        }
        pthread_mutex_unlock(&visual_mutex);

        // Track Status Panel (moved down, size unchanged)
        DrawRectangle(SCREEN_WIDTH - 280, 150, 270, 150, (Color){30, 25, 45, 230});
        DrawRectangleLines(SCREEN_WIDTH - 280, 150, 270, 150, (Color){150, 130, 200, 255});
        DrawText("TRACK STATUS", SCREEN_WIDTH - 280 + 135 - MeasureText("TRACK STATUS", 11)/2, 165, 11, (Color){255, 200, 100, 255});

        // Find where trains are actually moving on tracks (not on junctions)
        int moving_trains_on_track[TOTAL_TRACKS] = {0};
        
        pthread_mutex_lock(&visual_mutex);
        for(int v = 0; v < num_visual_trains; v++) {
            if(visual_trains[v].train_id != -1 && visual_trains[v].moving == 1) {
                // Only show on track if NOT using a junction
                if(visual_trains[v].using_junction == 0) {
                    int track_idx = visual_trains[v].current_track - 1;
                    moving_trains_on_track[track_idx] = visual_trains[v].train_id;
                }
            }
        }
        pthread_mutex_unlock(&visual_mutex);

        // Display track status
        int y_track = 190;
        for(int i = 0; i < TOTAL_TRACKS; i++) {
            char track_text[60];
            if(moving_trains_on_track[i] != 0) {
                sprintf(track_text, "Track %d: Train %d (MOVING)", i+1, moving_trains_on_track[i]);
                int text_width = MeasureText(track_text, 10);
                DrawText(track_text, SCREEN_WIDTH - 280 + (270 - text_width)/2, y_track, 10, (Color){255, 80, 80, 255});
            } else {
                sprintf(track_text, "Track %d: FREE", i+1);
                int text_width = MeasureText(track_text, 10);
                DrawText(track_text, SCREEN_WIDTH - 280 + (270 - text_width)/2, y_track, 10, (Color){80, 255, 80, 255});
            }
            y_track += 20;
        }

        // JUNCTION STATUS panel (BIGGER - moved down)
        DrawRectangle(SCREEN_WIDTH - 280, 310, 270, 170, (Color){30, 25, 45, 230});
        DrawRectangleLines(SCREEN_WIDTH - 280, 310, 270, 170, (Color){150, 130, 200, 255});
        DrawText("JUNCTION STATUS", SCREEN_WIDTH - 280 + 135 - MeasureText("JUNCTION STATUS", 11)/2, 315, 11, (Color){255, 200, 100, 255});
        
        int y_junc = 335;
        for(int j = 0; j < TOTAL_JUNCTIONS; j++) {
            char junc_text[60];
            if(junctions[j].occupied_by != -1) {
                sprintf(junc_text, "J%d: Train %d (BUSY)", j+1, junctions[j].occupied_by);
                int text_width = MeasureText(junc_text, 9);
                DrawText(junc_text, SCREEN_WIDTH - 280 + (270 - text_width)/2, y_junc, 9, (Color){255, 80, 80, 255});
            } else {
                sprintf(junc_text, "J%d: FREE", j+1);
                int text_width = MeasureText(junc_text, 9);
                DrawText(junc_text, SCREEN_WIDTH - 280 + (270 - text_width)/2, y_junc, 9, (Color){80, 255, 80, 255});
            }
            y_junc += 18;
        }

        // Current Loop Info Panel (BIGGER - moved down)
        DrawRectangle(SCREEN_WIDTH - 280, 490, 270, 110, (Color){30, 25, 45, 230});
        DrawRectangleLines(SCREEN_WIDTH - 280, 490, 270, 110, (Color){150, 130, 200, 255});
        DrawText("CURRENT LOOPS", SCREEN_WIDTH - 280 + 135 - MeasureText("CURRENT LOOPS", 11)/2, 495, 11, (Color){255, 200, 100, 255});

        int y_loop = 515;
        for(int i = 0; i < global_train_count && i < 5; i++) {
            char loop_text[50];
            if(global_trains[i].state == FINISHED) {
                sprintf(loop_text, "Train %d: COMPLETED", global_trains[i].id);
            } else {
                sprintf(loop_text, "Train %d: Loop %d of %d", global_trains[i].id, global_trains[i].loop_count + 1, global_trains[i].max_loops);
            }
            int text_width = MeasureText(loop_text, 9);
            DrawText(loop_text, SCREEN_WIDTH - 280 + (270 - text_width)/2, y_loop, 9, (Color){220, 220, 255, 255});
            y_loop += 18;
        }

        // Waiting Times Panel (moved down)
        DrawRectangle(SCREEN_WIDTH - 280, 610, 270, 120, (Color){30, 25, 45, 230});
        DrawRectangleLines(SCREEN_WIDTH - 280, 610, 270, 120, (Color){150, 130, 200, 255});
        DrawText("WAITING TIMES", SCREEN_WIDTH - 280 + 135 - MeasureText("WAITING TIMES", 9)/2, 615, 9, (Color){255, 200, 100, 255});

        int y_wait = 635;
        double total_all_waits = 0;
        int trains_with_data = 0;
        time_t current_time = time(NULL);

        for(int i = 0; i < global_train_count; i++) {
            char wait_text[80];
            int show_train = 0;
            double avg_wait = 0;
            
            // CASE 1: Train is currently waiting (CHOOSING DESTINATION or WAITING FOR DEPARTURE)
            if((global_trains[i].state == WAITING_SIGNAL || global_trains[i].state == WAITING_TRACK) && global_trains[i].is_currently_waiting) {
                double current_wait = difftime(current_time, global_trains[i].current_wait_start);
                if(global_trains[i].wait_count > 0) {
                    avg_wait = global_trains[i].total_wait_time / global_trains[i].wait_count;
                    sprintf(wait_text, "Train %d: Now %.0fs (Avg %.1fs)", global_trains[i].id, current_wait, avg_wait);
                    total_all_waits += avg_wait;
                    trains_with_data++;
                } else {
                    sprintf(wait_text, "Train %d: Now %.0fs", global_trains[i].id, current_wait);
                }
                show_train = 1;
            }
            // CASE 2: Train has finished all loops (DONE)
            else if(global_trains[i].state == FINISHED && global_trains[i].wait_count > 0) {
                avg_wait = global_trains[i].total_wait_time / global_trains[i].wait_count;
                sprintf(wait_text, "Train %d: Avg %.1fs (DONE)", global_trains[i].id, avg_wait);
                total_all_waits += avg_wait;
                trains_with_data++;
                show_train = 1;
            }
            // CASE 3: Train is moving or at station (show average only if it has waited before)
            else if(global_trains[i].wait_count > 0 && (global_trains[i].state == MOVING || global_trains[i].waiting_at_bottom)) {
                avg_wait = global_trains[i].total_wait_time / global_trains[i].wait_count;
                sprintf(wait_text, "Train %d: Avg %.1fs (MOVING)", global_trains[i].id, avg_wait);
                total_all_waits += avg_wait;
                trains_with_data++;
                show_train = 1;
            }
            
            if(show_train) {
                int text_width = MeasureText(wait_text, 9);
                DrawText(wait_text, SCREEN_WIDTH - 280 + (270 - text_width)/2, y_wait, 9, (Color){220, 220, 255, 255});
                y_wait += 16;
            }
        }
        
        if(y_wait == 625) {
            DrawText("No data yet", SCREEN_WIDTH - 280 + 135 - MeasureText("No data yet", 9)/2, y_wait, 9, (Color){255, 200, 100, 255});
        }

        // Total Average Wait Time Panel (moved down)
        DrawRectangle(SCREEN_WIDTH - 280, 740, 270, 55, (Color){30, 25, 45, 230});
        DrawRectangleLines(SCREEN_WIDTH - 280, 740, 270, 55, (Color){150, 130, 200, 255});
        DrawText("TOTAL AVG WAIT TIME", SCREEN_WIDTH - 280 + 135 - MeasureText("TOTAL AVG WAIT TIME", 9)/2, 745, 9, (Color){255, 200, 100, 255});

        if(trains_with_data > 0) {
            double total_avg = total_all_waits / trains_with_data;
            char total_text[50];
            sprintf(total_text, "%.1f seconds (over %d trains)", total_avg, trains_with_data);
            int text_width = MeasureText(total_text, 11);
            DrawText(total_text, SCREEN_WIDTH - 280 + (270 - text_width)/2, 768, 11, (Color){100, 255, 200, 255});
        } else {
            DrawText("Calculating...", SCREEN_WIDTH - 280 + 135 - MeasureText("Calculating...", 11)/2, 768, 11, (Color){255, 200, 100, 255});
        }
// Legend (Bottom Left) - your existing legend code continues here
        
        // Legend
        DrawRectangle(10, SCREEN_HEIGHT - 300, 220, 120, (Color){30, 25, 45, 230});
        DrawRectangleLines(10, SCREEN_HEIGHT - 300, 220, 120, (Color){150, 130, 200, 255});
        DrawText("SYMBOLS:", 75, SCREEN_HEIGHT - 320, 15, (Color){255, 200, 100, 255});
        DrawCircle(25, SCREEN_HEIGHT - 280, 8,GREEN);
        DrawText("Green Junction (Free)", 60, SCREEN_HEIGHT - 285, 12, (Color){220, 220, 255, 255});
        DrawCircle(25, SCREEN_HEIGHT - 260, 8, RED);
        DrawText("Red Junction (Occupied)", 60, SCREEN_HEIGHT - 265, 12, (Color){220, 220, 255, 255});
        DrawRectangle(15, SCREEN_HEIGHT - 250, 20, 15, (Color){100, 150, 255, 255});
        DrawText("Bright: Moving", 60, SCREEN_HEIGHT - 245, 12, (Color){220, 220, 255, 255});
        Color dull_blue = {50, 75, 128, 200};
        DrawRectangle(15, SCREEN_HEIGHT - 227, 20, 15, dull_blue);
        DrawText("Dull: Waiting", 60, SCREEN_HEIGHT - 225, 12, (Color){220, 220, 255, 255});
        DrawCircle(25, SCREEN_HEIGHT - 200, 7, YELLOW);
        DrawText("!", 25, SCREEN_HEIGHT-205, 12, BLACK);
        DrawText("Waiting at Station", 60, SCREEN_HEIGHT - 205, 12, (Color){220, 220, 255, 255});
        
        EndDrawing();
        
        usleep(16000);
    }
    
        simulation_running = 0;
    printf("\nSimulation completed. Press ESC to close window.\n");
    
    // Wait for ESC key to close
    while(!WindowShouldClose() && !IsKeyPressed(KEY_ESCAPE)) {
        BeginDrawing();
        ClearBackground((Color){20, 15, 35, 255});
        DrawText("SIMULATION COMPLETED", SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT/2 - 30, 25, (Color){100, 255, 100, 255});
        DrawText("Press ESC to exit", SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 + 20, 15, (Color){255, 200, 100, 255});
        
        // Also show final statistics on screen
        int completed = 0;
        for(int i = 0; i < global_train_count; i++) {
            if(global_trains[i].state == FINISHED) completed++;
        }
        char stats_text[100];
        sprintf(stats_text, "Completed: %d/%d trains", completed, global_train_count);
        DrawText(stats_text, SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 + 60, 12, (Color){220, 220, 255, 255});
        
        EndDrawing();
    }
    
    CloseWindow();
    return NULL;
}
