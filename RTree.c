#include <stdio.h>
<<<<<<< HEAD
#include <stdbool.h>
#define M 4
#define m 2
#define DIM 2
=======
#include <stdlib.h>
#include <stdbool.h>
#include<limits.h>
#include <string.h>

#define M 4
#define m 2
#define DIM 2
#define C_l 4
#define C_n 4

typedef struct rtree rtree;
typedef struct rtree *RTREE;
typedef struct node node;
typedef struct node *NODE;
typedef struct data data;
typedef struct data *DATA;
typedef struct rectangle rectangle;
typedef struct rectangle *RECTANGLE;
typedef struct element element;
>>>>>>> 06463a27e279544f76cb21e8a7b8b26ee21c5ff3

struct rtree 
{
    int cnt; // No.of total Nodes
<<<<<<< HEAD
    int height; // 
    struct node* root;
};

struct rectangle
{
    int low[DIM], high[DIM];
};
// node represents a tree node of the tree.
// C code based on explanation of ChatGPT
// struct HRtree {
//     int min, max, bits;
//     struct node *root;
//     struct h *hf;
//     int size;
// };
// struct HRtree *new_tree(int min, int max, int bits) {
//     struct HRtree *rt = malloc(sizeof(struct HRtree));
//     if (rt == NULL) {
//         perror("malloc error");
//         exit(EXIT_FAILURE);
//     }

//     rt->hf = NULL;
//     rt->root = NULL;

//     if (bits < DEFAULT_RESOLUTION) {
//         bits = DEFAULT_RESOLUTION;
//     }

//     if (min < 0) {
//         min = DEFAULT_MIN_NODE_ENTRIES;
//     }

//     if (max < 0) {
//         max = DEFAULT_MAX_NODE_ENTRIES;
//     }

//     if (max < min) {
//         fprintf(stderr, "Minimum number of nodes should be less than Maximum number of nodes and not vice versa.\n");
//         exit(EXIT_FAILURE);
//     }

//     struct h *hf = NULL;
//     int err = h_new(&hf, (uint32_t) bits, DIM);

//     if (err != 0) {
//         fprintf(stderr, "Error creating Hilbert curve instance\n");
//         exit(EXIT_FAILURE);
//     }

//     struct node *root = new_node(min, max);
//     root->leaf = 1;

//     rt->min = min;
//     rt->max = max;
//     rt->bits = bits;
//     rt->hf = hf;
//     rt->root = root;

//     return rt;
// }
// type node struct
// {
//     min, max int 
//     parent *node 
//     left, right *node 
//     leaf bool 
//     entries *entryList 
//      lhv *big.Int 
//      bb *rectangle // bounding-box of all children of this entry
// } 
// type entry struct {
    // 	bb   *rectangle // bounding-box of of this entry
    // 	node *node
    // 	obj  Rectangle
    // 	h    *big.Int // hilbert value
    // 	leaf bool
// }
=======
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

int min(int a, int b) {
    return (a < b) ? a : b;
}

int max(int a, int b) {
    return (a > b) ? a : b;
}


//Leaf has C_l entries of the form (R, obj_id)
//Non-leaf has C_n entries of the form (R, ptr, LHV)
>>>>>>> 06463a27e279544f76cb21e8a7b8b26ee21c5ff3
struct node
{
    int num_entries;
    rectangle rects[M];
    bool isLeaf;
<<<<<<< HEAD
    int hilbertValue;
    struct node* left, right;
    int lhv; // Something related to Hilbert value
};
int HRtree_size(struct HRtree *tree) {
    return tree->size;
}

const char *HRtree_string(struct HRtree *tree) {
    return "(HRtree)";
}

struct node *newNode(int min, int max) {
    struct node *n = (struct node*)malloc(sizeof(struct node));
    n->min = min;
    n->max = max;
    n->parent = NULL;
    n->left = NULL;
    n->right = NULL;
    n->leaf = false;
    n->entries = newList(max);
    mpz_init(n->lhv);
    n->bb = NULL;
    return n;
}

