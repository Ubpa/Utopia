#pragma once

#include <UDRefl/UDRefl.hpp>

namespace Ubpa::Utopia {
    // Forward
    ////////////

    template<typename T, typename Deleter = std::default_delete<T>>
    class UniqueVar;
    template<typename T>
    class SharedVar;
    template<typename T>
    class WeakVar;

    // UniqueVar
    //////////////////

    template<typename T, typename Deleter>
    class UniqueVar {
        static_assert(!std::is_const_v<T>);

    public:
        using unique_pointer_type = std::unique_ptr<T, Deleter>;
        using pointer = typename unique_pointer_type::pointer;
        using pointer_to_const = std::add_pointer_t<const std::remove_pointer_t<pointer>>;
        using element_type = typename unique_pointer_type::element_type;
        using deleter_type = Deleter;

        // Constructor
        ////////////////

        constexpr UniqueVar() noexcept = default;
        constexpr UniqueVar(std::nullptr_t) noexcept {}
        explicit UniqueVar(pointer p) noexcept : ptr{ p } {}

        template<typename D = Deleter, std::enable_if_t<std::is_constructible_v<D, const D&>, int> = 0>
        UniqueVar(pointer p, const Deleter& d) noexcept : ptr{ p,d } {}
        template<typename D = Deleter,
            std::enable_if_t<std::conjunction_v<std::negation<std::is_reference<D>>, std::is_constructible<D, D>>, int> = 0>
            UniqueVar(pointer p, Deleter&& d) noexcept : ptr{ p,std::move(d) } {}
        template<typename D = Deleter,
            std::enable_if_t<std::conjunction_v<std::is_reference<D>, std::is_constructible<D, std::remove_reference_t<D>>>, int> = 0>
            UniqueVar(pointer p, std::remove_reference_t<Deleter>&& d) = delete;

        UniqueVar(UniqueVar&& obj) noexcept : ptr{ std::move(obj.ptr) } {}
        UniqueVar(unique_pointer_type&& ptr) noexcept : ptr{ std::move(ptr) } {}

        template<typename U, typename E>
        UniqueVar(UniqueVar<U, E>&& obj) noexcept : ptr{ std::move(obj.ptr) } {}
        template<typename U, typename E>
        UniqueVar(std::unique_ptr<U, E>&& ptr) noexcept : ptr{ std::move(ptr) } {}

        // array-version

        template<typename U,
            typename V = T, std::enable_if_t<std::is_array_v<V>, int> = 0>
            explicit UniqueVar(U p) noexcept : ptr{ p } {}

        template<typename U,
            typename D = Deleter, std::enable_if_t<std::is_constructible_v<D, const D&>, int> = 0,
            typename V = T, std::enable_if_t<std::is_array_v<V>, int> = 0>
            UniqueVar(U p, const Deleter& d) noexcept : ptr{ p,d } {}

        template<typename U,
            typename D = Deleter, std::enable_if_t<std::conjunction_v<std::negation<std::is_reference<D>>, std::is_constructible<D, D>>, int> = 0,
            typename V = T, std::enable_if_t<std::is_array_v<V>, int> = 0>
            UniqueVar(U p, Deleter&& d) noexcept : ptr{ p,std::move(d) } {}

        template<typename U,
            typename D = Deleter, std::enable_if_t<std::conjunction_v<std::is_reference<D>, std::is_constructible<D, std::remove_reference_t<D>>>, int> = 0,
            typename V = T, std::enable_if_t<std::is_array_v<V>, int> = 0>
            UniqueVar(U p, std::remove_reference_t<Deleter>&& d) = delete;

        // Assign
        ///////////

        UniqueVar& operator=(UniqueVar&& r) noexcept {
            ptr = std::move(r.ptr);
            return *this;
        }

        template<typename U, typename E>
        UniqueVar& operator=(UniqueVar<U, E>&& r) noexcept {
            ptr = std::move(r.ptr);
            return *this;
        }

        UniqueVar& operator=(unique_pointer_type&& r) noexcept {
            ptr = std::move(r);
            return *this;
        }

        template<typename U, typename E>
        UniqueVar& operator=(std::unique_ptr<U, E>&& r) noexcept {
            ptr = std::move(r);
            return *this;
        }

        UniqueVar& operator=(std::nullptr_t) noexcept {
            reset();
            return *this;
        }

        // Modifiers
        //////////////

        pointer          release() noexcept { return ptr.release(); }
        pointer_to_const release() const noexcept { return ptr.release(); }

        template<typename U = T, std::enable_if_t<!std::is_array_v<U>, int> = 0>
        void reset(pointer p = pointer{}) noexcept { ptr.reset(p); }
        template<typename U = T, std::enable_if_t<std::is_array_v<U>, int> = 0>
        void reset(U p) noexcept { ptr.reset(p); }
        template<typename U = T, std::enable_if_t<std::is_array_v<U>, int> = 0>
        void reset(std::nullptr_t p = nullptr) noexcept { ptr.reset(p); }

        void swap(UniqueVar& rhs) noexcept { ptr.swap(rhs.ptr); }
        void swap(unique_pointer_type& rhs) noexcept { ptr.swap(rhs); }

        // Observers
        //////////////

        pointer          get() noexcept { return ptr.get(); }
        pointer_to_const get() const noexcept { return ptr.get(); }

        Deleter& get_deleter() noexcept { return ptr.get_deleter(); }
        const Deleter& get_deleter() const noexcept { return ptr.get_deleter(); }

        explicit operator bool() const noexcept { return static_cast<bool>(ptr); }

        template <typename U = T, std::enable_if_t<!std::disjunction_v<std::is_array<U>, std::is_void<U>>, int> = 0>
        U& operator*() & noexcept { return *ptr; }
        template <typename U = T, std::enable_if_t<!std::disjunction_v<std::is_array<U>, std::is_void<U>>, int> = 0>
        U        operator*() && noexcept { return std::move(*ptr); }
        template <typename U = T, std::enable_if_t<!std::disjunction_v<std::is_array<U>, std::is_void<U>>, int> = 0>
        const U& operator*() const& noexcept { return *ptr; }

