#include "RTree.c"

void delete(rtree* r, element to_delete ){
    NODE temp = findNode(r, to_delete);
    int i;
    for (i = 0 ; i < temp->num_entries; i++)
    {
        if (temp->elements[i].x == to_delete.x && temp->elements[i].y == to_delete.y){
            temp->elements[i]=NULL;
            break;
        }
    }
    for(i;i<temp->num_entries;i++){
        temp->elements[i]=temp->elements[i+1];
    }
    temp->num_entries--;
    if (temp->num_entries >= m){}
    else{
        bool check=true;
        while(!check)
            check=checkCousins(r, temp);
    }

}
NODE findNode (rtree* r, element to_delete){
    NODE temp = r->root;
    while (temp->isLeaf != 1){
        for (int i = 0 ; i < temp->num_entries ; i++){
            if (to_delete.x >= temp->rects[i].low.x && to_delete.y >= temp->rects[i].low.y)
            {
                if (to_delete.x <= temp->rects[i].high.x && to_delete.y <= temp->rects[i].high.y){
                    temp = temp->children[i];
                    break;       
                }
            }
        }
    }
    return temp;
}

// temp1 left
// temp2 right
// at end right one vanishes
void mergeUnderFLows(rtree* r,NODE temp1, NODE temp2 ){
    NODE par = temp1->parent;
    int k;
    for (int k = 0 ; k < temp2->num_entries ; k++)
    {
        temp1->elements[temp1->num_entries + k] = temp2->elements[k];
        temp2->elements[k]=NULL;
    }
    temp1->num_entries = temp1->num_entries + temp2->num_entries;
    free(temp2);
    int i;
    for (i = 0 ; i < par->num_entries - 1 ; i++)
    {
        if (par->children[i] == temp1)
            break;
    }
    i++;
    for (; i < par->num_entries-1 ; i++)
        par->children[i] = par->children[i+1];
    par->children[par->num_entries-1] = NULL;
    par->num_entries--;
}
bool checkCousins (rtree * r, NODE temp)
{
    if (temp->left->num_entries > m)
    {
        temp->elements[m-1] = temp->left->elements[temp->left->num_entries-1];
        temp->left->elements[temp->left->num_entries-1] = NULL;
        temp->left->num_entries--;
        temp->num_entries++;
        return true;
    }
    else if (temp->right->num_entries > m)
    {
        temp->elements[m-1] = temp->right->elements[0];
        for (int i = 1; i < temp->right->num_entries ; i++)
            temp->right->elements[i-1] = temp->right->elements[i];
        temp->right->elements[temp->right->num_entries-1] = NULL;
        temp->right->num_entries--;
        temp->num_entries++;
        return true;
    }
    else if(temp->right == NULL)
    {
        mergeUnderFLows(r, temp->left, temp);
        if (temp->parent->num_entries < m) return false;
        return true;
    }
    else
    {
        mergeUnderFLows(r, temp, temp->right);
        if (temp->parent->num_entries < m) return false;
        return true;
    }
}
    //         8
    //         // temp1 = 4
    //         // temp2 = 10
    //     4  1   5   4   10
    // 4  7 5 6 7       1 2 3 7 8 