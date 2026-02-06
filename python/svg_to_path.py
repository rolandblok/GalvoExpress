#create a cubocbezier, and interpolate points along the curve
# Add this import instead
import time
import numpy as np
import math

from trav_sales import traveling_salesman, sort_paths_by_proximity

def cubic_bezier(p0, p1, p2, p3, num_points=10):
    t = np.linspace(0, 1, num_points)
    points = ((1 - t) ** 3)[:, None] * np.array(p0) + \
             3 * ((1 - t) ** 2)[:, None] * t[:, None] * np.array(p1) + \
             3 * (1 - t)[:, None] * (t ** 2)[:, None] * np.array(p2) + \
             (t ** 3)[:, None] * np.array(p3) 
    return points


# read SVG file and convert to SVG path data
def svg_to_svg_path(svg_file):
    from xml.dom import minidom

    # function to create a path list of paths
    # a path is a list of numpy points.
    paths = []
    bounds_min = [float('inf'), float('inf')]
    bounds_max = [float('-inf'), float('-inf')]
    def update_bounds(x, y):
        nonlocal bounds_min, bounds_max
        bounds_min = [min(bounds_min[0], x), min(bounds_min[1], y)]
        bounds_max = [max(bounds_max[0], x), max(bounds_max[1], y)]
    
    # Load and parse the SVG file
    doc = minidom.parse(svg_file)

    # Iterate through all elements in the SVG
    for element in doc.getElementsByTagName('*'):
        if element.tagName == 'path':
            # Extract the 'd' attribute which contains the path data
            d = element.getAttribute('d')
            d = d.replace('\n', ' ')
            d = d.replace(',', ' ')
            # add whitespace around commands
            import re
            commands = d.split()
            current_pos = (0, 0)
            start_pos = (0, 0)
            cmd = ''
            path_data = []
            i = 0   
            while i < len(commands):
                # if the command is a float, repeat the last command
                if re.match(r'-?\d*\.?\d+', commands[i]):
                    if cmd == 'M':
                        cmd = 'L'
                    elif cmd == 'm':
                        cmd = 'l'
                    commands.insert(i, cmd)

                cmd = commands[i]
                if cmd == 'M':
                    x = float(commands[i + 1])
                    y = float(commands[i + 2])
                    current_pos = (x, y)
                    start_pos = (x, y)
                    update_bounds(x, y)
                    # if path_data is not empty, start a new path
                    if len(path_data) > 0:
                        paths.append(path_data)
                    path_data = []
                    path_data.append(current_pos)
                    i += 3
                elif cmd == 'm':
                    x = current_pos[0] + float(commands[i + 1])
                    y = current_pos[1] + float(commands[i + 2])
                    current_pos = (x, y)
                    start_pos = (x, y)
                    update_bounds(x, y)
                    # if path_data is not empty, start a new path
                    if len(path_data) > 0:
                        paths.append(path_data)
                        path_data = []
                    path_data.append(current_pos)
                    i += 3
                elif cmd == 'L':
                    x = float(commands[i + 1])
                    y = float(commands[i + 2])
                    current_pos = (x, y)
                    update_bounds(x, y)
                    path_data.append(current_pos)
                    i += 3
                elif cmd == 'l':
                    x = current_pos[0] + float(commands[i + 1])
                    y = current_pos[1] + float(commands[i + 2])
                    current_pos = (x, y)
                    update_bounds(x, y)
                    path_data.append(current_pos)
                    i += 3
                elif cmd == 'H':
                    x = float(commands[i + 1])
                    current_pos = (x, current_pos[1])
                    update_bounds(x, current_pos[1])
                    path_data.append(current_pos)
                    i += 2
                elif cmd == 'h':
                    x = current_pos[0] + float(commands[i + 1])
                    current_pos = (x, current_pos[1])
                    update_bounds(x, current_pos[1])
                    path_data.append(current_pos)
                    i += 2
                elif cmd == 'V':
                    y = float(commands[i + 1])
                    current_pos = (current_pos[0], y)
                    update_bounds(current_pos[0], y)
                    path_data.append(current_pos)
                    i += 2
                elif cmd == 'v':
                    y = current_pos[1] + float(commands[i + 1])
                    current_pos = (current_pos[0], y)
                    update_bounds(current_pos[0], y)
                    path_data.append(current_pos)
                    i += 2
                elif cmd == 'Z' or cmd == 'z':
                    current_pos = start_pos
                    update_bounds(current_pos[0], current_pos[1])
                    path_data.append(current_pos)
                    i += 1
                elif cmd == 'C': # cubic Bezier curve (not implemented)
                    prev = current_pos
                    p1 = (float(commands[i + 1]), float(commands[i + 2]))
                    p2 = (float(commands[i + 3]), float(commands[i + 4]))   
                    p3 = (float(commands[i + 5]), float(commands[i + 6]))
                    bezier_points = cubic_bezier(prev, p1, p2, p3, num_points=4)
                    for bp in bezier_points[1:]:
                        path_data.append((bp[0], bp[1]))
                        update_bounds(bp[0], bp[1])
                    current_pos = bezier_points[-1]
                    i += 7  # Move to the next command
                elif cmd == 'c': # cubic Bezier curve (not implemented)
                    prev = current_pos
                    p1 = (current_pos[0] + float(commands[i + 1]), current_pos[1] + float(commands[i + 2]))
                    p2 = (current_pos[0] + float(commands[i + 3]), current_pos[1] + float(commands[i + 4]))   
                    p3 = (current_pos[0] + float(commands[i + 5]), current_pos[1] + float(commands[i + 6]))
                    bezier_points = cubic_bezier(prev, p1, p2, p3, num_points=4)
                    for bp in bezier_points[1:]:
                        path_data.append((bp[0], bp[1]))
                        update_bounds(bp[0], bp[1])
                    current_pos = bezier_points[-1]
                    i += 7  # Move to the next command
                
                else:
                    print(f"Unhandled command: {cmd}")
                    i += 1  # Skip unhandled commands for simplicity
            paths.append(path_data)
        elif element.tagName == 'rect':
            print("rect not supported yet")
        elif element.tagName == 'circle':
            print("circle not supported yet")
        elif element.tagName == 'ellipse':
           print("ellipse not supported yet")
        elif element.tagName == 'line':
            print("line not supported yet")
    doc.unlink()
    return paths, (bounds_min, bounds_max)

