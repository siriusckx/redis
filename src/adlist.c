/* adlist.c - A generic doubly linked list implementation
 *
 * Copyright (c) 2006-2010, Salvatore Sanfilippo <antirez at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


#include <stdlib.h>
#include "adlist.h"
#include "zmalloc.h"

/* Create a new list. The created list can be freed with
 * AlFreeList(), but private value of every node need to be freed
 * by the user before to call AlFreeList().
 *
 * 创建一个新的列表，被创建的列表能够被 AlFreeList() 释放，在调用 AlFreeList()之前，
 * 列表中节点的每个私有值，需要用户自己提前释放
 * 
 * On error, NULL is returned. Otherwise the pointer to the new list. 
 * 
 * 如遇错误返回NULL，否则返回链表的指针
 * 
 * T =O(1)
 * */
list *listCreate(void)
{
    struct list *list;

    if ((list = zmalloc(sizeof(*list))) == NULL)
        return NULL;

    //链表的头和尾节点都指向空
    list->head = list->tail = NULL;
    list->len = 0;
    list->dup = NULL;
    list->free = NULL;
    list->match = NULL;
    return list;
}

/**在不破坏链表本身结构前提下，将链表置空
 * 
 * T = O(N)
 */
/* Remove all the elements from the list without destroying the list itself. */
void listEmpty(list *list)
{
    unsigned long len;
    listNode *current, *next;

    //从头节点开始，将连接中的节点资源逐个释放

    //当前节点，指向头节点
    current = list->head;
    len = list->len;
    while(len--) {
        //记录下一个节点的位置
        next = current->next;

        //释放当前节点值对应内存
        if (list->free) list->free(current->value);

        //释放当前指针内存
        zfree(current);

        //当前指针指向下一个节点
        current = next;
    }

    //最后将头节点置空
    list->head = list->tail = NULL;

    //最后将尾节点置0
    list->len = 0;
}

/* Free the whole list.
 *
 * 释放整个链表以及链表中的所有节点
 * 
 * T = O(N)
 * This function can't fail. */
void listRelease(list *list)
{
    //先释放链表中各节点
    listEmpty(list);

    //再释放链表结构本身
    zfree(list);
}

/* Add a new node to the list, to head, containing the specified 'value'
 * pointer as value.
 *
 * 将一个包含有给定值指针 value 的新节点添加到链表的表头
 * 
 * On error, NULL is returned and no operation is performed (i.e. the
 * list remains unaltered).
 * 
 * 如果为新节点分配内存出错，那么不执行任何动作，仅返回NULL
 * 
 * On success the 'list' pointer you pass to the function is returned. 
 *
 * 如果执行成功，返回传入的链表指针
 * 
 * T = O(1) 
 */
list *listAddNodeHead(list *list, void *value)
{
    listNode *node;

    //创建新的节点，为节点分配内存
    if ((node = zmalloc(sizeof(*node))) == NULL)
        return NULL;
    
    //设置节点的内容
    node->value = value;

    if (list->len == 0) {

        //空链表，则将头节点和尾节点都指向新增的节点
        list->head = list->tail = node;

        //空链表中新增节点，该节点上一个和下一个节点都置为NULL
        node->prev = node->next = NULL;
    } else {
        
        //新增加节点的前一个元素置为NULL
        node->prev = NULL;

        //新增加节点的后一个元素指向头节点，进行双向当中的向后绑定
        node->next = list->head;

        //将头节点的前一个元素指向新增加的节点，进行双向当中的向前绑定
        list->head->prev = node;

        //将列表的头节点指向当前节点
        list->head = node;
    }

    //链表长度加1
    list->len++;

    //返回链表头新增元素后的链表
    return list;
}

/* Add a new node to the list, to tail, containing the specified 'value'
 * pointer as value.
 *
 * 将一个包含有给定值指针 value 的新节点添加到链表的表尾
 * 
 * On error, NULL is returned and no operation is performed (i.e. the
 * list remains unaltered).
 * 
 * 如果为新节点分配内存出错，那么不执行任何动作，仅返回NULL
 * 
 * On success the 'list' pointer you pass to the function is returned. 
 * 
 * 如果执行成功，返回传入的链表指针
 * 
 * T = O(1)
 */
