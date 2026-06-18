#pragma once

namespace godot_mcp {

template <typename T>
struct MemdeleteGuard {
    T *ptr = nullptr;

    explicit MemdeleteGuard(T *p) : ptr(p) {}

    ~MemdeleteGuard() {
        if (ptr) {
            memdelete(ptr);
        }
    }

    void dismiss() { ptr = nullptr; }

    T *get() const { return ptr; }

    T *operator->() const { return ptr; }

    MemdeleteGuard(const MemdeleteGuard &) = delete;
    MemdeleteGuard &operator=(const MemdeleteGuard &) = delete;

    MemdeleteGuard(MemdeleteGuard &&other) noexcept : ptr(other.ptr) {
        other.ptr = nullptr;
    }

    MemdeleteGuard &operator=(MemdeleteGuard &&other) noexcept {
        if (this != &other) {
            if (ptr) memdelete(ptr);
            ptr = other.ptr;
            other.ptr = nullptr;
        }
        return *this;
    }
};

} // namespace godot_mcp
