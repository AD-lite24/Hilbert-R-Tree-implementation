#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "rtree.h"

#define M 4
#define m 2
#define DIM 2

struct rtree 
{
    int cnt; // No.of total Nodes
    int height; // 
    struct node* root;
    // void *(*malloc)(size_t);
    // void (*free)(void *);
    
};

struct rectangle
{
    double start[DIM];
    double end[DIM];
};
struct node
{

    int num_entries;
    struct node* parent;
    struct rectangle rects[M];
    bool isLeaf;
    union {
        struct node *children[M];
        void * items[M];
    };
};
// struct data
// {
//     void * item;
// };
typedef struct rtree rtree;
typedef struct rtree* RTREE;
typedef struct node node;
typedef struct node* NODE;
typedef struct data data;
typedef struct data* DATA;
typedef struct rectangle rectangle;
typedef struct rectangle* RECTANGLE;

RTREE createNewRTree()
{
    RTREE newRTree = RTREE(malloc(sizeof(rtree)));
    newRTree->cnt = 0;
    newRTree->height = 0;
    newRTree->root = NULL;
}
NODE createNewNode(bool isLeaf, )
{
    NODE newNode = NODE(malloc(sizeof(node)));
    newNode->
}
RECTANGLE createNewRectangle (int leftTop, int leftBottom, int rightTop, int rightBottom);
int findArea(NODE temp);
NODE insertNode(NODE temp);
void deleteNode(DATA item);
bool isOverlap(RECTANGLE rect1, RECTANGLE rect2);
int AreaOverlap(RECTANGLE rect1, RECTANGLE rect2); // If one rect is completely in another
void enlargement(NODE parent, NODE child);