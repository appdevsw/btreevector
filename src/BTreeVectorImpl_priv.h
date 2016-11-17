/*
 * Author: appdevsw@wp.pl
 *
 */


#ifndef BTREEVECTORIMPL_H_
#define BTREEVECTORIMPL_H_

#include <iostream>
#include <cstring>
#include <algorithm>
#include <assert.h>

//forward declaration
template<typename T, int MAX_NODE_BLOCK_SIZE, int MAX_LEAF_BLOCK_SIZE>
class BTreeVector;

template<typename T, int MAX_NODE_BLOCK_SIZE = 16, int MAX_LEAF_BLOCK_SIZE = 128>
class BTreeVectorImpl
{
    friend class BTreeVector<T, MAX_NODE_BLOCK_SIZE, MAX_LEAF_BLOCK_SIZE> ;

    static_assert((MAX_NODE_BLOCK_SIZE % 2) == 0 && MAX_NODE_BLOCK_SIZE >= 4,"Template parameter MAX_NODE_BLOCK_SIZE must be even and not less than 4");
    static_assert((MAX_LEAF_BLOCK_SIZE % 2) == 0 && MAX_LEAF_BLOCK_SIZE >= 4,"Template parameter MAX_LEAF_BLOCK_SIZE must be even and not less than 4");

    struct Node;
    struct Path;

    Node * root;
    int structModCount = 0;
    Path cachePath = Path(this);

    template<typename BT, bool BT_IS_TRIVIAL, int INCREASE_PRC, int MAX_SIZE>
    struct DataBlock
    {
    private:
        typedef DataBlock<BT, BT_IS_TRIVIAL, INCREASE_PRC, MAX_SIZE> ThisDataBlock;
        BT * buf, *orgBuf;
        int bufSize = MAX_SIZE >> 1;
        int count = 0;
        const static bool moveopt = true;

    public:

        DataBlock()
        {
            if (BT_IS_TRIVIAL)
                orgBuf = buf = (BT*) malloc(bufSize * sizeof(BT));
            else
                orgBuf = buf = new BT[bufSize];
        }

        ~DataBlock()
        {
            if (BT_IS_TRIVIAL)
                free(orgBuf);
            else
                delete[] orgBuf;
        }

        inline BT get(int idx)
        {
            return buf[idx];
        }

        inline BT & getRef(int idx)
        {
            return buf[idx];
        }

        inline int size()
        {
            return count;
        }

        void add(BT element)
        {
            ensure(count + 1);
            buf[count++] = std::move(element);
        }

        void add(int idx, BT element)
        {
            if (moveopt && idx == 0 && buf != orgBuf)
            {
                buf--;
                bufSize++;
            } else
            {
                expand(idx, 1);
            }
            count++;
            buf[idx] = std::move(element);
        }

        void set(int idx, BT element)
        {
            buf[idx] = std::move(element);
        }

        void remove(int idx)
        {
            if (moveopt && idx == 0)
            {
                buf++;
                bufSize--;
                count--;
            } else
            {
                removeRange(idx, 1);
            }
        }

        void insertRange(ThisDataBlock * dst, int from, int to, int cnt)
        {
            assert(dst != this);
            dst->expand(to, cnt);
            xmemmove(&dst->buf[to], &buf[from], cnt);
            dst->count += cnt;
        }

        void removeRange(int start, int cnt)
        {
            int end = start + cnt;
            if (end < count)
                xmemmove(&buf[start], &buf[end], (count - end));
            count -= cnt;
        }

    private:
        void ensure(int size)
        {
            if (size <= bufSize)
                return;
            int newSize = std::min(MAX_SIZE, std::max(size, bufSize * INCREASE_PRC / 100));
            assert(newSize >= size);
            xrealloc(newSize);
        }

