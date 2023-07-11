#ifndef BPLUS_TREE_H
#define BPLUS_TREE_H

typedef int KEY;
typedef int VALUE;

typedef struct bplus_node_s* bplus_node_pt; /*结点指针pointer*/

typedef struct bplus_node_s{
    int keynum;
    KEY *keys;                                  /*主键，最大max,最小min,空间大小为max+1*/
    VALUE *data;                                /*内部节点data为空,否则最大max,最小min,空间为max+1*/    
    bplus_node_pt *child;                       /*叶子结点child为null,最大是max+1,最小是min+1(和关键字个数有关),空间大小就是m+2*/
    bplus_node_pt parent;                       
    bplus_node_pt next;                         /*兄弟结点*/
}bplus_node_t;

//继续定义树指针和树结构

typedef struct bplus_tree_s* bplus_tree_pt;

typedef struct bplus_tree_s {
    bplus_node_pt root;
    int max;
    int min;
} bplus_tree_t;

//所有指针的指针，都可以理解为面向对象的设计方法，方便构建一堆对象(数组的思想)
inline int bplus_tree_create(bplus_tree_pt * _tree, int m);
inline int bplus_tree_insert(bplus_tree_pt tree, KEY key, VALUE value);

inline VALUE *bplus_tree_search(bplus_tree_pt tree, KEY key);

inline void bplus_tree_delete(bplus_tree_pt tree, KEY key);
inline void bplus_tree_print(bplus_tree_pt tree);
//对外暴露这些函数应该就够了


#endif