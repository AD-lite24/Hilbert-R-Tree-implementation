#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include<limits.h>
#include <string.h>
#include <stdint.h>


#define M 4
#define m 2
#define DIM 2
#define C_l 4
#define C_n 4
// Assumes coordinates are 32-bit signed integers
#define COORD_BITS 16                   // Number of bits used for each coordinate
#define HILBERT_BITS (COORD_BITS * 2) // Total number of bits in Hilbert curve



typedef struct rtree rtree;
typedef struct rtree *RTREE;
typedef struct node node;
typedef struct node *NODE;
typedef struct data data;
typedef struct data *DATA;
typedef struct rectangle rectangle;
typedef struct rectangle *RECTANGLE;
typedef struct element element;
typedef struct element* ELEMENT;

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
    int hilbertValue;       //Can be LHV or the hilbert value depending on the type
};

int min(int a, int b) {
    return (a < b) ? a : b;
}

int max(int a, int b) {
    return (a > b) ? a : b;
}


//Leaf has C_l entries of the form (R, obj_id)
//Non-leaf has C_n entries of the form (R, ptr, LHV)
struct node
{
    int num_entries;
    RECTANGLE rects[M] ;
    bool isLeaf;
    int lhv; // will be -1 if it is a leaf node
    NODE parent;
    union
    {
        NODE children[M] ;
        ELEMENT elements[M] ;
    };
};

// Convert a coordinate to a binary number
int coord_to_binary(int32_t coord)
{
    return (int)(coord - INT32_MIN);
}

// Interleave the bits of two binary numbers
int interleave_bits(int x, int y)
{
    int z = 0;
    for (int i = 0; i < COORD_BITS; i++)
    {
        z |= ((x & (1 << i)) << i) | ((y & (1 << i)) << (i + 1));
    }
    return z;
}

// Compute the Hilbert value of a binary number from a 16 order curve
int hilbert_value(int z)
{
    int h = 0;
    for (int i = HILBERT_BITS - 1; i >= 0; i--)
    {
        h ^= ((z >> i) & 1) << (i * 2 + 1);
        if ((i & 1) == 0)
        {
            int t = ((h >> 2) & 0x3) | ((h & 0x3) << 2);
            h = (h & ~0x3) | t;
        }
    }
    return h;
}


int hilbert_rect_center(RECTANGLE r)
{
    int32_t xmid = (r->low.x + r->high.x) / 2;
    int32_t ymid = (r->low.y + r->high.y) / 2;
    int x = coord_to_binary(xmid);
    int y = coord_to_binary(ymid);
    int z = interleave_bits(x, y);
    return hilbert_value(z);
}

RECTANGLE createNewRectangle(int lowx, int lowy, int highx, int highy)
{
    RECTANGLE newRec = (RECTANGLE) malloc (sizeof(rectangle));
    newRec->low.x = lowx;
    newRec->low.y = lowy;
    newRec->high.y = highy;
    newRec->high.x = highx;
    newRec->hilbertValue=hilbert_rect_center(newRec);
    return newRec;
}

NODE createNewNode(bool isLeaf)
{
    NODE newNode = (NODE)malloc(sizeof(node));
    memset(newNode, 0, sizeof(node));
    if (isLeaf)
    {
        newNode->isLeaf = true;
        newNode->lhv = -1;
        for(int i = 0 ; i < M ; i++)
        {
            newNode->elements[i] = NULL;
        }
    }
    else
    {
        for(int i = 0 ; i < M ; i++)
        {
            newNode->children[i] = NULL;
        }
        newNode->isLeaf = false;
    }
    for(int i = 0 ; i < M ; i++)
    {
        newNode->rects[i] = NULL;
    }
    return newNode; 
}

