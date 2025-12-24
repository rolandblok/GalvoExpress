

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
                    print("C command not implemented yet")
                    current_pos = (float(commands[i + 5]), float(commands[i + 6]))
                    update_bounds(current_pos[0], current_pos[1])
                    i += 7  # Skip for simplicity
                elif cmd == 'c': # cubic Bezier curve (not implemented)
                    print("c command not implemented yet")
                    current_pos = (current_pos[0] + float(commands[i + 5]), current_pos[1] + float(commands[i + 6]))
                    update_bounds(current_pos[0], current_pos[1])
                    i += 7  # Skip for simplicity
                
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
    canvas = tk.Canvas(root, width=width, height=height, bg='white')
    canvas.pack()

    # Draw the paths on the canvas
    for path in scaled_paths:
        for i in range(len(path) - 1):
            canvas.create_line(path[i][0], path[i][1], path[i + 1][0], path[i + 1][1], fill='black', width=2)

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