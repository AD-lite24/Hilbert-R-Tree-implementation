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
#define COORD_BITS 16                 // Number of bits used for each coordinate
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
typedef struct element *ELEMENT;

NODE hilbert_choose_sibling(NODE node, RECTANGLE new_rect);

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
    int hilbertValue; // Can be LHV or the hilbert value depending on the type
};

int min(int a, int b)
{
    return (a < b) ? a : b;
}

int max(int a, int b)
{
    return (a > b) ? a : b;
}

// Leaf has C_l entries of the form (R, obj_id)
// Non-leaf has C_n entries of the form (R, ptr, LHV)
struct node
{
    int num_entries;
    RECTANGLE rects[M];
    bool isLeaf;
    int lhv; // will be -1 if it is a leaf node
    NODE parent;
    union
    {
        NODE children[M];
        ELEMENT elements[M];
    };
};

int hilbert_entry_cmp(RECTANGLE r1, RECTANGLE r2);
NODE handleOverflow(NODE n, RECTANGLE r);
NODE hilbert_choose_sibling(NODE nn, RECTANGLE new_rect);
NODE chooseLeaf(RECTANGLE R, int h, NODE n);
void insertRect(RECTANGLE R, NODE n, rtree *tree);
void adjustTree(NODE parent, NODE nn);
NODE handleOverflowNonLeaf(NODE parent, NODE nn);
NODE hilbert_choose_sibling_non_leaf(NODE par, NODE nn);

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

uint32_t hilbert_rect_center(RECTANGLE r)
{
    int32_t xmid = (r->low.x + r->high.x) / 2;
    int32_t ymid = (r->low.y + r->high.y) / 2;
    return xy2d(16, xmid, ymid);
}

RECTANGLE createNewRectangle(int lowx, int lowy, int highx, int highy)
{
    RECTANGLE newRec = (RECTANGLE)malloc(sizeof(rectangle));
    newRec->low.x = lowx;
    newRec->low.y = lowy;
    newRec->high.y = highy;
    newRec->high.x = highx;
    newRec->hilbertValue = hilbert_rect_center(newRec);
    return newRec;
}

NODE createNewNode(bool isLeaf)
{
    NODE newNode = (NODE)malloc(sizeof(node));
    memset(newNode, 0, sizeof(node));
    for (int i = 0; i < M; i++)
    {
        newNode->elements[i] = NULL;
        newNode->children[i] = NULL;
    }
    for (int i = 0; i < M; i++)
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
    for (int i = 0; i < M; i++)
    {
        newNode->rects[i] = NULL;
    }
    return newNode;
}

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

int hilbert_entry_cmp(RECTANGLE r1, RECTANGLE r2)
{
    if (r1->hilbertValue < r2->hilbertValue)
        return -1;
    else if (r1->hilbertValue > r2->hilbertValue)
        return 1;
    else
        return 0;
}

