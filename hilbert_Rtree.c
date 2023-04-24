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
    int cnt;    // No.of total Nodes
    int height; // height of the tree
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

// Leaf has C_l entries of the form (R, obj_id)
// Non-leaf has C_n entries of the form (R, ptr, LHV)
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

void splitNode(NODE n, NODE nn)
{
    // Split n into nodes n and nn
    // Assign the values to new node
    nn->isLeaf = n->isLeaf;
    nn->num_entries = n->num_entries / 2;
    nn->lhv = n->lhv;
    // Move entries to nn
    for (int i = nn->num_entries - 1; i >= 0; i--)
    {
        nn->rects[i] = n->rects[i + nn->num_entries];
        nn->children[i] = n->children[i + nn->num_entries];
        nn->elements[i] = n->elements[i + nn->num_entries];
    }
    // Update the number of entries
    n->num_entries = n->num_entries - nn->num_entries;
}

int calculateIncrease(rectangle R1, rectangle R2)
{
    // Calculate the minimum bounding rectangle of R1 and R2
    int x_min = MIN(R1.low.x, R2.low.x);
    int y_min = MIN(R1.low.y, R2.low.y);
    int x_max = MAX(R1.high.x, R2.high.x);
    int y_max = MAX(R1.high.y, R2.high.y);
    rectangle MBR = {{x_min, y_min}, {x_max, y_max}, 0};

    // Calculate the increase in area of R1 if R2 is added to it
    int area1 = (R1.high.x - R1.low.x) * (R1.high.y - R1.low.y);
    int area2 = (R2.high.x - R2.low.x) * (R2.high.y - R2.low.y);
    int mbr_area = (MBR.high.x - MBR.low.x) * (MBR.high.y - MBR.low.y);
    return mbr_area - area1 - area2;
}

int calculateAreaDifference(rectangle R1, rectangle R2)
{
    // Calculate the difference in area between R1 and R2
    int area1 = (R1.high.x - R1.low.x) * (R1.high.y - R1.low.y);
    int area2 = (R2.high.x - R2.low.x) * (R2.high.y - R2.low.y);
    rectangle MBR = {{MIN(R1.low.x, R2.low.x), MIN(R1.low.y, R2.low.y)}, {MAX(R1.high.x, R2.high.x), MAX(R1.high.y, R2.high.y)}, 0};
    int mbr_area = (MBR.high.x - MBR.low.x) * (MBR.high.y - MBR.low.y);
    return mbr_area - area1 - area2;
}

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

// Returns true when rect2 is contained in rect
bool check_container_rectangle(RECTANGLE rect, RECTANGLE rect2)
{
    if (rect2->low.x < rect->low.x || rect2->high.x > rect->high.x)
        return false;
    if (rect2->low.y < rect->low.y || rect2->high.y > rect->high.y)
        return false;

    return true;
}

// Returns true when rect2 intersects rect
bool check_intersection_rectangle(RECTANGLE rect, RECTANGLE rect2)
{
    if (rect2->low.x > rect->low.x || rect2->high.x < rect->high.x)
        return false;
    if (rect2->low.y > rect->low.y || rect2->high.y < rect->high.y)
        return false;

    return true;
}

// Returns true if rectangles are equal
bool check_equal_rectangle(RECTANGLE rect, RECTANGLE rect2)
{
    if (rect->high.x != rect2->high.x || rect->high.y != rect2->high.y)
        return false;
    if (rect->low.x != rect2->low.x || rect->low.y != rect2->low.y)
        return false;
    return true;
}

// Returns area of the rectangle
int area_of_rectangle(RECTANGLE rect)
{
    int area;
    area = abs(rect->high.x - rect->low.x) * abs(rect->high.y - rect->low.y);
    return area;
}

// Expand rect to size of rect2
void expand_rectangle(RECTANGLE rect, RECTANGLE rect2)
{
    if (rect2->high.x > rect->high.x)
        rect->high.x = rect2->high.x;
    if (rect2->high.y > rect->high.y)
        rect->high.y = rect2->high.y;

    if (rect2->low.x > rect->low.x)
        rect->low.x = rect2->low.x;
    if (rect2->low.x > rect->low.x)
        rect->low.x = rect2->low.x;
}


void search(RECTANGLE rect1, NODE root){
    NODE temp = root;
    if(temp!=NULL&&temp->isLeaf==false){
        for(int i=0; i<M; i++){
            if(&temp->children[i]!=NULL&&check_intersection_rectangle(&(temp->rects[i]), rect1)){
                search(rect1, temp->children[i]);
            }
        }
    }
    else if(temp->isLeaf==true){
        for(int i=0; i<M; i++){
            if(&temp->rects[i]!=NULL && check_intersection_rectangle(&temp->rects[i], rect1)){
                    printf("Rectangle found having the coordinates (%d, %d), (%d, %d)\n", temp->rects[i].high.x, temp->rects[i].high.y, temp->rects[i].low.x, temp->rects[i].low.y);
            }
        }
    }
    else{
        print("No rectangle found");
        return;
    }
    
}


RECTANGLE createNewRectangle(int leftTop, int leftBottom, int rightTop, int rightBottom);
int findArea(NODE temp);
NODE insertNode(NODE temp);
void deleteNode(DATA item);
bool isOverlap(RECTANGLE rect1, RECTANGLE rect2);
int AreaOverlap(RECTANGLE rect1, RECTANGLE rect2); // If one rect is completely in another
void enlargement(NODE parent, NODE child);

int main()
{
    
}