        template <typename U = T, typename Elem = element_type, std::enable_if_t<std::is_array_v<U>, int> = 0>
        pointer          operator->() noexcept { return ptr.operator->(); }
        template <typename U = T, typename Elem = element_type, std::enable_if_t<std::is_array_v<U>, int> = 0>
        pointer_to_const operator->() const noexcept { return ptr.operator->(); }

        template <typename U = T, typename Elem = element_type, std::enable_if_t<std::is_array_v<U>, int> = 0>
        Elem& operator[](std::ptrdiff_t idx) noexcept { return ptr[idx]; }
        template <typename U = T, typename Elem = element_type, std::enable_if_t<std::is_array_v<U>, int> = 0>
        const Elem& operator[](std::ptrdiff_t idx) const noexcept { return ptr[idx]; }

    private:
        std::unique_ptr<T, Deleter> ptr;
    };

    // SharedVar
    //////////////////

    template<typename T>
    class SharedVar {
        static_assert(!std::is_const_v<T>);

    public:
        //using SharedVar_type = SharedVar<T>;
        using WeakVar_type = WeakVar<T>;
        using shared_pointer_type = std::shared_ptr<T>;
        using weak_pointer_type = std::weak_ptr<T>;
        using unique_pointer_type = std::unique_ptr<T>;
        using element_type = typename shared_pointer_type::element_type;

        // Constructor
        ////////////////

        constexpr SharedVar() noexcept = default;
        constexpr SharedVar(std::nullptr_t) noexcept {}
        template<typename U>
        explicit SharedVar(U* ptr) : ptr{ ptr } {}
        template<typename U, typename Deleter>
        SharedVar(U* ptr, Deleter d) : ptr{ ptr, std::move(d) } {}
        template<typename Deleter>
        SharedVar(std::nullptr_t ptr, Deleter d) : ptr{ ptr, std::move(d) } {}
        template<typename U, typename Deleter, typename Alloc>
        SharedVar(U* ptr, Deleter d, Alloc alloc) : ptr{ ptr, std::move(d), std::move(alloc) } {}
        template<typename Deleter, typename Alloc>
        SharedVar(std::nullptr_t ptr, Deleter d, Alloc alloc) : ptr{ ptr, std::move(d), std::move(alloc) } {}

        template<typename U>
        explicit SharedVar(const std::weak_ptr<U>& ptr) : ptr{ ptr } {}
        template<typename U>
        explicit SharedVar(WeakVar<U>& obj) : ptr{ obj.ptr } {}

        template<typename Y, typename Deleter>
        SharedVar(std::unique_ptr<Y, Deleter>&& r) : ptr{ std::move(r) } {}
        template<typename Y, typename Deleter>
        SharedVar(UniqueVar<Y, Deleter>&& obj) : ptr{ std::move(obj.ptr) } {}

        SharedVar(const shared_pointer_type& ptr) noexcept : ptr{ ptr } {}
        SharedVar(shared_pointer_type&& ptr) noexcept : ptr{ std::move(ptr) } {}
        template<typename U>
        SharedVar(const std::shared_ptr<U>& ptr) noexcept : ptr{ ptr } {}
        template<typename U>
        SharedVar(std::shared_ptr<U>&& ptr) noexcept : ptr{ std::move(ptr) } {}
        template<typename U>
        SharedVar(const std::shared_ptr<U>& r, std::remove_extent_t<T>* ptr) noexcept : ptr{ r,ptr } {}

        SharedVar(const SharedVar& obj) noexcept : ptr{ obj.ptr } {}
        SharedVar(SharedVar&& obj) noexcept : ptr{ std::move(obj.ptr) } {}
        template<typename U>
        SharedVar(SharedVar<U>& obj) noexcept : ptr{ obj.ptr } {}
        template<typename U>
        SharedVar(SharedVar<U>&& obj) noexcept : ptr{ std::move(obj.ptr) } {}
        template<typename U>
        SharedVar(SharedVar<U>& r, std::remove_extent_t<T>* ptr) noexcept : ptr{ r.ptr,ptr } {}

        SharedVar(UDRefl::SharedObject obj) noexcept {
            if (obj.GetType().Is<T>())
                ptr = std::reinterpret_pointer_cast<T>(obj.GetBuffer());
        }

        // Assign
        ///////////

        SharedVar& operator=(const SharedVar& rhs) noexcept {
            ptr = rhs.ptr;
            return *this;
        }

        template <typename U>
        SharedVar& operator=(const SharedVar<U>& rhs) noexcept {
            ptr = rhs.ptr;
            return *this;
        }

        SharedVar& operator=(SharedVar&& rhs) noexcept {
            ptr = std::move(rhs.ptr);
            return *this;
        }

        template <typename U>
        SharedVar& operator=(SharedVar<U>&& rhs) noexcept {
            ptr = std::move(rhs.ptr);
            return *this;
        }

        template <typename U, typename Deleter>
        SharedVar& operator=(UniqueVar<U, Deleter>&& rhs) {
            ptr = std::move(rhs.ptr);
            return *this;
        }

        SharedVar& operator=(const shared_pointer_type& rhs) noexcept {
            ptr = rhs;
            return *this;
        }

        template <typename U>
        SharedVar& operator=(const std::shared_ptr<U>& rhs) noexcept {
            ptr = rhs;
            return *this;
        }

        SharedVar& operator=(shared_pointer_type&& rhs) noexcept {
            ptr = std::move(rhs);
            return *this;
        }

        template <typename U>
        SharedVar& operator=(std::shared_ptr<U>&& rhs) noexcept {
            ptr = std::move(rhs);
            return *this;
        }