NODE handleOverflowNonLeaf(NODE parent, NODE nn)
{
    NODE uncle = (NODE)malloc(sizeof(struct node));
    NODE final_boss = parent->parent;
    NODE coop_sibling_non_leaf = hilbert_choose_sibling_non_leaf(parent, nn);

    int sum_ele = parent->num_entries + coop_sibling_non_leaf->num_entries;
    // for (int i = 0; i < final_boss->num_entries; i++)
    // {
    //     sum_ele += final_boss->children[i]->num_entries;
    // }
    // if (sum_ele == M * 2 && final_boss->num_entries < M)
    // {
    //     final_boss->children[final_boss->num_entries] = uncle;
    // }
    NODE *sorted_nodes = (NODE *)malloc(sizeof(NODE) * (sum_ele + 1));
    RECTANGLE *sorted_rects = (RECTANGLE *)malloc(sizeof(RECTANGLE) * (sum_ele + 1));
    sorted_nodes[sum_ele] = nn;
    sorted_rects[sum_ele] = findMBR(nn);
    int k = 0;
    if (coop_sibling_non_leaf->lhv > parent->lhv)
    {
        for (int j = 0; j < parent->num_entries && k < sum_ele; j++)
        {
            sorted_nodes[k++] = parent->children[j];
            sorted_rects[k++] = parent->rects[j];
        }
        for (int j = 0; j < coop_sibling_non_leaf->num_entries && k < sum_ele; j++)
        {
            sorted_nodes[k++] = coop_sibling_non_leaf->children[j];
            sorted_rects[k++] = coop_sibling_non_leaf->rects[j];
        }
    }
    else
    {
        for (int j = 0; j < coop_sibling_non_leaf->num_entries && k < sum_ele; j++)
        {
            sorted_nodes[k++] = coop_sibling_non_leaf->children[j];
            sorted_rects[k++] = coop_sibling_non_leaf->rects[j];
        }
        for (int j = 0; j < parent->num_entries && k < sum_ele; j++)
        {
            sorted_nodes[k++] = parent->children[j];
            sorted_rects[k++] = parent->rects[j];
        }
    }
    int i = sum_ele;
    while (i > 0 && sorted_nodes[i - 1]->lhv > nn->lhv)
    {
        sorted_nodes[i] = sorted_nodes[i - 1];
        sorted_rects[i] = sorted_rects[i - 1];
        i--;
    }
    sorted_nodes[i] = nn;
    sorted_rects[i] = findMBR(nn);
    if (sum_ele == M * 2)
    {

        if (coop_sibling_non_leaf->lhv > parent->lhv)
        {
            for (int i = 0; i < 3; i++)
            {
                parent->children[i] = sorted_nodes[i];
                parent->rects[i] = sorted_rects[i];
            }
            for (int i = 0; i < 3; i++)
            {
                coop_sibling_non_leaf->children[i] = sorted_nodes[i + 3];
                coop_sibling_non_leaf->rects[i] = sorted_rects[i + 3];
            }
            for (int i = 0; i < 3; i++)
            {
                uncle->children[i] = sorted_nodes[i + 6];
                uncle->rects[i] = sorted_rects[i + 6];
            }
            uncle->parent = parent->parent;
            uncle->num_entries = 3;
            uncle->isLeaf = false;
            for (int i = 0; i < uncle->num_entries; i++)
                uncle->lhv = max(uncle->lhv, uncle->children[i]->lhv);
        }
        else
        {
            for (int i = 0; i < 3; i++)
            {
                coop_sibling_non_leaf->children[i] = sorted_nodes[i];
                coop_sibling_non_leaf->rects[i] = sorted_rects[i];
            }
            for (int i = 0; i < 3; i++)
            {
                parent->children[i] = sorted_nodes[i + 3];
                parent->rects[i] = sorted_rects[i + 3];
            }
            for (int i = 0; i < 3; i++)
            {
                uncle->children[i] = sorted_nodes[i + 6];
                uncle->rects[i] = sorted_rects[i + 6];
            }
            uncle->parent = parent->parent;
            uncle->num_entries = 3;
            uncle->isLeaf = false;
            uncle->lhv = max(uncle->children[0]->lhv, max(uncle->children[1]->lhv, uncle->children[2]->lhv));
        }
        parent->num_entries = 3;
        coop_sibling_non_leaf->num_entries = 3;
        return uncle;
    }
    else
    {
        if (coop_sibling_non_leaf->lhv > parent->lhv)
        {

            for (int i = 0; i < 4; i++)
            {
                parent->children[i] = sorted_nodes[i];
                parent->rects[i] = sorted_rects[i];
            }
            for (int i = 0; i < sum_ele - 3; i++)
            {
                coop_sibling_non_leaf->children[i] = sorted_nodes[i + 4];
                coop_sibling_non_leaf->rects[i] = sorted_rects[i + 4];
            }
        }
        else
        {
            for (int i = 0; i < 4; i++)
            {
                coop_sibling_non_leaf->children[i] = sorted_nodes[i];
                coop_sibling_non_leaf->rects[i] = sorted_rects[i];
            }
            for (int i = 0; i < sum_ele - 3; i++)
            {
                parent->children[i] = sorted_nodes[i + 4];
                parent->rects[i] = sorted_rects[i + 4];
            }
        }
        parent->num_entries = 4;
        coop_sibling_non_leaf->num_entries = sum_ele - 3;
        return NULL;
    }
}
// hilbert_choose_sibling();

