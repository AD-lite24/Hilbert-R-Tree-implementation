#include <stdio.h>
#include <stdbool.h>
#define M 4
#define m 2
#define DIM 2

typedef struct rtree rtree;
typedef struct rtree *RTREE;
typedef struct node node;
typedef struct node *NODE;
typedef struct data data;
typedef struct data *DATA;
typedef struct rectangle rectangle;
typedef struct rectangle *RECTANGLE;
typedef struct element element;

struct rtree 
{
    int cnt; // No.of total Nodes
    int height; //height of the tree
    NODE root;
};

struct element
{
    int x;
    int y;
};

struct rectangle
{
    element low, high;
    int hilbertValue;
};

//Leaf has C_l entries of the form (R, obj_id)
//Non-leaf has C_n entries of the form (R, ptr, LHV)
struct node
{
    int num_entries;
    rectangle rects[M];
    bool isLeaf;
    int lhv; // will be -1 if it is a leaf node
    union
    {
        NODE children[M];
        element elements[M];
    };
};

void splitNode(NODE* n, NODE* nn) {
    // Split n into nodes n and nn
    // Assign the values to new node
    nn->isLeaf = n->isLeaf;
    nn->num_entries = n->num_entries / 2;
    nn->lhv = n->lhv;
    // Move entries to nn
    for (int i = nn->num_entries - 1; i >= 0; i--) {
        nn->rects[i] = n->rects[i + nn->num_entries];
        nn->children[i] = n->children[i + nn->num_entries];
        nn->elements[i] = n->elements[i + nn->num_entries];
    }
    // Update the number of entries
    n->num_entries = n->num_entries - nn->num_entries;
}

void adjustTree(NODE* n, NODE* nn, rtree* tree) {
    if (n == &tree->root) {
        // Create a new root if it is required
        NODE* new_root = malloc(sizeof(NODE));
        new_root->isLeaf = false;
        new_root->num_entries = 1;
        new_root->lhv = -1;
        new_root->children[0] = n;
        new_root->children[1] = nn;
        // Update the root node of the tree
        tree->root = *new_root;
        tree->height++;
    } else {
        // Add nn to the parent of n
        NODE* p = n->parent;
        int i = 0;
        while (p->children[i] != n) i++;
        for (int j = p->num_entries - 1; j >= i + 1; j--) {
            p->children[j + 1] = p->children[j];
            p->rects[j + 1] = p->rects[j];
            p->elements[j + 1] = p->elements[j];
        }
        p->children[i + 1] = nn;
        p->rects[i + 1] = nn->rects[0];
        p->elements[i + 1] = nn->elements[0];
        p->num_entries++;
        // If the parent of n becomes overflow, split it recursively
        if (p->num_entries == C_n) {
            NODE* new_node = malloc(sizeof(NODE));
            new_node->parent = p->parent;
            splitNode(p, new_node);
            adjustTree(p, new_node, tree);
        }
    }
}



void insertRect(rectangle R, NODE* n, rtree* tree) {
    if (n->isLeaf) {
        // If n is a leaf node, insert R into it
        if (n->num_entries < C_l) {
            int i = n->num_entries - 1;
            while (i >= 0 && n->rects[i].hilbertValue > R.hilbertValue) {
                n->rects[i + 1] = n->rects[i];
                n->elements[i + 1] = n->elements[i];
                i--;
            }
            n->rects[i + 1] = R;
            n->elements[i + 1] = R.low;
            n->num_entries++;
            // Update the LHV of the node and its ancestors
            NODE* p = n;
            while (p != NULL) {
                int max_lhv = -1;
                for (int i = 0; i < p->num_entries; i++) {
                    if (p->children[i]->lhv > max_lhv) {
                        max_lhv = p->children[i]->lhv;
                    }
                }
                p->lhv = max_lhv;
                p = p->parent;
            }
        } else {
            // If n is a full leaf node, split it and adjust the tree
            NODE* nn = malloc(sizeof(NODE));
            nn->parent = n->parent;
            nn->isLeaf = true;
            splitNode(n, nn);
            adjustTree(n, nn, tree);
            // Insert R into the appropriate leaf node
            if (R.hilbertValue <= nn->rects[0].hilbertValue) {
                insertRect(R, n, tree);
            } else {
                insertRect(R, nn, tree);
            }
        }
    } else {
        // If n is not a leaf node, insert R into the appropriate child node
        int i = 0;
        int min_increase = INT_MAX;
        NODE* c = NULL;
        while (i < n->num_entries) {
            rectangle Rn = n->rects[i];
            int increase = calculateIncrease(Rn, R);
            if (increase < min_increase) {
                min_increase = increase;
                c = n->children[i];
            } else if (increase == min_increase) {
                int diff1 = calculateAreaDifference(c->rects[0], R);
                int diff2 = calculateAreaDifference(n->children[i]->rects[0], R);
                if (diff1 < diff2) {
                    c = n->children[i];
                }
            }
            i++;
        }
        insertRect(R, c, tree);
    }
}

