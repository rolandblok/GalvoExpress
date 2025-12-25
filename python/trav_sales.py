# solve shortest path between paths
import math
import random

def calculate_distance(point1, point2):
    return math.sqrt((point1[0] - point2[0]) ** 2 + (point1[1] - point2[1]) ** 2)

def sort_paths_by_proximity(paths):
    sorted_paths = []
    if not paths:
        return sorted_paths

    remaining_paths = paths.copy()  # Work with a copy instead of modifying input
    current_path = remaining_paths.pop(0)
    sorted_paths.append(current_path)

    while remaining_paths:
        last_point = current_path[-1]
        closest_index = -1
        closest_distance = float('inf')
        for i, path in enumerate(remaining_paths):
            start_point = path[0]
            distance = calculate_distance(last_point, start_point)
            if distance < closest_distance:
                closest_distance = distance
                closest_index = i
        current_path = remaining_paths.pop(closest_index)
        sorted_paths.append(current_path)

    return sorted_paths

# Algorithm: The SalesManSolver class uses simulated annealing, which is a probabilistic optimization technique that iteratively improves a solution by:

# Starting with a random order of points
# Making random swaps between positions
# Accepting swaps that improve the solution or occasionally accepting worse swaps (based on temperature)
# Gradually lowering the temperature to converge to a solution
# Key components:

# PathSales class: Manages the current tour order and calculates distance deltas
# SalesManSolver class: Orchestrates the solving process with temperature control
# Temperature-based acceptance: Uses Math.exp(-delta / temp) to decide whether to accept worse swaps
# Starting conditions:

# Initial temperature is set to 100 * distance_between_first_two_points
# Temperature decreases each iteration by a coefficient (default: 1 - 5e-7)
# Convergence: The algorithm stops when either:

# Temperature drops below 1e-6
# Maximum steps limit is reached

class PathSales:
    def __init__(self, points):
        self.points = points
        self.order = list(range(len(points)))
        self.distance = self.calculate_total_distance()

    def calculate_total_distance(self):
        total_distance = 0
        for i in range(len(self.order)):
            point1 = self.points[self.order[i]]
            point2 = self.points[self.order[(i + 1) % len(self.order)]]
            total_distance += calculate_distance(point1, point2)
        return total_distance

    def swap(self, i, j):
        self.order[i], self.order[j] = self.order[j], self.order[i]

    def delta_distance(self, i, j):
        n = len(self.order)
        a, b = self.points[self.order[i]], self.points[self.order[j]]
        a_prev = self.points[self.order[i - 1]]
        b_next = self.points[self.order[(j + 1) % n]]

        before_swap = calculate_distance(a_prev, a) + calculate_distance(b, b_next)
        after_swap = calculate_distance(a_prev, b) + calculate_distance(a, b_next)

        return after_swap - before_swap
    
class SalesManSolver:
    def __init__(self, points, initial_temp, temp_decrease_coeff=1 - 5e-7, max_steps=1000000):
        self.points = points
        self.temp = initial_temp
        self.temp_decrease_coeff = temp_decrease_coeff
        self.max_steps = max_steps

    def solve(self):
        path_sales = PathSales(self.points)
        steps = 0

        while self.temp > 1e-6 and steps < self.max_steps:
            i, j = sorted(random.sample(range(len(self.points)), 2))
            delta = path_sales.delta_distance(i, j)

            if delta < 0 or random.random() < math.exp(-delta / self.temp):
                path_sales.swap(i, j)
                path_sales.distance += delta

            self.temp *= self.temp_decrease_coeff
            steps += 1

        return path_sales.order


def traveling_salesman(paths):

    # sort first by proximity to get a good initial guess
    paths_copy = paths.copy()  # Create a copy to avoid modifying the original
    paths_sorted = sort_paths_by_proximity(paths_copy)

    # we don't use,makes it worse
    # # Extract the starting point of each path for optimization
    # path_start_points = [path[0] for path in paths_sorted   ]
    # # Solve the traveling salesman problem for the starting points
    # optimized_indices = SalesManSolver(path_start_points, initial_temp=100 * calculate_distance(path_start_points[0], path_start_points[1])).solve()
    
    # # Return paths in the optimized order
    # return [paths_sorted[i] for i in optimized_indices]
    return paths_sorted
