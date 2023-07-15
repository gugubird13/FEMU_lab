#ifndef BPLUS_TREE_H
#define BPLUS_TREE_H

#include "stdint.h"
struct ppa;
typedef uint64_t KEY;
typedef struct ppa *VALUE;
//typedef int VALUE;

typedef struct bplus_node_s* bplus_node_pt;

typedef struct bplus_node_s {
    int keynum;
    KEY *keys;            /* 主健，最大max个,最小min个, 空间max+1*/
    VALUE *data;          /* 数据，最大max个,最小min个, 空间max+1，内部节点时，data是NULL */
    bplus_node_pt *child; /* 子节点，最大max+1个，最小min + 1个, 空间max+2，叶子节点时，child是NULL */
    bplus_node_pt parent;
    bplus_node_pt next;   /* 兄弟节点，仅是叶子节点时有值 */
} bplus_node_t;

typedef struct bplus_tree_s* bplus_tree_pt;

typedef struct bplus_tree_s {
    bplus_node_pt root;
    int max;
    int min;
} bplus_tree_t;

int bplus_tree_create(bplus_tree_pt *_tree, int m);
int bplus_tree_insert(bplus_tree_pt tree, KEY key, VALUE value);
VALUE bplus_tree_search(bplus_tree_pt tree, KEY key);
void bplus_tree_delete(bplus_tree_pt tree, KEY key);
void bplus_tree_destroy(bplus_tree_pt tree);
void bplus_tree_print(bplus_tree_pt tree);
void bplus_tree_export_dot(bplus_tree_pt tree, char *filename, char *label);

#endif