int calculateIncrease(rectangle R1, rectangle R2) {
    // Calculate the minimum bounding rectangle of R1 and R2
    int x_min = MIN(R1.low.x, R2.low.x);
    int y_min = MIN(R1.low.y, R2.low.y);
    int x_max = MAX(R1.high.x, R2.high.x);
    int y_max = MAX(R1.high.y, R2.high.y);
    rectangle MBR = { {x_min, y_min}, {x_max, y_max}, 0 };

    // Calculate the increase in area of R1 if R2 is added to it
    int area1 = (R1.high.x - R1.low.x) * (R1.high.y - R1.low.y);
    int area2 = (R2.high.x - R2.low.x) * (R2.high.y - R2.low.y);
    int mbr_area = (MBR.high.x - MBR.low.x) * (MBR.high.y - MBR.low.y);
    return mbr_area - area1 - area2;
}

int calculateAreaDifference(rectangle R1, rectangle R2) {
    // Calculate the difference in area between R1 and R2
    int area1 = (R1.high.x - R1.low.x) * (R1.high.y - R1.low.y);
    int area2 = (R2.high.x - R2.low.x) * (R2.high.y - R2.low.y);
    rectangle MBR = { {MIN(R1.low.x, R2.low.x), MIN(R1.low.y, R2.low.y)}, {MAX(R1.high.x, R2.high.x), MAX(R1.high.y, R2.high.y)}, 0 };
    int mbr_area = (MBR.high.x - MBR.low.x) * (MBR.high.y - MBR.low.y);
    return mbr_area - area1 - area2;
}


// int HRtree_size(struct HRtree *tree) {
//     return tree->size;
// }

const char *HRtree_string(struct HRtree *tree) {
    return "(HRtree)";
}

// struct node *newNode(int min, int max) {
//     struct node *n = (struct node*)malloc(sizeof(struct node));
//     n->min = min;
//     n->max = max;
//     n->parent = NULL;
//     n->left = NULL;
//     n->right = NULL;
//     n->leaf = false;
//     n->entries = newList(max);
//     mpz_init(n->lhv);
//     n->bb = NULL;
//     return n;
// }

// const char *node_string(struct node *n) {
//     char *str = (char*)malloc(sizeof(char)*100);
//     snprintf(str, 100, "node{leaf: %d, entries: %p, lhv: %Zd}", n->leaf, n->entries, n->lhv);
//     return str;
// }

// struct entry *node_get_entries(struct node *n, int *n_entries) {
//     struct entry *entries = NULL;
//     entries = entryList_get_entries(n->entries, n_entries);
//     return entries;
// }

struct data
{
    void * item;
};

RTREE createNewRTree()
{
    RTREE newRTree = malloc(sizeof(rtree));
    newRTree->cnt = 0;
    newRTree->height = 0;
    newRTree->root = NULL;
}

NODE createNewNode(bool isLeaf)
{
    NODE newNode = (NODE)malloc(sizeof(node));
    memset(newNode, 0, sizeof(node));

    if (isLeaf)
    {
        newNode->isLeaf = true;
        newNode->lhv = -1;
    }
    else
        newNode->isLeaf = false;

    return newNode; 
}

//Returns true when rect2 is contained in rect
bool rect_contains(RECTANGLE rect, RECTANGLE rect2)
{
    if (rect2->low.x < rect->low.x || rect2->high.x > rect->high.x)
        return false;
    if (rect2->low.y < rect->low.y || rect2->high.y > rect->high.y)
        return false;

    return true;
}

//Returns true when rect2 intersects rect
bool rect_intersects(RECTANGLE rect, RECTANGLE rect2)
{
    if (rect2->low.x > rect->low.x || rect2->high.x < rect->high.x)
        return false;
    if (rect2->low.y > rect->low.y || rect2->high.y < rect->high.y)
        return false;

    return true;
}

RECTANGLE createNewRectangle (int leftTop, int leftBottom, int rightTop, int rightBottom);
int findArea(NODE temp);
NODE insertNode(NODE temp);
void deleteNode(DATA item);
bool isOverlap(RECTANGLE rect1, RECTANGLE rect2);
int AreaOverlap(RECTANGLE rect1, RECTANGLE rect2); // If one rect is completely in another
void enlargement(NODE parent, NODE child);
