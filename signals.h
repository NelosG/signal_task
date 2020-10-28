#pragma once

#include <functional>

#include "intrusive_list.h"

namespace signals {
template <typename T> class signal;

template <typename... Args> class signal<void(Args...)> {
private:
  using slot_t = std::function<void(Args...)>;
  struct connection_tag;

public:
  class connection : public intrusive::list_element<connection_tag> {
  public:
    connection() = default;

    connection(signal *sig, slot_t slot) noexcept
        : sig(sig), slot(std::move(slot)) {
      sig->followers.push_front(*this);
    }

    connection(connection &&other) noexcept
        : slot(std::move(other.slot)), sig(other.sig) {
      sig->followers.insert(followers_list::iterator(&other), *this);
      other.unlink();
      change_enclosing_iterators(
          other, [&](auto) { return followers_list::iterator(this); });
    }
    void disconnect() noexcept {
      if(isLinked()) {

        change_enclosing_iterators(*this,
                                   [](auto cur) { return std::next(cur); });
        unlink();
        slot = {};
        sig = nullptr;
      }
    }

    connection &operator=(connection &&other) noexcept {
      if (this != &other) {
        disconnect();
        sig = other.sig;
        slot = std::move(other.slot);
        sig->followers.insert(followers_list::iterator(&other), *this);
        other.unlink();
        change_enclosing_iterators(
            other, [&](auto) { return followers_list::iterator(this); });
      }
      return *this;
    }

    ~connection() { disconnect(); }

  private:
    template <class IteratorSupplier>
    void change_enclosing_iterators(const connection &other,
                                    IteratorSupplier supplier) const noexcept {
      if (isLinked())
        for (iteration_stack *el = sig->top_iterator; el; el = el->next)
          if (el->current != sig->followers.end() && &*el->current == &other) {
            el->current = supplier(el->current);
          }
    }
    friend class signal;
    signal *sig;
    slot_t slot;
  };

  signal() = default;

  signal(const signal &) = delete;

  signal &operator=(const signal &) = delete;

  ~signal() {
    for (iteration_stack *el = top_iterator; el != nullptr; el = el->next) {
      el->sig = nullptr;
    }
  }

  connection connect(slot_t slot) noexcept {
    return connection(this, std::move(slot));
  }

  void operator()(Args... args) const {
    iteration_stack el(this);
    while (el.current != followers.end()) {
      auto copy = el.current++;
      copy->slot(args...);
      if (el.sig == nullptr)
        return;
    }
  }

private:
  using followers_list = intrusive::list<connection, connection_tag>;

  struct iteration_stack {
    explicit iteration_stack(signal const *sig)
        : sig(sig), current(sig->followers.begin()), next(sig->top_iterator) {
      sig->top_iterator = this;
    }
    ~iteration_stack() {
      if (sig != nullptr) {
        sig->top_iterator = next;
      }
    }
    typename followers_list::const_iterator current;
    iteration_stack *next;
    signal const *sig;
  };

  followers_list followers;
  mutable iteration_stack *top_iterator = nullptr;
};
}
