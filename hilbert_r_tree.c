#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <string.h>
#include <stdint.h>
#define M 4
#define m 2
#define ORDER 5

typedef struct rtree rtree;
typedef struct rtree *RTREE;
typedef struct node node;
typedef struct node *NODE;
typedef struct rectangle rectangle;
typedef struct rectangle *RECTANGLE;
typedef struct element element;

struct rtree {
    int cnt;    // No.of total Nodes
    int height; // height of the tree
    NODE root;
};

// Stores the 2d tuple
struct element {
    int x;
    int y;
};

struct rectangle {
    element low, high;
    int hilbertValue; // Can be LHV or the hilbert value depending on the type
};

int min(int a, int b) {
    return (a < b) ? a : b;
}
int max(int a, int b) {
    return (a > b) ? a : b;
}

struct node {
    int num_entries;
    RECTANGLE rects[M];
    bool isLeaf;
    int lhv; 
    NODE parent;
    NODE children[M];
};

// Declaring functions
NODE handleOverflow(NODE n, RECTANGLE r);
NODE hilbert_choose_sibling(NODE nn, RECTANGLE new_rect);
NODE chooseLeaf(RECTANGLE R, NODE n);
void insertRect(RECTANGLE r, RTREE tree);
void adjustTree(NODE parent, NODE nn, RTREE tree);  
NODE handleOverflowNonLeaf(NODE parent, NODE nn);
NODE hilbert_choose_sibling_non_leaf(NODE par, NODE nn);
NODE hilbert_choose_sibling(NODE node, RECTANGLE new_rect);

