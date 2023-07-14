#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bplus_tree.h"

/* 
 * a==b，返回0, a>b，返回正数，a<b 返回负数
 * 
 */
#define item_cmp(a, b) ((a) - (b))


static bplus_node_pt bplus_node_new_leaf(int m);
static bplus_node_pt bplus_node_new_internal(int m);
static int binary_search(KEY *keys, KEY key, int left, int right, int *index);
static int _bplus_tree_insert(bplus_tree_pt tree, bplus_node_pt node, KEY key, VALUE value);
static void _bplus_leaf_left_rotate(bplus_node_pt node, int index);
static void _bplus_leaf_right_rotate(bplus_node_pt node, int index);
static void _bplus_internal_left_rotate(bplus_node_pt node, int index);
static void _bplus_internal_right_rotate(bplus_node_pt node, int index);
static void _bplus_node_merge(bplus_node_pt node, int index);
static void _bplus_tree_delete(bplus_tree_pt tree, bplus_node_pt node, int index);
static void _bplus_node_destory(bplus_node_pt node);

int 
bplus_tree_create(bplus_tree_pt *_tree, int m)
{
    bplus_tree_pt tree = (bplus_tree_pt)malloc(sizeof(bplus_tree_t));
    if (tree == NULL) {
        return 0;
    }
    tree->root = NULL;
    tree->max = m;
    tree->min = m/2;
    *_tree = tree;
    return 1;
}

/* 
 *        Name:  bplus_node_new_leaf
 * Description:  创建新的叶子节点
 * 
 */
static bplus_node_pt 
bplus_node_new_leaf(int m)
{
    bplus_node_pt node = (bplus_node_pt)malloc(sizeof(bplus_node_t));
    if (node == NULL) {
        return NULL;
    }
    node->parent = NULL;
    node->next = NULL;
    node->keynum = 0;
    node->keys = (KEY *)malloc(sizeof(KEY)*(m + 1));
    node->data = (VALUE *)malloc(sizeof(VALUE)*(m + 1));
    if (node->keys == NULL || node->data == NULL) {
        free(node->keys);
        free(node->data);
        free(node);
        return NULL;
    }
    node->child = NULL;
    return node;
}

/* 
 *        Name:  bplus_node_new_internal
 * Description:  创建新的内部节点
 * 
 */
static bplus_node_pt 
bplus_node_new_internal(int m)
{
    bplus_node_pt node = (bplus_node_pt)malloc(sizeof(bplus_node_t));
    if (node == NULL) {
        return NULL;
    }
    node->parent = NULL;
    node->next = NULL;
    node->keynum = 0;
    node->keys = (KEY *)malloc(sizeof(KEY)*(m + 1));
    node->data = NULL;
    node->child = (bplus_node_pt *)malloc(sizeof(bplus_node_pt)*(m + 2));
    if (node->keys == NULL || node->child == NULL) {
        free(node->keys);
        free(node->child);
        free(node);
        return NULL;
    }
    return node;
}

/* 
 *        Name:  binary_search
 * Description:  二分查找，找到数组中指定key的位置，如果存在，返回1, 否则返回0
 *               如果找到索引为该值位置，否则返回右邻值位置(child的位置)
 * 
 */
static int 
binary_search(KEY *keys, KEY key, int left, int right, int *index)
{
    int mid;

    while (left <= right) {
        mid = (left + right)/2;
        if (item_cmp(keys[mid], key) > 0) {
            right = mid - 1;
        }
        else if (item_cmp(keys[mid], key) < 0) {
            left = mid + 1;
        }
        else {
            *index = mid;
            return 1;
        }
    }

    *index = left;
    return 0;
}


/* 
 *        Name:  _bplus_tree_insert
 * Description:  元素插入及分裂操作
 * 
 */
