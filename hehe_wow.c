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

        // insert r into rects according to h value
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
        // put 3 in parent and 2 in uncl
        for (int i = 0; i < 3; i++)
        {
            par->children[i] = childs[i];
            if (childs[i]->lhv > my_lhv)
                my_lhv = childs[i]->lhv;
        }
        par->rects[3] = NULL;
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
        // tree.root = new_root;
        // uncle->parent = new_root;
        // par->parent = new_root;
        // new_root->children[0] = par;
        // new_root->children[1] = uncle;
        // new_root->num_entries = 2;
        // nn->parent = uncle;;
        for (int i = 0 ; i < par->num_entries ; i++)
            par->children[i]->parent = par;
        for (int i = 0 ; i < uncle->num_entries ; i++)
            uncle->children[i]->parent = uncle;

        return uncle;
    }

    NODE uncle = (NODE)malloc(sizeof(struct node));
    NODE final_boss = par->parent;
    NODE coop_sibling_non_leaf = hilbert_choose_sibling_non_leaf(par, nn);

    int sum_ele = par->num_entries + coop_sibling_non_leaf->num_entries;
    // for (int i = 0; i < final_boss->num_entries; i++)
    // {
    //     sum_ele += final_boss->children[i]->num_entries;
    // }
    // if (sum_ele == M * 2 && final_boss->num_entries < M)
    // {
    //     final_boss->children[final_boss->num_entries] = uncle;
    // }
    NODE *sorted_nodes = (NODE *)malloc(sizeof(NODE) * (sum_ele + 1));
    // RECTANGLE *sorted_rects = (RECTANGLE *)malloc(sizeof(RECTANGLE) * (sum_ele + 1));
    sorted_nodes[sum_ele] = nn;
    // sorted_rects[sum_ele] = findMBR(nn);
    int k = 0;
    if (coop_sibling_non_leaf->lhv > par->lhv)
    {
        for (int j = 0; j < par->num_entries && k < sum_ele; j++)
        {
            sorted_nodes[k++] = par->children[j];
            // sorted_rects[k++] = par->rects[j];
        }
        for (int j = 0; j < coop_sibling_non_leaf->num_entries && k < sum_ele; j++)
        {
            sorted_nodes[k++] = coop_sibling_non_leaf->children[j];
            // sorted_rects[k++] = coop_sibling_non_leaf->rects[j];
        }
    }
    else
    {
        for (int j = 0; j < coop_sibling_non_leaf->num_entries && k < sum_ele; j++)
        {
            sorted_nodes[k++] = coop_sibling_non_leaf->children[j];
            // sorted_rects[k++] = coop_sibling_non_leaf->rects[j];
        }
        for (int j = 0; j < par->num_entries && k < sum_ele; j++)
        {
            sorted_nodes[k++] = par->children[j];
            // sorted_rects[k++] = par->rects[j];
        }
    }
    int i = sum_ele;
    while (i > 0 && sorted_nodes[i - 1]->lhv > nn->lhv)
    {
        sorted_nodes[i] = sorted_nodes[i - 1];
        // sorted_rects[i] = sorted_rects[i - 1];
        i--;
    }
    sorted_nodes[i] = nn;
    // sorted_rects[i] = findMBR(nn);
    if (sum_ele == M * 2)
    {

        if (coop_sibling_non_leaf->lhv > par->lhv)
        {
            for (int i = 0; i < 3; i++)
            {
                par->children[i] = sorted_nodes[i];
                par->children[i]->parent = par;
                // par->rects[i] = sorted_rects[i];
            }           
            for (int i = 0; i < 3; i++)
            {
                coop_sibling_non_leaf->children[i] = sorted_nodes[i + 3];
                coop_sibling_non_leaf->children[i]->parent = coop_sibling_non_leaf;
                // coop_sibling_non_leaf->rects[i] = sorted_rects[i + 3];
            }
            for (int i = 0; i < 3; i++)
            {
                uncle->children[i] = sorted_nodes[i + 6];
                uncle->children[i]->parent = uncle;
                // uncle->rects[i] = sorted_rects[i + 6];
            }
            uncle->parent = par->parent;
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
                coop_sibling_non_leaf->children[i]->parent = coop_sibling_non_leaf;
                // coop_sibling_non_leaf->rects[i] = sorted_rects[i];
            }
            for (int i = 0; i < 3; i++)
            {
                par->children[i] = sorted_nodes[i + 3];
                par->children[i]->parent = par;
                // par->rects[i] = sorted_rects[i + 3];
            }
            for (int i = 0; i < 3; i++)
            {
                uncle->children[i] = sorted_nodes[i + 6];
                uncle->children[i]->parent = uncle;
                // uncle->rects[i] = sorted_rects[i + 6];
            }
            uncle->parent = par->parent;
            uncle->num_entries = 3;
            uncle->isLeaf = false;
            uncle->lhv = max(uncle->children[0]->lhv, max(uncle->children[1]->lhv, uncle->children[2]->lhv));
        }
        par->num_entries = 3;
        coop_sibling_non_leaf->num_entries = 3;
        return uncle;
    }
    else
    {
        if (coop_sibling_non_leaf->lhv > par->lhv)
        {

            for (int i = 0; i < 4; i++)
            {
                par->children[i] = sorted_nodes[i];
                par->children[i]->parent = par;
                // par->rects[i] = sorted_rects[i];
            }
            for (int i = 0; i < sum_ele - 3; i++)
            {
                coop_sibling_non_leaf->children[i] = sorted_nodes[i + 4];
                coop_sibling_non_leaf->children[i]->parent = coop_sibling_non_leaf;
                // coop_sibling_non_leaf->rects[i] = sorted_rects[i + 4];
            }
        }
        else
        {
            for (int i = 0; i < 4; i++)
            {
                coop_sibling_non_leaf->children[i] = sorted_nodes[i];
                coop_sibling_non_leaf->children[i]->parent = coop_sibling_non_leaf;
                // coop_sibling_non_leaf->rects[i] = sorted_rects[i];
            }
            for (int i = 0; i < sum_ele - 3; i++)
            {
                par->children[i] = sorted_nodes[i + 4];
                par->children[i]->parent = par;
                // par->rects[i] = sorted_rects[i + 4];
            }
        }
        par->num_entries = 4;
        coop_sibling_non_leaf->num_entries = sum_ele - 3;
        return NULL;
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
        int i = 3;
        while (i >= 0 && rects[i]->hilbertValue > r->hilbertValue)
        {
            rects[i + 1] = rects[i];
            i--;
        }
        rects[i + 1] = r;

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
        int i = n->num_entries + coop_sibling->num_entries - 1;
        while (i > 0 && r->hilbertValue < rects[i]->hilbertValue)
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

NODE hilbert_choose_sibling_non_leaf(NODE par, NODE nn)
{
    int node_par_index;
    NODE coop_uncle = (NODE)malloc(sizeof(struct node));
    NODE grand_parent = par->parent;
    NODE left_uncle = NULL;
    NODE right_uncle = NULL;

    printf("hehe leaf : %p : %d\n", grand_parent, node_par_index);
    printf("hehe leaf : %p : %d\n", par, node_par_index);

    // if (grand_parent != NULL)
    // {

    // }
    for (int i = 0; grand_parent != NULL && i < grand_parent->num_entries; i++)
    {
        if (par == grand_parent->children[i])
            node_par_index = i;
    }
    printf("hehe leaf : %p : %d\n", nn, node_par_index);
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
    // Root level leaf
            // if (nn->lhv == 151) printf("\n\n\nhehe\n %p\n", nn->parent);
        // nn->parent = parent;

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
                }
                else
                {
                    new_root->children[0] = parent;
                    new_root->children[1] = pp;
                }
                tree.root = new_root;
                tree.height++;
                tree.cnt++;
                parent->parent = new_root;
                pp->parent = new_root;
                new_root->num_entries = 2;
                tree.root = new_root;
                // uncle->parent = new_root;
                // par->parent = new_root;
                // new_root->children[0] = par;
                // new_root->children[1] = uncle;
                // new_root->num_entries = 2;
            }
        }

        // Overflow handled without splittin
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
        parent->children[i + 1] = nn;
        parent->num_entries++;
        nn->parent = parent;
        if (parent->lhv < nn->lhv)
            parent->lhv = nn->lhv;
    }
}

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
    printf("%s Parent : %p\n", hello, hehe->parent);

    printf("%s is leaf node: %d\n", hello, hehe->isLeaf);
    printf("%s LHV: %d\n", hello, hehe->lhv);
    printf("Number of entries in %s: %d\n", hello, hehe->num_entries);
    // if (!hehe->isLeaf)
    // {
    //     printf("Children/Elements adresses of %s : \n", hello);
    //     for (int i = 0; i < M; i++)
    //         printf("\tChild/Ele %d : %p\n", i, hehe->children[i]);
    // }
    // else
    // {
    //     printf("Rectangle adresses of %s : \n", hello);
    //     for (int i = 0; i < M; i++)
    //         printf("\tRect %d : %p\n", i, hehe->rects[i]);

    //     printf("Rectangles of %s : \n", hello);
    //     for (int i = 0; i < hehe->num_entries; i++)
    //     {
    //         printf("\tRect %d\n", i);
    //         printf("\t\tCoords : (%d,%d)\n", hehe->rects[i]->high.x, hehe->rects[i]->high.y);
    //         printf("\t\tHilberVal : %d\n", hehe->rects[i]->hilbertValue);
    //     }
    // }
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

