#ifndef _HEAPSORT_H
#define _HEAPSORT_H

template<typename Type, int N, template<class ...> class compare = std::greater>
struct heap {
    std::array<Type, N> store;
    size_t count = 0;
    compare<> cmp;

    heap(const std::array<Type, N>& arr) {
        count = 0;
        for (int i = 0; i < N; i++) {
            push(arr[i]);
        }
    }

    void push(Type t) {
        // push t to the bottom and sift it up
        auto index = count++;
        while (true) {
            if (index == 0) {
                store[0] = t;
                break;
            }
            auto parent_index = (index - 1) / 2;
            auto parent = store[parent_index];
            if (cmp(t, parent)) {
                store[index] = parent;
                index = parent_index;
            } else {
                store[index] = t;
                break;
            }
        }
    }
    Type pop() {
        // return the top element
        // move the bottom element to the top and sift it down
        auto root = store[0];
        auto t = store[--count];

        auto index = 0;
        while (true) {
            auto ci1 = index * 2 + 1;
            auto ci2 = ci1 + 1; // index * 2 + 2
            
            if (ci1 > count - 1) {
                // no children
                store[index] = t;
                break;
            } else if (ci1 == count - 1) {
                // 1 child
                auto c1 = store[ci1];
                if (cmp(c1, t)) {
                    store[index] = c1;
                    index = ci1;
                } else {
                    store[index] = t;
                    break; // done!
                }
            } else {
                // 2 children, use the largest
                auto c1 = store[ci1];
                auto c2 = store[ci2];
                
                // which child will we use
                auto c = c1;
                auto ci = ci1;
                if (cmp(c2, c1)) {
                    c = c2;
                    ci = ci2;
                }

                // can defo factor this out
                if (cmp(c, t)) {
                    store[index] = c;
                    index = ci;
                } else {
                    store[index] = t;
                    break; // done!
                }
            }
        }

        return root;
    }
};

template<typename Type, int N, template<class ...> class Cmp = std::greater>
void heapsort(const std::array<Type, N>& input, std::array<Type, N>& output) {
    auto h = heap<Type, N, Cmp>(input);
    for (int i = 0; i < N; i++) {
        output[i] = h.pop();
    }
}

template<typename Type, size_t N>
ostream& operator << (ostream& o, const array<Type, N>& h) {
    if (N) {
        o << h[0];
        for (int i = 1; i < N; i++) {
            o << ", " << h[i];
        }
    }
    return o;
}

#endif