        void xrealloc(int newSize)
        {
            int diff = buf - orgBuf;
            if (diff + bufSize >= newSize)
            {
                xmemmove(orgBuf, buf, count);
                buf = orgBuf;
                bufSize += diff;
            } else
            {
                BT * newBuf;
                if (BT_IS_TRIVIAL)
                {
                    if (diff)
                    {
                        newBuf = (BT*) malloc(newSize * sizeof(BT));
                        assert(newBuf != nullptr);
                        xmemmove(newBuf, buf, count);
                        free(orgBuf);
                    } else
                    {
                        newBuf = (BT*) realloc(orgBuf, newSize * sizeof(BT));
                        assert(newBuf != nullptr);
                    }
                } else
                {
                    newBuf = new BT[newSize];
                    xmemmove(newBuf, buf, count);
                    delete[] orgBuf;
                }
                orgBuf = buf = newBuf;
                bufSize = newSize;
            }
        }

        void xmemmove(BT * dst, BT * src, int cnt, bool reverse = false)
        {
            if (BT_IS_TRIVIAL)
                memmove(dst, src, cnt * sizeof(BT));
            else if (reverse)
                xreversemove(src, cnt, dst);
            else
                xmove(src, cnt, dst);
        }

        void expand(int from, int cnt)
        {
            ensure(count + cnt);
            if (from < count)
                xmemmove(&buf[from + cnt], &buf[from], (count - from), true);
        }

        inline void xreversemove(BT * src, int cnt, BT * dst)
        {
            while (cnt-- > 0)
                dst[cnt] = std::move(src[cnt]);
        }

        inline void xmove(BT * src, int cnt, BT * dst)
        {
            for (int i = 0; i < cnt; i++)
                dst[i] = std::move(src[i]);
        }

    };

    struct Node
    {
        typedef DataBlock<Node *, true, 200, MAX_NODE_BLOCK_SIZE> InternalNodeDataBlock;
        typedef DataBlock<T, std::is_pod<T>::value, 200, MAX_LEAF_BLOCK_SIZE> LeafDataBlock;

        union Data
        {
            InternalNodeDataBlock * childrenNodes;
            LeafDataBlock * childrenValues;
        } data;
        int count = 0;
        bool isLeaf;

        Node(bool isLeaf)
        {
            this->isLeaf = isLeaf;
            if (isLeaf)
                data.childrenValues = new LeafDataBlock();
            else
                data.childrenNodes = new InternalNodeDataBlock();
        }

        ~Node()
        {
            if (isLeaf)
                delete data.childrenValues;
            else
                delete data.childrenNodes;
        }

        inline int csize()
        {
            return isLeaf ? data.childrenValues->size() : data.childrenNodes->size();
        }

        inline int maxBlockSize()
        {
            return isLeaf ? MAX_LEAF_BLOCK_SIZE : MAX_NODE_BLOCK_SIZE;
        }

        inline int halfBlockSize()
        {
            return maxBlockSize() >> 1;
        }

    };

    struct PathNode
    {
        int childIdx = 0;
        int countedPos = 0;
        Node * childNode = nullptr;
        Node * node = nullptr;
        PathNode * nextDown = nullptr;
        PathNode * parent = nullptr;

        PathNode()
        {
        }

        ~ PathNode()
        {
        }

        void init(Node * n)
        {
            node = n;
            countedPos = childIdx = 0;
        }

        Node * findChild(int pos)
        {
            if (node->isLeaf)
            {
                countedPos = childIdx = pos;
                return childNode = nullptr;
            }
            int size = node->data.childrenNodes->size();
            if (pos > node->count >> 1)
            {
                int sum = node->count;
                for (childIdx = size - 1; childIdx >= 0; childIdx--)
                {
                    childNode = node->data.childrenNodes->get(childIdx);
                    sum -= childNode->count;
                    if (sum <= pos)
                    {
                        countedPos = pos - sum;
                        return childNode;
                    }
                }
            } else
            {

                int sum = 0;
                for (childIdx = 0; childIdx < size; childIdx++)
                {
                    childNode = node->data.childrenNodes->get(childIdx);
                    sum += childNode->count;
                    if (sum > pos)
                    {
                        countedPos = pos - sum + childNode->count;
                        return childNode;
                    }
                }
            }
            std::cerr << "index out of range: " << pos << "\n";
            throw;
        }
    };

    struct Path
    {
        PathNode * pn = nullptr;
        Node * child = nullptr;
        BTreeVectorImpl * bta;
        PathNode * pathRoot = nullptr;
    public:
        int position = 0;
        PathNode * pathLeaf = nullptr;
        int modCount = -1;