        template <typename U, typename Deleter>
        SharedVar& operator=(std::unique_ptr<U, Deleter>&& rhs) {
            ptr = std::move(rhs);
            return *this;
        }

        SharedVar& operator=(std::nullptr_t) noexcept {
            reset();
            return *this;
        }

        SharedVar& operator=(UDRefl::SharedObject obj) noexcept {
            if (obj.GetType().Is<T>())
                ptr = std::reinterpret_pointer_cast<T>(obj.GetBuffer());
            return *this;
        }

        // Cast
        /////////

        shared_pointer_type&     cast_to_shared_ptr()      & noexcept { return ptr; }
        shared_pointer_type      cast_to_shared_ptr()     && noexcept { return std::move(ptr); }
        std::shared_ptr<const T> cast_to_shared_ptr() const& noexcept { return std::static_pointer_cast<const T>(ptr); }

        UDRefl::SharedObject cast_to_shared_obj()       noexcept { return { Type_of<T>, ptr }; }
        UDRefl::SharedObject cast_to_shared_obj() const noexcept { return { Type_of<const T>, ptr }; }

        operator shared_pointer_type&    ()      & noexcept { return ptr; }
        operator shared_pointer_type     ()     && noexcept { return std::move(ptr); }
        operator std::shared_ptr<const T>() const& noexcept { return ptr; }

        operator UDRefl::SharedObject()       noexcept { return { Type_of<T>, ptr }; }
        operator UDRefl::SharedObject() const noexcept { return { Type_of<const T>, ptr }; }

        // Modifiers
        //////////////

        void reset() noexcept { ptr.reset(); }
        template<typename U>
        void reset(U* ptrU) { ptr.reset(ptrU); }
        template<typename U, typename Deleter>
        void reset(U* ptrU, Deleter d) { ptr.reset(ptrU, std::move(d)); }
        template<typename U, typename Deleter, typename Alloc>
        void reset(U* ptrU, Deleter d, Alloc alloc) { ptr.reset(ptrU, std::move(d), std::move(alloc)); }

        void swap(SharedVar& rhs) noexcept { return ptr.swap(rhs.ptr); }
        void swap(shared_pointer_type& rhs) noexcept { return ptr.swap(rhs); }

        // Observers
        //////////////

        T* get() noexcept { return ptr.get(); }
        const T* get() const noexcept { return ptr.get(); }

        long use_count() const noexcept { return ptr.use_count(); }

        template <typename U = T, std::enable_if_t<!std::disjunction_v<std::is_array<U>, std::is_void<U>>, int> = 0>
        U& operator*() noexcept { return ptr.operator*(); }
        template <typename U = T, std::enable_if_t<!std::disjunction_v<std::is_array<U>, std::is_void<U>>, int> = 0>
        const U& operator*() const noexcept { return ptr.operator*(); }

        template <typename U = T, std::enable_if_t<!std::is_array_v<U>, int> = 0>
        U* operator->() noexcept { return ptr.operator->(); }
        template <typename U = T, std::enable_if_t<!std::is_array_v<U>, int> = 0>
        const U* operator->() const noexcept { return ptr.operator->(); }

        template <typename U = T, typename Elem = std::remove_extent_t<T>, std::enable_if_t<std::is_array_v<U>, int> = 0>
        Elem& operator[](std::ptrdiff_t idx) noexcept { return ptr[idx]; }
        template <typename U = T, typename Elem = std::remove_extent_t<T>, std::enable_if_t<std::is_array_v<U>, int> = 0>
        const Elem& operator[](std::ptrdiff_t idx) const noexcept { return ptr[idx]; }

        explicit operator bool() const noexcept { return static_cast<bool>(ptr); }

        template <typename U>
        bool owner_before(const SharedVar<U>& rhs) const noexcept { return ptr.owner_before(rhs.ptr); }
        template <typename U>
        bool owner_before(const std::shared_ptr<U>& rhs) const noexcept { return ptr.owner_before(rhs); }
        template <typename U>
        bool owner_before(const WeakVar<U>& rhs) const noexcept { return ptr.owner_before(rhs.ptr); }
        template <typename U>
        bool owner_before(const std::weak_ptr<U>& rhs) const noexcept { return ptr.owner_before(rhs); }

        template <typename U>
        bool owner_after(const SharedVar<U>& rhs) const noexcept { return rhs.ptr.owner_before(ptr); }
        template <typename U>
        bool owner_after(const std::shared_ptr<U>& rhs) const noexcept { return rhs.owner_before(ptr); }
        template <typename U>
        bool owner_after(const WeakVar<U>& rhs) const noexcept { return rhs.ptr.owner_before(ptr); }
        template <typename U>
        bool owner_after(const std::weak_ptr<U>& rhs) const noexcept { return rhs.owner_before(ptr); }

    private:
        shared_pointer_type ptr;
    };

    // WeakVar
    //////////////////

    template<typename T>
    class WeakVar {
        static_assert(!std::is_const_v<T>);

    public:
        using SharedVar_type = SharedVar<T>;
        //using WeakVar_type = WeakVar<T>;
        using shared_pointer_type = std::shared_ptr<T>;
        using weak_pointer_type = std::weak_ptr<T>;
        using unique_pointer_type = std::unique_ptr<T>;
        using element_type = typename weak_pointer_type::element_type;

        // Constructor
        ////////////////

        constexpr WeakVar() noexcept = default;

        WeakVar(WeakVar& obj) noexcept : ptr{ obj.ptr } {}
        WeakVar(WeakVar&& obj) noexcept : ptr{ std::move(obj.ptr) } {}
        template<typename U>
        WeakVar(WeakVar<U>& obj) noexcept : ptr{ obj.ptr } {}
        template<typename U>
        WeakVar(WeakVar<U>&& obj) noexcept : ptr{ std::move(obj.ptr) } {}