const char *node_string(struct node *n) {
    char *str = (char*)malloc(sizeof(char)*100);
    snprintf(str, 100, "node{leaf: %d, entries: %p, lhv: %Zd}", n->leaf, n->entries, n->lhv);
    return str;
}

struct entry *node_get_entries(struct node *n, int *n_entries) {
    struct entry *entries = NULL;
    entries = entryList_get_entries(n->entries, n_entries);
    return entries;
}
// struct node
// {
//     enum kind kind; // LEAF or BRANCH
//     int count;      // number of rects
//     struct rect rects[MAX_ENTRIES];
//     union
//     {
//         struct node *children[MAX_ENTRIES];
//         struct item items[MAX_ENTRIES];
//     };
// };
=======
    int lhv; // will be -1 if it is a leaf node
    NODE parent;
    union
    {
        NODE children[M];
        element elements[M];
    };
};

void splitNode(NODE n, NODE nn) {
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

void adjustTree(NODE n, NODE nn, rtree* tree) {
    if (n == &tree->root) {
        // Create a new root if it is required
        NODE new_root = malloc(sizeof(node));
        new_root->isLeaf = false;
        new_root->num_entries = 1;
        new_root->lhv = -1;
        new_root->children[0] = n;
        new_root->children[1] = nn;
        // Update the root node of the tree
        tree->root = new_root;
        tree->height++;
    } else {
        // Add nn to the parent of n
        NODE p = n->parent;
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
            NODE new_node = malloc(sizeof(node));
            new_node->parent = p->parent;
            splitNode(p, new_node);
            adjustTree(p, new_node, tree);
        }
    }
}

NODE chooseLeaf(rectangle R, int h, NODE n) {
    while (!n->isLeaf) {
        int i = 0;
        int min_lhv = INT_MAX;
        NODE c = NULL;
        while (i < n->num_entries) {
            rectangle Rn = n->rects[i];
            if (Rn.hilbertValue >= h && Rn.hilbertValue < min_lhv) {
                min_lhv = Rn.hilbertValue;
                c = n->children[i];
            }
            i++;
        }
        n = c;
    }
    return n;
}

void insertRect(rectangle R, NODE n, rtree* tree) {
    int h = R.hilbertValue;
    NODE leaf = chooseLeaf(R, h, n);
    if (leaf->num_entries < C_l) {
        // If leaf is not full, insert R into it
        int i = leaf->num_entries - 1;
        while (i >= 0 && leaf->rects[i].hilbertValue > R.hilbertValue) {
            leaf->rects[i + 1] = leaf->rects[i];
            leaf->elements[i + 1] = leaf->elements[i];
            i--;
        }
        leaf->rects[i + 1] = R;
        leaf->elements[i + 1] = R.low;
        leaf->num_entries++;
        // Update the LHV of the node and its ancestors
        NODE p = leaf;
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
        // If leaf is full, split it and adjust the tree
        NODE new_leaf = malloc(sizeof(node));
        new_leaf->parent = leaf->parent;
        new_leaf->isLeaf = true;
        splitNode(leaf, new_leaf);
        adjustTree(leaf, new_leaf, tree);
        // Insert R into the appropriate leaf node
        if (h <= new_leaf->rects[0].hilbertValue) {
            insertRect(R, leaf, tree);
        } else {
            insertRect(R, new_leaf, tree);
        }
    }
}


int calculateIncrease(rectangle R1, rectangle R2) {
    // Calculate the minimum bounding rectangle of R1 and R2
    int x_min = min(R1.low.x, R2.low.x);
    int y_min = min(R1.low.y, R2.low.y);
    int x_max = max(R1.high.x, R2.high.x);
    int y_max = max(R1.high.y, R2.high.y);
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
    rectangle MBR = { {min(R1.low.x, R2.low.x), min(R1.low.y, R2.low.y)}, {max(R1.high.x, R2.high.x), max(R1.high.y, R2.high.y)}, 0 };
    int mbr_area = (MBR.high.x - MBR.low.x) * (MBR.high.y - MBR.low.y);
    return mbr_area - area1 - area2;
}

