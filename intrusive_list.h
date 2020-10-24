#pragma once
#include <cstddef>
#include <iterator>
//the file hasn't changed from intrusive_list_task
namespace intrusive {

    template<typename Tag>
    class list_element;

    template<typename Type, typename Tag>
    class list;

    template<typename Type, typename Tag>
    class Iterator;


    struct tag_default;

    template<typename Tag=tag_default>
    class list_element {
    public:
        friend class list<list_element<Tag>, Tag>;
        friend class Iterator<list_element<Tag>, Tag>;
        friend class Iterator<const list_element<Tag>, Tag>;

        list_element() noexcept: next(this), prev(this) {}

        ~list_element() { unlink(); }

        list_element(const list_element &) = delete;

        list_element(list_element &&other) noexcept {
            if (other.isLinked()) {
                Link(other.prev, this);
                Link(this, other.next);
                Link(&other, &other);
            } else
                Link(this, this);
        }

        list_element *next;
        list_element *prev;

        void unlink() {
            if (next != this)
                next->prev = prev;
            if (prev != this)
                prev->next = next;
            prev = next = this;
        }

        bool isLinked() const { return next != this; }

        static void Link(list_element *u, list_element *v) {
            u->next = v;
            v->prev = u;
        }
    };

    template<typename Type, typename Tag>
    class Iterator {
    public:
        friend class list<std::remove_const_t<Type>, Tag>;
        friend class Iterator<std::remove_const_t<Type>, Tag>;
        friend class Iterator<std::add_const_t<Type>, Tag>;

        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = std::remove_const_t<Type>;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type *;
        using reference = value_type &;


        using Node = list_element<Tag>;

        Iterator() noexcept = default;

        explicit Iterator(Node *node) : node(node) {}

        Iterator &operator+=(int k) noexcept {
            if(k < 0) operator-=(-k);
            while(k > 0) {
                ++(*this);
                k--;
            }
        }

        Iterator &operator-=(int k) noexcept {
            if(k < 0) operator+=(-k);
            while(k > 0) {
                --(*this);
                k--;
            }
        }

        Iterator &operator++() noexcept {
            node = node->next;
            return *this;
        }

        Iterator &operator--() noexcept {
            node = node->prev;
            return *this;
        }

        Iterator operator++(int) noexcept {
            Iterator it = *this;
            node = node->next;
            return it;
        }

        Iterator operator--(int) noexcept {
            Iterator it = *this;
            node = node->prev;
            return it;
        }

        template<typename otherIt>
        Iterator(otherIt other, std::enable_if_t<
                std::is_same_v<otherIt, Iterator<std::remove_const_t<Type>, Tag>> &&
                std::is_const_v<Type>>* = nullptr) noexcept
                :node(other.node)
        {}

        bool operator==(const Iterator &other) const noexcept {
            return other.node == node;
        }

        bool operator!=(const Iterator &other) const noexcept {
            return other.node != node;
        }

        Type *operator->() const noexcept {
            return static_cast<Type *>(node);
        }

        Type &operator*() const noexcept {
            return *static_cast<Type *>(node);
        }

    private:
        Node *node;
    };

    template<typename Type, typename Tag=tag_default>
    class list {
        friend class Iterator<std::remove_const_t<Type>, Tag>;
        friend class Iterator<std::add_const_t<Type>, Tag>;

    public:

        using iterator = Iterator<Type, Tag>;
        using const_iterator = Iterator<const Type, Tag>;

        static_assert(std::is_convertible_v<Type &, list_element<Tag> &>,
                      "value type is not convertible to list_element");

        list() noexcept = default;

        list(list&& l) noexcept {
            this->splice(end(), l, l.begin(), l.end());
        };

        list(const list &) = delete;

        ~list() { clear(); }

        list &operator=(const list &) = delete;

        list &operator=(list &&l) noexcept {
            clear();
            splice(end(), l, l.begin(), l.end());
            return *this;
        }


        void pop_front() noexcept { head.next->unlink(); }

        void pop_back() noexcept { head.prev->unlink(); }

        void push_front(list_element<Tag> &u) noexcept {
            insert(begin(), u);
        }

        void push_back(list_element<Tag> &u) noexcept {
            insert(end(), u);
        }

        bool empty() const noexcept { return !head.isLinked(); }

        void splice(const_iterator pos, list &, const_iterator first, const_iterator last) noexcept {
            if (pos == first || first == last)
                return;
            auto *copy_last = last.node->prev;
            first.node->prev->next = copy_last->next;
            copy_last->next->prev = first.node->prev;
            pos.node->prev->next = first.node;
            first.node->prev = pos.node->prev;
            pos.node->prev = copy_last;
            copy_last->next = pos.node;
        }
        iterator insert(const_iterator pos, list_element<Tag> &u) noexcept {
            pos.node->prev->next = &u;
            u.prev = pos.node->prev;
            u.next = pos.node;
            pos.node->prev = &u;
            return iterator(&u);
        }

        iterator erase(iterator it) noexcept {
            iterator copy(it.node->next);
            it.node->unlink();
            return copy;
        }


        Type &front() noexcept {
            return *static_cast<Type *>(head.next);
        }

        Type &back() noexcept {
            return *static_cast<Type *>(head.prev);
        }

        const Type &front() const noexcept {
            return *static_cast<Type *>(head.next);
        }

        const Type &back() const noexcept {
            return *static_cast<Type *>(head.prev);
        }

        iterator begin() noexcept {
            return iterator(head.next);
        }

        iterator end() noexcept {
            return iterator(const_cast<list_element<Tag> *>(&head));
        }

        const_iterator begin() const noexcept {
            return const_iterator(head.next);
        }

        const_iterator end() const noexcept {
            return const_iterator(const_cast<list_element<Tag> *>(&head));
        }


        void clear() {
            while (!empty()) {
                pop_back();
            }
        }

    private:
        list_element<Tag> head;
    };
}
