#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <string.h>
#include <stdint.h>
int k_k=0;
#define M 4
#define m 2
#define DIM 2
#define C_l 4
#define C_n 4
// Assumes coordinates are 32-bit signed integers
#define COORD_BITS 16                 // Number of bits used for each coordinate
#define HILBERT_BITS (COORD_BITS * 2) // Total number of bits in Hilbert curve



/********************************
    LEFTOVER WORK
    Count and Hieght of tree is not being updated
    MBRs karna hai
    Use this code for ppt
    Cleanup
*/
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
NODE chooseLeaf(RECTANGLE R, NODE n);
void insertRect(RECTANGLE R, NODE n, rtree *tree);
void adjustTree(NODE parent, NODE nn);  
NODE handleOverflowNonLeaf(NODE parent, NODE nn);
NODE hilbert_choose_sibling_non_leaf(NODE par, NODE nn);

rtree tree;
void rot(int n, int *x, int *y, int rx, int ry)
{
    if (ry == 0)
    {
        if (rx == 1)
        {
            *x = n - 1 - *x;
            *y = n - 1 - *y;
        }
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
    return xy2d(128, xmid, ymid);
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
            xmin = min(xmin, curr_node->rects[i]->low.x);
            ymin = min(ymin, curr_node->rects[i]->low.y);
            ymax = max(ymax, curr_node->rects[i]->high.y);
            xmax = max(xmax, curr_node->rects[i]->high.x);
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

NODE handleOverflowNonLeaf(NODE par, NODE nn)
{
    // if parent is root and not leaf
    if (par->parent == NULL)
    {
        NODE new_root = createNewNode(false);
        NODE uncle = createNewNode(false);

        NODE childs[5];

        for (int i = 0; i < par->num_entries; i++)
        {
            childs[i] = par->children[i];
        }

        // insert children according to lhv
        // rects[4] = r;
        childs[4] = nn;
        int i = 3;
        while (i >= 0 && childs[i]->lhv > nn->lhv)
        {
            childs[i + 1] = childs[i];
            i--;
        }
        childs[i + 1] = nn;

        int my_lhv = INT_MIN;
        // put 3 in par and 2 in uncle
        for (int i = 0; i < 3; i++)
        {
            par->children[i] = childs[i];
            if (childs[i]->lhv > my_lhv)
                my_lhv = childs[i]->lhv;
        }
        par->children[3] = NULL;
        par->num_entries = 3;
        par->lhv = my_lhv;

        my_lhv = INT_MIN;
        for (int i = 3; i < 5; i++)
        {
            uncle->children[i - 3] = childs[i];
            if (childs[i]->lhv > my_lhv)
                my_lhv = childs[i]->lhv;
        }
        uncle->num_entries = 2;
        uncle->lhv = my_lhv;
        uncle->children[2] = NULL;
        uncle->children[3] = NULL;

        // Set parent pointers
        for (int i = 0 ; i < par->num_entries ; i++)
            par->children[i]->parent = par;
        for (int i = 0 ; i < uncle->num_entries ; i++)
            uncle->children[i]->parent = uncle;

        return uncle;
    }

    else
    {
        NODE coop_sibling = hilbert_choose_sibling_non_leaf(par, nn);

        // Need to create new parent
        if (coop_sibling->num_entries==4)
        {
            NODE pp = createNewNode(false);
            NODE childs[5];

            //Store all children in childs[] array
            for (int i = 0; i < M; i++)
            {
                childs[i] = par->children[i];
            }
            childs[4] = nn;

            //insert nn into childs sorted
            int j = 3;
            while (j >= 0 && childs[j]->lhv > nn->lhv)
            {
                childs[j + 1] = childs[j];
                j--;
            }
            childs[j + 1] = nn;

            int my_lhv = INT_MIN;
            // put 3 in par and 2 in pp
            for (int i = 0; i < 3; i++)
            {
                par->children[i] = childs[i];
                if (childs[i]->lhv > my_lhv)
                    my_lhv = childs[i]->lhv;
            }
            par->children[3] = NULL;
            par->num_entries = 3;
            par->lhv = my_lhv;

            my_lhv = INT_MIN;
            for (int i = 3; i < 5; i++)
            {
                pp->children[i - 3] = childs[i];
                if (childs[i]->lhv > my_lhv)
                    my_lhv = childs[i]->lhv;
            }
            pp->num_entries = 2;
            pp->lhv = my_lhv;
            pp->children[2] = NULL;
            pp->children[3] = NULL;

            // Set parent pointers 
            for (int i = 0; i < par->num_entries; i++)
                par->children[i]->parent = par;
            for (int i = 0; i < pp->num_entries; i++)
                pp->children[i]->parent = pp;

            return pp;
        }

        // Accomodate into sibling 
        else
        {
            int first_rearrange = (par->num_entries + coop_sibling->num_entries + 1) / 2;
            int second_rearrange = par->num_entries + coop_sibling->num_entries + 1 - first_rearrange;

            NODE childs[par->num_entries + coop_sibling->num_entries + 1];
            
            // Store all children in childs[] array

            if (par->lhv < coop_sibling->lhv)
            {
                for (int i = 0; i < par->num_entries; i++)
                    childs[i] = par->children[i];
                
                for (int i = 0; i < coop_sibling->num_entries; i++)
                    childs[i + par->num_entries] = coop_sibling->children[i];

                childs[par->num_entries + coop_sibling->num_entries] = nn;
            }
            else
            {
                for (int i = 0; i < coop_sibling->num_entries; i++)
                    childs[i] = coop_sibling->children[i];

                for (int i = 0; i < par->num_entries; i++)
                    childs[i + coop_sibling->num_entries] = par->children[i];

                childs[par->num_entries + coop_sibling->num_entries] = nn;
            }

            int j = par->num_entries + coop_sibling->num_entries - 1;
            while (j >= 0 && nn->lhv < childs[j]->lhv)
            {
                childs[j+1] = childs[j];
                j--;
            }
            childs[j+1] = nn;

            if (par->lhv < coop_sibling->lhv)
            {
                int lhv = INT_MIN;
                for (int i = 0; i < first_rearrange; i++)
                {
                    par->children[i] = childs[i];
                    if (childs[i]->lhv > lhv)
                        lhv = childs[i]->lhv;
                }
                par->num_entries = first_rearrange;
                par->lhv = lhv;

                lhv = INT_MIN;
                for (int i = 0; i < second_rearrange; i++)
                {
                    coop_sibling->children[i] = childs[first_rearrange + i];
                    if (childs[first_rearrange + i]->lhv > lhv)
                        lhv = childs[first_rearrange + i]->lhv;
                }
                coop_sibling->num_entries = second_rearrange;
                coop_sibling->lhv = lhv;
                
            }

            else
            {
                int lhv = INT_MIN;
                for (int i = 0; i < first_rearrange; i++)
                {
                    coop_sibling->children[i] = childs[i];
                    if (childs[i]->lhv > lhv)
                        lhv = childs[i]->lhv;
                }
                coop_sibling->num_entries = first_rearrange;
                coop_sibling->lhv = lhv;

                lhv = INT_MIN;
                for (int i = 0; i < second_rearrange; i++)
                {
                    par->children[i] = childs[first_rearrange + i];
                    if (childs[first_rearrange + i]->lhv > lhv)
                        lhv = childs[first_rearrange + i]->lhv;
                }
                par->num_entries = second_rearrange;
                par->lhv = lhv;
            }

            // Set parent pointers
            for (int i = 0; i < par->num_entries; i++)
                par->children[i]->parent = par;
            for (int i = 0; i < coop_sibling->num_entries; i++)
                coop_sibling->children[i]->parent = coop_sibling;

            return NULL;
        }
    }
}

NODE handleOverflow(NODE n, RECTANGLE r)
{
    // It is root but leaf so split has to occur
    if (n->parent == NULL)
    {
        NODE nn = createNewNode(true);

        RECTANGLE rects[5];

        for (int i = 0; i < n->num_entries; i++)
        {
            rects[i] = n->rects[i];
        }

        // insert r into rects according to h value
        rects[4] = r;
        int j = 3;
        while (j >= 0 && rects[j]->hilbertValue > r->hilbertValue)
        {
            rects[j + 1] = rects[j];
            j--;
        }
        rects[j + 1] = r;

        int lhv = INT_MIN;
        // put 3 in n and 2 in nn
        for (int i = 0; i < 3; i++)
        {
            n->rects[i] = rects[i];
            if (rects[i]->hilbertValue > lhv)
                lhv = rects[i]->hilbertValue;
        }
        n->rects[3] = NULL;
        n->num_entries = 3;
        n->lhv = lhv;
        lhv = INT_MIN;
        for (int i = 3; i < 5; i++)
        {
            nn->rects[i - 3] = rects[i];
            if (rects[i]->hilbertValue > lhv)
                lhv = rects[i]->hilbertValue;
        }
        nn->num_entries = 2;
        nn->lhv = lhv;
        nn->rects[2] = NULL;
        nn->rects[3] = NULL;
        return nn;
    }

    else
    {
        NODE parent = n->parent;
        NODE coop_sibling = hilbert_choose_sibling(n, r); // this function is not verified to be correct
        // check it

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
        int i = n->num_entries + coop_sibling->num_entries - 1;
        while (i >= 0 && r->hilbertValue < rects[i]->hilbertValue)
        {
            rects[i + 1] = rects[i];
            i--;
        }
        rects[i + 1] = r;

        // If coop sibling is also full then split
        if (coop_sibling->num_entries == M)
        {
            if (coop_sibling->lhv > n->lhv)
            {
                NODE nn = createNewNode(n->isLeaf);
                int lhv = INT_MIN;

                // Initialize rects of nn to null
                for (int i = 0; i < M; i++)
                    nn->rects[i] = NULL;

                for (int i = 0; i < 3; i++)
                {
                    n->rects[i] = rects[i];
                    if (rects[i]->hilbertValue > lhv)
                        lhv = rects[i]->hilbertValue;
                }
                n->num_entries = 3;
                n->lhv = lhv;

                lhv = INT_MIN;
                for (int i = 3; i < 6; i++)
                {
                    coop_sibling->rects[i - 3] = rects[i];
                    if (rects[i]->hilbertValue > lhv)
                        lhv = rects[i]->hilbertValue;
                }
                coop_sibling->num_entries = 3;
                coop_sibling->lhv = lhv;

                lhv = INT_MIN;
                for (int i = 6; i < 9; i++)
                {
                    nn->rects[i - 6] = rects[i];
                    if (rects[i]->hilbertValue > lhv)
                        lhv = rects[i]->hilbertValue;
                }
                nn->num_entries = 3;
                nn->lhv = lhv;

                return nn;
            }
            else
            {
                NODE nn = createNewNode(n->isLeaf);

                // Initialize rects of nn to null
                for (int i = 0; i < M; i++)
                    nn->rects[i] = NULL;

                int lhv = INT_MIN;

                for (int i = 0; i < 3; i++)
                {
                    coop_sibling->rects[i] = rects[i];
                    if (rects[i]->hilbertValue > lhv)
                        lhv = rects[i]->hilbertValue;
                }
                coop_sibling->num_entries = 3;
                coop_sibling->rects[3] = NULL;
                coop_sibling->children[3] = NULL;
                coop_sibling->lhv = lhv;

                lhv = INT_MIN;
                for (int i = 3; i < 6; i++)
                {
                    n->rects[i - 3] = rects[i];
                    if (rects[i]->hilbertValue > lhv)
                        lhv = rects[i]->hilbertValue;
                }
                n->num_entries = 3;
                n->lhv = lhv;
                n->rects[3] = NULL;
                n->children[3] = NULL;
                lhv = INT_MIN;
                for (int i = 6; i < 9; i++)
                {
                    nn->rects[i - 6] = rects[i];
                    if (rects[i]->hilbertValue > lhv)
                        lhv = rects[i]->hilbertValue;
                }
                nn->num_entries = 3;
                nn->lhv = lhv;
                nn->rects[3] = NULL;
                nn->children[3] = NULL;

                return nn;
            }
        }

        // Rearrange entries
        else
        {
            // Number of elements that go in the first and second node respectively
            int first_rearrange = (n->num_entries + coop_sibling->num_entries + 1) / 2;
            int second_rearrange = n->num_entries + coop_sibling->num_entries + 1 - first_rearrange;

            // ONLY WORKS FOR LEAF NODES
            if (coop_sibling->lhv > n->lhv)
            {
                int lhv = INT_MIN;
                for (int i = 0; i < first_rearrange; i++)
                {
                    n->rects[i] = rects[i];
                    if (rects[i]->hilbertValue > lhv)
                        lhv = rects[i]->hilbertValue;
                }
                n->num_entries = first_rearrange;
                n->lhv = lhv;

                lhv = INT_MIN;
                for (int i = 0; i < second_rearrange; i++)
                {
                    coop_sibling->rects[i] = rects[first_rearrange + i];
                    if (rects[first_rearrange + i]->hilbertValue > lhv)
                        lhv = rects[first_rearrange + i]->hilbertValue;
                }
                coop_sibling->num_entries = second_rearrange;
                coop_sibling->lhv = lhv;
            }

            else
            {
                int lhv = INT_MIN;
                for (int i = 0; i < first_rearrange; i++)
                {
                    coop_sibling->rects[i] = rects[i];
                    if (rects[i]->hilbertValue > lhv)
                        lhv = rects[i]->hilbertValue;
                }
                coop_sibling->num_entries = first_rearrange;
                coop_sibling->lhv = lhv;

                lhv = INT_MIN;
                for (int i = 0; i < second_rearrange; i++)
                {
                    n->rects[i] = rects[first_rearrange + i];
                    if (rects[first_rearrange + i]->hilbertValue > lhv)
                        lhv = rects[first_rearrange + i]->hilbertValue;
                }

                n->num_entries = second_rearrange;
                n->lhv = lhv;
            }

            return NULL;
        }
    }
}

// Choose sibling functions but for non leaf nodes
NODE hilbert_choose_sibling_non_leaf(NODE par, NODE nn)
{
    int node_par_index;
    NODE coop_uncle = (NODE)malloc(sizeof(struct node));
    NODE grand_parent = par->parent;
    NODE left_uncle = NULL;
    NODE right_uncle = NULL;

    for (int i = 0; grand_parent != NULL && i < grand_parent->num_entries; i++)
    {
        if (par == grand_parent->children[i])
            node_par_index = i;
    }
    if (node_par_index > 0)
        left_uncle = grand_parent->children[node_par_index - 1];
    if (node_par_index < 3)
        right_uncle = grand_parent->children[node_par_index + 1];

    NODE *childs = par->children;
    int median_hv = par->children[par->num_entries / 2]->lhv;
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

// Choose the cooperating sibling for the overflowing node for the case isLeaf true
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

NODE chooseLeaf(RECTANGLE r, NODE root)
{
    NODE N = root;

    if (N->isLeaf)
        return N;

    int chosen_entry = N->num_entries - 1; // assuming all entries are sorted
    int max_lhv = INT_MAX;
    for (int i = 0; i < N->num_entries; i++)
    {
        if (N->children[i]->lhv > r->hilbertValue)
        {
            if (N->children[i]->lhv < max_lhv)
            {
                chosen_entry = i;
                max_lhv = N->children[i]->lhv;
            }
        }
    }

    NODE chosen = chooseLeaf(r, N->children[chosen_entry]);
    return chosen;
}

void insertRect(RECTANGLE r, NODE root, RTREE tree)
{
    NODE leaf = chooseLeaf(r, root);

    // Leaf full
    if (leaf->num_entries == M)
    {
        NODE new_leaf = handleOverflow(leaf, r);

        // No split occured if new_leaf is null
        if (!new_leaf)
            return;

        // Split occured
        else
        {
            adjustTree(leaf->parent, new_leaf);

            // Root but leaf split
            if (leaf->parent == NULL)
            {
                NODE new_root = createNewNode(false);

                if (leaf->lhv < new_leaf->lhv)
                {
                    new_root->children[0] = leaf;
                    new_root->children[1] = new_leaf;

                    leaf->parent = new_root;
                    new_leaf->parent = new_root;
                }

                else
                {
                    new_root->children[0] = new_leaf;
                    new_root->children[1] = leaf;

                    new_leaf->parent = new_root;
                    leaf->parent = new_root;
                }

                new_root->lhv = max(new_root->children[0]->lhv, new_root->children[1]->lhv);
                new_root->num_entries = 2;
                tree->root = new_root;
                tree->height++;
                tree->cnt++;
            }
        }
    }

    // Leaf not full: insert directly into leaf
    else
    {
        leaf->rects[leaf->num_entries] = r;
        int i = leaf->num_entries - 1;
        while (i >= 0 && leaf->rects[i]->hilbertValue > r->hilbertValue)
        {
            leaf->rects[i + 1] = leaf->rects[i];
            i--;
        }
        leaf->rects[i + 1] = r;
        leaf->num_entries++;

        if (r->hilbertValue > leaf->lhv)
            leaf->lhv = r->hilbertValue;
    }
}

void adjustTree(NODE parent, NODE nn)
{
    if (parent == NULL)
        return;

    // Overflow
    if (parent->num_entries == M)
    {
        NODE pp = handleOverflowNonLeaf(parent, nn);

        // If pp was created
        if (pp)
        {
            parent->lhv = 0;
            for (int j = 0; j < parent->num_entries; j++)
                parent->lhv = max(parent->lhv, parent->children[j]->lhv);
            pp->lhv = 0;
            for (int j = 0; j < pp->num_entries; j++)
                pp->lhv = max(pp->lhv, pp->children[j]->lhv);
            for(int i = 0; i< parent->num_entries; i++){
                parent->rects[i] = findMBR(parent->children[i]);
            }
            for(int i = 0; i< pp->num_entries; i++){
                pp->rects[i] = findMBR(pp->children[i]);
            }
            if (parent->parent)
                adjustTree(parent->parent, pp);

            // Initiate root split
            else
            {
                NODE new_root = createNewNode(false);
                if (pp->lhv < parent->lhv)
                {
                    new_root->children[0] = pp;
                    new_root->children[1] = parent;
                    new_root->rects[0] = findMBR(pp);
                    new_root->rects[1] = findMBR(parent);
                }
                else
                {
                    new_root->children[0] = parent;
                    new_root->children[1] = pp;
                    new_root->rects[0] = findMBR(parent);
                    new_root->rects[1] = findMBR(pp);
                }
                
                tree.root = new_root;
                tree.height++;
                tree.cnt++;
                parent->parent = new_root;
                pp->parent = new_root;
                new_root->num_entries = 2;
                tree.root = new_root;
            }
        }
        // Overflow handled without splitting
    }
    // Else fit NN in the parent
    else
    {
        int i = parent->num_entries - 1;
        parent->children[parent->num_entries] = nn;
        while (i >= 0 && parent->children[i]->lhv > nn->lhv)
        {
            parent->children[i + 1] = parent->children[i];
            i--;
        }
        for(int i = 0; i< parent->num_entries; i++){
            parent->rects[i] = findMBR(parent->children[i]);
        }
        if(parent->parent!=NULL){
            NODE grand_parent = parent->parent;
            for(int i=0; i<grand_parent->num_entries; i++){
                if(grand_parent->children[i]==parent){
                    grand_parent->rects[i] = findMBR(parent);
                }
            }
        }
        parent->children[i + 1] = nn;
        parent->num_entries++;
        nn->parent = parent;
        if (parent->lhv < nn->lhv)
            parent->lhv = nn->lhv;
    }
}

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

    for (int i = 0; i < root->num_entries; i++)
    {
        RECTANGLE rect = root->rects[i];
        if (rect)
            printf("(%d,%d), (%d,%d)\n", rect->high.x, rect->high.y, rect->low.x, rect->low.y);
    }

    if (!root->isLeaf)
    {
        for (int i = 0; i < root->num_entries; i++)
        {
            NODE child = root->children[i];
            if (child)
                preOrderTraversal(child);
        }
    }
}

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


void myPrint(char *hello, NODE hehe, FILE* hoola)
{
    // printf("%s node: %p\n", hello, hehe);
    // printf("%s Parent : %p\n", hello, hehe->parent);
    // printf("%s is leaf node: %d\n", hello, hehe->isLeaf);
    // printf("%s LHV: %d\n", hello, hehe->lhv);
    printf("Number of entries in %s: %d\n", hello, hehe->num_entries);
    if (!hehe->isLeaf)
    {
        printf("Children/Elements adresses of %s : \n", hello);
        for (int i = 0; i < M; i++)
            printf("\tChild/Ele %d : %p\n", i, hehe->children[i]);
    }
    else
    {
        printf("Woza nigga hehe : \t\t\t\t\t%d\n",++k_k);
        // printf("My LHV : %d\n", hehe->lhv);
        printf("Rectangle adresses of %s : \n", hello);
        for (int i = 0; i < M; i++)
            printf("\tRect %d : %p\n", i, hehe->rects[i]);

        printf("Rectangles of %s : \n", hello);
        for (int i = 0; i < hehe->num_entries; i++)
        {
            printf("\tRect %d\n", i);
            printf("\t\tCoords : (%d,%d)\n", hehe->rects[i]->high.x, hehe->rects[i]->high.y);
            printf("\t\tHilberVal : %d\n", hehe->rects[i]->hilbertValue);
            // fprintf(hoola, "%d, %d, %d\n", hehe->rects[i]->high.x, hehe->rects[i]->high.y, hehe->rects[i]->hilbertValue);
        }
    }
    // for (int i = 0; i < hehe->num_entries; i++)
    // {
    //     printf("   Entry %d:\n", i);
    //     // printf("     Rect: ((%d, %d), (%d, %d))\n", hehe->rects[i]->low.x, hehe->rects[i]->low.y, hehe->rects[i]->high.x, hehe->rects[i]->high.y);
    //     printf("     LHV: %d\n", hehe->rects[i]->hilbertValue);
    //     if (hehe->isLeaf)
    //     {
    //         printf("     Element: (%d, %d)\n", hehe->elements[i]->x, hehe->elements[i]->y);
    //     }
    //     // else
    //     // {
    //     // }
    //     printf("     Child node: %p\n", hehe->children[i]);
    // }
}


void hehePrint(NODE root, FILE* hoola)
{
    printf("\t\t\t\t\t Hehe beitch ass bitch : %d\n", root->lhv);

    if (!root->isLeaf)
    {
        for (int i = 0 ; i < root->num_entries ; i++)
        {
            hehePrint(root->children[i], hoola);
        }
    }
    else
    {
        myPrint("ko", root, hoola);
    }

}
int main(int argc, char const *argv[])
{
    tree.cnt = 0;
    tree.height = 1;
    tree.root = malloc(sizeof(node));
    tree.root->isLeaf = true;
    tree.root->num_entries = 0;
    tree.root->lhv = -1;
    tree.root->parent = NULL;

    FILE* fptr = fopen("okoll.txt", "r");
    int x,y;
    int i = 1;
    RECTANGLE nonoono;
    while (fscanf (fptr, "%d %d\n", &x, &y) != EOF)
    {
        i++;
        // if (i == ) break;
        // else printf("heh\n");
        // printf("x,y : %d, %d i : %d\n", x, y, i++);
        // if (i == 30) myPrint("Root", tree.root);
        // if (i == 30) myPrint("Child 1", tree.root->children[0]);
        // if (i == 30) myPrint("Chi;d 2", tree.root->children[1]);
        nonoono = createNewRectangle(x,y,x,y);
        insertRect(nonoono, tree.root, &tree);
        // printf("Hval of the zamn node is  : %d\n", nonoono->hilbertValue);
    }
    // myPrint("Root", tree.root);
    // myPrint("Child 1", tree.root->children[0]);
    // myPrint("Child 2", tree.root->children[1]);

    fclose(fptr);
    // fptr = fopen("bleh.csv", "w");
    // hehePrint(tree.root, fptr);
    preOrderTraversal(tree.root);
    // fclose(fptr);

    printf("isvnof %d \n", tree.root->children[1]->lhv);



    return 0;
}

/*
    // 1. (1,9) - 66
    // 2. (2,20) - 54 ok
    // 3. (2,19) - 9 ok
    // 4. (3,20) - 53
    // 5. (2,10) - 72
    // 6. (8,5) - 217
    // 7. (4,5) - 33
    // 8. (3, 4) - 53 ok
    // 9. (3, 5) - 52 ok
    // 10. (2, 4) - 54 ok
    // 11. (2, 5) - 55 ok
    // 12. (8, 15) - 149
    // 13. 8 14 - 148
    // 14. 7 15 - 106
    // 15. 9 14 - 151
    // 16. 9 15 - 150
    17. 9 16 - 235
    18. 9 17 - 232
    19. 12 17 - 243
    20. 11 18 - 225
    21. 1 20 - 57
*/

/*
877422 134867
877432 134867
877412 134887
877509 135014
877422 135014
877509 134877
877335 134740
877422 134740
877335 134877
877509 134740
877335 135014
877559 134964
877559 134877
877422 134964
877285 134790
285152 520229
285162 520239
285162 520229
285152 520239
285142 520219
285142 520229
285152 520219
285162 520219
285142 520239
285239 520366
285152 520366
285239 520229
285065 520092
285152 520092
285065 520229
285239 520092
285065 520366
285289 520316
285289 520229
285152 520316
285015 520142
333378 557892
333388 557902
333388 557892
333378 557902
333368 557882
333368 557892
333378 557882
333388 557882
333368 557902
333465 558029
333378 558029
333465 557892
333291 557755
333378 557755
333291 557892
333465 557755
333291 558029
333515 557979
333515 557892
333378 557979
333241 557805
321924 527106
321934 527116
321934 527106
321924 527116
321914 527096
321914 527106
321924 527096
321934 527096
321914 527116
322011 527243
321924 527243
322011 527106
321837 526969
321924 526969
321837 527106
322011 526969
321837 527243
322061 527193
322061 527106
321924 527193
321787 527019
636628 533309
636638 533319
636638 533309
636628 533319
636618 533299
636618 533309
636628 533299
636638 533299
636618 533319
636715 533446
*/