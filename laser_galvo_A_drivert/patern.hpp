#pragma once

void patern_create_square(int xmin, int xmax, int ymin, int ymax, bool laser_on = true);
void patern_create_circle();
void patern_double_square(bool one_on, bool two_on);
void patern_create_point(int x, int y, bool laser_on);
void patern_add_line(int x1, int y1, int x2, int y2, bool laser_on = true, float step_distance = 200);

void patern_setup();
void patern_get_next_step(int & x, int & y, bool & laser_on);   
int  patern_get_length();

void patern_upload_start();
void patern_upload_step(int x, int y, bool laser_on);
void patern_upload_stop();