

#create a cubocbezier, and interpolate points along the curve
import numpy as np
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
            print(d)
            # add whitespace around commands
            import re
            commands = d.split()
            print(commands)
            current_pos = (0, 0)
            start_pos = (0, 0)
            cmd = ''
            path_data = []
            i = 0   
            while i < len(commands):
                print(commands[i])
                # if the command is a float, repeat the last command
                if re.match(r'-?\d*\.?\d+', commands[i]):
                    commands.insert(i, cmd)

                cmd = commands[i]
                print(f"cmd: {cmd}" )
                if cmd == 'M':
                    x = float(commands[i + 1])
                    y = float(commands[i + 2])
                    current_pos = (x, y)
                    start_pos = (x, y)
                    update_bounds(x, y)
                    path_data.append(current_pos)
                    i += 3
                elif cmd == 'm':
                    x = current_pos[0] + float(commands[i + 1])
                    y = current_pos[1] + float(commands[i + 2])
                    current_pos = (x, y)
                    start_pos = (x, y)
                    update_bounds(x, y)
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
                    bezier_points = cubic_bezier(prev, p1, p2, p3, num_points=10)
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
                    bezier_points = cubic_bezier(prev, p1, p2, p3, num_points=10)
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

# function to create TK window that scales the SVG and draws the path data
def display_svg_path(paths, min_max):
    import tkinter as tk
    from tkinter import filedialog
    import serial.tools.list_ports
    
    WIDTH = 800
    HEIGHT = 600

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
    for path in paths:
        scaled_path = []
        for point in path:
            x = point[0] * scale + x_offset
            y = point[1] * scale + y_offset
            scaled_path.append((x, y))
        scaled_paths.append(scaled_path)

    # revert y axis
    for path in scaled_paths:
        for i in range(len(path)):
            path[i] = (path[i][0], height - path[i][1]) 

# Create a Tkinter window
    root = tk.Tk()
    root.title("SVG Path Display")
    
    # Create canvas
    canvas = tk.Canvas(root, width=width, height=height, bg='white')
    canvas.pack()

    # Draw the paths on the canvas
    for path in scaled_paths:
        for i in range(len(path) - 1):
            canvas.create_line(path[i][0], path[i][1], path[i + 1][0], path[i + 1][1], fill='black', width=2)

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
            ser = serial.Serial(com_port, 115200, timeout=1)
            print(f"Opened COM port: {com_port}")
            for path in paths:
                for point in path:
                    command = f"G1 X{point[0]:.2f} Y{point[1]:.2f}\n"
                    ser.write(command.encode('utf-8'))
                    response = ser.readline().decode('utf-8').strip()
                    print(f"Sent: {command.strip()}, Received: {response}")
            ser.close()
            print("Finished uploading SVG path data.")
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
    svg_file = '../SVG/Aircision_logo.svg'  # Replace with your SVG file path
    path_data, min_max = svg_to_svg_path(svg_file)
    print("SVG Path Data:")
    # print the path data each element on a new line
    for path in path_data:
        print(path)
    display_svg_path(path_data, min_max)