// return uncle;

NODE handleOverflow(NODE n, RECTANGLE r)
{
    if (n->parent == NULL)
    { // if n is a root
        // NODE new_node = createNewNode(n->children[0]->isLeaf);
        ELEMENT new_element = (ELEMENT)malloc(sizeof(struct element));
        new_element->x = r->low.x;
        new_element->y = r->low.y;
        NODE nn = createNewNode(n->isLeaf);
        NODE new_parent = createNewNode(false);
        ELEMENT hilbert_sorted_array[M + 1];
        RECTANGLE hilbert_sorted_array_rects[M + 1];
        for (int i = 0; i < M + 1; i++)
        {
            if (i == M)
            {
                hilbert_sorted_array[i] = new_element;
                hilbert_sorted_array_rects[i] = r;
            }
            else
            {
                hilbert_sorted_array[i] = n->elements[i];
                hilbert_sorted_array_rects[i] = n->rects[i];
            }
        }
        int i = M;
        while (i > 0 && hilbert_sorted_array_rects[i - 1]->hilbertValue > r->hilbertValue)
        {
            hilbert_sorted_array[i] = hilbert_sorted_array[i - 1];
            hilbert_sorted_array_rects[i] = hilbert_sorted_array_rects[i - 1];
            i--;
        }
        hilbert_sorted_array[i] = new_element;
        hilbert_sorted_array_rects[i] = r;
        n->num_entries = M / 2 + 1;
        nn->num_entries = M / 2;
        for (int i = 0; i < M / 2 + 1; i++)
        {
            n->elements[i] = hilbert_sorted_array[i];
            n->rects[i] = hilbert_sorted_array_rects[i];
        }
        for (int i = M / 2 + 1; i < M + 1; i++)
        {
            nn->elements[i - M / 2 - 1] = hilbert_sorted_array[i];
            nn->rects[i - M / 2 - 1] = hilbert_sorted_array_rects[i];
        }
        n->lhv = n->rects[M / 2]->hilbertValue;

        nn->lhv = nn->rects[0]->hilbertValue;
        n->parent = new_parent;
        nn->parent = new_parent;
        new_parent->num_entries = 2;
        new_parent->children[0] = n;
        new_parent->children[1] = nn;
        new_parent->parent = NULL;
        new_parent->lhv = n->lhv > nn->lhv ? n->lhv : nn->lhv;
        new_parent->rects[0] = findMBR(n);
        new_parent->rects[1] = findMBR(nn);
        new_parent->children[0]->lhv = max(n->rects[0]->hilbertValue, max(n->rects[1]->hilbertValue, n->rects[2]->hilbertValue));
        new_parent->children[1]->lhv = max(nn->rects[0]->hilbertValue, nn->rects[1]->hilbertValue);
        new_parent->isLeaf = false;

        // rects array of new_parent has to be initialised with mbr of n and nn
        return new_parent;
    }

    else
    {
        NODE parent = n->parent;
        NODE coop_sibling = hilbert_choose_sibling(n, r); // this function is not verified to be correct
        // check it

        int curr_node_index;
        int coop_sibling_index;

        for (int i = 0; i < M; i++)
        {
            if (parent->children[i] == n)
                curr_node_index = i;
            if (parent->children[i] == coop_sibling)
                coop_sibling_index = i;
        }

        // rects to store all the rectangles from the two nodes
        RECTANGLE rects[n->num_entries + coop_sibling->num_entries + 1];

        if (coop_sibling->lhv > n->lhv)
        {
            for (int i = 0; i < n->num_entries; i++)
                rects[i] = n->rects[i];
            for (int i = 0; i < coop_sibling->num_entries; i++)
                rects[i + n->num_entries] = coop_sibling->rects[i];
        }
        else
        {
            for (int i = 0; i < coop_sibling->num_entries; i++)
                rects[i] = coop_sibling->rects[i];
            for (int i = 0; i < n->num_entries; i++)
                rects[i + coop_sibling->num_entries] = n->rects[i];
        }
        rects[n->num_entries + coop_sibling->num_entries] = r;
        int i = n->num_entries + coop_sibling->num_entries;
        while (i > 0 && r->hilbertValue < rects[i]->hilbertValue)
            rects[i+1] = rects[i];
        rects[i] = r;

        // // qsort(rects, n->num_entries + coop_sibling->num_entries + 1, sizeof(RECTANGLE), hilbert_entry_cmp);
        // for (int i = 1; i < n->num_entries + coop_sibling->num_entries + 1; i++)
        // {
        //     for (int j = i; j < n->num_entries + coop_sibling->num_entries + 1; j++)
        //     {
        //         if (rects[j]->hilbertValue > rects[j - 1]->hilbertValue)
        //         {
        //             RECTANGLE x = rects[j];
        //             rects[j] = rects[j - 1];
        //             rects[j - 1] = x;
        //         }
        //     }
        // }
        // If coop sibling is also full then split
        if (coop_sibling->num_entries == M)
        {
            if (coop_sibling->lhv > n->lhv)
            {
                NODE nn = createNewNode(n->isLeaf);
                for (int i = 0; i < M; i++)
                    nn->rects[i] = NULL;
                for (int i = 0; i < M; i++)
                {
                    n->rects[i] = rects[i];
                }
                for (int i = M; i < 2 * M; i++)
                {
                    coop_sibling->rects[i - M] = rects[i];
                }
                nn->rects[0] = rects[2 * M];
                return nn;
            }
            else
            {
                NODE nn = createNewNode(n->isLeaf);
                for (int i = 0; i < M; i++)
                    nn->rects[i] = NULL;
                for (int i = 0; i < M; i++)
                {
                    coop_sibling->rects[i] = rects[i];
                }
                for (int i = M; i < 2 * M; i++)
                {
                    n->rects[i - M] = rects[i];
                }
                nn->rects[0] = rects[2 * M];
                return nn;
            }
        }

        // Rearrange entries
        else
        {
            int first_rearrange = n->num_entries + coop_sibling->num_entries / 2;
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

NODE hilbert_choose_sibling_non_leaf(NODE par, NODE nn)
{
    int node_par_index;
    NODE coop_uncle = (NODE)malloc(sizeof(struct node));
    NODE grand_parent = par->parent;
    NODE left_uncle = NULL;
    NODE right_uncle = NULL;

    for (int i = 0; grand_parent != NULL && i < grand_parent->num_entries; i++)
    {
        if (par = grand_parent->children[i])
            node_par_index = i;
    }

    if (node_par_index > 0)
        left_uncle = grand_parent->children[node_par_index - 1];
    if (node_par_index < 3)
        right_uncle = grand_parent->children[node_par_index + 1];

    RECTANGLE *rects = par->rects;
    int median_hv = par->rects[par->num_entries / 2]->hilbertValue;
    if (nn->lhv > median_hv)
    {
        if (!right_uncle)
            return left_uncle;
        return right_uncle;
    }
    else
    {
        if (!left_uncle)
            return right_uncle;
        return left_uncle;
    }
}

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
    // left_sibling = parent->children[0];
    // right_sibling = parent->children[3];
    if (node_index > 0)
        left_sibling = parent->children[node_index - 1];
    if (node_index < 3)
        right_sibling = parent->children[node_index + 1];

    RECTANGLE *rects = nn->rects;
    int median_hv = nn->rects[nn->num_entries / 2]->hilbertValue;
    if (new_rect->hilbertValue > median_hv)
    {
        if (!right_sibling)
            return left_sibling;
        return right_sibling;
    }
    else
    {
        if (!left_sibling) 
            return right_sibling;
        return left_sibling;
    }
}

// void splitNode(NODE n, NODE nn)
// {
//     // Split n into nodes n and nn
//     // Assign the values to new node
//     nn->isLeaf = n->isLeaf;
//     nn->num_entries = n->num_entries / 2;
//     nn->lhv = n->lhv;
//     // Move entries to nn
//     for (int i = nn->num_entries - 1; i >= 0; i--)
//     {
//         nn->rects[i] = n->rects[i + nn->num_entries];
//         nn->children[i] = n->children[i + nn->num_entries];
//         nn->elements[i] = n->elements[i + nn->num_entries];
//         n->rects[i + nn->num_entries] = NULL;
//         n->children[i + nn->num_entries] = NULL;
//         n->elements[i + nn->num_entries] = NULL;
//     }
//     n->num_entries = n->num_entries - nn->num_entries;
//     // Update the number of entries
//     // n->num_entries = n->num_entries - nn->num_entries;
// }

NODE chooseLeaf(RECTANGLE R, int h, NODE n)
{
    // while (!n->isLeaf)
    // {
    //     int i = 0;
    //     int min_lhv = INT_MAX;
    //     NODE c = NULL;
    //     // while (i < n->num_entries)
    //     // {
    //     //     rectangle Rn = *(n->rects[i]);

    //     //     if (Rn.hilbertValue >= h && Rn.hilbertValue < min_lhv)
    //     //     {
    //     //         min_lhv = Rn.hilbertValue;
    //     //         c = n->children[i];
    //     //         i = 0;
    //     //     }
    //     //     i++;
    //     // }
        

    //     if (c == NULL)
    //     {
    //         c = n->children[n->num_entries - 1];
    //     } // break;
    //     n = c;
    // }
    // return n;

    if (n->isLeaf) return n;
    for (int i = 0 ; i < n->num_entries ; i++)
    {
        rectangle lowlo = *(n->children[i]->rects[0]);
        rectangle higlo = *(n->children[i]->rects[n->children[i]->num_entries-1]);
        if (h >= lowlo.hilbertValue && h <= higlo.hilbertValue)
            return chooseLeaf(R, h, n->children[i]);
    }
    if (h >= n->rects[n->num_entries-1]->hilbertValue)
        return (R, h, n->children[n->num_entries-1]);
    if (h <= n->rects[0]->hilbertValue)
        return (R, h, n->children[0]);
}

// NODE choose_leaf(RECTANGLE r, NODE root)
// {
//     NODE N = root;

//     if (N->isLeaf)
//         return N;

//     int chosen_entry ;
//     for (int i = 0; i < N->num_entries; i++)
//     {
//         if (N->children[])
//     }
// }

void insertRect(RECTANGLE R, NODE n, rtree *tree)
{
    int h = R->hilbertValue;
    NODE leaf = chooseLeaf(R, h, n);
    NODE parent_of_leaf = leaf->parent;
    int sum_ele = 0;
    for (int i = 0; parent_of_leaf != NULL && i < parent_of_leaf->num_entries; i++)
    {
        sum_ele += parent_of_leaf->children[i]->num_entries;
    }
    if (leaf->num_entries < C_l)
    {
        // If leaf is not full, insert R into it
        int i = leaf->num_entries - 1;
        while (i >= 0 && leaf->rects[i]->hilbertValue > R->hilbertValue)
        {
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
        while (p != NULL)
        {
            int max_lhv = -1;
            for (int i = 0; i < p->num_entries; i++)
            {
                if (p != leaf && p->children[i]->lhv > max_lhv)
                {
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
    else if (parent_of_leaf != NULL && sum_ele < M * parent_of_leaf->num_entries)
    {
        ELEMENT elements_sorted[sum_ele + 1];
        RECTANGLE rectangles_sorted[sum_ele + 1];
        elements_sorted[sum_ele] = &(R->low);
        rectangles_sorted[sum_ele] = R;
        int k1 = 0, k2 = 0;
        for (int i = 0; i < parent_of_leaf->num_entries; i++)
        {
            for (int j = 0; j < parent_of_leaf->children[i]->num_entries; j++)
            {
                elements_sorted[k1++] = parent_of_leaf->children[i]->elements[j];
                rectangles_sorted[k2++] = parent_of_leaf->children[i]->rects[j];
            }
        }
        int i = sum_ele;
        while (rectangles_sorted[i - 1]->hilbertValue > R->hilbertValue && i > 0)
        {
            elements_sorted[i] = elements_sorted[i - 1];
            rectangles_sorted[i] = rectangles_sorted[i - 1];
            i--;
        }
        elements_sorted[i] = &(R->low);
        rectangles_sorted[i] = R;
        k1 = 0, k2 = 0;
        for (int i = 0; i < parent_of_leaf->num_entries; i++)
        {
            int j = 0;
            for (; j < M && k1 < sum_ele + 1 && k2 < sum_ele + 1; j++)
            {
                parent_of_leaf->children[i]->rects[j] = rectangles_sorted[k1++];
                parent_of_leaf->children[i]->elements[j] = elements_sorted[k2++];
            }
            parent_of_leaf->children[i]->num_entries = j;
        }
    }
    else if (parent_of_leaf != NULL && sum_ele == M * parent_of_leaf->num_entries && parent_of_leaf->num_entries < M)
    {
        // parent_of_leaf->children[num_entries] = (NODE)malloc(sizeof(struct node));
        NODE *n = (NODE *)malloc(sizeof(NODE) * parent_of_leaf->num_entries);
        for (int i = 0; i < parent_of_leaf->num_entries; i++)
        {
            n[i] = parent_of_leaf->children[i];
        }
        // handleOverflow(parent_of_leaf->children[parent_of_leaf->num_entries-1],R);
        NODE nn = handleOverflow(leaf, R);
        if (nn != NULL)
            adjustTree(nn->parent, nn);
        // correct this:
    }
    else
    {
        NODE new_parent = handleOverflow(n, R);
        tree->root = new_parent;
    }
}

void adjustTree(NODE parent, NODE nn)
{
    // NODE sample_node = parent->children[0];

    // while (parent != NULL)
    // Root level
    if (parent == NULL)
        return;

    // Split parent here
    if (parent->num_entries == M)
    {
        NODE pp = handleOverflowNonLeaf(parent, nn);
        parent->lhv = 0;
        for (int j = 0; j < parent->num_entries; j++)
            parent->lhv = max(parent->lhv, parent->children[j]->lhv);
        pp->lhv = 0;
        for (int j = 0; j < pp->num_entries; j++)
            pp->lhv = max(pp->lhv, pp->children[j]->lhv);
        adjustTree(parent->parent, pp);
    }
    // Else fit NN in the parent
    else
    {
        int hVal = nn->lhv;
        int i;
        for (i = parent->num_entries - 1; i >= 0; i--)
        {
            if (parent->children[i]->lhv > hVal)
            {
                parent->children[i + 1] = parent->children[i];
                parent->rects[i + 1] = parent->rects[i];
            }
        }
        parent->children[i] = nn;
        parent->rects[i] = findMBR(nn);
        parent->num_entries++;
        parent->lhv = 0;
        for (int j = 0; j < parent->num_entries; j++)
            parent->lhv = max(parent->lhv, parent->children[j]->lhv);
    }
}

/* Helper function to adjust tree after an insertion */
// void adjustTree(NODE node) {
//     while (node != NULL) {
//         for (int i = 0; i < node->num_entries; i++) {
//             RECTANGLE R = node->rects[i];
//             int old_lhv = node->lhv;
//             if (i == 0 || R->hilbertValue > node->lhv) {
//                 node->lhv = R->hilbertValue;
//             }
//             if (old_lhv != node->lhv) {
//                 if (node->parent != NULL) {
//                     for (int j = 0; j < node->parent->num_entries; j++) {
//                         if (node->parent->children[j] == node) {
//                             node->parent->rects[j]->hilbertValue = node->lhv;
//                             break;
//                         }
//                     }
//                 }
//                 else {
//                     // Update the root node's LHV
//                     // node->parent = (NODE)malloc(sizeof(node));
//                     // node->parent->isLeaf = false;
//                     // node->parent->lhv = node->lhv;
//                     // node->parent->num_entries = 1;
//                     // node->parent->children[0] = node;
//                     // node->parent->rects[0] = R;
//                     // node->parent->rects[0]->hilbertValue = node->lhv;
//                     // node->parent->parent = NULL;
//                     // node = node->parent;
//                     break;
//                 }
//             }
//         }
//         node = node->parent;
//     }
// }

// void AdjustTree(NODE node, NODE new_node) {
//     NODE n = node;
//     while (n != NULL) {
//         if (n->parent == NULL) {
//             if (new_node != NULL) {
//                 // If the root node splits, create a new root and make it the parent of the split nodes
//                 NODE new_root = (NODE) malloc(sizeof(struct node));
//                 new_root->isLeaf = false;
//                 new_root->num_entries = 2;
//                 new_root->children[0] = n;
//                 new_root->children[1] = new_node;
//                 new_root->rects[0] = getMBR(n);
//                 new_root->rects[1] = getMBR(new_node);
//                 new_root->lhv = getMaxLHV(new_root);
//                 n->parent = new_root;
//                 new_node->parent = new_root;
//             }
//             return;
//         }

//         // Propagate node split upward
//         NODE parent = n->parent;
//         NODE new_parent = NULL;
//         if (new_node != NULL) {
//             RECTANGLE r = getUnion(getMBR(n), getMBR(new_node));
//             new_parent = parent->num_entries == M ? handleOverflow(parent, r) : parent;
//         }

//         // Adjust the MBRs and LHV in the parent level
//         for (int i = 0; i < parent->num_entries; i++) {
//             if (parent->children[i] == n) {
//                 parent->rects[i] = getMBR(n);
//                 parent->rects[i]->hilbertValue = n->lhv;
//             }
//         }
//         parent->lhv = getMaxLHV(parent);

//         // Move up to next level
//         n = parent;
//         new_node = new_parent;
//     }
// }

int calculateIncrease(rectangle R1, rectangle R2)
{
    // Calculate the minimum bounding rectangle of R1 and R2
    int x_min = min(R1.low.x, R2.low.x);
    int y_min = min(R1.low.y, R2.low.y);
    int x_max = max(R1.high.x, R2.high.x);
    int y_max = max(R1.high.y, R2.high.y);
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
    rectangle MBR = {{min(R1.low.x, R2.low.x), min(R1.low.y, R2.low.y)}, {max(R1.high.x, R2.high.x), max(R1.high.y, R2.high.y)}, 0};
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

void printHilbertTree(NODE root, int depth)
{
    if (root == NULL)
    {
        return;
    }

    // Print the current node
    for (int i = 0; i < depth; i++)
    {
        printf("  ");
    }
    printf("Node with %d entries:\n", root->num_entries);
    for (int i = 0; i < root->num_entries; i++)
    {
        for (int j = 0; j < depth; j++)
        {
            printf("  ");
        }
        printf("  Entry %d:\n", i);
        for (int j = 0; j < depth; j++)
        {
            printf("  ");
        }
        if (root->rects != NULL && root->rects[i] != NULL)
        {
            printf("    Rect: (%d, %d, %d, %d)\n", root->rects[i]->low.x, root->rects[i]->low.y, root->rects[i]->high.x, root->rects[i]->high.y);
        }
        else
        {
            printf("    Rect: NULL\n");
        }
        for (int j = 0; j < depth; j++)
        {
            printf("  ");
        }
        if (root->elements != NULL && root->elements[i] != NULL)
        {
            printf("    Element: %d %d\n", root->elements[i]->x, root->elements[i]->y);
        }
        else
        {
            printf("    Element: NULL\n");
        }
    }

    // Recursively print the children nodes
    for (int i = 0; i < root->num_entries + 1; i++)
    {
        if (root->children != NULL && root->children[i] != NULL)
        {
            printHilbertTree(root->children[i], depth + 1);
        }
    }
}

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
    void *item;
};

RTREE createNewRTree()
{
    RTREE newRTree = malloc(sizeof(rtree));
    newRTree->cnt = 0;
    newRTree->height = 0;
    newRTree->root = NULL;
}

// Returns true when rect2 is contained in rect
bool rect_contains(RECTANGLE rect, RECTANGLE rect2)
{
    if (rect2->low.x < rect->low.x || rect2->high.x > rect->high.x)
        return false;
    if (rect2->low.y < rect->low.y || rect2->high.y > rect->high.y)
        return false;

    return true;
}

// Returns true when rect2 intersects rect
bool rect_intersects(RECTANGLE rect, RECTANGLE rect2)
{
    if (rect2->low.x > rect->low.x || rect2->high.x < rect->high.x)
        return false;
    if (rect2->low.y > rect->low.y || rect2->high.y < rect->high.y)
        return false;

    return true;
}

void myPrint(char *hello, NODE hehe)
{
    printf("%s node: %p\n", hello, hehe);
    printf("%s is leaf node: %d\n", hello, hehe->isLeaf);
    printf("%s LHV: %d\n", hello, hehe->lhv);
    printf("Number of entries in %s: %d\n", hello, hehe->num_entries);
    printf("Children adresses of %s : \n", hello);
    for (int i = 0; i < M; i++)
        printf("\tChild %d : %p\n", i, hehe->children[i]);
    printf("Elements adresses of %s : \n", hello);
    for (int i = 0; i < M; i++)
        printf("\tElements %d : %p\n", i, hehe->elements[i]);
    for (int i = 0; i < hehe->num_entries; i++)
    {
        printf("   Entry %d:\n", i);
        printf("     Rect: ((%d, %d), (%d, %d))\n", hehe->rects[i]->low.x, hehe->rects[i]->low.y, hehe->rects[i]->high.x, hehe->rects[i]->high.y);
        printf("     LHV: %d\n", hehe->rects[i]->hilbertValue);
        if (hehe->isLeaf)
        {
            printf("     Element: (%d, %d)\n", hehe->elements[i]->x, hehe->elements[i]->y);
        }
        else
        {
        }
        printf("     Child node: %p\n", hehe->children[i]);
    }
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

    RECTANGLE r1 = (createNewRectangle(1, 9, 1, 9));
    insertRect(r1, tree.root, &tree);

    RECTANGLE r2 = (createNewRectangle(2, 20, 2, 20));
    insertRect(r2, tree.root, &tree);

    RECTANGLE r4 = (createNewRectangle(3, 20, 3, 20));
    insertRect(r4, tree.root, &tree);

    RECTANGLE r5 = (createNewRectangle(2, 10, 2, 10));
    insertRect(r5, tree.root, &tree);
    RECTANGLE r3 = (createNewRectangle(2, 19, 2, 19));
    insertRect(r3, tree.root, &tree);
    RECTANGLE r6 = (createNewRectangle(8, 5, 8, 5));
    insertRect(r6, tree.root, &tree);
    RECTANGLE r7 = createNewRectangle(4,5,4,5);
    insertRect(r7, tree.root, &tree);
    RECTANGLE r8 = createNewRectangle(3,4,3,4);
    insertRect(r8, tree.root, &tree);
    RECTANGLE r9 = createNewRectangle(3,5,3,5);
    insertRect(r9, tree.root, &tree);
    // RECTANGLE r10 = createNewRectangle(2,4,2,4);
    // insertRect(r10, tree.root, &tree);
    // RECTANGLE r10 = createNewRectangle(2,5,2,5);
    // insertRect(r10, tree.root, &tree);
    // RECTANGLE r10 = createNewRectangle(8,15,8,15);
    // insertRect(r10, tree.root, &tree);
    printf("Number of nodes: %d\n", tree.cnt);
    printf("Tree height: %d\n", tree.height);
    myPrint("Root", tree.root);
    myPrint("Child 1", tree.root->children[0]);
    myPrint("Child 2", tree.root->children[1]);
    // myPrint("Child 3", tree.root->children[2]);

    return 0;
}

/*
    (1,9) - 66
    (2,20) - 54
    (2,19) - 9
    (3,20) - 53
    (2,10) - 72
    (8,5) - 217
    (4,5) - 33
    (3, 4) - 53
    (3, 5) - 52
    (2, 4) - 54
    (2, 5) - 55
    (8, 15) - 149
    8 14
    7 15
    9 14
    9 15
    9 16
    9 17
    12 17
    11 18
    1 20
*/