static int 
_bplus_tree_insert(bplus_tree_pt tree, bplus_node_pt node, KEY key, VALUE value)
{
    KEY temp;
    bplus_node_pt parent, node2;
    int i, mid;

    /* 插入叶子节点 */
    for (i=node->keynum; i>0 && item_cmp(node->keys[i-1], key) > 0; i--) {
        node->keys[i] = node->keys[i - 1];
        node->data[i] = node->data[i - 1];
    }

    node->keys[i] = key;
    node->data[i] = value;
    node->keynum++;

    while (node->keynum > tree->max) {

        /* 分裂节点 */
        if (node->child == NULL) {
            /* 叶子节点分裂 */
            node2 = bplus_node_new_leaf(tree->max);
        }
        else {
            /* 内部节点分裂 */
            node2 = bplus_node_new_internal(tree->max);
        }
        if (node2 == NULL) {
            return 0;
        }

        /* 拷贝数据 */
        mid = node->keynum/2;
        temp = node->keys[mid];
        if (node->child == NULL) {
            node2->keynum = node->keynum - mid;
            memcpy(node2->keys, node->keys + mid, sizeof(KEY)*(node2->keynum));
            memcpy(node2->data, node->data + mid, sizeof(KEY)*(node2->keynum));
            node2->next = node->next;
            node->next = node2;
        }
        else {
            node2->keynum = node->keynum - mid - 1;
            memcpy(node2->keys, node->keys + mid + 1, sizeof(KEY)*(node2->keynum));
            memcpy(node2->child, node->child + mid + 1, sizeof(bplus_node_pt)*(node->keynum - mid));
            /* 重设父指针 */
            for (i=0; i<=node2->keynum; i++) {
                node2->child[i]->parent = node2;
            }
        }
        node->keynum = mid;

        /* 插入父节点 */
        parent = node->parent;
        if (parent == NULL) {
            /* 生成新的树根节点 */
            parent = bplus_node_new_internal(tree->max);
            if (parent == NULL) {
                return 0;
            }
            parent->child[0] = node;
            node->parent = parent;
            tree->root = parent;
        }
        /* 增加数据和右子树 */
        for (i=parent->keynum; i>0 && item_cmp(parent->keys[i-1], temp) > 0; i--) {
            parent->keys[i] = parent->keys[i - 1];
            parent->child[i + 1] = parent->child[i];
        }
        parent->keys[i] = temp;
        parent->child[i + 1] = node2;
        parent->keynum++;

        node2->parent = parent;
        node = parent;
    }

    return 1;
}

int 
bplus_tree_insert(bplus_tree_pt tree, KEY key, VALUE value)
{
    bplus_node_pt node;
    int ret, index;
    if (tree->root == NULL) {
        node = bplus_node_new_leaf(tree->max);
        if (node == NULL) {
            return 0;
        }
        tree->root = node;
    }
    node = tree->root;
    /* 查找到叶节点 */
    while (node->child != NULL) {
        ret = binary_search(node->keys, key, 0, node->keynum - 1, &index);
        if (ret == 1) {
            index++;
        }
        node = node->child[index];
    }
    ret = binary_search(node->keys, key, 0, node->keynum - 1, &index);
    if (ret == 1) {
        fprintf(stderr, "[%s][%d] The node is exist!\n", __FILE__, __LINE__);
        return 0;
    }
    _bplus_tree_insert(tree, node, key, value);
    return 1;
}

VALUE * 
bplus_tree_search(bplus_tree_pt tree, KEY key)
{
    bplus_node_pt node = tree->root;
    int ret = 0, index;
    if (node == NULL) {
        return NULL;
    }

    ret = binary_search(node->keys, key, 0, node->keynum - 1, &index);
    while (node->child != NULL) {
        if (ret == 1) {
            index++;
        }
        node = node->child[index];
        ret = binary_search(node->keys, key, 0, node->keynum - 1, &index);
    }

    if (ret == 0) {
        return NULL;
    }

    return &node->data[index];
}

/* 
 *        Name:  _bplus_leaf_left_rotate
 * Description:  叶子节点值左旋转，把右节点的第一个值移到左节点
 * 
 */
static void 
_bplus_leaf_left_rotate(bplus_node_pt node, int index)
{
    int i;
    bplus_node_pt left, right;
    left = node->child[index];
    right = node->child[index + 1];

    left->keys[left->keynum] = right->keys[0];
    left->data[left->keynum] = right->data[0];
    left->keynum++;

    for (i=0; i<right->keynum - 1; i++) {
        right->keys[i] = right->keys[i + 1];
        right->data[i] = right->data[i + 1];
    }
    right->keynum--;

    node->keys[index] = right->keys[0];
}

/* 
 *        Name:  _bplus_leaf_right_rotate
 * Description:  叶子节点值右旋转，把左节点的最后一个值移到右节点
 * 
 */
static void 
_bplus_leaf_right_rotate(bplus_node_pt node, int index)
{
    int i;
    bplus_node_pt left, right;
    left = node->child[index];
    right = node->child[index + 1];

    for (i=right->keynum; i > 0; i--) {
        right->keys[i] = right->keys[i - 1];
    }
    right->keynum++;

    right->keys[0] = left->keys[left->keynum - 1];
    right->data[0] = left->data[left->keynum - 1];

    left->keynum--;

    node->keys[index] = right->keys[0];
}

