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

RECTANGLE createNewRectangle (int leftTop, int leftBottom, int rightTop, int rightBottom);
int findArea(NODE temp);
NODE insertNode(NODE temp);
void deleteNode(DATA item);
bool isOverlap(RECTANGLE rect1, RECTANGLE rect2);
int AreaOverlap(RECTANGLE rect1, RECTANGLE rect2); // If one rect is completely in another
void enlargement(NODE parent, NODE child);