list *listAddNodeTail(list *list, void *value)
{
    listNode *node;

    //为新节点分配内存
    if ((node = zmalloc(sizeof(*node))) == NULL)
        return NULL;

    //为新的节点赋值
    node->value = value;
    if (list->len == 0) {

        //链表为空，头节点和尾节点都指向新增的节点
        list->head = list->tail = node;

        //新增节点的前向和后向指针都设置为NULL
        node->prev = node->next = NULL;
    } else {

        //新增节点的前向指针，指向尾节点
        node->prev = list->tail;

        //新增节点的后向指针，指向NULL
        node->next = NULL;

        //将尾节点的后向指针，指向当前节点
        list->tail->next = node;

        //将新增节点作为尾节点
        list->tail = node;
    }

    //更新链表长度
    list->len++;

    //返回链表指针
    return list;
}

/**
 * 
 * 创建一个包含值 value 的新节点，并将它插入到 old_node 的之前或之后
 * 
 * 如果 after 为 0 , 将新节点插入到 old_node 之前。
 * 如果 after 为 1 , 将新节点插入到 old_node 之后。
 * 
 * T = O(1)
 */
list *listInsertNode(list *list, listNode *old_node, void *value, int after) {
    listNode *node;

    //为新创建节点，分配内存
    if ((node = zmalloc(sizeof(*node))) == NULL)
        return NULL;

    //为新的节点，赋值数据
    node->value = value;
    if (after) {
        // 在指定节点之后插入数据
        //节点前向指针指向特定的节点
        node->prev = old_node;

        //节点后向指针指向特定节点的下一个节点
        node->next = old_node->next;

        //特定的节点是尾节点，还需要将尾节点指针重新指向新增的节点
        if (list->tail == old_node) {
            list->tail = node;
        }
    } else {

        // 在指定节点之前插入数据
        //节点后向指针，指向特定的节点
        node->next = old_node;

        //节点的前向指针，指向特定结点的前向节点
        node->prev = old_node->prev;

        //特定的节点是头节点，还需要将头节点指针重新指向新增的节点
        if (list->head == old_node) {
            list->head = node;
        }
    }
    
    //建立前后节点彼此之间的双向关联，以下最容易被忽略

    //插入的节点存在前向节点，需要将插入节点对应的前向节点的后向指针，指向插入的节点
    if (node->prev != NULL) {
        node->prev->next = node;
    }

    //插入的节点存在后向节点，需要将插入节点对应的后向节点的前向指针，指向当前节点
    if (node->next != NULL) {
        node->next->prev = node;
    }

    //更新链表的长度
    list->len++;

    //返回链表的指针
    return list;
}

/* Remove the specified node from the specified list.
 * It's up to the caller to free the private value of the node.
 *
 * 从链表 list 中删除给定节点 node
 * 
 * T = O(1)
 * This function can't fail. */
void listDelNode(list *list, listNode *node)
{
    //节点存在前向节占，将节点对应前向节点的后向指针，指向节点的后向节点
    if (node->prev)
        node->prev->next = node->next;
    else
        //节点不存在前向指针，说明是头节点，将头节点提向节点的后续指针
        list->head = node->next;

    //节点存在后向节点，将节点的后向指针，指向节点的前向节点
    if (node->next)
        node->next->prev = node->prev;
    else
        //节点不存在后向节点，说明是尾节点，将尾节点指向节点的前向节点
        list->tail = node->prev;

    //链表存在释放节点值函数，将节点数据对应资源释放
    if (list->free) list->free(node->value);

    //释放待删除节点内容
    zfree(node);

    //更新边表长度
    list->len--;
}

/* Returns a list iterator 'iter'. After the initialization every
 * call to listNext() will return the next element of the list.
 *
 * 为给定链表创建一个迭代器，
 * 之后每次对这个迭代器调用 listNext 都返回被迭代到的链表节点
 * 
 * direction 参数决定了迭代器的迭代方向：
 * AL_START_HEAD: 从表头向表尾迭代
 * AL_START_TAIL: 从表尾向表头迭代
 * 
 * T = O(1)
 * This function can't fail. */
listIter *listGetIterator(list *list, int direction)
{
    //声明迭代器指针
    listIter *iter;

    //为迭代器分配内存
    if ((iter = zmalloc(sizeof(*iter))) == NULL) return NULL;

    //前向迭代器，迭代器后向指针，指向头节点
    if (direction == AL_START_HEAD)
        iter->next = list->head;
    else

    //后向迭代器，迭代器后向指针，指向尾节点
        iter->next = list->tail;

    //设置迭代器迭代方向属性
    iter->direction = direction;

    //返回迭代器
    return iter;
}

