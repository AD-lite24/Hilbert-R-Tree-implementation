#include "RTree.c"
int main()
{
    rtree tree;
    tree.cnt = 0;
    tree.height = 1;
    tree.root = malloc(sizeof(node));
    tree.root->isLeaf = false;
    tree.root->num_entries = 4;
    tree.root->lhv = -1;
    tree.root->parent = NULL;

    // Insert some rectangles
    rectangle r1 = {{2, 2}, {4, 4}, 0};
    insertRect(r1, tree.root, &tree);
    

    // rectangle r2 = {{5, 6}, {7, 8}, 1};
    // insertRect(r2, tree.root, &tree);

    // rectangle r3 = {{1, 3}, {2, 4}, 2};
    // insertRect(r3, tree.root, &tree);

    // rectangle r4 = {{7, 1}, {8, 2}, 3};
    // insertRect(r4, tree.root, &tree);

    // Print the resulting tree
    printf("Number of nodes: %d\n", tree.cnt);
    printf("Tree height: %d\n", tree.height);
    printf("Root node: %p\n", tree.root);
    printf("Root is leaf node: %d\n", tree.root->isLeaf);
    printf("Root LHV: %d\n", tree.root->lhv);
    printf("Number of entries in root: %d\n", tree.root->num_entries);
    for (int i = 0; i < tree.root->num_entries; i++) {
        printf("  Entry %d:\n", i);
        printf("    Rect: ((%d, %d), (%d, %d))\n", tree.root->rects[i].low.x, tree.root->rects[i].low.y, tree.root->rects[i].high.x, tree.root->rects[i].high.y);
        printf("    LHV: %d\n", tree.root->rects[i].hilbertValue);
        if (tree.root->isLeaf) {
            printf("    Element: (%d, %d)\n", tree.root->elements[i].x, tree.root->elements[i].y);
        } else {
            printf("    Child node: %p\n", tree.root->children[i]);
        }
    }
    return 0;
}