#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bplus_tree.h"

#define item_cmp(a,b) ((a)- (b))
/*写一个compare函数*/

//这里先声明内部封装的函数
static bplus_node_pt bplus_node_new_leaf(int m);
static bplus_node_pt bplus_node_new_internal(int m); // 分别是叶子结点和内部结点

static int binary_search(KEY *keys, KEY key, int left, int right, int *index);
static int _bplus_tree_insert(bplus_tree_pt tree, bplus_node_pt node, KEY key, VALUE value);

//插入时涉及翻转，所以构建以下函数
static void _bplus_leaf_left_rotate(bplus_node_pt node, int index);
static void _bplus_leaf_right_rotate(bplus_node_pt node, int index);
static void _bplus_internal_left_rotate(bplus_node_pt node, int index);
static void _bplus_internal_right_rotate(bplus_node_pt node, int index);

/*以及合并与删除*/
static void _bplus_node_merge(bplus_node_pt node, int index);
static void _bplus_tree_delete(bplus_tree_pt tree, bplus_node_pt node, int index);
static void _bplus_node_destory(bplus_node_pt node);

/*参数给一个指针的指针*/
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

//叶子结点没有child，有key有data
static bplus_node_pt bplus_node_new_leaf(int m){
    bplus_node_pt node = (bplus_node_pt)malloc(sizeof(bplus_node_t));
    if(node == NULL){
        return NULL;
    }

    node->keynum = 0;
    node->keys = (KEY*)malloc((m+1)*sizeof(KEY));
    node->data = (VALUE*)malloc((m+1)*sizeof(VALUE));
    node->parent = NULL;
    node->next = NULL;
    node->child = NULL;
    if(node->keys == NULL || node->data == NULL){
        free(node->keys);
        free(node->data);
        return NULL;
    }
    
    return node;
}

//内部结点没有data，有key有child
static bplus_node_pt bplus_node_new_internal(int m){
    bplus_node_pt node = (bplus_node_pt) malloc(sizeof(bplus_node_t));
    if(node == NULL){
        return NULL;
    }

    node->keynum = 0;
    node->keys = (KEY*)malloc((m+1)*sizeof(KEY));
    node->data = NULL;
    node->parent = NULL;
    node->next = NULL;
    node->child = (bplus_node_pt*)malloc(sizeof(bplus_node_pt)*(m+2));
    if(node->keys == NULL || node->child == NULL){
        free(node->keys);
        free(node->child);
        return NULL;
    }
    
    return node;
}

/*接下来是二分查找*/
static int binary_search(KEY *keys, KEY key, int left, int right, int *index){
    int mid;
    while(left <= right){
        mid = (left+right) / 2;
        if(item_cmp(keys[mid], key) > 0){
            right = mid - 1;
        }
        else if(item_cmp(keys[mid], key) < 0){
            left = mid + 1;
        }
        else{
            *index = mid; /*找到了*/
            return 1;
        }
    }

    /*接下来是没找到的情况*/
    *index = left;  // left = mid + 1 && left > right, 找右位，与指针对应
    return 0;
}

/*B+ 树的插入是需要对叶子和内部结点插入,这种分情况,所以封装成内部函数*/
static int _bplus_tree_insert(bplus_tree_pt tree, bplus_node_pt node, KEY key, VALUE value){
    // 记录关键信息
    KEY temp;
    bplus_node_pt parent, node2;
    int i, mid;

    /*插入叶子结点,有个多余的申请空间,在外部函数调用之前需要判断*/
    for(i=node->keynum; i> 0 && item_cmp(node->keys[i-1], key)>0; i--){
        node->keys[i] = key;
        node->data[i] = value;
    }

    /*当找到要插入的位置时*/
    node->keys[i] = key;
    node->data[i] = value;
    node->keynum ++;

    // 判断插入后需不需要分裂
    while(node->keynum > tree->max){
        //进行分裂
        if(node->child == NULL){
            //第一遍
            node2 = bplus_node_new_leaf(tree->max);
        }
        else{
            //往回找
            node2 = bplus_node_new_internal(tree->max);
        }
        if (node2 == NULL){
            return 0;
        }

        //结点分裂出来就需要拷贝数据，这里定义左开右闭
        mid = node->keynum/2;
        temp = node->keys[mid]; // 插入的键值
        //再分类讨论
        if(node->child == NULL){
            node2->keynum = node->keynum - mid;
            memcpy(node2->keys, node->keys + mid, sizeof(KEY)*node2->keynum);
            memcpy(node2->data, node->keys + mid, sizeof(VALUE)*node2->keynum);
            node2->next = node->next;
            node->next = node2;
        }
        else{
            //如果是内部节点，除了复制内存之外，还需要改变父指针,选择-1是因为前面进行了移动
            node2->keynum = node->keynum - mid - 1;
            memcpy(node2->keys, node->keys + mid + 1, sizeof(KEY)*node2->keynum);
            memcpy(node2->child, node->keys + mid + 1, sizeof(bplus_node_pt)*node2->keynum);
            for(i=0; i<node2->keynum; i++){
                /*改变孩子的父指针*/
                node2->child[i]->parent = node2;
            }
        }

        node->keynum = mid;

        /*父节点的插入*/
        parent = node->parent;
        if(parent == NULL){
            //如果本身就是最顶层的,需要生成一个新的根
            parent = bplus_node_new_internal(tree->max);
            if(parent == NULL){
                return 0;
            }
            parent->child[0] = node;
            node->parent = parent;
            tree->root = parent;
        }
        
        /*添加数据，和node2有关*/
        for(i=parent->keynum;i>0 && item_cmp(parent->keys[i-1], temp);i--){
            //和插入的key比较
            parent->keys[i] = parent->keys[i-1];
            parent->child[i+1] = parent->child[i];
        }

        parent->keys[i] = temp;
        parent->child[i+1] = node2;
        parent->keynum++;

        node2->parent = parent;
        node = parent; // 往回找
    }

    return 1;
}

