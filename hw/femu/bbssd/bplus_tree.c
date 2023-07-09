#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bplus_tree.h"

#define item_cmp(a,b) ((a)- (b))

//这里先声明内部封装的函数
static bplus_node_pt bplus_node_new_leaf(int m);
static bplus_node_pt bplus_node_new_internal(int m); // 分别是叶子结点和内部结点

static int binary_search(KEY *keys, KEY key, int left, int right, int *index);
static int _bplus_tree_insert(bplus_tree_pt tree, bplus_node_pt node, )




inline int bplus_tree_create(bplus_tree_pt * _tree, int m){
    /*根据m也就是关键字支持的个数创建树*/
    bplus_tree_pt tree = (bplus_tree_pt)malloc(sizeof(bplus_tree_t));
    if(tree == NULL){
        return 0;
    }
    tree->root  = NULL;
    tree->max   = m;
    tree->min   = m/2;
    *_tree = tree;
    return 1;
}

