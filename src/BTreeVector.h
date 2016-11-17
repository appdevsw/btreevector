/*
 * Author: appdevsw@wp.pl
 *
 */

#ifndef SRC_BTREEVECTOR_H_
#define SRC_BTREEVECTOR_H_

#include "BTreeVectorImpl_priv.h"

template<typename T, int MAX_NODE_BLOCK_SIZE = 16, int MAX_LEAF_BLOCK_SIZE = 128>
class BTreeVector
{
private:
    BTreeVectorImpl<T, MAX_NODE_BLOCK_SIZE, MAX_LEAF_BLOCK_SIZE> impl;
public:

    inline void clear()
    {
        impl.clear();
    }

    inline unsigned size()
    {
        return impl.size();
    }

    inline T get(int pos)
    {
        return impl.get(pos);
    }

    inline T & operator[](int pos)
    {
        return impl[pos];
    }

    inline T set(int pos, T element)
    {
        return impl.set(pos, element);
    }

    inline void add(T element)
    {
        impl.add(element);
    }

    inline void add(int pos, T element)
    {
        impl.add(pos, element);
    }

    inline void remove(int pos)
    {
        impl.remove(pos);
    }

};

#endif /* SRC_BTREEVECTOR_H_ */