// Find hilbert values for coordinate rectangle (low, high)
void rot(int n, int *x, int *y, int rx, int ry) {
    if (ry == 0) {
        if (rx == 1) {
            *x = n - 1 - *x;
            *y = n - 1 - *y;
        }
        int t = *x;
        *x = *y;
        *y = t;
    }
}
int xy2d(int n, int x, int y) {
    int rx, ry, s, d = 0;
    for (s = n / 2; s > 0; s /= 2) {
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
    int square_size = (1 << ORDER);
    return xy2d(square_size, xmid, ymid);
}

// Create a new rectangle pointer with the given parameters
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

// Create new node
NODE createNewNode(bool isLeaf) 
{
    NODE newNode = (NODE)malloc(sizeof(node));
    memset(newNode, 0, sizeof(node));
    for (int i = 0; i < M; i++) {
        newNode->children[i] = NULL;
        newNode->rects[i] = NULL;
    }
    newNode->isLeaf = isLeaf;
    newNode->lhv = -1;
    return newNode;
}

// Find MBR for a given node
RECTANGLE findMBR(NODE curr_node) {
    RECTANGLE newRec = (RECTANGLE)malloc(sizeof(rectangle));
    int xmin = INT_MAX, xmax = -1, ymin = INT_MAX, ymax = -1;
    // For each node, MBR of node is calculate via the nodes' children's Rectangles (or, just the nodes' rectangles, if the node is a leaf)
    if (curr_node->isLeaf == 1) {
        for (int i = 0; i < curr_node->num_entries; i++) {
            xmin = min(xmin, curr_node->rects[i]->low.x);
            ymin = min(ymin, curr_node->rects[i]->low.y);
            ymax = max(ymax, curr_node->rects[i]->high.y);
            xmax = max(xmax, curr_node->rects[i]->high.x);
        }
    }
    else {
        for (int i = 0; i < curr_node->num_entries; i++) {
            int j=0;
            while(j<M && curr_node->children[i]->rects[j]){
                xmin = min(xmin, (curr_node->children[i]->rects[j])->low.x);
                ymin = min(ymin, (curr_node->children[i]->rects[j])->low.y);
                ymax = max(ymax, (curr_node->children[i]->rects[j])->high.y);
                xmax = max(xmax, (curr_node->children[i]->rects[j])->high.x);
                j++;
            }
        }
    }
    newRec->low.x = xmin;
    newRec->high.x = xmax;
    newRec->low.y = ymin;
    newRec->high.y = ymax;
    // Calculating Hilbert Value through formula using the MBR found 
    newRec->hilbertValue = hilbert_rect_center(newRec);
    return newRec;
}

// Handles overflow in NON LEAF nodes while inserting 
NODE handleOverflowNonLeaf(NODE par, NODE nn) {
    // if parent is root and not leaf
    if (par->parent == NULL) 
    {
        // new_root will later become the tree's new root
        // Current root will become a child of the root
        // Uncle will become another child of the root.
        NODE new_root = createNewNode(false);
        NODE uncle = createNewNode(false);

        // Redistributing current root's children into 2 nodes
        NODE childs[5];
        for (int i = 0; i < par->num_entries; i++) 
            childs[i] = par->children[i];
        // insert children according to LHV
        childs[4] = nn;
        int i = 3;
        while (i >= 0 && childs[i]->lhv > nn->lhv) {
            childs[i + 1] = childs[i];
            i--;
        }
        childs[i + 1] = nn;

        // Assigning variables of structs
        int my_lhv = INT_MIN;
        // put 3 in par and 2 in uncle
        for (int i = 0; i < 3; i++) {
            par->children[i] = childs[i];
            if (childs[i]->lhv > my_lhv)
                my_lhv = childs[i]->lhv;
        }
        par->children[3] = NULL;
        par->num_entries = 3;
        par->lhv = my_lhv;

            
        for (int i = 3; i < 5; i++) {
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
    // If parent is not root or leaf
    else {
        // Finding co-operating sibling 
        NODE coop_sibling = hilbert_choose_sibling_non_leaf(par, nn);
        // If co-operating sibling is full, we will create a new node
        if (coop_sibling->num_entries==M) {
            NODE pp = createNewNode(false);

            // Redistributing current and sibling's children into 2 nodes now
            NODE childs[5];
            //Store all children in childs[] array
            for (int i = 0; i < M; i++) {
                childs[i] = par->children[i];
            }
            childs[4] = nn;
            //insert nn into childs sorted
            int j = 3;
            while (j >= 0 && childs[j]->lhv > nn->lhv) {
                childs[j + 1] = childs[j];
                j--;
            }
            childs[j + 1] = nn;

            // insert children according to LHV
            // Assigning struct's variables
            int my_lhv = INT_MIN;
            // putting 3 nodes in par and 2 in pp
            for (int i = 0; i < 3; i++) {
                par->children[i] = childs[i];
                if (childs[i]->lhv > my_lhv)
                    my_lhv = childs[i]->lhv;
            }
            par->children[3] = NULL;
            par->num_entries = 3;
            par->lhv = my_lhv;

            my_lhv = INT_MIN;
            for (int i = 3; i < 5; i++) {
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
        // When co-operating sibling is not full
        // Accomodate into sibling 
        else {
            int first_rearrange = (par->num_entries + coop_sibling->num_entries + 1) / 2;
            int second_rearrange = par->num_entries + coop_sibling->num_entries + 1 - first_rearrange;

            // Redistributing current and sibling's children into 2 nodes now
            NODE childs[par->num_entries + coop_sibling->num_entries + 1];
            // Store all children in childs[] array
            if (par->lhv < coop_sibling->lhv) {
                for (int i = 0; i < par->num_entries; i++)
                    childs[i] = par->children[i];
                
                for (int i = 0; i < coop_sibling->num_entries; i++)
                    childs[i + par->num_entries] = coop_sibling->children[i];

                childs[par->num_entries + coop_sibling->num_entries] = nn;
            }
            else {
                for (int i = 0; i < coop_sibling->num_entries; i++)
                    childs[i] = coop_sibling->children[i];

                for (int i = 0; i < par->num_entries; i++)
                    childs[i + coop_sibling->num_entries] = par->children[i];

                childs[par->num_entries + coop_sibling->num_entries] = nn;
            }
            // Insertion sort to insert nn
            int j = par->num_entries + coop_sibling->num_entries - 1;
            while (j >= 0 && nn->lhv < childs[j]->lhv) {
                childs[j+1] = childs[j];
                j--;
            }
            childs[j+1] = nn;

            // To choose rearranging order, depends on which side was coop_sibling was on with respect to par
            // Assigning struct's variables
            if (par->lhv < coop_sibling->lhv) {
                int lhv = INT_MIN;
                for (int i = 0; i < first_rearrange; i++) {
                    par->children[i] = childs[i];
                    if (childs[i]->lhv > lhv)
                        lhv = childs[i]->lhv;
                }
                par->num_entries = first_rearrange;
                par->lhv = lhv;

                lhv = INT_MIN;
                for (int i = 0; i < second_rearrange; i++) {
                    coop_sibling->children[i] = childs[first_rearrange + i];
                    if (childs[first_rearrange + i]->lhv > lhv)
                        lhv = childs[first_rearrange + i]->lhv;
                }
                coop_sibling->num_entries = second_rearrange;
                coop_sibling->lhv = lhv;
                
            }

            else {
                int lhv = INT_MIN;
                for (int i = 0; i < first_rearrange; i++) {
                    coop_sibling->children[i] = childs[i];
                    if (childs[i]->lhv > lhv)
                        lhv = childs[i]->lhv;
                }
                coop_sibling->num_entries = first_rearrange;
                coop_sibling->lhv = lhv;

                lhv = INT_MIN;
                for (int i = 0; i < second_rearrange; i++) {
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

// Handles overflow in LEAF nodes while inserting
NODE handleOverflow(NODE n, RECTANGLE r) {
    // When n is root, this function creates a new node, which will become sibling of old root
    if (n->parent == NULL) {
        NODE nn = createNewNode(true);
        // Rearranging all entries between root and nn
        RECTANGLE rects[5];
        for (int i = 0; i < n->num_entries; i++) {
            rects[i] = n->rects[i];
        }
        // insert r into rects according to h value
        rects[4] = r;
        int j = 3;
        while (j >= 0 && rects[j]->hilbertValue > r->hilbertValue) {
            rects[j + 1] = rects[j];
            j--;
        }
        rects[j + 1] = r;
        // Assigning structure variables
        int lhv = INT_MIN;
        // put 3 in n and 2 in nn
        for (int i = 0; i < 3; i++) {
            n->rects[i] = rects[i];
            if (rects[i]->hilbertValue > lhv)
                lhv = rects[i]->hilbertValue;
        }
        n->rects[3] = NULL;
        n->num_entries = 3;
        n->lhv = lhv;
        lhv = INT_MIN;
        for (int i = 3; i < 5; i++) {
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
    // When n is not root, we find a cooperating sibling of n, and adjust new entry amongst these nodes
    else {
        NODE parent = n->parent;
        NODE coop_sibling = hilbert_choose_sibling(n, r); 
        // rects to store all the rectangles from the two nodes
        RECTANGLE rects[n->num_entries + coop_sibling->num_entries + 1];

        // Maintaining sorted order of rectangles
        if (coop_sibling->lhv > n->lhv) {
            for (int i = 0; i < n->num_entries; i++)
                rects[i] = n->rects[i];
            for (int i = 0; i < coop_sibling->num_entries; i++)
                rects[i + n->num_entries] = coop_sibling->rects[i];
        }
        else {
            for (int i = 0; i < coop_sibling->num_entries; i++)
                rects[i] = coop_sibling->rects[i];
            for (int i = 0; i < n->num_entries; i++)
                rects[i + coop_sibling->num_entries] = n->rects[i];
        }
        // Assigning inserted rectangle in array, by insertion sort
        rects[n->num_entries + coop_sibling->num_entries] = r;
        int i = n->num_entries + coop_sibling->num_entries - 1;
        while (i >= 0 && r->hilbertValue < rects[i]->hilbertValue) {
            rects[i + 1] = rects[i];
            i--;
        }
        rects[i + 1] = r;

        // If coop sibling is also full then split
        if (coop_sibling->num_entries == M) {
            if (coop_sibling->lhv > n->lhv) {

                NODE nn = createNewNode(n->isLeaf);
                int lhv = INT_MIN;
                // Initialize rects of nn to null

                for (int i = 0; i < 3; i++) {
                    n->rects[i] = rects[i];
                    if (rects[i]->hilbertValue > lhv)
                        lhv = rects[i]->hilbertValue;
                }
                n->num_entries = 3;
                n->lhv = lhv;

                lhv = INT_MIN;
                for (int i = 3; i < 6; i++) {
                    coop_sibling->rects[i - 3] = rects[i];
                    if (rects[i]->hilbertValue > lhv)
                        lhv = rects[i]->hilbertValue;
                }
                coop_sibling->num_entries = 3;
                coop_sibling->lhv = lhv;

                lhv = INT_MIN;
                for (int i = 6; i < 9; i++) {
                    nn->rects[i - 6] = rects[i];
                    if (rects[i]->hilbertValue > lhv)
                        lhv = rects[i]->hilbertValue;
                }
                nn->num_entries = 3;
                nn->lhv = lhv;

                return nn;
            }
            else {
                NODE nn = createNewNode(n->isLeaf);

                // Initialize rects of nn to null
                for (int i = 0; i < M; i++)
                    nn->rects[i] = NULL;

                int lhv = INT_MIN;

                for (int i = 0; i < 3; i++) {
                    coop_sibling->rects[i] = rects[i];
                    if (rects[i]->hilbertValue > lhv)
                        lhv = rects[i]->hilbertValue;
                }
                coop_sibling->num_entries = 3;
                coop_sibling->rects[3] = NULL;
                coop_sibling->children[3] = NULL;
                coop_sibling->lhv = lhv;

                lhv = INT_MIN;
                for (int i = 3; i < 6; i++) {
                    n->rects[i - 3] = rects[i];
                    if (rects[i]->hilbertValue > lhv)
                        lhv = rects[i]->hilbertValue;
                }
                n->num_entries = 3;
                n->lhv = lhv;
                n->rects[3] = NULL;
                n->children[3] = NULL;
                lhv = INT_MIN;
                for (int i = 6; i < 9; i++) {
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
        else {
            // Number of elements that go in the first and second node respectively
            int first_rearrange = (n->num_entries + coop_sibling->num_entries + 1) / 2;
            int second_rearrange = n->num_entries + coop_sibling->num_entries + 1 - first_rearrange;

            if (coop_sibling->lhv > n->lhv) {
                int lhv = INT_MIN;
                for (int i = 0; i < first_rearrange; i++) {
                    n->rects[i] = rects[i];
                    if (rects[i]->hilbertValue > lhv)
                        lhv = rects[i]->hilbertValue;
                }
                n->num_entries = first_rearrange;
                n->lhv = lhv;

                lhv = INT_MIN;
                for (int i = 0; i < second_rearrange; i++) {
                    coop_sibling->rects[i] = rects[first_rearrange + i];
                    if (rects[first_rearrange + i]->hilbertValue > lhv)
                        lhv = rects[first_rearrange + i]->hilbertValue;
                }
                coop_sibling->num_entries = second_rearrange;
                coop_sibling->lhv = lhv;
            }

            else {
                int lhv = INT_MIN;
                for (int i = 0; i < first_rearrange; i++) {
                    coop_sibling->rects[i] = rects[i];
                    if (rects[i]->hilbertValue > lhv)
                        lhv = rects[i]->hilbertValue;
                }
                coop_sibling->num_entries = first_rearrange;
                coop_sibling->lhv = lhv;

                lhv = INT_MIN;
                for (int i = 0; i < second_rearrange; i++) {
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
NODE hilbert_choose_sibling_non_leaf(NODE par, NODE nn) {
    int node_par_index;
    NODE coop_uncle = (NODE)malloc(sizeof(struct node));
    NODE grand_parent = par->parent;
    NODE left_uncle = NULL;
    NODE right_uncle = NULL;

    for (int i = 0; grand_parent != NULL && i < grand_parent->num_entries; i++) {
        if (par == grand_parent->children[i])
            node_par_index = i;
    }
    if (node_par_index > 0)
        left_uncle = grand_parent->children[node_par_index - 1];
    if (node_par_index < 3)
        right_uncle = grand_parent->children[node_par_index + 1];

    NODE *childs = par->children;
    int median_hv = par->children[par->num_entries / 2]->lhv;

    // Chooses sibling based on the median lhv of overflowing node and lhv of new insertion
    if (nn->lhv > median_hv) {
        // Returns left sibling if right sibling does not exist
        if (!right_uncle)
            return left_uncle;
        return right_uncle;
    }
    else {
        if (!left_uncle)
            return right_uncle;
        return left_uncle;
    }
}

// Choose the cooperating sibling for the overflowing node when it is a LEAF node
NODE hilbert_choose_sibling(NODE nn, RECTANGLE new_rect) {
    int node_index;
    NODE parent = nn->parent;
    NODE left_sibling = NULL;
    NODE right_sibling = NULL;
    for (int i = 0; parent != NULL && i < parent->num_entries; i++) {
        if (nn == parent->children[i])
            node_index = i;
    }

    if (node_index > 0)
        left_sibling = parent->children[node_index - 1];
    if (node_index < 3)
        right_sibling = parent->children[node_index + 1];

    RECTANGLE *rects = nn->rects;
    int median_hv = nn->rects[nn->num_entries / 2]->hilbertValue;
    if (new_rect->hilbertValue > median_hv) {
        if (!right_sibling)
            return left_sibling;
        return right_sibling;
    }
    else {
        if (!left_sibling)
            return right_sibling;
        return left_sibling;
    }
}

// Choose a leaf node to insert rectangle r into
NODE chooseLeaf(RECTANGLE r, NODE root) {
    NODE N = root;

    if (N->isLeaf)
        return N;

    int chosen_entry = N->num_entries - 1; 
    int max_lhv = INT_MAX;
    for (int i = 0; i < N->num_entries; i++) {
        if (N->children[i]->lhv > r->hilbertValue) {
            if (N->children[i]->lhv < max_lhv) {
                chosen_entry = i;
                max_lhv = N->children[i]->lhv;
            }
        }
    }
    return chooseLeaf(r, N->children[chosen_entry]);
}

// Insert rectangle r into the hilbert r tree
void insertRect(RECTANGLE r, RTREE tree) {
    NODE root = tree->root;
    tree->cnt++;
    NODE leaf = chooseLeaf(r, root);

    // Leaf full
    if (leaf->num_entries == M) {   
        NODE new_leaf = handleOverflow(leaf, r);
        // No split occured if new_leaf is null
        if (!new_leaf)
            return;

        // Split occured
        else {
            adjustTree(leaf->parent, new_leaf, tree);

            // Root but leaf split
            if (leaf->parent == NULL) {
                NODE new_root = createNewNode(false);

                if (leaf->lhv < new_leaf->lhv) {
                    new_root->children[0] = leaf;
                    new_root->children[1] = new_leaf;
                }
                else {
                    new_root->children[0] = new_leaf;
                    new_root->children[1] = leaf;
                }
                new_root->rects[0] = findMBR(new_root->children[0]);
                new_root->rects[1] = findMBR(new_root->children[1]);
                leaf->parent = new_root;
                new_leaf->parent = new_root;

                new_root->lhv = max(new_root->children[0]->lhv, new_root->children[1]->lhv);
                new_root->num_entries = 2;
                tree->root = new_root;
                tree->height++;
            }
        }
    }

    // Leaf not full: insert directly into leaf
    else {
        leaf->rects[leaf->num_entries] = r;
        int i = leaf->num_entries - 1;
        while (i >= 0 && leaf->rects[i]->hilbertValue > r->hilbertValue) {
            leaf->rects[i + 1] = leaf->rects[i];
            i--;
        }
        leaf->rects[i + 1] = r;
        leaf->num_entries++;

        if (r->hilbertValue > leaf->lhv)
            leaf->lhv = r->hilbertValue;
    }
}

// Adjust the tree parameters post insertion and propogate any splts upwards
void adjustTree(NODE parent, NODE nn, RTREE tree) {
    
    if (parent == NULL)
        return;

    // Overflow
    if (parent->num_entries == M) {
        NODE pp = handleOverflowNonLeaf(parent, nn);

        // If pp was created
        if (pp) {
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
                adjustTree(parent->parent, pp, tree);

            // Initiate root split
            else {
                NODE new_root = createNewNode(false);
                if (pp->lhv < parent->lhv) {
                    new_root->children[0] = pp;
                    new_root->children[1] = parent;
                    new_root->rects[0] = findMBR(pp);
                    new_root->rects[1] = findMBR(parent);
                }
                else {
                    new_root->children[0] = parent;
                    new_root->children[1] = pp;
                    new_root->rects[0] = findMBR(parent);
                    new_root->rects[1] = findMBR(pp);
                }
                
                tree->root = new_root;
                tree->height++;
                parent->parent = new_root;
                pp->parent = new_root;
                new_root->num_entries = 2;
                tree->root = new_root;
            }
        }
        // Overflow handled without splitting
    }
    // Else fit NN in the parent
    else {
        int i = parent->num_entries - 1;
        parent->children[parent->num_entries] = nn;
        while (i >= 0 && parent->children[i]->lhv > nn->lhv) {
            parent->children[i + 1] = parent->children[i];
            i--;
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
        for(int i = 0; i< parent->num_entries; i++){
            parent->rects[i] = findMBR(parent->children[i]);
        }
        if (parent->lhv < nn->lhv)
            parent->lhv = nn->lhv;
    }
}

// Check if rectangles r1 and r2 interesect
int intersects(RECTANGLE r1, RECTANGLE r2) {
    bool ok = false;
    // Finding by checking if any corner of r2 lies inside r1
    if ((r2->high.x >= r1->low.x) && (r2->high.x <= r1->high.x) && (r2->high.y >= r1->low.y) && (r2->high.y <= r1->high.y)) ok = true;
    if ((r2->low.x >= r1->low.x )&& (r2->low.x <= r1->high.x) && (r2->high.y >= r1->low.y) && (r2->high.y <= r1->high.y)) ok = true;
    if ((r2->high.x >= r1->low.x) && (r2->high.x <= r1->high.x) && (r2->low.y >= r1->low.y) && (r2->low.y <= r1->high.y)) ok = true;
    if ((r2->low.x >= r1->low.x )&& (r2->low.x <= r1->high.x) && (r2->low.y >= r1->low.y) && (r2->low.y <= r1->high.y)) ok = true;
    return (ok);
}

// Search for rectangle rect1 in the hilbert r tree
bool printed; // Maintaining a variable to check if rectangle is intersected by any or not
void search(RECTANGLE rect1, NODE root){
    // Resetting printed to false
    if (root->parent == NULL) printed = false;
    NODE temp = root;
    // If root is leaf
    if (temp->isLeaf) {
        RECTANGLE one = findMBR(temp);
        printf("Rectangle found having the coordinates (%d, %d), (%d, %d)\n", one->high.x, one->high.y, one->low.x, one->low.y);
        return;
    }
    if(!temp->isLeaf)
    {
        // Parsing through all leaf nodes to see if intersections exist
        if (temp->children[0]->isLeaf) {
            for(int i=0; i<M; i++){
                if (temp->rects[i]) {
                    if (intersects(temp->rects[i], rect1)){
                        printf("Rectangle found having the coordinates (%d, %d), (%d, %d)\n", temp->rects[i]->high.x, temp->rects[i]->high.y, temp->rects[i]->low.x, temp->rects[i]->low.y);
                        printed = true;
                    }
                }
            }
        }
        // Parsing through non-leaf nodes, if intersection exists, recursively going deeper in the tree.
        else{
            for(int i=0; i<M; i++)
            if (temp->rects[i])
            if(intersects((temp->rects[i]), rect1)){
                if (temp->children[i])
                search(rect1, temp->children[i]);
            }
        }
    }
    if (printed == false) {
        printf("No intersection found\n");
        printed = true;
    }
}

// Pre order traversal of the root
void preOrderTraversal(NODE root) {
    if (!root)
        return;
    // if we reach leaf then we will just print the values of the rectangles in the leaf node
    if (root->isLeaf) {
        printf("Printing external node\nPoints in node :\n");
        for (int i = 0; i < root->num_entries; i++) {
            if (root->rects[i])
            printf("\t(%d,%d) and hv: %d\n", root->rects[i]->high.x, root->rects[i]->high.y, root->rects[i]->hilbertValue);
        }
    }
    // else we will print the internal node in similar fashion and call the function for the child of the node recursively
    else {
        printf("Printing internal node\nRectangles contained : \n");
        for (int i = 0; i < root->num_entries; i++) {
            if (root->rects[i])
            printf("\t top-right: (%d,%d), bottom-left: (%d,%d), lhv: %d\n", root->rects[i]->high.x, root->rects[i]->high.y, root->rects[i]->low.x, root->rects[i]->low.y, root->children[i]->lhv);
        }
        for (int i = 0; i < root->num_entries; i++) {
            NODE child = root->children[i];
            preOrderTraversal(child);
        }
    }
}

// To create a new R tree
RTREE createNewRTree() {
    RTREE newRTree = malloc(sizeof(rtree));
    newRTree->cnt = 0;
    newRTree->height = 0;
    newRTree->root = NULL;
}

int main(int argc, char const *argv[]) {
    // creating a new tree and reading data from the file to insert values into it  
    RTREE tree;
    tree = (createNewRTree());
    tree->root = malloc(sizeof(node));
    tree->root->isLeaf = true;
    tree->root->num_entries = 0;
    tree->root->lhv = -1;
    tree->root->parent = NULL;

    // Change file name as per convenience here
    FILE* fptr = fopen("input.txt", "r");
    int x,y;
    RECTANGLE temp_insertion_node;
    while (fscanf (fptr, "%d %d\n", &x, &y) != EOF) {
        temp_insertion_node = createNewRectangle(x,y,x,y);
        insertRect(temp_insertion_node, tree);
    }

    printf("Height of tree is %d and no. of data points is %d\n", tree->height, tree->cnt);

    // Preorder traversal called here
    printf("**********Printing the pre-order traversal************\n\n");
    preOrderTraversal(tree->root);

    // Search function called here. To call the function, uncomment next 3 lines, and put your own data in variables xleft, xright, ybottom, ytop;
    // int xleft = ; 
    // int xright = ; 
    // int ybottom = ; 
    // int ytop = ; 
    // printf("\n\n***********Searching for the data rectangle with Top right point : (%d, %d) and Bottom Left point : (%d,%d) *****************\n\n", xright, ytop, xleft, ybottom);
    // printf("Printing all the intersecting Rectangles : \n");
    // RECTANGLE to_search = createNewRectangle(xleft, ybottom, xright, ytop);
    // search(to_search, tree->root);
    return 0;
}