        WeakVar(const weak_pointer_type& ptr) noexcept : ptr{ ptr } {}
        WeakVar(weak_pointer_type&& ptr) noexcept : ptr{ std::move(ptr) } {}
        template<typename U>
        WeakVar(const std::weak_ptr<U>& ptr) noexcept : ptr{ ptr } {}
        template<typename U>
        WeakVar(std::weak_ptr<U>&& ptr) noexcept : ptr{ std::move(ptr) } {}

        template<typename U>
        WeakVar(SharedVar<U>& obj) noexcept : ptr{ obj.ptr } {}
        template<typename U>
        WeakVar(const std::shared_ptr<U>& ptr) noexcept : ptr{ ptr } {}

        // Assign
        ///////////

        WeakVar& operator=(WeakVar& rhs) noexcept {
            ptr = rhs.ptr;
            return *this;
        }

        template <typename U>
        WeakVar& operator=(WeakVar<U>& rhs) noexcept {
            ptr = rhs.ptr;
            return *this;
        }

        WeakVar& operator=(WeakVar&& rhs) noexcept {
            ptr = std::move(rhs.ptr);
            return *this;
        }

        template <typename U>
        WeakVar& operator=(WeakVar<U>&& rhs) noexcept {
            ptr = std::move(rhs.ptr);
            return *this;
        }

        template <typename U>
        WeakVar& operator=(SharedVar<U>& rhs) noexcept {
            ptr = rhs.ptr;
            return *this;
        }

        WeakVar& operator=(const weak_pointer_type& rhs) noexcept {
            ptr = rhs;
            return *this;
        }

        template <typename U>
        WeakVar& operator=(const std::weak_ptr<U>& rhs) noexcept {
            ptr = rhs;
            return *this;
        }

        WeakVar& operator=(weak_pointer_type&& rhs) noexcept {
            ptr = std::move(rhs);
            return *this;
        }

        template <typename U>
        WeakVar& operator=(std::weak_ptr<U>&& rhs) noexcept {
            ptr = std::move(rhs);
            return *this;
        }

        template <typename U>
        WeakVar& operator=(const std::shared_ptr<U>& rhs) noexcept {
            ptr = rhs;
            return *this;
        }

        // Cast
        /////////

        weak_pointer_type&     cast_to_weak_ptr()      & noexcept { return ptr; }
        weak_pointer_type      cast_to_weak_ptr()     && noexcept { return std::move(ptr); }
        std::weak_ptr<const T> cast_to_weak_ptr() const& noexcept { return std::static_pointer_cast<const T>(ptr); }

        operator weak_pointer_type&    ()      & noexcept { return ptr; }
        operator weak_pointer_type     ()     && noexcept { return std::move(ptr); }
        operator std::weak_ptr<const T>() const& noexcept { return ptr; }

        // Modifiers
        //////////////

        void reset() noexcept { ptr.reset(); }

        void swap(WeakVar& rhs) noexcept { return ptr.swap(rhs.ptr); }
        void swap(weak_pointer_type& rhs) noexcept { return ptr.swap(rhs); }

        // Observers
        //////////////

        long use_count() const noexcept { return ptr.use_count(); }

        bool expired() const noexcept { return ptr.expired(); }

        SharedVar_type          lock() noexcept { return { ptr.lock() }; }
        const SharedVar_type    lock() const noexcept { return { ptr.lock() }; }
        shared_pointer_type      lock_to_shared_ptr() noexcept { return ptr.lock(); }
        std::shared_ptr<const T> lock_to_shared_ptr() const noexcept { return std::static_pointer_cast<const T>(ptr.lock()); }

        template <typename U>
        bool owner_before(const SharedVar<U>& rhs) const noexcept { return ptr.owner_before(rhs.ptr); }
        template <typename U>
        bool owner_before(const std::shared_ptr<U>& rhs) const noexcept { return ptr.owner_before(rhs); }
        template <typename U>
        bool owner_before(const WeakVar<U>& rhs) const noexcept { return ptr.owner_before(rhs.ptr); }
        template <typename U>
        bool owner_before(const std::weak_ptr<U>& rhs) const noexcept { return ptr.owner_before(rhs); }

        template <typename U>
        bool owner_after(const SharedVar<U>& rhs) const noexcept { return rhs.ptr.owner_before(ptr); }
        template <typename U>
        bool owner_after(const std::shared_ptr<U>& rhs) const noexcept { return rhs.owner_before(ptr); }
        template <typename U>
        bool owner_after(const WeakVar<U>& rhs) const noexcept { return rhs.ptr.owner_before(ptr); }
        template <typename U>
        bool owner_after(const std::weak_ptr<U>& rhs) const noexcept { return rhs.owner_before(ptr); }

    private:
        weak_pointer_type ptr;
    };

    // make object
    ////////////////

    template <typename T, class... Args>
    SharedVar<T> make_SharedVar(Args&&... args) {
        return { std::make_shared<T>(std::forward<Args>(args)...) };
    }

    template <typename T, std::enable_if_t<std::is_array_v<T>, int> = 0>
    SharedVar<T> make_SharedVar(std::size_t size) {
        return { std::make_shared<T>(size) };
    }

    template <typename T, class Alloc, class... Args>
    SharedVar<T> allocate_SharedVar(const Alloc& alloc, Args&&... args) {
        return { std::allocate_shared<T>(alloc, std::forward<Args>(args)...) };
    }

    template <typename T, class... Args>
    UniqueVar<T> make_UniqueVar(Args&&... args) {
        return { std::make_unique<T>(std::forward<Args>(args)...) };
    }

    template <typename T, std::enable_if_t<std::is_array_v<T>, int> = 0>
    UniqueVar<T> make_UniqueVar(std::size_t size) {
        return { std::make_unique<T>(size) };
    }

    // cast
    /////////