/* 
 *        Name:  _bplus_internal_left_rotate
 * Description:  节点值左旋转，把右节点的第一个值移到父节点，父节点的对应值移到左节点
 * 
 */
static void 
_bplus_internal_left_rotate(bplus_node_pt node, int index)
{
    int i;
    bplus_node_pt left, right;
    left = node->child[index];
    right = node->child[index + 1];

    left->keys[left->keynum] = node->keys[index];
    left->child[left->keynum + 1] = right->child[0];
    left->keynum++;

    node->keys[index] = right->keys[0];

    for (i=0; i<right->keynum - 1; i++) {
        right->keys[i] = right->keys[i + 1];
    }
    for (i=0; i<right->keynum; i++) {
        right->child[i] = right->child[i + 1];
    }
    right->keynum--;
}

/* 
 *        Name:  _bplus_internal_right_rotate
 * Description:  节点值右旋转，把左节点的最后一个值移到父节点，父节点的对应值移到右节点
 * 
 */
static void 
_bplus_internal_right_rotate(bplus_node_pt node, int index)
{
    int i;
    bplus_node_pt left, right;
    left = node->child[index];
    right = node->child[index + 1];

    for (i=right->keynum; i>0; i--) {
        right->keys[i] = right->keys[i - 1];
    }
    right->keys[0] = node->keys[index];
    for (i=right->keynum + 1; i>0; i--) {
        right->child[i] = right->child[i - 1];
    }
    right->child[0] = left->child[left->keynum];
    right->keynum++;

    node->keys[index] = left->keys[left->keynum - 1];

    left->keynum--;
}

/* 
 *        Name:  _bplus_node_merge
 * Description:  合并节点元素的左右两个子节点
 * 
 */
static void 
_bplus_node_merge(bplus_node_pt node, int index)
{
    int i;
    bplus_node_pt left, right;
    left = node->child[index];
    right = node->child[index + 1];

    if (left->child != NULL) {
        /* 修改左子节点 */
        left->keys[left->keynum] = node->keys[index];

        memcpy(left->keys + left->keynum + 1, right->keys, sizeof(KEY)*(right->keynum));

        if (left->child != NULL) {
            for (i=0; i<=right->keynum; i++) {
                right->child[i]->parent = left;
                left->child[left->keynum + i + 1] = right->child[i];
            }
        }
        left->keynum = left->keynum + right->keynum + 1;
    }
    else {
        memcpy(left->keys + left->keynum, right->keys, sizeof(KEY)*(right->keynum));
        memcpy(left->data + left->keynum, right->data, sizeof(VALUE)*(right->keynum));
        left->keynum = left->keynum + right->keynum;
    }

    /* 修改父节点 */
    for (i=index; i<node->keynum - 1; i++) {
        node->keys[i] = node->keys[i + 1];
    }
    for (i=index + 1; i<node->keynum; i++) {
        node->child[i] = node->child[i + 1];
    }
    node->keynum--;

    /* 释放右节点 */
    free(right->keys);
    free(right->data);
    free(right->child);
    free(right);
}

/* 
 *        Name:  _bplus_tree_delete
 * Description:  数据删除
 * 
 */
static void 
_bplus_tree_delete(bplus_tree_pt tree, bplus_node_pt node, int index)
{
    bplus_node_pt parent, sibling;
    int ret, i;
    /* 删除叶节点指定值 */
    for (i=index; i<node->keynum - 1; i++) {
        node->keys[i] = node->keys[i + 1];
        node->data[i] = node->data[i + 1];
    }
    node->keynum--;
    parent = node->parent;

    while (node->keynum < tree->min && parent != NULL) {
        /* 寻找当前节点在父节点的位置 */
        for (index=0; index<=parent->keynum && parent->child[index] != node; index++);
        if (index > parent->keynum) {
            fprintf(stderr, "[%s][%d]Didn't find node in parent's children array !\n", __FILE__, __LINE__);
            return;
        }

        if (index == 0) {
            /* 
             * 节点是父第0个子节点，兄弟是第1个子节点
             * 中间的分隔元素是父中的第0个元素
             * 
             */
            sibling = parent->child[1];
            if (sibling->keynum > tree->min) {
                /* 从兄弟迁移数据 */
                if (node->child == NULL) {
                    _bplus_leaf_left_rotate(parent, 0); 
                }
                else {
                    _bplus_internal_left_rotate(parent, 0); 
                }

            }
            else {
                /* 合并兄弟 */
                _bplus_node_merge(parent, 0);
            }
        }
        else {
            /* 
             * 节点是父中第index个子节点，兄弟是第index - 1个子节点
             * 中间的分隔元素是父中的第index - 1个元素
             * 
             */
            sibling = parent->child[index - 1];
            if (sibling->keynum > tree->min) {
                /* 从兄弟迁移数据 */
                if (node->child == NULL) {
                    _bplus_leaf_right_rotate(parent, index - 1);
                }
                else {
                    _bplus_internal_right_rotate(parent, index - 1);
                }
            }
            else {
                /* 合并兄弟 */
                _bplus_node_merge(parent, index - 1);
            }
        }

        node = parent;
        parent = node->parent;
    }
    if (tree->root->keynum == 0 && tree->root->child != NULL) {
        node = tree->root;
        tree->root = node->child[0];
        free(node->keys);
        free(node->data);
        free(node->child);
        free(node);
    }
}

