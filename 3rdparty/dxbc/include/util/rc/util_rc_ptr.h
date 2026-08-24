#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <ostream>
#include <utility>

namespace dxvk {

  /**
   * \brief Pointer for reference-counted objects
   * 
   * This only requires the given type to implement \c incRef
   * and \c decRef methods that adjust the reference count.
   * \tparam T Object type
   */
  template<typename T>
  class Rc {
    template<typename Tx>
    friend class Rc;
  public:

    Rc() = default;
    Rc(std::nullptr_t) { }

    Rc(T* object)
    : m_object(object) {
      this->incRef();
    }

    Rc(const Rc& other)
    : m_object(other.m_object) {
      this->incRef();
    }

    template<typename Tx>
    Rc(const Rc<Tx>& other)
    : m_object(other.m_object) {
      this->incRef();
    }

    Rc(Rc&& other)
    : m_object(other.m_object) {
      other.m_object = nullptr;
    }

    template<typename Tx>
    Rc(Rc<Tx>&& other)
    : m_object(other.m_object) {
      other.m_object = nullptr;
    }

    Rc& operator = (std::nullptr_t) {
      this->decRef();
      m_object = nullptr;
      return *this;
    }

    Rc& operator = (const Rc& other) {
      other.incRef();
      this->decRef();
      m_object = other.m_object;
      return *this;
    }

    template<typename Tx>
    Rc& operator = (const Rc<Tx>& other) {
      other.incRef();
      this->decRef();
      m_object = other.m_object;
      return *this;
    }

    Rc& operator = (Rc&& other) {
      this->decRef();
      this->m_object = other.m_object;
      other.m_object = nullptr;
      return *this;
    }

    template<typename Tx>
    Rc& operator = (Rc<Tx>&& other) {
      this->decRef();
      this->m_object = other.m_object;
      other.m_object = nullptr;
      return *this;
    }

    ~Rc() {
      this->decRef();
    }

    T& operator *  () const { return *m_object; }
    T* operator -> () const { return  m_object; }
    T* ptr() const { return m_object; }

    template<typename Tx> bool operator == (const Rc<Tx>& other) const { return m_object == other.m_object; }
    template<typename Tx> bool operator != (const Rc<Tx>& other) const { return m_object != other.m_object; }

    template<typename Tx> bool operator == (Tx* other) const { return m_object == other; }
    template<typename Tx> bool operator != (Tx* other) const { return m_object != other; }

    bool operator == (std::nullptr_t) const { return m_object == nullptr; }
    bool operator != (std::nullptr_t) const { return m_object != nullptr; }
    
    explicit operator bool () const {
      return m_object != nullptr;
    }

    /**
     * \brief Sets pointer without acquiring a reference
     *
     * Must only be use when a reference has been taken via
     * other means.
     * \param [in] object Object pointer
     */
    void unsafeInsert(T* object) {
      this->decRef();
      m_object = object;
    }

    /**
     * \brief Extracts raw pointer
     *
     * Sets the smart pointer to null without decrementing the
     * reference count. Must only be used when the reference
     * count is decremented in some other way.
     * \returns Pointer to owned object
     */
    T* unsafeExtract() {
      return std::exchange(m_object, nullptr);
    }

    /**
     * \brief Creates smart pointer without taking reference
     *
     * Must only be used when a refernece has been obtained via other means.
     * \param [in] object Pointer to object to take ownership of
     */
    static Rc<T> unsafeCreate(T* object) {
      return Rc<T>(object, false);
    }

  private:

    T* m_object = nullptr;

    explicit Rc(T* object, bool)
    : m_object(object) { }

    force_inline void incRef() const {
      if (m_object != nullptr)
        m_object->incRef();
    }

    force_inline void decRef() const {
      if (m_object != nullptr) {
        if constexpr (std::is_void_v<decltype(m_object->decRef())>) {
          m_object->decRef();
        } else {
          // Deprecated, objects should manage themselves now.
          if (!m_object->decRef())
            delete m_object;
        }
      }
    }

  };

  template<typename Tx, typename Ty>
  bool operator == (Tx* a, const Rc<Ty>& b) { return b == a; }

  template<typename Tx, typename Ty>
  bool operator != (Tx* a, const Rc<Ty>& b) { return b != a; }

  struct RcHash {
    template<typename T>
    size_t operator () (const Rc<T>& rc) const {
      return reinterpret_cast<uintptr_t>(rc.ptr()) / sizeof(T);
    }
  };

}

template<typename T>
std::ostream& operator << (std::ostream& os, const dxvk::Rc<T>& rc) {
  return os << rc.ptr();
}