// NODE handleOverflow(NODE n, RECTANGLE r){
//     if(n->parent==NULL&&n->isLeaf==true){ //if n is a root
//         NODE new_node = createNewNode(n->children[0]->isLeaf);
//         NODE nn = createNewNode(n->isLeaf);
//         NODE new_parent = createNewNode(false);
//         NODE hilbert_sorted_array[M+1];
//         RECTANGLE hilbert_sorted_array_rects[M+1];
//         for(int i=0; i<M+1; i++){
//             if(i==M){
//                 hilbert_sorted_array[i] = new_node;
//                 hilbert_sorted_array_rects[i] = r;
//             }
//             else{
//                 hilbert_sorted_array[i] = n->children[i];
//                 hilbert_sorted_array_rects[i] = n->rects[i];
//             }
//         }
//         int i = M;
//         for(; i>0; i--){
//             if(hilbert_sorted_array_rects[i-1]->hilbertValue > r->hilbertValue){
//                 hilbert_sorted_array[i] = hilbert_sorted_array[i-1];
//                 hilbert_sorted_array_rects[i] = hilbert_sorted_array_rects[i-1];
//             }
//         }
//         hilbert_sorted_array[i] = new_node;
//         hilbert_sorted_array_rects[i] = r;
//         n->num_entries = M/2 + 1;
//         nn->num_entries = M/2;
//         for(int i=0; i<M/2+1; i++){
//             n->children[i] = hilbert_sorted_array[i];
//             n->rects[i] = hilbert_sorted_array_rects[i];
//         }
//         for(int i=M/2+1; i<M+1; i++){
//             nn->children[i-M/2-1] = hilbert_sorted_array[i];
//             nn->rects[i-M/2-1] = hilbert_sorted_array_rects[i];
//         }
//         n->lhv = n->rects[M/2]->hilbertValue;

//         nn->lhv = nn->rects[0]->hilbertValue;
//         n->parent = new_parent;
//         nn->parent = new_parent;
//         new_parent->num_entries = 2;
//         new_parent->children[0] = n;
//         new_parent->children[1] = nn;
//         new_parent->parent = NULL;
//         new_parent->lhv = n->lhv>nn->lhv ? n->lhv : nn->lhv;
//         // rects array of new_parent has to be initialised with mbr of n and nn
//         return new_parent;
//     }
// }

NODE handleOverlow(NODE N, RECTANGLE r)
{
    NODE parent = N->parent;
    RECTANGLE rects[17];
    int k = 0;

    for (int i = 0; i < M; i++)
    {
        NODE child = parent->children[i];
        
        if (child)
        {
            for (int j = 0; j < M; j++)
            {
                if(child->rects[j])
                    rects[k++] = child->rects[j];
            }
        }
    }

    rects[16] = r;  //Adding rect to the set of all sibling rects
    if (k==15)  //No space in any siblings, hence split
    {
        NODE new=createNewNode(false);
        int x=0;
        NODE temp=N->parent->children[x];
        while(temp!=NULL){
            x++;
            temp=N->parent->children[x];
        }
        temp=new;

    }

    else
    {
        
    }

}
/*
In a Hilbert R-tree, the Hilbert value is a measure of the spatial location of a point in the space. To distribute points evenly among all nodes according to their Hilbert value, you can follow the steps outlined below:

Calculate the Hilbert value for each point that you want to insert into the tree. You can use a Hilbert curve algorithm to do this.

Sort the points in ascending order of their Hilbert value.

Calculate the number of nodes in the tree. You can do this by dividing the total number of points by the maximum number of points that can be stored in a node.

Divide the sorted list of points into equal-sized groups, where each group contains (number of points / number of nodes) points.

Traverse the tree, and for each node, choose the group of points that has the Hilbert values closest to the Hilbert value of the node. You can use a binary search algorithm to find the closest group.

Insert the chosen group of points into the node.

By distributing points evenly among all nodes according to their Hilbert value, you can ensure that the tree is balanced and efficient for searching and querying spatial data.
*/

// void handle_overflow(struct hilbert_node *node) {
//     int num_entries = node->count;
//     int split_point = num_entries / 2;
//     int num_groups = (num_entries % 2 == 0) ? 2 : 3;

