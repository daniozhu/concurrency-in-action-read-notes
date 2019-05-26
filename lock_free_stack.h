#pragma once

#include <atomic>

template <typename T>
class lock_free_stack
{
private:
    struct node
    {
        T data;
        node* next;
        node(const T& _data)
            :data(_data), next(nullptr)
        {
        }
    };

    std::atomic<node*> head;

public:
    void push(T const& data)
    {
        // note: const new_node, means the pointer new_node cannot pointer to other object, but the content of the new_node is still modifiable.
        node* const new_node = new node(data); 
        new_node->next = head.load();

        // if head != new_node->next, (for example, head has been modified by other thread which also pushed an new node and changed head to point to that new node)
        // set new_node->next to the modfied head (new_node->next = head.load()), and the function returns false, so the loop continues, now the head equals to the new_node->next,
        //  so set head to the new_node (head = new_node.load()).
        while (!head.compare_exchange_weak(new_node->next, new_node));
    }

};
