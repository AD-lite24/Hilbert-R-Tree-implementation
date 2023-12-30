# Hilbert R-tree C Implementation

The Hilbert R-tree is a spatial index structure that combines the concepts of the R-tree and the Hilbert curve. It is designed to efficiently index multidimensional spatial data, providing fast searching and retrieval of data based on their spatial proximity.

The Hilbert curve, on the other hand, is a space-filling curve that maps multidimensional data into a one-dimensional space. It has the property of preserving locality, meaning that nearby points in the multidimensional space tend to be mapped to nearby points on the curve.

By combining the R-tree and the Hilbert curve, the Hilbert R-tree achieves both efficient spatial indexing and improved data locality. The index structure organizes the objects using the R-tree's hierarchical approach, while the Hilbert curve is used to order the entries within each node. This ordering helps to reduce the overlap of bounding boxes and improves the overall query performance.

[Link to paper](https://www.vldb.org/conf/1994/P500.PDF)

### Usage

To run the code, firstly, add a file named "input.txt" in which, all the sample data, of 2D points is stored, to the current Code directory. Then, run the command on command line :

```
gcc DSA_assignment_group_7.c -o a.exe && ./a.exe
```

If you have the data stored in a file with different name, add the file to the directory, and change file name in line 872 of the C code file as per your convenience.

On running this code, we see the PreOrder Taraversal of the tree, which is a function preOrderTraversal() in the main function.

Some Assumptions taken for this code :
1. We have taken order of Hilbert curve to be 5 currently, which gives an accuracy of order 5 for hilbert values. To change value of this accuracy variable, change the value on line number 9.
2. Seeing the node structure, if a node is a leaf, it will only contain Rectangles, and, the children array will not be accessed, and would be NULL. If the node is not a leaf, children array will contain addresses of the node's children nodes, and rects[i] will hold the Maximum Bounding Rectangle of the ith child of the node
3. We have implemented a 2-3 splitting as was given in the research paper, except when splitting at root node (where 1-2 splitting is done).

### Contributers

* Aditya Dandwate
* Yashvardhan Batwara
* Rakshit Aggrawal
* Meet Vithalani
* Harsh Deshpande