    template<typename Ty1, typename Ty2>
    SharedVar<Ty1> static_var_cast(SharedVar<Ty2>&& other) noexcept {
        return { std::static_pointer_cast<Ty1>(std::move(other).cast_to_shared_ptr()) };
    }

    template<typename Ty1, typename Ty2>
    SharedVar<Ty1> static_var_cast(SharedVar<Ty2>& other) noexcept {
        return { std::static_pointer_cast<Ty1>(other.cast_to_shared_ptr()) };
    }

    template<typename Ty1, typename Ty2>
    const SharedVar<Ty1> static_var_cast(const SharedVar<Ty2>& other) noexcept {
        return { static_var_cast<Ty1>(const_cast<SharedVar<Ty2>&>(other)) };
    }

    template<typename Ty1, typename Ty2>
    SharedVar<Ty1> dynamic_var_cast(SharedVar<Ty2>&& other) noexcept {
        return { std::dynamic_pointer_cast<Ty1>(std::move(other.cast_to_shared_ptr())) };
    }

    template<typename Ty1, typename Ty2>
    SharedVar<Ty1> dynamic_var_cast(SharedVar<Ty2>& other) noexcept {
        return { std::dynamic_pointer_cast<Ty1>(other.cast_to_shared_ptr()) };
    }

    template<typename Ty1, typename Ty2>
    const SharedVar<Ty1> dynamic_var_cast(const SharedVar<Ty2>& other) noexcept {
        return { dynamic_var_cast<Ty1>(const_cast<SharedVar<Ty2>&>(other)) };
    }

    template<typename Ty1, typename Ty2>
    SharedVar<Ty1> reinterpret_var_cast(SharedVar<Ty2>&& other) noexcept {
        return { std::reinterpret_pointer_cast<Ty1>(std::move(other.cast_to_shared_ptr())) };
    }

    template<typename Ty1, typename Ty2>
    SharedVar<Ty1> reinterpret_var_cast(SharedVar<Ty2>& other) noexcept {
        return { std::reinterpret_pointer_cast<Ty1>(other.cast_to_shared_ptr()) };
    }

    template<typename Ty1, typename Ty2>
    const SharedVar<Ty1> reinterpret_var_cast(const SharedVar<Ty2>& other) noexcept {
        return { reinterpret_var_cast<Ty1>(const_cast<SharedVar<Ty2>&>(other)) };
    }

    // Deduction Guides
    /////////////////////

    template<typename T>
    SharedVar(WeakVar<T>)->SharedVar<T>;
    template<typename T>
    SharedVar(std::weak_ptr<T>)->SharedVar<T>;
    template<typename T, typename Deleter>
    SharedVar(UniqueVar<T, Deleter>)->SharedVar<T>;
    template<typename T, typename Deleter>
    SharedVar(std::unique_ptr<T, Deleter>)->SharedVar<T>;

    template<typename T>
    WeakVar(SharedVar<T>)->WeakVar<T>;
    template<typename T>
    WeakVar(std::shared_ptr<T>)->WeakVar<T>;
}

// Hash
/////////

template<typename T>
struct std::hash<Ubpa::Utopia::SharedVar<T>> {
    constexpr std::size_t operator()(const Ubpa::Utopia::SharedVar<T>& obj) noexcept {
        return std::hash<typename std::shared_ptr<T>::element_type*>()(obj.get());
    }
};

template<typename T, typename Deleter>
struct std::hash<Ubpa::Utopia::UniqueVar<T, Deleter>> {
    constexpr std::size_t operator()(const Ubpa::Utopia::UniqueVar<T, Deleter>& obj) noexcept {
        return std::hash<typename std::unique_ptr<T, Deleter>::pointer>()(obj.get());
    }
};

// Compare
////////////

template<typename Ty1, typename Ty2>
bool operator==(const Ubpa::Utopia::SharedVar<Ty1>& left, const Ubpa::Utopia::SharedVar<Ty2>& right) noexcept {
    return left.get() == right.get();
}

template<typename Ty1, typename Ty2>
bool operator!=(const Ubpa::Utopia::SharedVar<Ty1>& left, const Ubpa::Utopia::SharedVar<Ty2>& right) noexcept {
    return left.get() != right.get();
}

template<typename Ty1, typename Ty2>
bool operator<(const Ubpa::Utopia::SharedVar<Ty1>& left, const Ubpa::Utopia::SharedVar<Ty2>& right) noexcept {
    return left.get() < right.get();
}

template<typename Ty1, typename Ty2>
bool operator>=(const Ubpa::Utopia::SharedVar<Ty1>& left, const Ubpa::Utopia::SharedVar<Ty2>& right) noexcept {
    return left.get() >= right.get();
}

template<typename Ty1, typename Ty2>
bool operator>(const Ubpa::Utopia::SharedVar<Ty1>& left, const Ubpa::Utopia::SharedVar<Ty2>& right) noexcept {
    return left.get() > right.get();
}

template<typename Ty1, typename Ty2>
bool operator<=(const Ubpa::Utopia::SharedVar<Ty1>& left, const Ubpa::Utopia::SharedVar<Ty2>& right) noexcept {
    return left.get() <= right.get();
}

template<typename Ty1, typename Ty2>
bool operator==(const std::shared_ptr<Ty1>& left, const Ubpa::Utopia::SharedVar<Ty2>& right) noexcept {
    return left.get() == right.get();
}

template<typename Ty1, typename Ty2>
bool operator!=(const std::shared_ptr<Ty1>& left, const Ubpa::Utopia::SharedVar<Ty2>& right) noexcept {
    return left.get() != right.get();
}

template<typename Ty1, typename Ty2>
bool operator<(const std::shared_ptr<Ty1>& left, const Ubpa::Utopia::SharedVar<Ty2>& right) noexcept {
    return left.get() < right.get();
}

template<typename Ty1, typename Ty2>
bool operator>=(const std::shared_ptr<Ty1>& left, const Ubpa::Utopia::SharedVar<Ty2>& right) noexcept {
    return left.get() >= right.get();
}

