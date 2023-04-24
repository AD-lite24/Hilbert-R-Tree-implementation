void insertRect(rectangle R, NODE *n, rtree *tree)
{
    int h = R.hilbertValue;
    NODE *leaf = chooseLeaf(R, h, n);
    if (leaf->num_entries < C_l)
    {
        // If leaf is not full, insert R into it
        int i = leaf->num_entries - 1;
        while (i >= 0 && leaf->rects[i].hilbertValue > R.hilbertValue)
        {
            leaf->rects[i + 1] = leaf->rects[i];
            leaf->elements[i + 1] = leaf->elements[i];
            i--;
        }
        leaf->rects[i + 1] = R;
        leaf->elements[i + 1] = R.low;
        leaf->num_entries++;
        // Update the LHV of the node and its ancestors
        NODE *p = leaf;
        while (p != NULL)
        {
            int max_lhv = -1;
            for (int i = 0; i < p->num_entries; i++)
            {
                if (p->children[i]->lhv > max_lhv)
                {
                    max_lhv = p->children[i]->lhv;
                }
            }
            p->lhv = max_lhv;
            p = p->parent;
        }
    }
    else
    {
        // If leaf is full, split it and adjust the tree
        NODE *new_leaf = malloc(sizeof(NODE));
        new_leaf->parent = leaf->parent;
        new_leaf->isLeaf = true;
        splitNode(leaf, new_leaf);
        adjustTree(leaf, new_leaf, tree);
        // Insert R into the appropriate leaf node
        if (h <= new_leaf->rects[0].hilbertValue)
        {
            insertRect(R, leaf, tree);
        }
        else
        {
            insertRect(R, new_leaf, tree);
        }
    }
}

NODE *chooseLeaf(rectangle R, int h, NODE *n)
{
    while (!n->isLeaf)
    {
        int i = 0;
        int min_lhv = INT_MAX;
        NODE *c = NULL;
        while (i < n->num_entries)
        {
            rectangle Rn = n->rects[i];
            if (Rn.hilbertValue >= h && Rn.hilbertValue < min_lhv)
            {
                min_lhv = Rn.hilbertValue;
                c = n->children[i];
            }
            i++;
        }
        n = c;
    }
    return n;
}

void adjustTree(NODE n, NODE nn, rtree *tree)
{
    if (n == &tree->root)
    {
        // Create a new root if it is required
        NODE new_root = malloc(sizeof(NODE));
        new_root->isLeaf = false;
        new_root->num_entries = 1;
        new_root->lhv = -1;
        new_root->children[0] = n;
        new_root->children[1] = nn;
        // Update the root node of the tree
        tree->root = new_root;
        tree->height++;
    }
    else
    {
        // Add nn to the parent of n
        NODE p = n->parent;
        int i = 0;
        while (p->children[i] != n)
            i++;
        for (int j = p->num_entries - 1; j >= i + 1; j--)
        {
            p->children[j + 1] = p->children[j];
            p->rects[j + 1] = p->rects[j];
            p->elements[j + 1] = p->elements[j];
        }
        p->children[i + 1] = nn;
        p->rects[i + 1] = nn->rects[0];
        p->elements[i + 1] = nn->elements[0];
        p->num_entries++;
        // If the parent of n becomes overflow, split it recursively
        if (p->num_entries == C_n)
        {
            NODE *new_node = malloc(sizeof(NODE));
            new_node->parent = p->parent;
            splitNode(p, new_node);
            adjustTree(p, new_node, tree);
        }
    }
}