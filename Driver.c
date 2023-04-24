#include "hilbert_Rtree.c"
#include <stdlib.h>
int main()
{
    RTREE r = (RTREE)malloc (sizeof(rtree));
    r->cnt = 25;
    r->height = 3;
    r->root = (NODE)malloc(sizeof(NODE));
    NODE newnode = r->root;
    newnode->isLeaf = 0;
    newnode->num_entries = 4;
    NODE bigBox1, bigBox2, bigBox3, bigBox4;
    NODE box1, box2, box3, box4, box5, box6, box7, box8, box9, box10;
    
    // bottom left
    bigBox1 = createNewNode(0, 3);
    bigBox1->parent = newnode;
    bigBox1->rects[0] = createRectangle(2,4,4,6);
    bigBox1->rects[1] = createRectangle(5,1,7,5);
    bigBox1->rects[2] = createRectangle(5,7,8,10);

    box1 = createNewNode(1, 2);
    box1->parent = bigBox1;
    box1->elements[0] = createElement(2,4);
    box1->elements[1] = createElement(4,6);
    box1->rects[0] = createRectangle(2,4,2,4);
    box1->rects[1] = createRectangle(4,6,4,6);

    box2 = createNewNode(1, 2);
    box2->parent = bigBox1;
    box2->elements[0] = createElement(5,1);
    box2->elements[1] = createElement(7,5);
    box2->rects[0] = createRectangle(5,1,5,1);
    box2->rects[1] = createRectangle(7,5,7,5);

    box10 = createNewNode(1, 2);
    box10->parent = bigBox1;
    box10->elements[0] = createElement(5,7);
    box10->elements[1] = createElement(8,10);
    box10->rects[0] = createRectangle(5,7,5,7);
    box10->rects[1] = createRectangle(8,10,8,10);

    bigBox1->children[0] = box1;
    bigBox1->children[1] = box2;
    bigBox1->children[2] = box10;
    

    bigBox2 = createNewNode(0, 2);
    bigBox2->parent = newnode;
    bigBox2->rects[0] = createRectangle(1,8,5,12);
    bigBox2->rects[1] = createRectangle(6,15,7,30);

    box3 = createNewNode(1, 2);
    box3->parent = bigBox2;
    box3->elements[0] = createElement(1,8);
    box3->elements[1] = createElement(5,12);
    box3->rects[0] = createRectangle(1,8,1,8);
    box3->rects[1] = createRectangle(5,12,5,12);

    box4 = createNewNode(1, 2);
    box4->parent = bigBox2;
    box4->elements[0] = createElement(6,15);
    box4->elements[1] = createElement(7,30);
    box4->rects[0] = createRectangle(6,15,6,15);
    box4->rects[1] = createRectangle(7,30,7,30);

    bigBox2->children[0] = box3;
    bigBox2->children[1] = box4;

    //meet
    bigBox3 = createNewNode(0, 2);
    bigBox3->parent = newnode;
    bigBox3->rects[0] = createRectangle(15,3,16,4);
    bigBox3->rects[1] = createRectangle(17,4,20,7);

    box5 = createNewNode(1, 2);
    box5->parent = bigBox3;
    box5->elements[0] = createElement(15,3);
    box5->elements[1] = createElement(16,4);
    box5->rects[0] = createRectangle(15,3,15,3);
    box5->rects[1] = createRectangle(16,4,16,4);

    box6 = createNewNode(1, 2);
    box6->parent = bigBox3;
    box6->elements[0] = createElement(17,4);
    box6->elements[1] = createElement(20,7);
    box6->rects[0] = createRectangle(17,4,17,4);
    box6->rects[1] = createRectangle(20,7,20,7);

    bigBox3->children[0] = box5;
    bigBox3->children[1] = box6;

    
    //meet 
    bigBox4 = createNewNode(0, 3);
    bigBox4->parent = newnode;
    bigBox4->rects[0] = createRectangle(16,15,25,20);
    bigBox4->rects[1] = createRectangle(9,12,15,18);
    bigBox4->rects[2]=createRectangle(26,12,30,20);

    box7 = createNewNode(1, 2);
    box7->parent = bigBox4;
    box7->elements[0] = createElement(16,15);
    box7->elements[1] = createElement(25,20);
    box7->rects[0] = createRectangle(16,15,16,15);
    box7->rects[1] = createRectangle(25,20,25,20);

    box8 = createNewNode(1, 2);
    box8->parent = bigBox4;
    box8->elements[0] = createElement(9,12);
    box8->elements[1] = createElement(15,18);
    box8->rects[0] = createRectangle(9,12,9,12);
    box8->rects[1] = createRectangle(15,18,15,18);

    box9=createNewNode(1,2);
    box9->parent = bigBox4;
    box9->elements[0]=createElement(26,12);
    box9->elements[1]=createElement(30,20);
    box9->rects[0]=createRectangle(26,12,26,12);
    box9->rects[1]=createRectangle(30,20,30,20);
    

    bigBox4->children[0] = box7;
    bigBox4->children[1] = box8;
    bigBox4->children[2] = box9;


// Rakshit
    newnode->children[0] = bigBox1;
    newnode->children[1] = bigBox2;
    newnode->children[2] = bigBox3;
    newnode->children[3] = bigBox4;
    newnode->rects[0] = createRectangle(2,4,8,10);
    newnode->rects[1] = createRectangle(1,8,7,30);
    newnode->rects[2] = createRectangle(15,3,20,7);
    newnode->rects[3] = createRectangle(9,12,30,20);
    return 0;
}