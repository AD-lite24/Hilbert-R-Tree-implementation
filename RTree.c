#include <stdio.h>
int M,m;
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
    int topLeft, topRight, bottomLeft, bottomRight;
};
struct node
{
    int num_entries;
    void* entries[M];
    struct node* children[M];
    struct node* parent;
    struct rectangle coords;
    bool isLeaf;
};
struct data
{
    void * item;
};
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
