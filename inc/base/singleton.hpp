/**
 *******************************************************************************
 * @file    singleton.hpp
 * @brief   CRTP singleton helper for static application managers
 *******************************************************************************
 */

#ifndef OMNIGEN_H7_APP_BASE_SINGLETON_HPP
#define OMNIGEN_H7_APP_BASE_SINGLETON_HPP

/* ------- include ---------------------------------------------------------------------------------------------------*/

#include <cstddef>

namespace omnigen {

/* ------- class -----------------------------------------------------------------------------------------------------*/

template <typename T> class Singleton {
  public:
    static T& instance() {
        if (instance_ == nullptr) {
            instance_ = new (storage()) T();
        }

        return *instance_;
    }

    Singleton(const Singleton&)            = delete;
    Singleton& operator=(const Singleton&) = delete;

  protected:
    Singleton()  = default;
    ~Singleton() = default;

  private:
    static void* operator new(size_t, void* storage) { return storage; }

    static void* storage() {
        alignas(T) static unsigned char object_storage[sizeof(T)] = {};
        return object_storage;
    }

    static T* instance_;
};

template <typename T> T* Singleton<T>::instance_ = nullptr;

} // namespace omnigen

#endif /* OMNIGEN_H7_APP_BASE_SINGLETON_HPP */
