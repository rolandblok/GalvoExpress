


#include "patern.hpp"
#include <math.h>
#include <vector>


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

void patern_add_square(int xmin, int xmax, int ymin, int ymax, bool laser_on = true)  
{
    // xmin, ymin
    #define NO_STEPS_PER_SIDE 10
    for (int i = 0; i <= NO_STEPS_PER_SIDE; i++) {
        int x = xmin + ( (xmax - xmin) * i ) / NO_STEPS_PER_SIDE;
        patern_points.push_back({x, ymin, laser_on});
    }
    for (int i = 0; i <= NO_STEPS_PER_SIDE; i++) {
        int y = ymin + ( (ymax - ymin) * i ) / NO_STEPS_PER_SIDE;
        patern_points.push_back({xmax, y, laser_on});
    }
    for (int i = 0; i <= NO_STEPS_PER_SIDE; i++) {
        int x = xmax - ( (xmax - xmin) * i ) / NO_STEPS_PER_SIDE;
        patern_points.push_back({x, ymax, laser_on});
    }
    for (int i = 0; i <= NO_STEPS_PER_SIDE; i++) {
        int y = ymax - ( (ymax - ymin) * i ) / NO_STEPS_PER_SIDE;
        patern_points.push_back({xmin, y, laser_on});
    }
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

    patern_upload_step(0, 0, false); // start at origin with laser off
    patern_add_square(0, 0.75*MAX, 0, 0.75*MAX, one_on);

    patern_upload_step(0.25*MAX, 0.25*MAX, false); // laser off between squares
    patern_add_square(0.25*MAX, MAX, 0.25*MAX, MAX, two_on);
    
    patern_upload_stop();
}

void patern_create_circle()
{
    patern_points.clear();
    const int steps = 100;
    for (int i = 0; i < steps; i++) {
        float angle = (2.0f * 3.14159265f * i) / steps;
        int x = (int)( (MAX / 2) + (MAX / 2) * cos(angle) );
        int y = (int)( (MAX / 2) + (MAX / 2) * sin(angle) );
        patern_points.push_back({x, y, true});
    }
    patern_index = 0;
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
}