int main(int argc, char const *argv[])
{
    rtree tree;
    tree.cnt = 0;
    tree.height = 1;
    tree.root = malloc(sizeof(node));
    tree.root->isLeaf = true;
    tree.root->num_entries = 0;
    tree.root->lhv = -1;
    tree.root->parent = NULL;

    // Insert some rectangles
    rectangle r1 = {{2, 2}, {4, 4}, 0};
    insertRect(r1, tree.root, &tree);

    // rectangle r2 = {{5, 6}, {7, 8}, 1};
    // insertRect(r2, tree.root, &tree);

    // rectangle r3 = {{1, 3}, {2, 4}, 2};
    // insertRect(r3, tree.root, &tree);

    // rectangle r4 = {{7, 1}, {8, 2}, 3};
    // insertRect(r4, tree.root, &tree);

    // Print the resulting tree
    printf("Number of nodes: %d\n", tree.cnt);
    printf("Tree height: %d\n", tree.height);
    printf("Root node: %p\n", tree.root);
    printf("Root is leaf node: %d\n", tree.root->isLeaf);
    printf("Root LHV: %d\n", tree.root->lhv);
    printf("Number of entries in root: %d\n", tree.root->num_entries);
    for (int i = 0; i < tree.root->num_entries; i++) {
        printf("  Entry %d:\n", i);
        printf("    Rect: ((%d, %d), (%d, %d))\n", tree.root->rects[i].low.x, tree.root->rects[i].low.y, tree.root->rects[i].high.x, tree.root->rects[i].high.y);
        printf("    LHV: %d\n", tree.root->rects[i].hilbertValue);
        if (tree.root->isLeaf) {
            printf("    Element: (%d, %d)\n", tree.root->elements[i].x, tree.root->elements[i].y);
        } else {
            printf("    Child node: %p\n", tree.root->children[i]);
        }
    }
    return 0;
}



// int HRtree_size(struct HRtree *tree) {
//     return tree->size;
// }

// const char *HRtree_string(struct HRtree *tree) {
//     return "(HRtree)";
// }

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
>>>>>>> 06463a27e279544f76cb21e8a7b8b26ee21c5ff3

struct data
{
    void * item;
};
<<<<<<< HEAD

typedef struct rtree rtree;
typedef struct rtree* RTREE;
typedef struct node node;
typedef struct node* NODE;
typedef struct data data;
typedef struct data* DATA;
typedef struct rectangle rectangle;
typedef struct rectangle* RECTANGLE;
=======
>>>>>>> 06463a27e279544f76cb21e8a7b8b26ee21c5ff3

RTREE createNewRTree()
{
    RTREE newRTree = malloc(sizeof(rtree));
    newRTree->cnt = 0;
    newRTree->height = 0;
    newRTree->root = NULL;
}

<<<<<<< HEAD
NODE createNewNode(bool isLeaf, )
=======
NODE createNewNode(bool isLeaf)
>>>>>>> 06463a27e279544f76cb21e8a7b8b26ee21c5ff3
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

<<<<<<< HEAD
=======
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

>>>>>>> 06463a27e279544f76cb21e8a7b8b26ee21c5ff3
RECTANGLE createNewRectangle (int leftTop, int leftBottom, int rightTop, int rightBottom);
int findArea(NODE temp);
NODE insertNode(NODE temp);
void deleteNode(DATA item);
bool isOverlap(RECTANGLE rect1, RECTANGLE rect2);
int AreaOverlap(RECTANGLE rect1, RECTANGLE rect2); // If one rect is completely in another
void enlargement(NODE parent, NODE child);