//     // Allocate memory for the new nodes
//     struct hilbert_node **new_nodes = (struct hilbert_node **) malloc(num_groups * sizeof(struct hilbert_node *));
//     for (int i = 0; i < num_groups; i++) {
//         new_nodes[i] = (struct hilbert_node *) malloc(sizeof(struct hilbert_node));
//         new_nodes[i]->is_leaf = node->is_leaf;
//         new_nodes[i]->count = 0;
//         new_nodes[i]->capacity = node->capacity;
//         new_nodes[i]->hilbert_values = (uint32_t *) malloc(node->capacity * sizeof(uint32_t));

//         if (node->is_leaf) {
//             new_nodes[i]->data = (void **) malloc(node->capacity * sizeof(void *));
//         } else {
//             new_nodes[i]->children = (struct hilbert_node **) malloc(node->capacity * sizeof(struct hilbert_node *));
//         }
//     }

//     // Copy entries into the new nodes
//     for (int i = 0; i < num_entries; i++) {
//         int group = (i < split_point) ? 0 : (i < 2*split_point) ? 1 : 2;
//         int index = (group == 2) ? i - 2*split_point : (group == 1) ? i - split_point : i;
//         new_nodes[group]->hilbert_values[index] = node->hilbert_values[i];

//         if (node->is_leaf) {
//             new_nodes[group]->data[index] = node->data[i];
//         } else {
//             new_nodes[group]->children[index] = node->children[i];
//         }

//         new_nodes[group]->count++;
//     }

//     // Update the parent node to point to the new nodes
//     if (node->parent == NULL) {
//         // Create a new root node if necessary
//         node->parent = (struct hilbert_node *) malloc(sizeof(struct hilbert_node));
//         node->parent->is_leaf = 0;
//         node->parent->count = 1;
//         node->parent->capacity = node->capacity;
//         node->parent->hilbert_values = (uint32_t *) malloc(node->capacity * sizeof(uint32_t));
//         node->parent->children = (struct hilbert_node **) malloc(2 * sizeof(struct hilbert_node *));
//         node->parent->children[0] = node;
//         node->parent->children[1] = new_nodes[0];
//         node->parent->hilbert_values[0] = new_nodes[0]->hilbert_values[0];
//         new_nodes[0]->parent = node->parent;

//     } else {
//         // Insert the new nodes into the parent node
       


RECTANGLE findMBR (NODE curr_node)
{
    RECTANGLE newRec = (RECTANGLE)malloc (sizeof(rectangle));
    if (curr_node->isLeaf == 1)
    {
        int xmin = INT_MAX, xmax = -1, ymin = INT_MAX, ymax = -1;
        for (int i = 0 ; i < curr_node->num_entries ; i++)
        {
            xmin = min(xmin, curr_node->elements[i]->x);
            ymin = min(ymin, curr_node->elements[i]->y);
            ymax = max(ymax, curr_node->elements[i]->y);
            xmax = max(xmax, curr_node->elements[i]->x);
        }
        newRec->low.x = xmin;
        newRec->high.x = xmax;
        newRec->low.y = ymin;
        newRec->high.y = ymax;
    }
    else
    {
        int xmin = INT_MAX, xmax = -1, ymin = INT_MAX, ymax = -1;
        for (int i = 0 ; i < curr_node->num_entries ; i++)
        {
            xmin = min(xmin, findMBR(curr_node->children[i])->low.x);
            ymin = min(ymin, findMBR(curr_node->children[i])->low.y);
            ymax = max(ymax, findMBR(curr_node->children[i])->high.y);
            xmax = max(xmax, findMBR(curr_node->children[i])->high.x);
        }
        newRec->low.x = xmin;
        newRec->high.x = xmax;
        newRec->low.y = ymin;
        newRec->high.y = ymax;
    }
    newRec->hilbertValue = hilbert_rect_center(newRec);

    return newRec;
}


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
        n->rects[i + nn->num_entries] = NULL;
        n->children[i + nn->num_entries] = NULL;
        n->elements[i + nn->num_entries] = NULL;
    }
    n->num_entries = n->num_entries - nn->num_entries;
    // Update the number of entries
    // n->num_entries = n->num_entries - nn->num_entries;
}

