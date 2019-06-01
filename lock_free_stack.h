#pragma once

#include <atomic>
#include <memory>

template <typename T>
class lock_free_stack
{
private:
    struct node
    {
        std::shared_ptr<T> data;
        node* next;
        node(const T& _data)
            :data(std::make_shared<T>(_data)), next(nullptr)
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
        // set new_node->next to the modfied head (new_node->next = head), and the function returns false, so the loop continues, now the head equals to the new_node->next,
        //  so set head to the new_node (head = new_node).
        while (!head.compare_exchange_weak(new_node->next, new_node));
    }

    std::shared_ptr<T> pop()
    {
        node* old_head = head.load();
        while (old_head && !head.compare_exchange_weak(old_head, old_head->next));

        std::shared_ptr<T> data = old_head ? old_head->data : std::make_shared<T>();
        delete old_head;
        return data;
    }
};