int main(int argc, char const *argv[])
{
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
    RECTANGLE r3 = (createNewRectangle(2, 19, 2, 19));
    insertRect(r3, tree.root, &tree);
    RECTANGLE r4 = (createNewRectangle(3, 20, 3, 20));
    insertRect(r4, tree.root, &tree);

    RECTANGLE r5 = (createNewRectangle(2, 10, 2, 10));
    insertRect(r5, tree.root, &tree);
    RECTANGLE r6 = (createNewRectangle(8, 5, 8, 5));
    insertRect(r6, tree.root, &tree);
    RECTANGLE r7 = createNewRectangle(4, 5, 4, 5);
    insertRect(r7, tree.root, &tree);
    RECTANGLE r8 = createNewRectangle(3, 4, 3, 4);
    insertRect(r8, tree.root, &tree);
    RECTANGLE r9 = createNewRectangle(3, 5, 3, 5);
    insertRect(r9, tree.root, &tree);
    RECTANGLE r10 = createNewRectangle(2, 4, 2, 4);
    insertRect(r10, tree.root, &tree);
    RECTANGLE r11 = createNewRectangle(2, 5, 2, 5);
    insertRect(r11, tree.root, &tree);

    RECTANGLE r12 = createNewRectangle(8, 15, 8, 15);
    insertRect(r12, tree.root, &tree);
    RECTANGLE r13 = createNewRectangle(8, 14, 8, 14);
    insertRect(r13, tree.root, &tree);
    RECTANGLE r14 = createNewRectangle(7, 15, 7, 15);
    insertRect(r14, tree.root, &tree);
    // myPrint("Root", tree.root);
    // myPrint("Child 1", tree.root->children[0]);
    // myPrint("Child 2", tree.root->children[1]);
    // myPrint("Child 3", tree.root->children[2]);
    // myPrint("Child 4", tree.root->children[3]);
    RECTANGLE r15 = createNewRectangle(9, 14, 9, 14);
    insertRect(r15, tree.root, &tree);
    RECTANGLE r16 = createNewRectangle(9, 15, 9, 15);
    insertRect(r16, tree.root, &tree);
    RECTANGLE r17 = createNewRectangle(9, 16, 9, 16);
    insertRect(r17, tree.root, &tree);
    RECTANGLE r18 = createNewRectangle(9,17,9,17);
    insertRect(r18, tree.root, &tree);
    RECTANGLE r19 = createNewRectangle(12,17,12,17);
    insertRect(r19, tree.root, &tree);
    RECTANGLE r20 = createNewRectangle(11,18,11,18);
    insertRect(r20, tree.root, &tree);
    RECTANGLE r21 = createNewRectangle(1,20,1,20);
    insertRect(r21, tree.root, &tree);
    RECTANGLE r22 = createNewRectangle(1,2,1,2);
    insertRect(r22, tree.root, &tree);
    RECTANGLE r23 = createNewRectangle(5,18,5,18);
    insertRect(r23, tree.root, &tree);
    RECTANGLE r24 = createNewRectangle(19,20,19,20);
    insertRect(r24, tree.root, &tree);
    RECTANGLE r25 = createNewRectangle(10,2,10,2);
    insertRect(r25, tree.root, &tree);
    RECTANGLE r26 = createNewRectangle(14,17,14,17);
    insertRect(r26, tree.root, &tree);
    RECTANGLE r27 = createNewRectangle(11,16,11,16);
    insertRect(r27, tree.root, &tree);
    RECTANGLE r28 = createNewRectangle(5,1,5,1);
    insertRect(r28, tree.root, &tree);
    RECTANGLE r29 = createNewRectangle(10,21,10,21);
    insertRect(r29, tree.root, &tree);
    RECTANGLE r30 = createNewRectangle(17,5,17,5);
    insertRect(r30, tree.root, &tree);
    RECTANGLE r31 = createNewRectangle(12,3,12,3);
    insertRect(r31, tree.root, &tree);
    RECTANGLE r32 = createNewRectangle(14,12,14,12);
    insertRect(r32, tree.root, &tree);

    printf("*-------------------------*\n");
    printf("Number of nodes: %d\n", tree.cnt);
    printf("Tree height: %d\n", tree.height);
    myPrint("Root", tree.root);
    myPrint("Child 1", tree.root->children[0]);
    myPrint("Child 2", tree.root->children[1]);
    myPrint("Child 1 1", tree.root->children[0]->children[0]);
    myPrint("Child 1 2", tree.root->children[0]->children[1]);
    myPrint("Child 1 3", tree.root->children[0]->children[2]);
    myPrint("Child 1 4", tree.root->children[0]->children[3]);
    myPrint("Child 2 1", tree.root->children[1]->children[0]);
    myPrint("Child 2 2", tree.root->children[1]->children[1]);
    myPrint("Child 2 3", tree.root->children[1]->children[2]);
    myPrint("Child 2 4", tree.root->children[1]->children[3]);
    printf("Number of nodes: %d\n", tree.cnt);
    printf("Tree height: %d\n", tree.height);
    // printf("*-------------------------*\n");
    // printf("Number of nodes: %d\n", tree.cnt);
    // printf("Tree height: %d\n", tree.height);
    // myPrint("Root", tree.root);
    // myPrint("Child 1", tree.root->children[0]);
    // myPrint("Child 1 1", tree.root->children[0]->children[0]);
    // myPrint("Child 1 2", tree.root->children[0]->children[1]);
    // myPrint("Child 1 3", tree.root->children[0]->children[2]);
    // myPrint("Child 2 1", tree.root->children[1]->children[0]);
    // myPrint("Child 2 2", tree.root->children[1]->children[1]);


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