def scale_svg_paths(paths_in, min_max, WIDTH=800, HEIGHT=600, invert_x=False, invert_y=True):

    # create a scaling factor to fit the SVG into the window
# create a scaling factor to fit the SVG into the window
    bounds_min, bounds_max = min_max  
    svg_width = bounds_max[0] - bounds_min[0]
    svg_height = bounds_max[1] - bounds_min[1]
    scale_x = (WIDTH - 20) / svg_width
    scale_y = (HEIGHT - 20) / svg_height
    scale = min(scale_x, scale_y)
    width = int(svg_width * scale) + 20
    height = int(svg_height * scale) + 20
    x_offset = -bounds_min[0] * scale + 10
    y_offset = -bounds_min[1] * scale + 10

    scaled_paths = []
    for path in paths_in:
        scaled_path = []
        for point in path:
            x = int(point[0] * scale + x_offset)
            y = int(point[1] * scale + y_offset)
            scaled_path.append((x, y))
        scaled_paths.append(scaled_path)

    # revert y axis
    if invert_y:
        for path in scaled_paths:
            for i in range(len(path)):
                path[i] = (path[i][0], height - path[i][1]) 

    # revert x axis
    if invert_x:
        for path in scaled_paths:
            for i in range(len(path)):
                path[i] = (width - path[i][0], path[i][1])

    # center the paths
    x_center_offset = (WIDTH - width) // 2
    y_center_offset = (HEIGHT - height) // 2
    for path in scaled_paths:
        for i in range(len(path)):
            path[i] = (path[i][0] + x_center_offset, path[i][1] + y_center_offset)


    return scaled_paths



# function to create TK window that scales the SVG and draws the path data
def display_svg_path(paths, min_max):
    import tkinter as tk
    from tkinter import filedialog
    import serial.tools.list_ports

    width = 800
    height = 600
    
    scaled_paths = scale_svg_paths(paths, min_max, WIDTH=width, HEIGHT=height)