void adjustTree(NODE n, NODE nn, rtree* tree) {
    if (n == tree->root) {
        // Create a new root if it is required
        NODE new_root = createNewNode(false);
        // NODE new_root = malloc(sizeof(node));
        // new_root->isLeaf = false;
        new_root->num_entries = 2;
        new_root->lhv = -1;
        new_root->children[0] = n;
        new_root->children[1] = nn;
        // Update the root node of the tree
        n->parent=new_root;
        nn->parent=new_root;

        
        new_root->lhv = max(n->lhv, nn->lhv);
        tree->root = new_root;
        tree->height++;
    } else {
        // Add nn to the parent of n
        NODE p = n->parent;
        // if (p == NULL) {
        //     // n is the root node, no parent exists
        //     // create a new root node
        //     NODE new_root = malloc(sizeof(node));
        //     new_root->isLeaf = false;
        //     new_root->num_entries = 1;
        //     new_root->lhv = -1;
        //     new_root->children[0] = n;
        //     new_root->children[1] = nn;
        //     for (int i = 0 ; i < new_root->num_entries ; i++)
        //     new_root->rects[i]=findMBR(new_root->children[i]);
        //     new_root->lhv = max(n->lhv,nn->lhv);
        //     // Update the root node of the tree
        //     tree->root = new_root;
        //     tree->height++;
        //     return;
        // }
        // If the parent of n becomes overflow, split it recursively
        if (p->num_entries == C_n) {
            NODE new_node = createNewNode(false);
            // NODE new_node = malloc(sizeof(node));
            new_node->parent = p->parent;
            splitNode(p, new_node);
            adjustTree(p, new_node, tree);
        }
        // To see
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
        
    }
}