template<typename Ty1, typename Ty2>
bool operator>(const std::shared_ptr<Ty1>& left, const Ubpa::Utopia::SharedVar<Ty2>& right) noexcept {
    return left.get() > right.get();
}

template<typename Ty1, typename Ty2>
bool operator<=(const std::shared_ptr<Ty1>& left, const Ubpa::Utopia::SharedVar<Ty2>& right) noexcept {
    return left.get() <= right.get();
}

template<typename Ty1, typename Ty2>
bool operator==(const Ubpa::Utopia::SharedVar<Ty1>& left, const std::shared_ptr<Ty2>& right) noexcept {
    return left.get() == right.get();
}

template<typename Ty1, typename Ty2>
bool operator!=(const Ubpa::Utopia::SharedVar<Ty1>& left, const std::shared_ptr<Ty2>& right) noexcept {
    return left.get() != right.get();
}

template<typename Ty1, typename Ty2>
bool operator<(const Ubpa::Utopia::SharedVar<Ty1>& left, const std::shared_ptr<Ty2>& right) noexcept {
    return left.get() < right.get();
}

template<typename Ty1, typename Ty2>
bool operator>=(const Ubpa::Utopia::SharedVar<Ty1>& left, const std::shared_ptr<Ty2>& right) noexcept {
    return left.get() >= right.get();
}

template<typename Ty1, typename Ty2>
bool operator>(const Ubpa::Utopia::SharedVar<Ty1>& left, const std::shared_ptr<Ty2>& right) noexcept {
    return left.get() > right.get();
}

template<typename Ty1, typename Ty2>
bool operator<=(const Ubpa::Utopia::SharedVar<Ty1>& left, const std::shared_ptr<Ty2>& right) noexcept {
    return left.get() <= right.get();
}

template <typename T>
bool operator==(const Ubpa::Utopia::SharedVar<T>& left, std::nullptr_t) noexcept {
    return left.get() == nullptr;
}

template <typename T>
bool operator==(std::nullptr_t, const Ubpa::Utopia::SharedVar<T>& right) noexcept {
    return nullptr == right.get();
}

template <typename T>
bool operator!=(const Ubpa::Utopia::SharedVar<T>& left, std::nullptr_t) noexcept {
    return left.get() != nullptr;
}

template <typename T>
bool operator!=(std::nullptr_t, const Ubpa::Utopia::SharedVar<T>& right) noexcept {
    return nullptr != right.get();
}

template <typename T>
bool operator<(const Ubpa::Utopia::SharedVar<T>& left, std::nullptr_t) noexcept {
    return left.get() < static_cast<typename Ubpa::Utopia::SharedVar<T>::element_type*>(nullptr);
}

template <typename T>
bool operator<(std::nullptr_t, const Ubpa::Utopia::SharedVar<T>& right) noexcept {
    return static_cast<typename Ubpa::Utopia::SharedVar<T>::element_type*>(nullptr) < right.get();
}

template <typename T>
bool operator>=(const Ubpa::Utopia::SharedVar<T>& left, std::nullptr_t) noexcept {
    return left.get() >= static_cast<typename Ubpa::Utopia::SharedVar<T>::element_type*>(nullptr);
}

template <typename T>
bool operator>=(std::nullptr_t, const Ubpa::Utopia::SharedVar<T>& right) noexcept {
    return static_cast<typename Ubpa::Utopia::SharedVar<T>::element_type*>(nullptr) >= right.get();
}

template <typename T>
bool operator>(const Ubpa::Utopia::SharedVar<T>& left, std::nullptr_t) noexcept {
    return left.get() > static_cast<typename Ubpa::Utopia::SharedVar<T>::element_type*>(nullptr);
}

template <typename T>
bool operator>(std::nullptr_t, const Ubpa::Utopia::SharedVar<T>& right) noexcept {
    return static_cast<typename Ubpa::Utopia::SharedVar<T>::element_type*>(nullptr) > right.get();
}

template <typename T>
bool operator<=(const Ubpa::Utopia::SharedVar<T>& left, std::nullptr_t) noexcept {
    return left.get() <= static_cast<typename Ubpa::Utopia::SharedVar<T>::element_type*>(nullptr);
}

template <typename T>
bool operator<=(std::nullptr_t, const Ubpa::Utopia::SharedVar<T>& right) noexcept {
    return static_cast<typename Ubpa::Utopia::SharedVar<T>::element_type*>(nullptr) <= right.get();
}

template<typename Ty1, typename D1, typename Ty2, typename D2>
bool operator==(const Ubpa::Utopia::UniqueVar<Ty1, D1>& left, const Ubpa::Utopia::UniqueVar<Ty2, D2>& right) noexcept {
    return left.get() == right.get();
}

template<typename Ty1, typename D1, typename Ty2, typename D2>
bool operator!=(const Ubpa::Utopia::UniqueVar<Ty1, D1>& left, const Ubpa::Utopia::UniqueVar<Ty2, D2>& right) noexcept {
    return left.get() != right.get();
}

template<typename Ty1, typename D1, typename Ty2, typename D2>
bool operator<(const Ubpa::Utopia::UniqueVar<Ty1, D1>& left, const Ubpa::Utopia::UniqueVar<Ty2, D2>& right) noexcept {
    return left.get() < right.get();
}

template<typename Ty1, typename D1, typename Ty2, typename D2>
bool operator>=(const Ubpa::Utopia::UniqueVar<Ty1, D1>& left, const Ubpa::Utopia::UniqueVar<Ty2, D2>& right) noexcept {
    return left.get() >= right.get();
}

template<typename Ty1, typename D1, typename Ty2, typename D2>
bool operator>(const Ubpa::Utopia::UniqueVar<Ty1, D1>& left, const Ubpa::Utopia::UniqueVar<Ty2, D2>& right) noexcept {
    return left.get() > right.get();
}