inline int bplus_tree_insert(bplus_tree_pt tree, KEY key, VALUE value){
    bplus_node_pt node;
    int ret, index;
    if(tree->root == NULL){
        //如果是刚刚创建的话，就新建node给root
        node = bplus_node_new_leaf(tree->max);
        if(node == NULL){
            return 0;
        }
        tree->root = node;
    }
    //如果不是新创建就直接拿出来用
    node = tree->root;

    /*先找叶节点*/
    while(node->child!=NULL){
        ret = binary_search(node->keys, key, 0, node->keynum-1, &index);
        // 检查return值，并且找到对应的index
        if(ret == 1){
            index++;
        }
        node = node->child[index];  // 因为这里是左闭右开，所以找的是大于等于对应的指针，要++，这样就找到了对应的叶子node
    }
    
    ret = binary_search(node->keys, key, 0, node->keynum-1, &index); // 检查是否存在
    if(ret == 1){
        fprintf(stderr, "[%s][%d] The node is exist!\n", __FILE__, __LINE__);
        return 0;
    }

    _bplus_tree_insert(tree, node, key, value);
    return 1;
}

inline VALUE *bplus_tree_search(bplus_tree_pt tree, KEY key){
    bplus_node_pt node = tree->root;
    int ret=0, index;
    if(node == NULL){
        return NULL;
    }

    ret = binary_search(node->keys, key, 0, node->keynum-1, &index);
    while(node->child != NULL){
        if(ret == 1){
            //区分内部节点的key和非key,key要++,非KEY不需要,和设计有关
            index++;
        }
        node = node->child[index];
        ret = binary_search(node->keys, key, 0, node->keynum-1, &index);    
    }

    if(ret == 0){
        return NULL;
    }
    return &node->data[index];
}

//插入和排序完成了，接下来就是翻转,翻转主要分为两种,叶子和内部节点

/*左旋，把右边大于等于的放到左边去了，同步更新父亲结点*/
static void _bplus_leaf_left_rotate(bplus_node_pt node, int index){
    int i;
    bplus_node_pt left, right;
    left = node->child[index];
    right = node->child[index+1];

    left->keys[left->keynum] = right->keys[0];
    left->data[left->keynum] = right->data[0];

    left->keynum++;  //右边的补给了左边,这种情况一定是可以给的，所以不需要考虑超出了

    for(i=0;i<right->keynum-1;i++){
        //补齐
        right->keys[i] = right->keys[i+1];
        right->data[i] = right->data[i+1];
    }

    right->keynum--;
    node->keys[index] = right->keys[0]; // root 对应的位置更新为right的第一个(已经补齐了的第一个)
}

/*叶子结点的右旋，反过来*/
static void _bplus_leaf_right_rotate(bplus_node_pt node, int index){
    int i;
    bplus_node_pt left, right;
    left = node->child[index];
    right = node->child[index+1];

    for (i=right->keynum; i > 0; i--) {
        right->keys[i] = right->keys[i - 1];  //推一个空位给right
    }
    right->keynum++;

    right->keys[0] = left->keys[left->keynum - 1];
    right->data[0] = left->data[left->keynum - 1];

    left->keynum--;

    node->keys[index] = right->keys[0];
}

/*区别主要在于一个是 数据+key；另一个是child指针+key*/
static void _bplus_internal_left_rotate(bplus_node_pt node, int index){
    int i;
    
}