/* Release the iterator memory 
 *
 * 释放迭代器
 * 
 * T =O(1)
 * */
void listReleaseIterator(listIter *iter) {
    zfree(iter);
}

/**
 * 将迭代器的方向设置为 AL_START_HEAD
 * 并将迭代指针重新指向表头节点
 * 
 * T =O(1)
 */
/* Create an iterator in the list private iterator structure*/
void listRewind(list *list, listIter *li) {
    li->next = list->head;
    li->direction = AL_START_HEAD;
}

/**
 * 将迭代器的方向设置为 AL_START_TAIL,
 * 并将迭代指针重新指向表尾节点
 * 
 * T = O(1)
 */
void listRewindTail(list *list, listIter *li) {
    li->next = list->tail;
    li->direction = AL_START_TAIL;
}

/* Return the next element of an iterator.
 * It's valid to remove the currently returned element using
 * listDelNode(), but not to remove other elements.
 * 
 * 返回迭代器当前所指向的节点。
 * 删除当前节点是允许的，但不能修改链表里的其他节点。
 *
 * The function returns a pointer to the next element of the list,
 * or NULL if there are no more elements, so the classical usage patter
 * is:
 *
 * iter = listGetIterator(list,<direction>);
 * while ((node = listNext(iter)) != NULL) {
 *     doSomethingWith(listNodeValue(node));
 * }
 * 
 * 函数要么返回一个节点，要么返回NULL,常见的用法是
 * 
 * iter = listGetIterator(list, <direction>);
 * while((node = listNext(iter)) != NULL)
 * {
 *     doSomethingWith(listNodeValue(node));
 * }
 * 
 * T = O(1)
 *
 * */
listNode *listNext(listIter *iter)
{
    listNode *current = iter->next;

    if (current != NULL) {
        //根据方向选择下一个节点
        if (iter->direction == AL_START_HEAD)

            //保存下一个节点，防止当前节点被删除而造成指针丢失
            iter->next = current->next;
        else

            //保存下一个节点，防止当前节点被删除而造成指针丢失
            iter->next = current->prev;
    }
    return current;
}

/* Duplicate the whole list. On out of memory NULL is returned.
 * On success a copy of the original list is returned.
 *
 * 复制整个链表
 * 
 * 复制成功返回输入链表的副本
 * 如果因为内存不足而造成复制失败，返回 NULL 。
 * 
 * The 'Dup' method set with listSetDupMethod() function is used
 * to copy the node value. Otherwise the same pointer value of
 * the original node is used as value of the copied node.
 * 
 * 如果链表有设置复制函数 dup ,那么对值的复制将使用复制函数进行，
 * 否则，新节点将和旧节点共享同一个指针。
 *
 * The original list both on success or error is never modified. 
 * 
 * 无论复制是成功还是失败，输入链表都不会修改
 * 
 * T = O(N)
 * 
 */
list *listDup(list *orig)
{
    //用于复制的链表、迭代器、节点
    list *copy;
    listIter iter;
    listNode *node;

    //创建空链表用于存储复制的节点
    if ((copy = listCreate()) == NULL)
        return NULL;
    
    //设置用于存储复制节点链表的处理函数
    copy->dup = orig->dup;
    copy->free = orig->free;
    copy->match = orig->match;

    //将迭代器重置指向待复制链表的开头
    listRewind(orig, &iter);

    //利用迭代器，取节点
    while((node = listNext(&iter)) != NULL) {
        void *value;

        if (copy->dup) {
            //如果存在复制节点值的函数 dup, 则对原节点的 value 值进行复制（深拷贝）
            value = copy->dup(node->value);

            //其中一个节点复制失败，需要将已复制过的链表资源释放，返回 NULL
            if (value == NULL) {
                listRelease(copy);
                return NULL;
            }
        } else
            //未设置节点 value 对应的复制函数 dup, 新节点和原节点共享同一个值指针（浅拷贝）
            value = node->value;

        //将复制出来的节点值，往目标链表尾部添加，添加失败，则将已复制过的链表资源释放，返回NULL
        if (listAddNodeTail(copy, value) == NULL) {
            listRelease(copy);
            return NULL;
        }
    }

    //返回复制后的链表
    return copy;
}