template<typename Ty1, typename D1, typename Ty2, typename D2>
bool operator<=(const Ubpa::Utopia::UniqueVar<Ty1, D1>& left, const Ubpa::Utopia::UniqueVar<Ty2, D2>& right) noexcept {
    return left.get() <= right.get();
}

template<typename Ty1, typename D1, typename Ty2, typename D2>
bool operator==(const std::unique_ptr<Ty1, D1>& left, const Ubpa::Utopia::UniqueVar<Ty2, D2>& right) noexcept {
    return left.get() == right.get();
}

template<typename Ty1, typename D1, typename Ty2, typename D2>
bool operator!=(const std::unique_ptr<Ty1, D1>& left, const Ubpa::Utopia::UniqueVar<Ty2, D2>& right) noexcept {
    return left.get() != right.get();
}

template<typename Ty1, typename D1, typename Ty2, typename D2>
bool operator<(const std::unique_ptr<Ty1, D1>& left, const Ubpa::Utopia::UniqueVar<Ty2, D2>& right) noexcept {
    return left.get() < right.get();
}

template<typename Ty1, typename D1, typename Ty2, typename D2>
bool operator>=(const std::unique_ptr<Ty1, D1>& left, const Ubpa::Utopia::UniqueVar<Ty2, D2>& right) noexcept {
    return left.get() >= right.get();
}

template<typename Ty1, typename D1, typename Ty2, typename D2>
bool operator>(const std::unique_ptr<Ty1, D1>& left, const Ubpa::Utopia::UniqueVar<Ty2, D2>& right) noexcept {
    return left.get() > right.get();
}

template<typename Ty1, typename D1, typename Ty2, typename D2>
bool operator<=(const std::unique_ptr<Ty1, D1>& left, const Ubpa::Utopia::UniqueVar<Ty2, D2>& right) noexcept {
    return left.get() <= right.get();
}

template<typename Ty1, typename D1, typename Ty2, typename D2>
bool operator==(const Ubpa::Utopia::UniqueVar<Ty1, D1>& left, const std::unique_ptr<Ty2, D2>& right) noexcept {
    return left.get() == right.get();
}

template<typename Ty1, typename D1, typename Ty2, typename D2>
bool operator!=(const Ubpa::Utopia::UniqueVar<Ty1, D1>& left, const std::unique_ptr<Ty2, D2>& right) noexcept {
    return left.get() != right.get();
}

template<typename Ty1, typename D1, typename Ty2, typename D2>
bool operator<(const Ubpa::Utopia::UniqueVar<Ty1, D1>& left, const std::unique_ptr<Ty2, D2>& right) noexcept {
    return left.get() < right.get();
}

template<typename Ty1, typename D1, typename Ty2, typename D2>
bool operator>=(const Ubpa::Utopia::UniqueVar<Ty1, D1>& left, const std::unique_ptr<Ty2, D2>& right) noexcept {
    return left.get() >= right.get();
}

template<typename Ty1, typename D1, typename Ty2, typename D2>
bool operator>(const Ubpa::Utopia::UniqueVar<Ty1, D1>& left, const std::unique_ptr<Ty2, D2>& right) noexcept {
    return left.get() > right.get();
}

template<typename Ty1, typename D1, typename Ty2, typename D2>
bool operator<=(const Ubpa::Utopia::UniqueVar<Ty1, D1>& left, const std::unique_ptr<Ty2, D2>& right) noexcept {
    return left.get() <= right.get();
}

template <typename T, typename D>
bool operator==(const Ubpa::Utopia::UniqueVar<T, D>& left, std::nullptr_t) noexcept {
    return left.get() == nullptr;
}

template <typename T, typename D>
bool operator==(std::nullptr_t, const Ubpa::Utopia::UniqueVar<T, D>& right) noexcept {
    return nullptr == right.get();
}

template <typename T, typename D>
bool operator!=(const Ubpa::Utopia::UniqueVar<T, D>& left, std::nullptr_t) noexcept {
    return left.get() != nullptr;
}

template <typename T, typename D>
bool operator!=(std::nullptr_t, const Ubpa::Utopia::UniqueVar<T, D>& right) noexcept {
    return nullptr != right.get();
}

template <typename T, typename D>
bool operator<(const Ubpa::Utopia::UniqueVar<T, D>& left, std::nullptr_t) noexcept {
    return left.get() < static_cast<typename Ubpa::Utopia::UniqueVar<T, D>::pointer>(nullptr);
}

template <typename T, typename D>
bool operator<(std::nullptr_t, const Ubpa::Utopia::SharedVar<T>& right) noexcept {
    return static_cast<typename Ubpa::Utopia::UniqueVar<T, D>::pointer>(nullptr) < right.get();
}

template <typename T, typename D>
bool operator>=(const Ubpa::Utopia::SharedVar<T>& left, std::nullptr_t) noexcept {
    return left.get() >= static_cast<typename Ubpa::Utopia::UniqueVar<T, D>::pointer>(nullptr);
}

template <typename T, typename D>
bool operator>=(std::nullptr_t, const Ubpa::Utopia::SharedVar<T>& right) noexcept {
    return static_cast<typename Ubpa::Utopia::UniqueVar<T, D>::pointer>(nullptr) >= right.get();
}

template <typename T, typename D>
bool operator>(const Ubpa::Utopia::SharedVar<T>& left, std::nullptr_t) noexcept {
    return left.get() > static_cast<typename Ubpa::Utopia::UniqueVar<T, D>::pointer>(nullptr);
}

template <typename T, typename D>
bool operator>(std::nullptr_t, const Ubpa::Utopia::SharedVar<T>& right) noexcept {
    return static_cast<typename Ubpa::Utopia::UniqueVar<T, D>::pointer>(nullptr) > right.get();
}

