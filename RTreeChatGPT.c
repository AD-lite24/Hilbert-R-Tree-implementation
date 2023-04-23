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
// Split a node into two new nodes using the quadratic split algorithm
void split_node(HRTreeNode *node, HRTreeNode **new_node1, HRTreeNode **new_node2) {
    int i, j, seed1, seed2;
    choose_seeds(node, &seed1, &seed2);
    *new_node1 = malloc(sizeof(HRTreeNode));
    *new_node2 = malloc(sizeof(HRTreeNode));
    (*new_node1)->is_leaf = node->is_leaf;
    (*new_node2)->is_leaf = node->is_leaf;
    add_entry(*new_node1, node->entries[seed1]);
    add_entry(*new_node2, node->entries[seed2]);
    for (i = 0; i < node->num_entries; i++) {
        if (i != seed1 && i != seed2) {
            HRTreeNode *target_node = area((*new_node1)->rect) < area((*new_node2)->rect) ? *new_node1 : *new_node2;
            double enlargement1 = area(mbr(target_node->rect, node->entries[i]->point, DIMENSIONS)) - area(target_node->rect);
            double enlargement2 = area(mbr((*new_node1)->rect, (*new_node2)->rect)) - area((*new_node1)->rect) - area((*new_node2)->rect);
            j = enlargement1 < enlargement2 ? 0 : 1;
            add_entry(target_node->children[j], node->entries[i]);
            add_child(target_node, target_node->children[j]);
        }
    }
}

// Insert an entry into the Hilbert R-tree
void insert_entry(HRTreeNode **root, void *entry) {
    if (*root == NULL) {
        *root = malloc(sizeof(HRTreeNode));
        (*root)->is_leaf = 1;
        add_entry(*root, entry);
        return;
    }
    if ((*root)->is_leaf && (*root)->num_entries == MAX_ENTRIES) {
        HRTreeNode *new_node1, *new_node2;
        split_node(*root, &new_node1, &new_node2);
        *root = malloc(sizeof(HRTreeNode));
        (*root)->is_leaf = 0;
        add_child(*root, new_node1);
        add_child(*root, new_node2);
    }
    if (!(*root)->is_leaf) {
        int i, best_child_index = -1;
        double best_enlargement = -1;
        for (i = 0; i < (*root)->num_entries; i++) {
            double enlargement = area(mbr((*root)->children[i]->rect, ((Point *)entry)->point, DIMENSIONS)) - area((*root)->children[i]->rect);
            if (enlargement < best_enlargement || best_child_index == -1) {
                best_child_index = i;
                best_enlargement = enlargement;
            } else if (enlargement == best_enlargement && area((*root)->children[i]->rect) < area((*root)->children[best_child_index]->rect)) {
                best_child_index = i;
            }
        }
        insert_entry(&((*root)->children[best_child_index]), entry);
        (*root)->rect = mbr((*root)->rect, (*root)->children[best_child_index]->rect);
    }
    if ((*root)->is_leaf && (*root)->num_entries < MAX_ENTRIES) {
        add_entry(*root, entry);
    }
}

// Search for all entries in the tree that overlap the given rectangle
void search(HRTreeNode *node, Rect rect, void (*callback)(void *)) {
    int i;
    if (intersects(node->rect, rect)) {
        if (node->is_leaf) {
            for (i = 0; i < node->num_entries; i++) {
                if (intersects(node->entries[i]->rect, rect)) {
                    callback(node->entries[i]->data);
                }
            }
        } else {
            for (i = 0; i < node->num_entries; i++) {
                if (intersects(node->children[i]->rect, rect)) {
                    search(node->children[i], rect, callback);
                }
            }
        }
    }
}