NODE chooseLeaf(rectangle R, int h, NODE n) {
    while (!n->isLeaf) {
        int i = 0;
        int min_lhv = INT_MAX;
        NODE c = NULL;
        while (i < n->num_entries) {
            rectangle Rn = *(n->rects[i]);
            if (Rn.hilbertValue >= h && Rn.hilbertValue < min_lhv) {
                min_lhv = Rn.hilbertValue;
                c = n->children[i];
            }
            i++;
        }

        if (c == NULL) break;
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
        while (i >= 0 && leaf->rects[i]->hilbertValue > R.hilbertValue) {
            leaf->rects[i + 1] = leaf->rects[i];
            leaf->elements[i + 1] = leaf->elements[i];
            i--;
        }
        leaf->rects[i + 1] = (RECTANGLE) malloc (sizeof(rectangle));
        leaf->elements[i + 1] = (ELEMENT) malloc (sizeof(element));
        *(leaf->rects[i + 1]) = R;
        *(leaf->elements[i + 1]) = R.low;
        leaf->num_entries++;
        // Update the LHV of the node and its ancestors
        NODE p = leaf;
        while (p != NULL) {
            int max_lhv = -1;
            for (int i = 0; i < p->num_entries; i++) {
                if (p!=leaf && p->children[i]->lhv > max_lhv) {
                    max_lhv = p->children[i]->lhv;
                }
                else if (p == leaf)
                {
                    for (int i = 0; i < p->num_entries; i++)
                    {
                        max_lhv = max(max_lhv, p->rects[i]->hilbertValue);

                    }
                    
                }
            }
            p->lhv = max_lhv;
            p = p->parent;
        }
    } else {
        // If leaf is full, split it and adjust the tree
        // NODE new_parent= (NODE)malloc(sizeof(node));
        // for(int i = 0 ; i < M ; i++)
        // {
        //     new_parent->elements[i] = NULL;
        // }
        NODE new_leaf = createNewNode(true);
        // NODE new_leaf = malloc(sizeof(node));
        // for(int i = 0 ; i < M ; i++)
        // {
        //     new_leaf->elements[i] = NULL;
        // }
        new_leaf->parent = leaf->parent;
        new_leaf->isLeaf = true;
        splitNode(leaf, new_leaf);
        adjustTree(leaf, new_leaf, tree);
        // Insert R into the appropriate leaf node
        if (h <= new_leaf->rects[0]->hilbertValue) {
            insertRect(R, leaf, tree);
        } else {
            insertRect(R, new_leaf, tree);
        }
        for (int i = 0 ; i < leaf->parent->num_entries ; i++)
        leaf->parent->rects[i]=findMBR(leaf->parent->children[i]);
        // adjustTree(leaf,new_leaf,tree);
        // NODE new_parent=handleOverflow(n,&R);
        // tree->root=new_parent;
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

void rect_expand(RECTANGLE rect, RECTANGLE rect2)
{

    if (rect2->low.x < rect->low.x)
        rect->low.x = rect2->low.x;
    if (rect2->low.y < rect->low.y)
        rect->low.y = rect2->low.y;
    if (rect2->high.x > rect->high.x)
        rect->high.x = rect2->high.x;
    if (rect2->high.y > rect->high.y)
        rect->high.y = rect2->high.y;
}

// void printHilbertTree(NODE root, int depth) {
//     if (root == NULL) {
//         return;
//     }
//     // Print the current node
//     for (int i = 0; i < depth; i++) {
//         printf("  ");
//     }
//     printf("Node with %d entries:\n", root->num_entries);
//     for (int i = 0; i < root->num_entries; i++) {
//         for (int j = 0; j < depth; j++) {
//             printf("  ");
//         }
//         printf("  Entry %d:\n", i);
//         for (int j = 0; j < depth; j++) {
//             printf("  ");
//         }
//         printf("    Rect: (%d, %d, %d, %d)\n", root->rects[i]->low.x, root->rects[i]->low.y, root->rects[i]->high.x, root->rects[i]->high.y);
//         for (int j = 0; j < depth; j++) {
//             printf("  ");
//         }
//         printf("    Element: %d %d\n", root->elements[i]->x,root->elements[i]->y);
//     }

//     // Recursively print the children nodes
//     for (int i = 0; i < root->num_entries + 1; i++) {
//         printHilbertTree(root->children[i], depth + 1);
//     }
// }

void printHilbertTree(NODE root, int depth) {
    if (root == NULL) {
        return;
    }

    // Print the current node
    for (int i = 0; i < depth; i++) {
        printf("  ");
    }
    printf("Node with %d entries:\n", root->num_entries);
    for (int i = 0; i < root->num_entries; i++) {
        for (int j = 0; j < depth; j++) {
            printf("  ");
        }
        printf("  Entry %d:\n", i);
        for (int j = 0; j < depth; j++) {
            printf("  ");
        }
        if (root->rects != NULL && root->rects[i] != NULL) {
            printf("    Rect: (%d, %d, %d, %d)\n", root->rects[i]->low.x, root->rects[i]->low.y, root->rects[i]->high.x, root->rects[i]->high.y);
        } else {
            printf("    Rect: NULL\n");
        }
        for (int j = 0; j < depth; j++) {
            printf("  ");
        }
        if (root->elements != NULL && root->elements[i] != NULL) {
            printf("    Element: %d %d\n", root->elements[i]->x,root->elements[i]->y);
        } else {
            printf("    Element: NULL\n");
        }
    }

    // Recursively print the children nodes
    for (int i = 0; i < root->num_entries + 1; i++) {
        if (root->children != NULL && root->children[i] != NULL) {
            printHilbertTree(root->children[i], depth + 1);
        }
    }
}

// void printTree(RTREE r)
// {
//     printf("Number of nodes: %d\n", r.cnt);
//     printf("Tree height: %d\n", r.height);
//     printf("Root node: %p\n", r.root);
//     printf("Root is leaf node: %d\n", r.root->isLeaf);
//     printf("Root LHV: %d\n", r.root->lhv);
//     printf("Number of entries in root: %d\n", r.root->num_entries);
//     printf("Children's dimensions : \n")
//     for (int i = 0; i < r.root->num_entries; i++) {
//         printf("\tEntry %d:\n", i);
//         printf("\t\tRect: ((%d, %d), (%d, %d))\n", r.root->rects[i].low.x, r.root->rects[i].low.y, r.root->rects[i].high.x, r.root->rects[i].high.y);
//         printf("\t\tLHV: %d\n", r.root->rects[i].hilbertValue);
//         if (r.root->isLeaf) {
//             printf("\t\tElement: (%d, %d)\n", r.root->elements[i].x, r.root->elements[i].y);
//         } else {
//             printf("\t\tChild node: %p\n", r.root->children[i]);
//         }
//     }
//     if (r.root )

// }
void preOrderTraversal(NODE root)
{
    if (!root)
        return;

    if (root->isLeaf)
        printf("Printing external node\n");
    else
        printf("Printing internal node\n");

    for (int i = 0; i < M; i++)
    {
        RECTANGLE rect = root->rects[i];
        if (rect)
            printf("(%d,%d), (%d,%d)\n", rect->high.x, rect->high.y, rect->low.x, rect->low.y);
    }

    if (!root->isLeaf)
    {
        for (int i = 0; i < M; i++)
        {
            NODE child = root->children[i];
            if (child)
                preOrderTraversal(child);
        }
    }
}

// int HRr_size(struct HRtree *tree) {
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

// RECTANGLE createNewRectangle (int leftTop, int leftBottom, int rightTop, int rightBottom);
// int findArea(NODE temp);
// NODE insertNode(NODE temp);
// void deleteNode(DATA item);
// bool isOverlap(RECTANGLE rect1, RECTANGLE rect2);
// int AreaOverlap(RECTANGLE rect1, RECTANGLE rect2); // If one rect is completely in another
// void enlargement(NODE parent, NODE child);

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
    rectangle r1 = *(createNewRectangle(2, 4, 2, 4));
    insertRect(r1, tree.root, &tree);

    rectangle r2 = *(createNewRectangle(5, 6, 5, 6));
    insertRect(r2, tree.root, &tree);

    rectangle r3 = *(createNewRectangle(1, 3, 1, 3));
    insertRect(r3, tree.root, &tree);

    rectangle r4 = *(createNewRectangle(7, 1, 7, 1));
    insertRect(r4, tree.root, &tree);

    rectangle r5 = *(createNewRectangle(9, 6, 9, 6));
    insertRect(r5, tree.root, &tree);

    rectangle r6 = *(createNewRectangle(10, 15, 10, 15));
    insertRect(r6, tree.root, &tree);

    // preOrderTraversal(tree.root);

    // printHilbertTree(tree.root,0);

    printf("Number of nodes: %d\n", tree.cnt);
    printf("Tree height: %d\n", tree.height);
    printf("Root node: %p\n", tree.root);
    printf("Root is leaf node: %d\n", tree.root->isLeaf);
    printf("Root LHV: %d\n", tree.root->lhv);
    printf("Number of entries in root: %d\n", tree.root->num_entries);
    for (int i = 0; i < tree.root->num_entries; i++)
    {
        printf("  Entry %d:\n", i);
        printf("    Rect: ((%d, %d), (%d, %d))\n", tree.root->rects[i]->low.x, tree.root->rects[i]->low.y, tree.root->rects[i]->high.x, tree.root->rects[i]->high.y);
        printf("    LHV: %d\n", tree.root->rects[i]->hilbertValue);
        if (tree.root->isLeaf)
        {
            printf("    Element: (%d, %d)\n", tree.root->elements[i]->x, tree.root->elements[i]->y);
        }
        else
        {
            printf("    Child node: %p\n", tree.root->children[i]);
        }
    }
    printf("Child 1 node: %p\n", tree.root->children[0]);
    printf("Child 1 is leaf node: %d\n", tree.root->children[0]->isLeaf);
    printf("CHild 1 LHV: %d\n", tree.root->children[0]->lhv);
    printf("Number of entries in child 1: %d\n", tree.root->children[0]->num_entries);
    for (int i = 0; i < tree.root->children[0]->num_entries; i++)
    {
        printf("   Entry %d:\n", i);
        printf("     Rect: ((%d, %d), (%d, %d))\n", tree.root->children[0]->rects[i]->low.x, tree.root->children[0]->rects[i]->low.y, tree.root->children[0]->rects[i]->high.x, tree.root->children[0]->rects[i]->high.y);
        printf("     LHV: %d\n", tree.root->children[0]->rects[i]->hilbertValue);
        if (tree.root->children[0]->isLeaf)
        {
            printf("     Element: (%d, %d)\n", tree.root->children[0]->elements[i]->x, tree.root->children[0]->elements[i]->y);
        }
        else
        {
            printf("     Child node: %p\n", tree.root->children[0]->children[i]);
        }
    }
    printf("Child 2 node: %p\n", tree.root->children[1]);
    printf("Child 2 is leaf node: %d\n", tree.root->children[1]->isLeaf);
    printf("CHild 2 LHV: %d\n", tree.root->children[1]->lhv);
    printf("Number of entries in child 2: %d\n", tree.root->children[1]->num_entries);
    for (int i = 0; i < tree.root->children[1]->num_entries; i++)
    {
        printf("   Entry %d:\n", i);
        printf("     Rect: ((%d, %d), (%d, %d))\n", tree.root->children[1]->rects[i]->low.x, tree.root->children[1]->rects[i]->low.y, tree.root->children[1]->rects[i]->high.x, tree.root->children[1]->rects[i]->high.y);
        printf("     LHV: %d\n", tree.root->children[1]->rects[i]->hilbertValue);
        if (tree.root->children[1]->isLeaf)
        {
            printf("     Element: (%d, %d)\n", tree.root->children[1]->elements[i]->x, tree.root->children[1]->elements[i]->y);
        }
        else
        {
            printf("     Child node: %p\n", tree.root->children[1]->children[i]);
        }
    }
//     printf("Child 3 node: %p\n", tree.root->children[2]);
//     printf("Child 3 is leaf node: %d\n", tree.root->children[2]->isLeaf);
//     printf("CHild 3 LHV: %d\n", tree.root->children[2]->lhv);
//     printf("Number of entries in child 3: %d\n", tree.root->children[2]->num_entries);
//     for (int i = 0; i < tree.root->children[2]->num_entries; i++)
//     {
//         printf("   Entry %d:\n", i);
//         printf("     Rect: ((%d, %d), (%d, %d))\n", tree.root->children[2]->rects[i]->low.x, tree.root->children[2]->rects[i]->low.y, tree.root->children[2]->rects[i]->high.x, tree.root->children[2]->rects[i]->high.y);
//         printf("     LHV: %d\n", tree.root->children[2]->rects[i]->hilbertValue);
//         if (tree.root->children[2]->isLeaf)
//         {
//             printf("     Element: (%d, %d)\n", tree.root->children[2]->elements[i]->x, tree.root->children[2]->elements[i]->y);
//         }
//         else
//         {
//             printf("     Child node: %p\n", tree.root->children[2]->children[i]);
//         }
//     }

    // Print the resulting tree
    // printf("Number of nodes: %d\n", tree.cnt);
    // printf("Tree height: %d\n", tree.height);
    // printf("Root node: %p\n", tree.root);
    // printf("Root is leaf node: %d\n", tree.root->isLeaf);
    // printf("Root LHV: %d\n", tree.root->lhv);
    // printf("Number of entries in root: %d\n", tree.root->num_entries);
    // for (int i = 0; i < tree.root->num_entries; i++)
    // {
    //     printf("  Entry %d:\n", i);
    //     printf("    Rect: ((%d, %d), (%d, %d))\n", tree.root->rects[i]->low.x, tree.root->rects[i]->low.y, tree.root->rects[i]->high.x, tree.root->rects[i]->high.y);
    //     printf("    LHV: %d\n", tree.root->rects[i]->hilbertValue);
    //     if (tree.root->isLeaf)
    //     {
    //         printf("    Element: (%d, %d)\n", tree.root->elements[i]->x, tree.root->elements[i]->y);
    //     }
    //     else
    //     {
    //         printf("    Child node: %p\n", tree.root->children[i]);
    //     }
    // }

    return 0;
}


// rectangle r1 = {{2, 74}, {42, 4}, 0};
//     insertRect(r1, tree.root, &tree);
//     printf("%d %d %d %d %d\n", r1.low.x, r1.low.y, r1.high.x, r1.high.y, r1.hilbertValue);

//     rectangle r2 = {{5, 6}, {5, 6}, 5};
//     insertRect(r2, tree.root, &tree);

//     rectangle r3 = {{1, 3}, {1, 3}, 2};
//     insertRect(r3, tree.root, &tree);

//     rectangle r4 = {{7, 1}, {7, 1}, 3};
//     insertRect(r4, tree.root, &tree);

//     rectangle r5 = {{9, 2}, {9, 2}, 3};
//     insertRect(r5, tree.root, &tree);

    // preOrderTraversal(tree.root);

    // printHilbertTree(tree.root,0);

    // Print the resulting tree
    // printf("Number of nodes: %d\n", tree.cnt);
    // printf("Tree height: %d\n", tree.height);
    // printf("Root node: %p\n", tree.root);
    // printf("Root is leaf node: %d\n", tree.root->isLeaf);
    // printf("Root LHV: %d\n", tree.root->lhv);
    // printf("Number of entries in root: %d\n", tree.root->num_entries);
    // for (int i = 0; i < tree.root->num_entries; i++)
    // {
    //     printf("  Entry %d:\n", i);
    //     printf("    Rect: ((%d, %d), (%d, %d))\n", tree.root->rects[i]->low.x, tree.root->rects[i]->low.y, tree.root->rects[i]->high.x, tree.root->rects[i]->high.y);
    //     printf("    LHV: %d\n", tree.root->rects[i]->hilbertValue);
    //     if (tree.root->isLeaf)
    //     {
    //         printf("    Element: (%d, %d)\n", tree.root->elements[i]->x, tree.root->elements[i]->y);
    //     }
    //     else
    //     {
    //         printf("    Child node: %p\n", tree.root->children[i]);
    //     }
    // }
    // for (int i = 0; i < tree.root->children[0]->num_entries; i++)
    // {
    //     printf("  Entry %d:\n", i);
    //     printf("    Rect: ((%d, %d), (%d, %d))\n", tree.root->children[0]->rects[i]->low.x, tree.root->children[0]->rects[i]->low.y, tree.root->children[0]->rects[i]->high.x, tree.root->children[0]->rects[i]->high.y);
    //     printf("    LHV: %d\n", tree.root->children[0]->rects[i]->hilbertValue);
    //     if (tree.root->children[0]->isLeaf)
    //     {
    //         printf("    Element: (%d, %d)\n", tree.root->children[0]->elements[i]->x, tree.root->children[0]->elements[i]->y);
    //     }
    //     else
    //     {
    //         printf("    Child node: %p\n", tree.root->children[0]->children[i]);
    //     }
    // }
    // for (int i = 0; i < tree.root->children[1]->num_entries; i++)
    // {
    //     printf("  Entry %d:\n", i);
    //     printf("    Rect: ((%d, %d), (%d, %d))\n", tree.root->children[1]->rects[i]->low.x, tree.root->children[1]->rects[i]->low.y, tree.root->children[1]->rects[i]->high.x, tree.root->children[1]->rects[i]->high.y);
    //     printf("    LHV: %d\n", tree.root->children[1]->rects[i]->hilbertValue);
    //     if (tree.root->children[1]->isLeaf)
    //     {
    //         printf("    Element: (%d, %d)\n", tree.root->children[1]->elements[i]->x, tree.root->children[1]->elements[i]->y);
    //     }
    //     else
    //     {
    //         printf("    Child node: %p\n", tree.root->children[1]->children[i]);
    //     }
    // }