/* Search the list for a node matching a given key.
 * The match is performed using the 'match' method
 * set with listSetMatchMethod(). If no 'match' method
 * is set, the 'value' pointer of every node is directly
 * compared with the 'key' pointer.
 * 
 * 查找链表 list 中值和 key 匹配的节点。
 * 
 * 对比操作由链表的 match 函数负责进行，
 * 如果没有设置match函数，
 * 那么直接通过对比值的指针来决定是否匹配
 *
 * On success the first matching node pointer is returned
 * (search starts from head). If no matching node exists
 * NULL is returned. 
 * 
 * 如果匹配成功，那么第一个匹配的节点会被返回。
 * 如果没有匹配任何节点，那么返回 NULL
 * 
 * T = O(N)
 */
listNode *listSearchKey(list *list, void *key)
{
    //查找节点用到迭代器
    listIter iter;
    listNode *node;

    //将迭代器轩为链表的头节点位置
    listRewind(list, &iter);

    //从前往后查找节点
    while((node = listNext(&iter)) != NULL) {
        if (list->match) {
            //设置了匹配函数，按照匹配函数进行匹配查找节点，匹配成功，返回找到的节点
            if (list->match(node->value, key)) {
                return node;
            }
        } else {
            //未设置匹配函数，直接用 key 和节点的 value 进行比较，相等则返回节点
            if (key == node->value) {
                return node;
            }
        }
    }

    //没找到节点，返回NULL
    return NULL;
}

/* Return the element at the specified zero-based index
 * where 0 is the head, 1 is the element next to head
 * and so on. Negative integers are used in order to count
 * from the tail, -1 is the last element, -2 the penultimate
 * and so on. If the index is out of range NULL is returned. 
 * 
 * 返回链表在给定索引上的值。
 * 
 * 索引以 0 为起始，也可是负数， -1 表示链表最后一个节点，诸如此类。
 * 
 * 如果索引超出范围（out of range），返回 NULL .
 * 
 * T = O(N)
 * */
listNode *listIndex(list *list, long index) {
    //返回的节点
    listNode *n;

    if (index < 0) {
        //索引为负值，从链表后面开始遍历，直到遍历|index|-1个元素为止
        index = (-index)-1;
        n = list->tail;
        while(index-- && n) n = n->prev;
    } else {

        //索引为正，从链表头部开始遍历，直接遍历index 个元素为止
        n = list->head;
        while(index-- && n) n = n->next;
    }

    //返回查找到的节点
    return n;
}


/* Rotate the list removing the tail node and inserting it to the head. 
 *
 * 取出链表的表尾节点，并将它移动到表头，成为新的表头节点
 * 
 * T = O(1)
 * */
void listRotate(list *list) {
    //表尾节点
    listNode *tail = list->tail;

    //链表长度小于等于1，直接返回，结束程序
    if (listLength(list) <= 1) return;

    /* Detach current tail */
    //将表尾指针向前移一个节点，重新建立尾节点关系
    list->tail = tail->prev;
    list->tail->next = NULL;

    /* Move it as head */
    //插入到表头
    //设置表头节点的前向指指，指向取出来的尾节点
    list->head->prev = tail;

    //设置取出来的尾节点的前向指针为 NULL，因为即将要变成了头节点
    tail->prev = NULL;

    //设置取出来的尾节点的后向指针，指向原链表头节点
    tail->next = list->head;

    //原链表头节点指向提取出来的尾节点，新的头节点诞生
    list->head = tail;
}

/* Add all the elements of the list 'o' at the end of the
 * list 'l'. The list 'other' remains empty but otherwise valid. 
 * 
 * 两个链表的合并。
 * 
 * 将链表 o 合并到链表 l 的尾部
 * */
void listJoin(list *l, list *o) {
    //链表 o 存在头节点，将头节点的前向指针，指向链表 l 的尾节点
    if (o->head)
        o->head->prev = l->tail;

    //链表 l 存在尾节点，将尾节点的后向指针，指向链表 o 的头节点
    if (l->tail)
        l->tail->next = o->head;
    else

    //链表 l 不存在尾节点，将 l 头节点指向链表 o 的头节点
        l->head = o->head;

    //链表 o 的尾节点存在，最后应将链表 l 的尾节点，最终指向 o 的尾节点
    if (o->tail) l->tail = o->tail;

    //更新合并链表的总长度
    l->len += o->len;

    /* Setup other as an empty list. */
    //将合并到其他地方去的链表 other 置为空链表
    o->head = o->tail = NULL;
    o->len = 0;
}
