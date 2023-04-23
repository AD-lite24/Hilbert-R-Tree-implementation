#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define DIMENSIONS 2  // Number of dimensions for each point
#define MAX_ENTRIES 4  // Maximum number of entries in a node

typedef struct {
    double point[DIMENSIONS];
} Point;

typedef struct {
    Point min_corner;
    Point max_corner;
} Rectangle;

typedef struct HRTreeNode {
    Rectangle rect;
    int num_entries;
    void *entries[MAX_ENTRIES];
    struct HRTreeNode *children[MAX_ENTRIES];
    int is_leaf;
} HRTreeNode;

// Create a new point with the specified coordinates
Point new_point(double x, double y) {
    Point point;
    point.point[0] = x;
    point.point[1] = y;
    return point;
}

// Create a new rectangle with the specified corners
Rectangle new_rectangle(Point min_corner, Point max_corner) {
    Rectangle rect;
    rect.min_corner = min_corner;
    rect.max_corner = max_corner;
    return rect;
}

// Calculate the minimum bounding rectangle that contains two rectangles
Rectangle mbr(Rectangle rect1, Rectangle rect2) {
    Point min_corner, max_corner;
    int i;
    for (i = 0; i < DIMENSIONS; i++) {
        min_corner.point[i] = fmin(rect1.min_corner.point[i], rect2.min_corner.point[i]);
        max_corner.point[i] = fmax(rect1.max_corner.point[i], rect2.max_corner.point[i]);
    }
    return new_rectangle(min_corner, max_corner);
}

// Calculate the area of a rectangle
double area(Rectangle rect) {
    double a = 1;
    int i;
    for (i = 0; i < DIMENSIONS; i++) {
        a *= rect.max_corner.point[i] - rect.min_corner.point[i];
    }
    return a;
}

// Check if two rectangles overlap
int overlap(Rectangle rect1, Rectangle rect2) {
    int i;
    for (i = 0; i < DIMENSIONS; i++) {
        if (rect1.max_corner.point[i] < rect2.min_corner.point[i] ||
            rect1.min_corner.point[i] > rect2.max_corner.point[i]) {
            return 0;
        }
    }
    return 1;
}

// Choose two entries to be the seeds for a new node
void choose_seeds(HRTreeNode *node, int *seed1, int *seed2) {
    int i, j;
    double max_distance = -1;
    for (i = 0; i < node->num_entries; i++) {
        for (j = i + 1; j < node->num_entries; j++) {
            double distance = 0;
            int k;
            for (k = 0; k < DIMENSIONS; k++) {
                distance += pow(node->entries[i]->point[k] - node->entries[j]->point[k], 2);
            }
            if (distance > max_distance) {
                max_distance = distance;
                *seed1 = i;
                *seed2 = j;
            }
        }
    }
}

// Add an entry to a node
void add_entry(HRTreeNode *node, void *entry) {
    node->entries[node->num_entries] = entry;
    node->rect = mbr(node->rect, ((Point *)entry)->point, DIMENSIONS);
    node->num_entries++;
}

// Add a child node to a node
void add_child(HRTreeNode *node, HRTreeNode *child) {
    node->children[node->num_entries] = child;
    node->rect = mbr(node->rect, child->rect);
    node->num_entries++;
}