        Path(BTreeVectorImpl * bta)
        {
            this->bta = bta;
        }

        Path * getPathNodes(int pos)
        {
            position = pos;
            child = bta->root;
            pn = pathRoot;
            for (;;)
            {
                if (pn == nullptr)
                {
                    pn = new PathNode();
                    if (pathRoot == nullptr)
                        pathRoot = pn;
                    else
                        pathLeaf->nextDown = pn;
                    pn->parent = pathLeaf;
                    pathLeaf = pn;
                }
                pn->init(child);
                child = pn->findChild(pos);
                if (child == nullptr)
                    break;
                pos = pn->countedPos;
                pn = pn->nextDown;
            }
            pathLeaf = pn;
            return this;
        }

        ~Path()
        {
            while (pathRoot != nullptr)
            {
                PathNode * pn = pathRoot;
                pathRoot = pathRoot->nextDown;
                delete pn;
            }
        }

    };

// --- BTreeVectorImpl

    void deleteNodes(Node * node, int level)
    {
        if (!node->isLeaf)
            for (int i = 0; i < node->csize(); i++)
                deleteNodes(node->data.childrenNodes->get(i), level + 1);
        delete node;
    }

    void splitAdd(Node * node, int pos, Node * moveUpNode, T & element)
    {
        if (node->isLeaf)
            node->data.childrenValues->add(pos, element);
        else
        {
            assert(moveUpNode != nullptr);
            node->data.childrenNodes->add(pos, moveUpNode);
        }
    }

    Node * splitAndInsert(Node * moveUpNode, PathNode * pn, T & element)
    {
        int MAXSIZE = pn->node->maxBlockSize();
        int pos = pn->node->isLeaf ? pn->childIdx : pn->childIdx + 1;
        if (pn->node->csize() < MAXSIZE) // no split needed
        {
            splitAdd(pn->node, pos, moveUpNode, element);
            return nullptr;
        }
        int HALFSIZE = pn->node->halfBlockSize();
        // split
        structModCount++;
        Node * newNode = new Node(pn->node->isLeaf);
        move(pn->node, HALFSIZE, newNode, 0, pn->node->csize() - HALFSIZE, -1, nullptr);
        if (pos < HALFSIZE)
            splitAdd(pn->node, pos, moveUpNode, element);
        else
        {
            splitAdd(newNode, pos - HALFSIZE, moveUpNode, element);
            int moveCount = pn->node->isLeaf ? 1 : moveUpNode->count;
            newNode->count += moveCount;
            pn->node->count -= moveCount;
        }
        // root split
        if (pn->parent == nullptr)
        {
            root = new Node(false);
            root->data.childrenNodes->add(pn->node);
            root->data.childrenNodes->add(newNode);
            root->count = pn->node->count + newNode->count;
            //printf("new root %p\n", root);
        }
        return newNode;
    }

    bool mergeBlocksAfterDelete(Node * node, Node * parent, int myParentIdx)
    {
        int HALFSIZE = node->halfBlockSize();
        int size = node->csize();
        if (size >= HALFSIZE)
            return false;
        int MAXSIZE = node->maxBlockSize();
        // merge left
        Node * left = myParentIdx > 0 ? parent->data.childrenNodes->get(myParentIdx - 1) : nullptr;
        if (left != nullptr && left->csize() + size <= MAXSIZE)
        {
            move(node, 0, left, left->csize(), size, myParentIdx, parent);
            return true;
        }
        // merge right
        Node * right = myParentIdx < parent->csize() - 1 ? parent->data.childrenNodes->get(myParentIdx + 1) : nullptr;
        if (right != nullptr && right->csize() + size <= MAXSIZE)
        {
            move(right, 0, node, size, right->csize(), myParentIdx + 1, parent);
            return true;
        }

        // borrow right
        int diff = HALFSIZE - size;
        if (right != nullptr && right->csize() > HALFSIZE)
        {
            int avgCount = std::max(diff, (right->csize() - HALFSIZE) >> 1);
            move(right, 0, node, size, avgCount, -1, nullptr);
            return true;
        }
        // borrow left
        if (left != nullptr && left->csize() > HALFSIZE)
        {
            int avgCount = std::max(diff, (left->csize() - HALFSIZE) >> 1);
            move(left, left->csize() - avgCount, node, 0, avgCount, -1, nullptr);
            return true;
        }
        return false;
    }

