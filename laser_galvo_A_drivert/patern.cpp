


#include "patern.hpp"
#include <math.h>
#include <vector>
#include <Arduino.h>

#include <vector>
using namespace std;

#define MAX 4095

struct patern_point {
    int x;
    int y;
    bool laser_on;
} ;

int patern_index = 0;
vector<patern_point> patern_points;
bool upload_patern_active = false;  

void patern_setup()
{
    patern_create_square(0,0,MAX,MAX, true);
}

void patern_add_line(int x1, int y1, int x2, int y2, bool laser_on)
{
    #define STEP_DISTANCE 200
    int dx = x2 - x1;
    int dy = y2 - y1;
    float distance = sqrt( (dx*dx) + (dy*dy) );
    
    int NO_STEPS = max(1, (int)ceil(distance / STEP_DISTANCE));
      
    for (int i = 0; i <= NO_STEPS; i++) {
        float t = (float)i / NO_STEPS;  // 0.0 to 1.0
        int x = x1 + (int)(dx * t);
        int y = y1 + (int)(dy * t);
        patern_upload_step(x, y, laser_on);
    }
}

void patern_add_square(int xmin, int xmax, int ymin, int ymax, bool laser_on = true)  
{
    // xmin, ymin
    patern_add_line(xmin, ymin, xmax, ymin, laser_on);
    patern_add_line(xmax, ymin, xmax, ymax, laser_on);
    patern_add_line(xmax, ymax, xmin, ymax, laser_on);
    patern_add_line(xmin, ymax, xmin, ymin, laser_on);
}
void patern_create_square(int xmin, int xmax, int ymin, int ymax, bool laser_on)
{
    patern_upload_start();
    patern_add_square(xmin, xmax, ymin, ymax, laser_on);
    patern_upload_stop();
}

void patern_double_square(bool one_on, bool two_on)
{
    patern_upload_start();

    patern_add_square(0, 0.75*MAX, 0, 0.75*MAX, one_on);
    patern_add_line(0, 0, 0.25*MAX, 0.25*MAX, false); // laser off between squares

    patern_add_square(0.25*MAX, MAX, 0.25*MAX, MAX, two_on);
    patern_add_line(0.25*MAX, 0.25*MAX, 0, 0, false); // laser off between squares
    
    patern_upload_stop();
}

void patern_create_circle()
{
    patern_upload_start();
    const int steps = 100;
    for (int i = 0; i < steps; i++) {
        float angle = (2.0f * 3.14159265f * i) / steps;
        int x = (int)( (MAX / 2) + (MAX / 2) * cos(angle) );
        int y = (int)( (MAX / 2) + (MAX / 2) * sin(angle) );
        patern_upload_step(x, y, true);
    }
    patern_upload_stop();
}

void patern_create_point(int x, int y, bool laser_on)
{
    patern_upload_start();
    patern_upload_step(x, y, laser_on);
    patern_upload_stop();
}

void patern_get_next_step(int & x, int & y, bool &laser_on)
{
    if (patern_points.size() == 0)
    {
        x = 0;
        y = 0;
        laser_on = true;
        return;
    }
    if (upload_patern_active)
    {
        x = MAX/2;
        y = MAX/2;
        laser_on = true;
        return;
    }

    x = patern_points[patern_index].x;
    y = patern_points[patern_index].y;
    laser_on = patern_points[patern_index].laser_on;
    patern_index++;
    if (patern_index >= patern_points.size()) patern_index = 0;
    return;
}


void patern_upload_start()
{
    patern_index = 0;
    patern_points.clear();
    upload_patern_active = true;
}


void patern_upload_step(int x, int y, bool laser_on)
{
    if (upload_patern_active)
    {
        patern_points.push_back({x, y, laser_on});
    }
}
void patern_upload_stop()
{
    upload_patern_active = false;
    // log the patern to serial
    Serial.print("# Patern uploaded, length: ");
    Serial.println(patern_points.size());
    for (size_t i = 0; i < patern_points.size(); i++)
    {
        Serial.print("# ");
        Serial.print(i);
        Serial.print(": x=");
        Serial.print(patern_points[i].x);
        Serial.print(", y=");
        Serial.print(patern_points[i].y);
        Serial.print(", laser_on=");
        Serial.println(patern_points[i].laser_on ? "true" : "false");
    }

}

int patern_get_length()
{
    return patern_points.size();
}