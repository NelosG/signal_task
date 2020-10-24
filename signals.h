#pragma once

#include <functional>

#include "intrusive_list.h"

namespace signals {
    template<typename T>
    class signal;

    template<typename... Args>
    class signal<void(Args...)> {
    private:

        using slot_t = std::function<void(Args...)>;
        struct follower_tag;

        class base_follower : public intrusive::list_element<follower_tag> {
            using base_t = intrusive::list_element<follower_tag>;

        public:

            base_follower() = default;

            explicit base_follower(signal *sig) noexcept: signal_e(sig) {
                sig->followers.push_front(*this);
            }

            base_follower(base_follower &&other) noexcept: base_t(std::move(other)), signal_e(other.signal_e) {
                change_enclosing_iterators(other,
                                           [&](auto) { return followers_list::iterator(this); });
            }

            base_follower &operator=(base_follower &&other) noexcept {
                this->prev = other.prev;
                this->next = &other;
                if (other.prev != nullptr)
                    other.prev->next = this;
                other.prev = this;
                other.unlink();

                signal_e = other.signal_e;
                change_enclosing_iterators(other,
                                           [&](auto) { return followers_list::iterator(this); });
                return *this;
            }

            void disconnect() noexcept {
                change_enclosing_iterators(*this, [](auto cur) { return std::next(cur); });
                unlink();
            }

            ~base_follower() {
                change_enclosing_iterators(*this, [](auto cur) { return std::next(cur); });
            }

        private:
            template<class IteratorSupplier>
            void change_enclosing_iterators(const base_follower &other,
                                            IteratorSupplier supplier) const noexcept {
                if (isLinked())
                    for (iteration_stack *el = signal_e->top_iterator; el; el = el->next)
                        if (el->current != signal_e->followers.end() && &*el->current == &other) {
                            el->current = supplier(el->current);
                            return;
                        }
            }

            signal *signal_e;
        };

    public:

        class connection : public base_follower {
        public:

            connection() = default;

            connection(signal *sig, slot_t slot) noexcept
                    : base_follower(sig), slot(std::move(slot)) {}

            connection(connection &&) noexcept = default;

            connection &operator=(connection &&) noexcept = default;

            ~connection() = default;

        private:

            friend class signal;
            slot_t slot;
        };

        signal() = default;

        signal(const signal &) = delete;

        signal &operator=(const signal &) = delete;

        ~signal() {
            if (top_iterator != nullptr)
                top_iterator->destroyed = true;
        }

        connection connect(slot_t slot) noexcept {
            return connection(this, slot);
        }

        void operator()(Args... args) const {
            iteration_stack el = {followers.begin(), top_iterator, &top_iterator};
            top_iterator = &el;

            while (el.current != followers.end()) {
                auto &copy = static_cast<const connection &>(*(el.current++));
                copy.slot(args...);
                if (el.destroyed)
                    return;
            }
        }

    private:

        using followers_list = intrusive::list<base_follower, follower_tag>;

        struct iteration_stack {
            typename followers_list::const_iterator current;
            iteration_stack *next;
            iteration_stack **next_ptr;
            bool destroyed = false;

            ~iteration_stack() {
                if (destroyed) {
                    if (next != nullptr)
                        next->destroyed = true;
                } else {
                    *next_ptr = next;
                }
            }
        };

        followers_list followers;
        mutable iteration_stack *top_iterator = nullptr;
    };
}