    void move(Node * src, int from, Node * dst, int to, int cnt, int parentIdxToRemove, Node * parent)
    {
        structModCount++;
        int moveCount = 0;
        if (src->isLeaf)
        {
            src->data.childrenValues->insertRange(dst->data.childrenValues, from, to, cnt);
            moveCount += cnt;
        } else
        {
            src->data.childrenNodes->insertRange(dst->data.childrenNodes, from, to, cnt);
            for (int i = from; i < from + cnt; i++)
                moveCount += src->data.childrenNodes->get(i)->count;
        }
        dst->count += moveCount;
        if (parent != nullptr)
        {
            parent->data.childrenNodes->remove(parentIdxToRemove);
            delete src;
        } else
        {
            src->count -= moveCount;
            if (src->isLeaf)
                src->data.childrenValues->removeRange(from, cnt);
            else
            {
                src->data.childrenNodes->removeRange(from, cnt);
            }
        }
    }

    Path * getPath(const int pos, const int fromAdd = 0)
    {

        if (pos >= root->count + fromAdd || pos < 0)
        {
            std::cerr << "index " << pos << " out of range 0:" << (root->count - 1 + fromAdd) << "\n";
            throw;
        }
        if (cachePath.modCount == structModCount) // try get from cache
        {
            int diff = pos - cachePath.position;
            int idx = cachePath.pathLeaf->childIdx + diff;
            //printf("in cache diff %i idx %i size %i\n",diff,idx,root->count);
            int csize = cachePath.pathLeaf->node->data.childrenValues->size();
            if ((idx >= 0 && idx < csize) || (pos == root->count && idx == csize))
            {
                if (diff)
                {
                    cachePath.position += diff;
                    cachePath.pathLeaf->childIdx += diff;
                }
                return &cachePath;
            }
        }
        cachePath.modCount = structModCount;
        return cachePath.getPathNodes(pos);
    }

    BTreeVectorImpl()
    {
        this->root = new Node(true);
    }

    ~BTreeVectorImpl()
    {
        deleteNodes(root, 0);
    }

    void clear()
    {
        deleteNodes(root, 0);
        root = new Node(true);
        structModCount++;
    }

    inline unsigned size()
    {
        return root->count;
    }

    T get(int pos)
    {
        Path * path = getPath(pos);
        return path->pathLeaf->node->data.childrenValues->getRef(path->pathLeaf->childIdx);
    }

    T & operator[](int pos)
    {
        Path * path = getPath(pos);
        return path->pathLeaf->node->data.childrenValues->getRef(path->pathLeaf->childIdx);
    }

    T set(int pos, T element)
    {
        Path * path = getPath(pos);
        return path->pathLeaf->node->data.childrenValues->set(path->pathLeaf->childIdx, element);
    }

    void add(T element)
    {
        add(root->count, element);
    }

    void add(int pos, T element)
    {
        Path * path = getPath(pos, 1);
        Node * moveUpNode = nullptr;
        PathNode * pn = path->pathLeaf;
        do
        {
            pn->node->count++;
            if (moveUpNode != nullptr || pn->node->isLeaf)
                moveUpNode = splitAndInsert(moveUpNode, pn, element);
            pn = pn->parent;
        } while (pn != nullptr);
    }

    void remove(int pos)
    {
        Path * path = getPath(pos);
        path->pathLeaf->node->data.childrenValues->remove(path->pathLeaf->childIdx);
        bool merge = true;
        PathNode * pn = path->pathLeaf;
        do
        {
            pn->node->count--;
            if (merge && pn->parent != nullptr)
                merge = mergeBlocksAfterDelete(pn->node, pn->parent->node, pn->parent->childIdx);
            pn = pn->parent;
        } while (pn != nullptr);
        // trim depth
        while (root->csize() == 1 && !root->isLeaf)
        {
            Node * oldroot = root;
            root = root->data.childrenNodes->get(0);
            delete oldroot;
            structModCount++;
        }
    }

};

#endif /* BTREEVECTORIMPL_H_ */
