#include <stdio.h>
#include <stdbool.h>
#define M 4
#define m 2
#define DIM 2

struct rtree 
{
    int cnt; // No.of total Nodes
    int height; // 
    struct node* root;
};

struct rectangle
{
    int low[DIM], high[DIM];
};
// node represents a tree node of the tree.
// C code based on explanation of ChatGPT
// struct HRtree {
//     int min, max, bits;
//     struct node *root;
//     struct h *hf;
//     int size;
// };
// struct HRtree *new_tree(int min, int max, int bits) {
//     struct HRtree *rt = malloc(sizeof(struct HRtree));
//     if (rt == NULL) {
//         perror("malloc error");
//         exit(EXIT_FAILURE);
//     }

//     rt->hf = NULL;
//     rt->root = NULL;

//     if (bits < DEFAULT_RESOLUTION) {
//         bits = DEFAULT_RESOLUTION;
//     }

//     if (min < 0) {
//         min = DEFAULT_MIN_NODE_ENTRIES;
//     }

//     if (max < 0) {
//         max = DEFAULT_MAX_NODE_ENTRIES;
//     }

//     if (max < min) {
//         fprintf(stderr, "Minimum number of nodes should be less than Maximum number of nodes and not vice versa.\n");
//         exit(EXIT_FAILURE);
//     }

//     struct h *hf = NULL;
//     int err = h_new(&hf, (uint32_t) bits, DIM);

//     if (err != 0) {
//         fprintf(stderr, "Error creating Hilbert curve instance\n");
//         exit(EXIT_FAILURE);
//     }

//     struct node *root = new_node(min, max);
//     root->leaf = 1;

//     rt->min = min;
//     rt->max = max;
//     rt->bits = bits;
//     rt->hf = hf;
//     rt->root = root;

//     return rt;
// }
// type node struct
// {
//     min, max int 
//     parent *node 
//     left, right *node 
//     leaf bool 
//     entries *entryList 
//      lhv *big.Int 
//      bb *rectangle // bounding-box of all children of this entry
// } 
// type entry struct {
    // 	bb   *rectangle // bounding-box of of this entry
    // 	node *node
    // 	obj  Rectangle
    // 	h    *big.Int // hilbert value
    // 	leaf bool
// }
struct node
{
    int num_entries;
    void* entries[M];
    struct node* children[M];
    struct node* parent;
    struct rectangle coords;
    bool isLeaf;
    int hilbertValue;
    struct node* left, right;
    int lhv; // Something related to Hilbert value
};
int HRtree_size(struct HRtree *tree) {
    return tree->size;
}

const char *HRtree_string(struct HRtree *tree) {
    return "(HRtree)";
}

struct node *newNode(int min, int max) {
    struct node *n = (struct node*)malloc(sizeof(struct node));
    n->min = min;
    n->max = max;
    n->parent = NULL;
    n->left = NULL;
    n->right = NULL;
    n->leaf = false;
    n->entries = newList(max);
    mpz_init(n->lhv);
    n->bb = NULL;
    return n;
}

const char *node_string(struct node *n) {
    char *str = (char*)malloc(sizeof(char)*100);
    snprintf(str, 100, "node{leaf: %d, entries: %p, lhv: %Zd}", n->leaf, n->entries, n->lhv);
    return str;
}

struct entry *node_get_entries(struct node *n, int *n_entries) {
    struct entry *entries = NULL;
    entries = entryList_get_entries(n->entries, n_entries);
    return entries;
}
// struct node
// {
//     enum kind kind; // LEAF or BRANCH
//     int count;      // number of rects
//     struct rect rects[MAX_ENTRIES];
//     union
//     {
//         struct node *children[MAX_ENTRIES];
//         struct item items[MAX_ENTRIES];
//     };
// };

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