# Create a Tkinter window
    root = tk.Tk()
    root.title("SVG Path Display")
    
    # Create canvas
    canvas = tk.Canvas(root, width=width, height=height, bg='white')
    canvas.pack()

    # Draw the line to line point on the canvas
    for path_point in scaled_paths:
        for i in range(len(path_point) - 1):
            # set line color to black and width to 1
            canvas.create_line(path_point[i][0], path_point[i][1], path_point[i + 1][0], path_point[i + 1][1], fill='black', width=2)
            if i == 0:
                color = 'red'  # start point
            elif i == 1:
                color = 'orange'  # second point
            else:
                color = 'green'  # end point
            # draw a circle at each point
            canvas.create_oval(path_point[i][0]-3, path_point[i][1]-3, path_point[i][0]+3, path_point[i][1]+3, fill=color)

        # draw line to next path start
        if path_point != scaled_paths[-1]:
            next_path = scaled_paths[scaled_paths.index(path_point) + 1]
            canvas.create_line(path_point[-1][0], path_point[-1][1], next_path[0][0], next_path[0][1], fill='red', dash=(4, 2), arrow=tk.LAST, arrowshape=(8, 10, 3))
        else: #line to first path start
            first_path = scaled_paths[0]
            canvas.create_line(path_point[-1][0], path_point[-1][1], first_path[0][0], first_path[0][1], fill='red', dash=(4, 2), arrow=tk.LAST, arrowshape=(8, 10, 3))

    # Create a frame for controls
    control_frame = tk.Frame(root)
    control_frame.pack(fill=tk.X, padx=10, pady=10)

    # Upload button
    def upload_svg_to_com():
        com_port = com_var.get()
        if com_port == "No COM ports available":
            print("No COM port selected.")
            return
        try:
            # Prevent Arduino reset on connection
            ser = serial.Serial(com_port, 115200, timeout=1, 
                            dsrdtr=False, rtscts=False)
            ser.dtr = False
            ser.rts = False
                
            time.sleep(2)  # Longer delay for Arduino to stabilize
        
            # Clear any existing data in buffers
            ser.reset_input_buffer()
            ser.reset_output_buffer()

            # serial read res[ponse
            try:
                response = ser.readline().decode('utf-8')
                print(f"Initial Response: {response}")
            except Exception as e:
                print(f"Initial Response: Error reading from serial: {e}")
            except UnicodeDecodeError:
                print("Initial Response: Received undecodable data")   

            print(f"Opened COM port: {com_port}")
            scaled_paths = scale_svg_paths(paths, min_max, WIDTH=4095, HEIGHT=4095, invert_x= True, invert_y=False)
            
            ser.write("upload_start\n".encode('utf-8'))
            time.sleep(0.01)
            # Wait for upload_start confirmation
            response = ""
            for attempt in range(5):
                response = ser.readline().decode('utf-8').strip()
                print(f"Upload start response (attempt {attempt + 1}): {response}")
        
                if response == "# upload_start":
                    break
            if response != "# upload_start":
                ser.close()
                exit()
            
            countert = 0
            for path in scaled_paths[:]:
                laser_on = "FALSE"
                for point in path:
                    command = f"{point[0]},{point[1]},{laser_on}\n"
                    laser_on = "TRUE"
                    ser.write(command.encode('utf-8'))
                    ser.flush()
                    time.sleep(0.001)  # small delay to avoid overwhelming the serial buffer
                    # response = ser.readline().decode('utf-8').strip()
                    print(f"Sent: {command.strip()}")
                    
                countert += 1
                if countert == 15:
                    break
            
            ser.write("upload_end\n".encode('utf-8'))
            # Wait for upload_end confirmation
            for attempt in range(15):
                response = ser.readline().decode('utf-8').strip()
                print(f"Upload end response (attempt {attempt + 1}): {response}")

                # check if response starts with # upload_end
                if response.startswith("# upload_end"):
                    print("Upload end confirmed.")
                    # print number of samples:
                    count = sum(len(p) for p in scaled_paths)
                    print(f"Total number of points sent: {count}")
                    break

            # final check
            if not response.startswith("# upload_end"):
                print("Upload end not confirmed, something went wrong.")

            # read all responses until timeout
            # while True:
            #     response = ser.readline().decode('utf-8').strip()
            #     print(f"Response: {response}")

            ser.close()
        except Exception as e:
            print(f"Error: {e}")

    upload_button = tk.Button(control_frame, text="Upload SVG", command=upload_svg_to_com)
    upload_button.pack(side=tk.LEFT, padx=5)

    # COM port dropdown
    com_ports = [port.device for port in serial.tools.list_ports.comports()]
    if not com_ports:
        com_ports = ["No COM ports available"]
    
    tk.Label(control_frame, text="COM Port:").pack(side=tk.LEFT, padx=5)
    com_var = tk.StringVar(value=com_ports[0])
    com_dropdown = tk.OptionMenu(control_frame, com_var, *com_ports)
    com_dropdown.pack(side=tk.LEFT, padx=5)
    root.mainloop()

# Example usage
if __name__ == "__main__":
    svg_file = '../SVG/Aircision_logo_V3.svg'  # Replace with your SVG file path
    path_data, min_max = svg_to_svg_path(svg_file)
    # path_data = sort_paths_by_proximity(path_data)
    # 
    path_data = traveling_salesman(path_data)

    # calculate the total distance of the path, including lines between paths
    total_distance = 0
    for i in range(len(path_data)):
        path = path_data[i]
        # distance along the path
        for j in range(len(path) - 1):
            total_distance += math.sqrt((path[j][0] - path[j + 1][0]) ** 2 + (path[j][1] - path[j + 1][1]) ** 2)
        # distance to next path start
        if i < len(path_data) - 1:
            next_path = path_data[i + 1]
        else:
            next_path = path_data[0]
        total_distance += math.sqrt((path[-1][0] - next_path[0][0]) ** 2 + (path[-1][1] - next_path[0][1]) ** 2)
    print(f"Total path distance: {total_distance}")
    # no of lines:
    total_lines = sum(len(path) - 1 for path in path_data) + len(path_data)  # including lines between paths
    print(f"Total number of lines (including between paths): {total_lines}")

    # print("SVG Path Data:")
    # # print the path data each element on a new line
    # for path in path_data:
    #     print(path)
    display_svg_path(path_data, min_max)