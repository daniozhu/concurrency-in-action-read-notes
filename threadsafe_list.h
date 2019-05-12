#pragma once
#include <memory>
#include <mutex>

template<typename T>
class threadsafe_list
{
public:
    using dataType = std::shared_ptr<T>;

private:
    struct node 
    {
        std::mutex                             m;
        dataType                              data;
        std::unique_ptr<node>         next;

        node() : next() {}
        node(T const& value) : next(), data(std::make_shared<T>(value))
        {}
    };

    // head always the first node of the list wihtout user supplied value.
    // i.e. it is a special node.
    node    head;

public:
    threadsafe_list()
    {
    }

    ~threadsafe_list() = default;

    threadsafe_list(const threadsafe_list& other) = delete;
    threadsafe_list& operator=(const threadsafe_list& other) = delete;

    // add value to the front of the list, i.e. before where the head points to.
    void push_front(const T& value)
    {
        std::unique_ptr<node> pn = std::make_unique<node>(value);

        std::lock_guard<std::mutex> lk(head.m);
        pn->next = std::move(head.next);
        head.next = std::move(pn);
    }
    
    template<typename Function>
    void for_each(Function f)
    {
        // lock head first.
        std::unique_lock<std::mutex> lk(head.m);

        node* pcurrent = &head;
        while (node* pnext = pcurrent->next.get())
        {
            // lock next node.
            std::unique_lock<std::mutex> lk_next(pnext->m);

            // unlock previous node.
            lk.unlock();
            // process the next node.
            f(pnext->data); 
            // make next node as current.
            pcurrent = pnext;
            // move next lock to current.
            lk = std::move(lk_next);
        }
    }

    template<typename Predicate>
    std::shared_ptr<T> find_first_if(Predicate p)
    {
        // lock head first
        std::unique_lock<std::mutex> lk(head.m);

        node* pcurrent = &head;
        while  (node* pnext = pcurrent->next.get())
        {
            std::unique_lock<std::mutex> lk_next(pnext->m);
            lk.unlock();
            if (p(pnext->data))
            {
                return pnext->data;
            }

            pcurrent = pnext;
            lk = std::move(lk_next);
        }

        return std::make_shared<T>();
    }

    template<typename Predicate>
    void remove_if(Predicate p)
    {
        std::unique_lock<std::mutex> lk(head.m);
        node* pcurrent = &head;
        while (node* pnext = pcurrent->next.get())
        {
            std::unique_lock<std::mutex> lk_next(pnext->m);
            if (p(pnext->data))
            {
                // move the current's next to temp unique_ptr, so it won't be destroyed when assigning a new pointer below.
                std::unique_ptr<node> temp = std::move(pcurrent->next);
                pcurrent->next = std::move(pnext->next);

                // temp will be destroyed when leaving the scope, but its mutex is still locked, which will cause error, so unlock it first. 
                lk_next.unlock();
            }
            else
            {
                lk.unlock();
                pcurrent = pnext;
                lk = std::move(lk_next);
            }
        }
    }
};