template <typename T, typename D>
bool operator<=(const Ubpa::Utopia::SharedVar<T>& left, std::nullptr_t) noexcept {
    return left.get() <= static_cast<typename Ubpa::Utopia::UniqueVar<T, D>::pointer>(nullptr);
}

template <typename T, typename D>
bool operator<=(std::nullptr_t, const Ubpa::Utopia::SharedVar<T>& right) noexcept {
    return static_cast<typename Ubpa::Utopia::UniqueVar<T, D>::pointer>(nullptr) <= right.get();
}

// Output
///////////

template <class Elem, typename Traits, typename T>
std::basic_ostream<Elem, Traits>& operator<<(std::basic_ostream<Elem, Traits>& out, const Ubpa::Utopia::SharedVar<T>& obj) {
    return out << obj.get();
}

// Swap
/////////

namespace std {
    template <typename T>
    void swap(Ubpa::Utopia::SharedVar<T>& left, Ubpa::Utopia::SharedVar<T>& right) noexcept {
        left.swap(right);
    }

    template <typename T>
    void swap(Ubpa::Utopia::SharedVar<T>& left, shared_ptr<T>& right) noexcept {
        left.swap(right);
    }

    template <typename T>
    void swap(shared_ptr<T>& left, Ubpa::Utopia::SharedVar<T>& right) noexcept {
        right.swap(left);
    }

    template <typename T>
    void swap(Ubpa::Utopia::WeakVar<T>& left, Ubpa::Utopia::WeakVar<T>& right) noexcept {
        left.swap(right);
    }

    template <typename T>
    void swap(Ubpa::Utopia::WeakVar<T>& left, weak_ptr<T>& right) noexcept {
        left.swap(right);
    }

    template <typename T>
    void swap(weak_ptr<T>& left, Ubpa::Utopia::WeakVar<T>& right) noexcept {
        right.swap(left);
    }

    template <typename T, typename Deleter>
    void swap(Ubpa::Utopia::UniqueVar<T, Deleter>& left, Ubpa::Utopia::UniqueVar<T, Deleter>& right) noexcept {
        left.swap(right);
    }

    template <typename T, typename Deleter>
    void swap(Ubpa::Utopia::UniqueVar<T, Deleter>& left, unique_ptr<T, Deleter>& right) noexcept {
        left.swap(right);
    }

    template <typename T, typename Deleter>
    void swap(unique_ptr<T, Deleter>& left, Ubpa::Utopia::UniqueVar<T, Deleter>& right) noexcept {
        right.swap(left);
    }
}

// owner_less
///////////////

template<typename T>
struct std::owner_less<Ubpa::Utopia::SharedVar<T>> {
    bool operator()(const Ubpa::Utopia::SharedVar<T>& left, const Ubpa::Utopia::SharedVar<T>& right) const noexcept {
        return left.owner_before(right);
    }

    bool operator()(const Ubpa::Utopia::SharedVar<T>& left, const std::shared_ptr<T>& right) const noexcept {
        return left.owner_before(right);
    }

    bool operator()(const std::shared_ptr<T>& left, const Ubpa::Utopia::SharedVar<T>& right) const noexcept {
        return right.owner_after(left);
    }

    bool operator()(const Ubpa::Utopia::SharedVar<T>& left, const Ubpa::Utopia::WeakVar<T>& right) const noexcept {
        return left.owner_before(right);
    }

    bool operator()(const Ubpa::Utopia::WeakVar<T>& left, const Ubpa::Utopia::SharedVar<T>& right) const noexcept {
        return left.owner_before(right);
    }

    bool operator()(const Ubpa::Utopia::SharedVar<T>& left, const std::weak_ptr<T>& right) const noexcept {
        return left.owner_before(right);
    }

    bool operator()(const std::weak_ptr<T>& left, const Ubpa::Utopia::SharedVar<T>& right) const noexcept {
        return right.owner_after(right);
    }
};

template<typename T>
struct std::owner_less<Ubpa::Utopia::WeakVar<T>> {
    bool operator()(const Ubpa::Utopia::WeakVar<T>& left, const Ubpa::Utopia::WeakVar<T>& right) const noexcept {
        return left.owner_before(right);
    }

    bool operator()(const Ubpa::Utopia::WeakVar<T>& left, const std::weak_ptr<T>& right) const noexcept {
        return left.owner_before(right);
    }

    bool operator()(const std::weak_ptr<T>& left, const Ubpa::Utopia::WeakVar<T>& right) const noexcept {
        return right.owner_after(left);
    }
};

template<typename T>
struct Ubpa::UDRefl::details::TypeAutoRegister<Ubpa::Utopia::SharedVar<T>> {
    static void run(ReflMngr& mngr) {
        mngr.AddMethod<
            MemFuncOf<
                Ubpa::Utopia::SharedVar<T>,
                Ubpa::Utopia::SharedVar<T>&(Ubpa::UDRefl::SharedObject)
            >::template get(&Ubpa::Utopia::SharedVar<T>::operator=)
        >(Ubpa::UDRefl::NameIDRegistry::Meta::operator_assignment);
        mngr.AddConstructor<Ubpa::Utopia::SharedVar<T>, Ubpa::UDRefl::SharedObject>();
        mngr.AddMethod<
            MemFuncOf<
                Ubpa::Utopia::SharedVar<T>,
                Ubpa::UDRefl::SharedObject()
            >::template get(&Ubpa::Utopia::SharedVar<T>::cast_to_shared_obj)
        >("cast_to_shared_obj");
        mngr.AddMethod<
            MemFuncOf<
                Ubpa::Utopia::SharedVar<T>,
                Ubpa::UDRefl::SharedObject()const
            >::template get(&Ubpa::Utopia::SharedVar<T>::cast_to_shared_obj)
        >("cast_to_shared_obj");
        Ubpa::UDRefl::details::TypeAutoRegister_Default<Ubpa::Utopia::SharedVar<T>>::run(mngr);
    }
};
