#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
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

//Leaf has C_l entries of the form (R, obj_id)
//Non-leaf has C_n entries of the form (R, ptr, LHV)

int hilbert_rect_center(RECTANGLE r);

rtree tree;

RECTANGLE findMBR(NODE curr_node)
{
    RECTANGLE newRec = (RECTANGLE)malloc(sizeof(rectangle));
    if (curr_node->isLeaf == 1)
    {
        int xmin = INT_MAX, xmax = -1, ymin = INT_MAX, ymax = -1;
        for (int i = 0; i < curr_node->num_entries; i++)
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
        for (int i = 0; i < curr_node->num_entries; i++)
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

NODE hilbert_choose_sibling(NODE node, RECTANGLE new_rect);

NODE hilbert_choose_sibling(NODE nn, RECTANGLE new_rect)
{
    int node_index;
    NODE parent = nn->parent;
    NODE left_sibling = NULL;
    NODE right_sibling = NULL;
    for (int i = 0; parent != NULL && i < parent->num_entries; i++)
    {
        if (nn == parent->children[i])
            node_index = i;
    }

    // Lot of scope for fuck up here
    if (node_index > 0)
        left_sibling = parent->children[node_index - 1];
    if (node_index < 3)
        right_sibling = parent->children[node_index + 1];

    RECTANGLE *rects = nn->rects;
    int median_hv = nn->rects[nn->num_entries / 2]->hilbertValue;
    if (new_rect->hilbertValue > median_hv)
    {

        return right_sibling;
    }
    else
    {

        return left_sibling;
    }
}

void rot(int n, int *x, int *y, int rx, int ry)
{
    if (ry == 0)
    {
        if (rx == 1)
        {
            *x = n - 1 - *x;
            *y = n - 1 - *y;
        }

        // Swap x and y
        int t = *x;
        *x = *y;
        *y = t;
    }
}

int xy2d(int n, int x, int y)
{
    int rx, ry, s, d = 0;
    for (s = n / 2; s > 0; s /= 2)
    {
        rx = (x & s) > 0;
        ry = (y & s) > 0;
        d += s * s * ((3 * rx) ^ ry);
        rot(n, &x, &y, rx, ry);
    }
    return d;
}

int hilbert_rect_center(RECTANGLE r)
{
    int xmid = (r->low.x + r->high.x) / 2;
    int ymid = (r->low.y + r->high.y) / 2;
    // uint32_t x = coord_to_binary(xmid);
    // uint32_t y = coord_to_binary(ymid);
    // uint32_t z = interleave_bits(x, y);
    // return hilbert_value(z);
    return xy2d(16, xmid, ymid);
}

RECTANGLE getUnion(RECTANGLE R1, RECTANGLE R2) {
    RECTANGLE R=(RECTANGLE)malloc(sizeof(rectangle));

    R->low.x = (R1->low.x < R2->low.x) ? R1->low.x : R2->low.x;
    R->low.y = (R1->low.y < R2->low.y) ? R1->low.y : R2->low.y;
    R->high.x = (R1->high.x > R2->high.x) ? R1->high.x : R2->high.x;
    R->high.y = (R1->high.y > R2->high.y) ? R1->high.y : R2->high.y;
    R->hilbertValue = hilbert_rect_center(R);

    return R;
}

/* Helper function to get the MBR of a node */
RECTANGLE getMBR(NODE node) {
    RECTANGLE mbr = node->rects[0];
    for (int i = 1; i < node->num_entries; i++) {
        mbr = getUnion(mbr, node->rects[i]);
    }
    return mbr;
}

/* Helper function to get the maximum LHV of a node */
int getMaxLHV(NODE node) {
    int max_lhv = node->rects[0]->hilbertValue;
    for (int i = 1; i < node->num_entries; i++) {
        int lhv = node->rects[i]->hilbertValue;
        if (lhv > max_lhv) {
            max_lhv = lhv;
        }
    }
    return max_lhv;
}

NODE createNewNode(bool isLeaf) {
    NODE newNode = (NODE)malloc(sizeof(node));
    memset(newNode, 0, sizeof(node));
    for(int i = 0 ; i < M ; i++)
    {
        newNode->elements[i] = NULL;
        newNode->children[i] = NULL;
    }
    for(int i = 0 ; i < M ; i++)
    {
    }
    if (isLeaf)
    {
        newNode->isLeaf = true;
        newNode->lhv = -1;
    }
    else
    {
        newNode->isLeaf = false;
    }
    for(int i = 0 ; i < M ; i++)
    {
        newNode->rects[i] = NULL;
    }
    return newNode; 
}


// Helper function to choose the leaf node to insert a rectangle into
NODE chooseLeaf(RECTANGLE r, int hilbertValue, NODE node) {
    if (node->isLeaf) {
        return node;
    }
    NODE minNode = NULL;
    int minAreaIncrease = INT_MAX;
    for (int i = 0; i < node->num_entries; i++) {
        int areaIncrease = 0;
        RECTANGLE tempRect = node->rects[i];
        if (tempRect->low.x > r->high.x) {
            areaIncrease += (tempRect->low.x - r->high.x) * (tempRect->high.y - tempRect->low.y);
        }
        else if (tempRect->high.x < r->low.x) {
            areaIncrease += (r->low.x - tempRect->high.x) * (tempRect->high.y - tempRect->low.y);
        }
        if (tempRect->low.y > r->high.y) {
            areaIncrease += (tempRect->high.x - tempRect->low.x) * (tempRect->low.y - r->high.y);
        }
        else if (tempRect->high.y < r->low.y) {
            areaIncrease += (tempRect->high.x - tempRect->low.x) * (r->low.y - tempRect->high.y);
        }
        int tempHilbertValue = node->isLeaf ? hilbert_rect_center(tempRect) : node->children[i]->lhv;
        int hilbertDistance = abs(tempHilbertValue - hilbertValue);
        if (hilbertDistance == 0 && areaIncrease < minAreaIncrease) {
            minNode = node->children[i];
            minAreaIncrease = areaIncrease;
        }
        else if (hilbertDistance > 0 && hilbertDistance < minAreaIncrease) {
            minNode = node->children[i];
            minAreaIncrease = hilbertDistance;
        }
    }
    return chooseLeaf(r, hilbertValue, minNode);
}

NODE handleOverflow(NODE n, RECTANGLE r){
    if(n->parent==NULL){ //if n is a root
        // NODE new_node = createNewNode(n->children[0]->isLeaf);
        ELEMENT new_element = (ELEMENT)malloc(sizeof(struct element));
        new_element->x = r->low.x;
        new_element->y = r->low.y;
        NODE nn = createNewNode(n->isLeaf);
        NODE new_parent = createNewNode(false);
        ELEMENT hilbert_sorted_array[M+1];
        RECTANGLE hilbert_sorted_array_rects[M+1];
        for(int i=0; i<M+1; i++){
            if(i==M){
                hilbert_sorted_array[i] = new_element;
                hilbert_sorted_array_rects[i] = r;
            }
            else{
                hilbert_sorted_array[i] = n->elements[i];
                hilbert_sorted_array_rects[i] = n->rects[i];
            }
        }
        int i = M;
        while(i>0 && hilbert_sorted_array_rects[i-1]->hilbertValue > r->hilbertValue){
                hilbert_sorted_array[i] = hilbert_sorted_array[i-1];
                hilbert_sorted_array_rects[i] = hilbert_sorted_array_rects[i-1];
                i--;
        }
        hilbert_sorted_array[i] = new_element;
        hilbert_sorted_array_rects[i] = r;
        n->num_entries = M/2 + 1;
        nn->num_entries = M/2;
        for(int i=0; i<M/2+1; i++){
            n->elements[i] = hilbert_sorted_array[i];
            n->rects[i] = hilbert_sorted_array_rects[i];
        }
        for(int i=M/2+1; i<M+1; i++){
            nn->elements[i-M/2-1] = hilbert_sorted_array[i];
            nn->rects[i-M/2-1] = hilbert_sorted_array_rects[i];
        }
        n->lhv = n->rects[M/2]->hilbertValue;

        nn->lhv = nn->rects[0]->hilbertValue;
        n->parent = new_parent;
        nn->parent = new_parent;
        new_parent->num_entries = 2;
        new_parent->children[0] = n;
        new_parent->children[1] = nn;
        new_parent->parent = NULL;
        new_parent->lhv = n->lhv>nn->lhv ? n->lhv : nn->lhv;
        new_parent->rects[0] = getMBR(n);
        new_parent->rects[1] = getMBR(nn);
        new_parent->children[0]->lhv = max(n->rects[0]->hilbertValue, max(n->rects[1]->hilbertValue, n->rects[2]->hilbertValue));
        new_parent->children[1]->lhv = max(nn->rects[0]->hilbertValue, nn->rects[1]->hilbertValue);
        new_parent->isLeaf = false;

        // rects array of new_parent has to be initialised with mbr of n and nn
        return new_parent;
    }
    
    else
    {
        NODE parent = n->parent;        
        NODE coop_sibling = hilbert_choose_sibling(n, r); //this function is not verified to be correct
        //check it

        int curr_node_index;
        int coop_sibling_index;

        for (int i = 0; i < M; i++)
        {
            if (parent->children[i] == n)
                curr_node_index = i;
            if (parent->children[i] == coop_sibling)
                coop_sibling_index = i;
        }
        
        //rects to store all the rectangles from the two nodes
        RECTANGLE rects[n->num_entries + coop_sibling->num_entries + 1];
        

        for (int i = 0; i < n->num_entries; i++)
            rects[i] = n->rects[i];
        
        for (int i = 0; i < coop_sibling->num_entries; i++)
            rects[i+n->num_entries] = coop_sibling->rects[i];

        rects[n->num_entries + coop_sibling->num_entries] = r;

        // qsort(rects, n->num_entries + coop_sibling->num_entries + 1, sizeof(RECTANGLE), hilbert_entry_cmp);
        for (int i = 1 ; i < n->num_entries + coop_sibling->num_entries + 1 ; i++)
        {
            for (int j = i ; j < n->num_entries + coop_sibling->num_entries + 1; j++)
            {
                if (rects[j]->hilbertValue > rects[j-1]->hilbertValue)
                {
                    RECTANGLE x = rects[j];
                    rects[j] = rects[j-1];
                    rects[j-1] = x;
                }

            }
        }
        // If coop sibling is also full then split
        if (coop_sibling->num_entries == M)
        {
            if(coop_sibling->lhv>n->lhv){
                NODE nn = createNewNode(n->isLeaf);
                for(int i=0;i<M;i++)nn->rects[i]=NULL;
                for(int i=0;i<M;i++){
                    n->rects[i]=rects[i];
                }         
                for(int i=M;i<2*M;i++){
                    coop_sibling->rects[i-M]=rects[i];
                }
                nn->rects[0]=rects[2*M];
                return nn;
            }
            else
            {
                NODE nn = createNewNode(n->isLeaf);
                for(int i=0;i<M;i++)nn->rects[i]=NULL;
                for(int i=0;i<M;i++){
                    coop_sibling->rects[i]=rects[i];
                }         
                for(int i=M;i<2*M;i++){
                    n->rects[i-M]=rects[i];
                }
                nn->rects[0]=rects[2*M];
                return nn;
            }
        }

        // Rearrange entries
        else
        {
            int first_rearrange = n->num_entries + coop_sibling->num_entries/2;
            int second_rearrange = n->num_entries + coop_sibling->num_entries - first_rearrange;

            // ONLY WORKS FOR LEAF NODES
            if (coop_sibling->lhv > n->lhv)
            {
                for (int i = 0; i < first_rearrange; i++)
                    n->rects[i] = rects[i];

                for (int i = 0; i < second_rearrange; i++)
                    coop_sibling->rects[i] = rects[first_rearrange + i];
            }

            else
            {
                for (int i = 0; i < first_rearrange; i++)
                    coop_sibling->rects[i] = rects[i];

                for (int i = 0; i < second_rearrange; i++)
                    n->rects[i] = rects[first_rearrange + i];
            }
        }     
    }
}


// /* Helper function to split a node that has overflowed */
// NODE handleOverflow(NODE node, RECTANGLE r) {
//     if(node->parent==NULL){ //if n is a root
//         // NODE new_node = createNewNode(n->children[0]->isLeaf);
//         ELEMENT new_element = (ELEMENT)malloc(sizeof(struct element));
//         new_element->x = r->low.x;
//         new_element->y = r->low.y;
//         NODE nn = createNewNode(node->isLeaf);
//         NODE new_parent = createNewNode(false);
//         ELEMENT hilbert_sorted_array[M+1];
//         RECTANGLE hilbert_sorted_array_rects[M+1];
//         for(int i=0; i<M+1; i++){
//             if(i==M){
//                 hilbert_sorted_array[i] = new_element;
//                 hilbert_sorted_array_rects[i] = r;
//             }
//             else{
//                 hilbert_sorted_array[i] = node->elements[i];
//                 hilbert_sorted_array_rects[i] = node->rects[i];
//             }
//         }
//         int i = M;
//         while(i>0 && hilbert_sorted_array_rects[i-1]->hilbertValue > r->hilbertValue){
//                 hilbert_sorted_array[i] = hilbert_sorted_array[i-1];
//                 hilbert_sorted_array_rects[i] = hilbert_sorted_array_rects[i-1];
//                 i--;
//         }
//         hilbert_sorted_array[i] = new_element;
//         hilbert_sorted_array_rects[i] = r;
//         node->num_entries = M/2 + 1;
//         nn->num_entries = M/2;
//         for(int i=0; i<M/2+1; i++){
//             node->elements[i] = hilbert_sorted_array[i];
//             node->rects[i] = hilbert_sorted_array_rects[i];
//         }
//         for(int i=M/2+1; i<M+1; i++){
//             nn->elements[i-M/2-1] = hilbert_sorted_array[i];
//             nn->rects[i-M/2-1] = hilbert_sorted_array_rects[i];
//         }
//         node->lhv = node->rects[M/2]->hilbertValue;

//         nn->lhv = nn->rects[0]->hilbertValue;
//         node->parent = new_parent;
//         nn->parent = new_parent;
//         new_parent->num_entries = 2;
//         new_parent->children[0] = node;
//         new_parent->children[1] = nn;
//         new_parent->parent = NULL;
//         new_parent->lhv = node->lhv>nn->lhv ? node->lhv : nn->lhv;
//         new_parent->rects[0] = getMBR(node);
//         new_parent->rects[1] = getMBR(nn);
//         new_parent->children[0]->lhv = max(node->rects[0]->hilbertValue, max(node->rects[1]->hilbertValue, node->rects[2]->hilbertValue));
//         new_parent->children[1]->lhv = max(nn->rects[0]->hilbertValue, nn->rects[1]->hilbertValue);
//         new_parent->isLeaf = false;
//         tree.root=new_parent;
//         // rects array of new_parent has to be initialised with mbr of n and nn
//         return new_parent;
//     }
//     else{
//         NODE newNode = (NODE)malloc(sizeof(node));
//         newNode->isLeaf = node->isLeaf;
//         newNode->num_entries = 0;
//         newNode->parent = node->parent;
//         if (node->isLeaf) {
//             // Leaf node, use 2-3 splitting
//             int i = node->num_entries - 1;
//             while (i >= 0 && node->rects[i]->low.x > r->low.x) {
//                 node->rects[i + 1] = node->rects[i];
//                 i--;
//             }
//             node->rects[i + 1] = r;
//             node->num_entries++;
//             int splitIndex = (node->num_entries + 1) / 2;
//             newNode->num_entries = node->num_entries - splitIndex;
//             for (int j = 0; j < newNode->num_entries; j++) {
//                 newNode->rects[j] = node->rects[splitIndex + j];
//             }
//             node->num_entries = splitIndex;
//         }
//         else {
//             // Non-leaf node, use 2-3 splitting
//             int i = node->num_entries - 1;
//             while (i >= 0 && node->rects[i]->hilbertValue > r->hilbertValue) {
//                 node->rects[i + 1] = node->rects[i];
//                 node->children[i + 2] = node->children[i + 1];
//                 i--;
//             }
//             node->rects[i + 1] = r;
//             node->children[i + 2] = newNode;
//             node->num_entries++;
//             int splitIndex = (node->num_entries + 1) / 2;
//             newNode->num_entries = node->num_entries - splitIndex;
//             for (int j = 0; j < newNode->num_entries; j++) {
//                 newNode->rects[j] = node->rects[splitIndex + j];
//                 newNode->children[j] = node->children[splitIndex + j];
//                 newNode->children[j]->parent = newNode;
//             }
//             newNode->children[newNode->num_entries] = node->children[node->num_entries];
//             newNode->children[newNode->num_entries]->parent = newNode;
//             node->num_entries = splitIndex - 1;
//         }
//         return newNode;
//     }
// }

/* Helper function to adjust tree after an insertion */
void adjustTree(NODE node) {
    while (node != NULL) {
        for (int i = 0; i < node->num_entries; i++) {
            RECTANGLE R = node->rects[i];
            int old_lhv = node->lhv;
            if (i == 0 || R->hilbertValue > node->lhv) {
                node->lhv = R->hilbertValue;
            }
            if (old_lhv != node->lhv) {
                if (node->parent != NULL) {
                    for (int j = 0; j < node->parent->num_entries; j++) {
                        if (node->parent->children[j] == node) {
                            node->parent->rects[j]->hilbertValue = node->lhv;
                            break;
                        }
                    }
                }
                else {
                    // Update the root node's LHV
                    // node->parent = (NODE)malloc(sizeof(node));
                    // node->parent->isLeaf = false;
                    // node->parent->lhv = node->lhv;
                    // node->parent->num_entries = 1;
                    // node->parent->children[0] = node;
                    // node->parent->rects[0] = R;
                    // node->parent->rects[0]->hilbertValue = node->lhv;
                    // node->parent->parent = NULL;
                    // node = node->parent;
                    break;
                }
            }
        }
        node = node->parent;
    }
}

void AdjustTree(NODE node, NODE new_node) {
    NODE n = node;
    while (n != NULL) {
        if (n->parent == NULL) {
            if (new_node != NULL) {
                // If the root node splits, create a new root and make it the parent of the split nodes
                NODE new_root = (NODE) malloc(sizeof(struct node));
                new_root->isLeaf = false;
                new_root->num_entries = 2;
                new_root->children[0] = n;
                new_root->children[1] = new_node;
                new_root->rects[0] = getMBR(n);
                new_root->rects[1] = getMBR(new_node);
                new_root->lhv = getMaxLHV(new_root);
                n->parent = new_root;
                new_node->parent = new_root;
            }
            return;
        }

        // Propagate node split upward
        NODE parent = n->parent;
        NODE new_parent = NULL;
        if (new_node != NULL) {
            RECTANGLE r = getUnion(getMBR(n), getMBR(new_node));
            new_parent = parent->num_entries == M ? handleOverflow(parent, r) : parent;
        }

        // Adjust the MBRs and LHV in the parent level
        for (int i = 0; i < parent->num_entries; i++) {
            if (parent->children[i] == n) {
                parent->rects[i] = getMBR(n);
                parent->rects[i]->hilbertValue = n->lhv;
            }
        }
        parent->lhv = getMaxLHV(parent);

        // Move up to next level
        n = parent;
        new_node = new_parent;
    }
}


void insertRect(RECTANGLE R, NODE n, rtree* tree) {
    int h = R->hilbertValue;
    NODE leaf = chooseLeaf(R, h, n);
    NODE parent_of_leaf = leaf->parent;
    int sum_ele = 0;
    for(int i=0; parent_of_leaf!=NULL && i<parent_of_leaf->num_entries; i++){
                sum_ele += parent_of_leaf->children[i]->num_entries;
    }
    if (leaf->num_entries < C_l) {
        // If leaf is not full, insert R into it
        int i = leaf->num_entries - 1;
        while (i >= 0 && leaf->rects[i]->hilbertValue > R->hilbertValue) {
            leaf->rects[i + 1] = leaf->rects[i];
            leaf->elements[i + 1] = leaf->elements[i];
            i--;
        }
        // leaf->rects[i + 1] = (RECTANGLE) malloc (sizeof(rectangle));
        // leaf->elements[i + 1] = (ELEMENT) malloc (sizeof(element));
        (leaf->rects[i + 1]) = R;
        (leaf->elements[i + 1]) = &(R->low);
        // leaf->elements[i + 1] = &R.low;
        // leaf->rects[i + 1] = &R;
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
    }
    else if(parent_of_leaf!=NULL && sum_ele < M*parent_of_leaf->num_entries){
        ELEMENT elements_sorted[sum_ele + 1];
        RECTANGLE rectangles_sorted[sum_ele + 1];
        elements_sorted[sum_ele] = &(R->low);
        rectangles_sorted[sum_ele] = R;
        int k1 = 0, k2 = 0;
        for(int i=0; i<parent_of_leaf->num_entries; i++){
            for(int j = 0; j<parent_of_leaf->children[i]->num_entries; j++){
                elements_sorted[k1++] = parent_of_leaf->children[i]->elements[j];
                rectangles_sorted[k2++] = parent_of_leaf->children[i]->rects[j];
            }
        }
        int i = sum_ele;
        while(rectangles_sorted[i-1]->hilbertValue > R->hilbertValue&&i>0){
                elements_sorted[i] = elements_sorted[i-1];
                rectangles_sorted[i] = rectangles_sorted[i-1];
                i--;
        }
        elements_sorted[i] = &(R->low);
        rectangles_sorted[i] = R;
        k1 = 0, k2 = 0;
        for(int i=0; i<parent_of_leaf->num_entries;i++){
            int j = 0;
            for(; j<M && k1<sum_ele + 1 && k2 < sum_ele + 1; j++){
                parent_of_leaf->children[i]->rects[j] = rectangles_sorted[k1++];
                parent_of_leaf->children[i]->elements[j] = elements_sorted[k2++];
            }
            parent_of_leaf->children[i]->num_entries = j;
        }

    }
    else if (parent_of_leaf!=NULL && sum_ele==M*parent_of_leaf->num_entries && parent_of_leaf->num_entries<M){
        //parent_of_leaf->children[num_entries] = (NODE)malloc(sizeof(struct node));
        handleOverflow(parent_of_leaf->children[parent_of_leaf->num_entries-1],R);
        
    }
    else{
        // If leaf is full, split it and adjust the tree
        // NODE new_parent= (NODE)malloc(sizeof(node));
        // for(int i = 0 ; i < M ; i++)
        // {
        //     new_parent->elements[i] = NULL;
        // }
        // NODE new_leaf = createNewNode(true);
        // // NODE new_leaf = malloc(sizeof(node));
        // // for(int i = 0 ; i < M ; i++)
        // // {
        // //     new_leaf->elements[i] = NULL;
        // // }
        // new_leaf->parent = leaf->parent;
        // new_leaf->isLeaf = true;
        // splitNode(leaf, new_leaf);
        // adjustTree(leaf, new_leaf, tree);
        // // Insert R into the appropriate leaf node
        // if (h <= new_leaf->rects[0]->hilbertValue) {
        //     insertRect(R, leaf, tree);
        // } else {
        //     insertRect(R, new_leaf, tree);
        // }
        // for (int i = 0 ; i < leaf->parent->num_entries ; i++)
        // leaf->parent->rects[i]=findMBR(leaf->parent->children[i]);
        // adjustTree(leaf,new_leaf,tree);
        // NODE new_parent=handleOverflow(n,&R);
        // tree->root=new_parent;

        NODE new_parent = handleOverflow(n , R);
        tree->root = new_parent;
    }
}


NODE Insert(NODE root, RECTANGLE r) {
    int h = hilbert_rect_center(r);
    NODE leaf = chooseLeaf(r, h, root);
    if (leaf->num_entries < M) {
        // Leaf has room, insert the rectangle and return
        // InsertRect(leaf, r);
        leaf->rects[leaf->num_entries]=r;
        leaf->num_entries++;
        leaf->lhv=max(leaf->lhv,h);
        adjustTree(leaf);
        return root;
    }
    else {
        // Overflow
        NODE newLeaf = handleOverflow(leaf, r);
        NODE parent = leaf->parent;
        if (parent == NULL) {
            // Create new root
            parent = createNewNode(false);
            parent->children[0] = leaf;
            leaf->parent = parent;
            parent->lhv = leaf->lhv;
            parent->num_entries=1;
        }
        // Insert new leaf to parent and adjust tree
        // InsertNode(parent, newLeaf);
        parent->children[parent->num_entries]=newLeaf;
        AdjustTree(leaf,newLeaf);
        return root;
    }
}


RECTANGLE createNewRectangle(int lowx, int lowy, int highx, int highy) {
    RECTANGLE newRec = (RECTANGLE) malloc (sizeof(rectangle));
    newRec->low.x = lowx;
    newRec->low.y = lowy;
    newRec->high.y = highy;
    newRec->high.x = highx;
    newRec->hilbertValue=hilbert_rect_center(newRec);
    return newRec;
}

void myPrint (char* hello, NODE hehe)
{
    printf("%s node: %p\n", hello, hehe);
    printf("%s is leaf node: %d\n", hello, hehe->isLeaf);
    printf("%s LHV: %d\n", hello, hehe->lhv);
    printf("Number of entries in %s: %d\n", hello, hehe->num_entries);
    printf("Children adresses of %s : \n", hello);
    for (int i = 0 ; i < M ; i++) printf("\tChild %d : %p\n", i, hehe->children[i]);
    printf("Elements adresses of %s : \n", hello);
    for (int i = 0 ; i < M ; i++) printf("\tElements %d : %p\n", i, hehe->elements[i]);
    for (int i = 0; i < hehe->num_entries; i++)
    {
        printf("   Entry %d:\n", i);
        printf("     Rect: ((%d, %d), (%d, %d))\n", hehe->rects[i]->low.x, hehe->rects[i]->low.y, hehe->rects[i]->high.x, hehe->rects[i]->high.y);
        printf("     LHV: %d\n", hehe->rects[i]->hilbertValue);
        // if (hehe->isLeaf)
        // {
        //     printf("     Element: (%d, %d)\n", hehe->elements[i]->x, hehe->elements[i]->y);
        // }
        if(!hehe->isLeaf)
        {
        }
            printf("     Child node: %p\n", hehe->children[i]);
    }
}

void printTree(NODE root, int depth) {
    if (root == NULL) {
        return;
    }
    for (int i = 0; i < depth; i++) {
        printf("  ");
    }
    if (root->isLeaf) {
        printf("Leaf node: [");
        for (int i = 0; i < root->num_entries; i++) {
            printf("(%d,%d)", root->rects[i]->low.x, root->rects[i]->low.y);
            if (i < root->num_entries - 1) {
                printf(", ");
            }
        }
        printf("]\n");
    } else {
        printf("Internal node: [");
        for (int i = 0; i < root->num_entries; i++) {
            printf("%d", root->lhv);
            if (i < root->num_entries - 1) {
                printf(", ");
            }
        }
        printf("]\n");
        for (int i = 0; i < root->num_entries; i++) {
            printTree(root->children[i], depth + 1);
        }
    }
}


int main(int argc, char const *argv[])
{
    // RTREE tree = malloc (sizeof(rtree));

    tree.cnt = 0;
    tree.height = 1;
    tree.root = malloc(sizeof(node));
    tree.root->isLeaf = true;
    tree.root->num_entries = 0;
    tree.root->lhv = -1;
    tree.root->parent = NULL;

    RECTANGLE r1 = (createNewRectangle(1, 9, 1, 9));
    Insert(tree.root,r1);
    RECTANGLE r2 = (createNewRectangle(2, 20, 2, 20));
    Insert(tree.root,r2);
    RECTANGLE r3 = (createNewRectangle(2, 19, 2, 19));
    Insert(tree.root,r3);
    RECTANGLE r4 = (createNewRectangle(3, 20, 3, 20));
    Insert(tree.root,r4);
    RECTANGLE r5 = (createNewRectangle(2, 10, 2, 10));
    Insert(tree.root,r5);
    RECTANGLE r6 = (createNewRectangle(8, 5, 8, 5));
    Insert(tree.root,r6);
    RECTANGLE r7 = createNewRectangle(4,5,4,5);
    Insert(tree.root,r7);
    RECTANGLE r8 = createNewRectangle(3,4,3,4);
    Insert(tree.root,r8);
    RECTANGLE r9 = createNewRectangle(3,5,3,5);
    Insert(tree.root,r9);
    // RECTANGLE r10 = createNewRectangle(2,4,2,4);
    // Insert(tree.root,r10);
    // RECTANGLE r11 = createNewRectangle(2,5,2,5);
    // Insert(tree.root,r11);
    // RECTANGLE r12 = createNewRectangle(8,15,8,15);
    // Insert(tree.root,r12);
    // RECTANGLE r13 = createNewRectangle(8,14,8,14);
    // Insert(tree.root,r13);
    // RECTANGLE r14 = createNewRectangle(7,15,7,15);
    // Insert(tree.root,r14);
    // RECTANGLE r15 = createNewRectangle(9,14,9,14);
    // Insert(tree.root,r15);
    // RECTANGLE r16 = createNewRectangle(9,15,9,15);
    // Insert(tree.root,r16);
    // RECTANGLE r17 = createNewRectangle(9,16,9,16);
    // Insert(tree.root,r17);
    // RECTANGLE r18 = createNewRectangle(9,17,9,17);
    // Insert(tree.root,r18);
    // RECTANGLE r19 = createNewRectangle(12,17,12,17);
    // Insert(tree.root,r19);
    // // printTree(tree.root,0);
    // RECTANGLE r20 = createNewRectangle(11,18,11,18);
    // Insert(tree.root,r20);
    // RECTANGLE r21 = createNewRectangle(1,20,1,20);
    // Insert(tree.root,r21);
    printf("Number of nodes: %d\n", tree.cnt);
    printf("Tree height: %d\n", tree.height);
    // myPrint("Root", tree.root);
    // myPrint("Child 1", tree.root->children[0]);
    // myPrint("Child 2", tree.root->children[1]);
    // myPrint("Child 3", tree.root->children[2]);


    return 0;
}