void 
bplus_tree_delete(bplus_tree_pt tree, KEY key)
{
    bplus_node_pt node = tree->root, node2;
    int ret = 0, index;
    if (node == NULL) {
        return;
    }

    /* 查找到删除数据的节点 */
    while (node->child != NULL) {
        ret = binary_search(node->keys, key, 0, node->keynum - 1, &index);
        if (ret == 1) {
            index++;
        }
        node = node->child[index];
    }
    ret = binary_search(node->keys, key, 0, node->keynum - 1, &index);
    if (ret == 0) {
        return;
    }

    _bplus_tree_delete(tree, node, index);

    return;
}

/* 
 *        Name:  _bplus_node_destory
 * Description:  节点递归删除
 * 
 */
static void 
_bplus_node_destory(bplus_node_pt node)
{
    int i;
    if (node == NULL) {
        return;
    }
    if (node->child != NULL) {
        for (i=0; i<=node->keynum; i++) {
            _bplus_node_destory(node->child[i]);
        }
        free(node->child);
    }

    free(node->keys);
    free(node->data);
    free(node);
}

void 
bplus_tree_destroy(bplus_tree_pt tree)
{
    _bplus_node_destory(tree->root);
    free(tree);
}


static void 
bplus_node_printnode(KEY *key, int h)
{
    int i;
    for (i=0; i < h; i++) {
        printf("    ");
    }
    if (key == NULL) {
        printf("*\n");
    }
    else {
        printf("%d\n", *key);
    }
}

static void 
bplus_node_show(bplus_node_pt node, int h)
{
    int i;

    if (node == NULL) {
        return;
    }

    if (node->child != NULL) {
        bplus_node_show(node->child[0], h + 1);
    }
    for (i=0; i<node->keynum; i++) {
        bplus_node_printnode(&node->keys[i], h);
        if (node->child != NULL) {
            bplus_node_show(node->child[i + 1], h + 1);
        }
    }
}

void 
bplus_tree_print(bplus_tree_pt tree)
{
    bplus_node_show(tree->root, 0);
}

/* 打印节点计数变量 */
static int node_num = 0;

static void 
export_dot(FILE *fp, bplus_node_pt node, char *last_label, int field)
{
    int i;

    /* 打印节点 */
    char node_name[10] = {0};
    sprintf(node_name, "n%d", node_num);

    fprintf(fp, "%s[label=\"", node_name);
    if (node->child != NULL) {
        fprintf(fp, "<f0>");
        for (i=0; i<node->keynum; i++) {
            fprintf(fp, " | %d", node->keys[i]);
            fprintf(fp, " | <f%d>", i + 1);
        }
    }
    else {
        fprintf(fp, "(%d, %d)", node->keys[0], node->data[0]);
        for (i=1; i<node->keynum; i++) {
            fprintf(fp, " | (%d, %d)", node->keys[i], node->data[i]);
        }
    }
    fprintf(fp, "\"];\n");
    node_num++;

    /* 打印边 */
    if (last_label != NULL) {
        fprintf(fp, "%s:f%d->%s;\n", last_label, field, node_name);
    }

    if (node->child == NULL) {
        return;
    }

    /* 打印子节点 */
    for (i=0; i<=node->keynum; i++) {
        export_dot(fp, node->child[i], node_name, i);
    }
}


void bplus_tree_export_dot(bplus_tree_pt tree, char *filename, char *label)
{
    FILE *fp;
    fp = fopen(filename, "w");
    fprintf(fp, "digraph g{\nnode[shape=record];\nlabel=\"%s\";\nlabeljust=l;\nlabelloc=t;\nsplines=line;\n", label);
    export_dot(fp, tree->root, NULL, -1);
    fprintf(fp, "}\n");
    fclose(fp);
}