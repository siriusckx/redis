# 1. 前言
> 因为工作原因，需要对redis源码进行相应的分析和阅读，并在redis的基础上封装一个自己的存储结构来使用，现需要对redis进行全面深入的分析，先从源码阅读开始。

以下内容转载自：http://blog.huangz.me/diary/2014/how-to-read-redis-source-code.html

## 2 阅读数据结构实现

> 刚开始阅读Redis源码的时候，最好从数据结构的相关文件开始读起，因为这些文件和Redis中的其他部分耦合最少，并且这些文件所实现的数据结构在大部分算法书上都可以了解到，所以从这些文件开始读是最轻松的，难度也是最低的。

> 下表列出了Redis源码中，各个数据结构的实现文件：

|文件|内容|阅读情况|
|--|--|--|
|stds.h和sds.c|Redis的动态字符串实现|

## 2.1 SDS简单字符串

## 2.2 双向链表

## 2.3 字典（哈希表实现）

## 2.4 跳表
> Sorted Set 有两种编码方式 ziplist， skiplist。其实它和set很相似，也是依靠检查输入的第一个元素，如果第一个元素长度小于服务器属性 server.zset_max_ziplist_value 的值（默认为 64 ）并且服务器属性 server.zset_max_ziplist_entries 的值大于0时，就会默认创建为 ziplist 编码。否则，程序就创建为 skiplist 编码。同理可推得当新元素的长度超过 server.zset_max_ziplist_value 的值就会自动转换为 skiplist 编码。