// Copyright 2000-2021 Mark H. P. Lord

//
// CallbackN provides a callback mechanism, e.g.:
//
//     // A callback which returns a bool and has two parameters, both floats.
//     Callback2<bool, float, float> myCallback;
//
//     // Point myCallback to a member function of an object.
//     myCallback = MethodCallback(anyObject, &ObjectClass::takeTwoFloatsAndReturnBool);
//
//     // Invoke the callback.
//     bool result = myCallback(1.0f, 2.0f);
//
// Note that MethodCallback (which calls a member function) could have been a FunctionCallback, which calls a plain
// old function, or a SelectorCallback, which invokes a method of an Objective C object.
//
// CallbackList provides a means of invoking multiple Callbacks. For example, you could have the following (note that
// CallbackLists don't have a return type):
//
//     class ProjectManager {
//     public:
//         CallbackList1<ProjectManager*> projectListChangedCallback;
//
// then you could add a method to invoke with:
//
//     manager->projectListChangedCallback += MethodCallback(this, &MyClass::pojectListChanged);
//
// and when the ProjectManager invokes all the callbacks:
//
//     projectListChangedCallback(this);
//
// the method `void MyClass::projectListChanged(ProjectManager*)` would be invoked on your object.
//
// A MethodCallback can take an object as the receiver, instead of a pointer to an object, allowing you to use
// smart pointers or other wrappers. The only requirement on the receiver is that it has an operator Type * () and an
// operator == (Type*), and that it be below a certain compile-time maximum size (at least two pointers):
//
//     // object will be retained until the callback is destructed
//     aCallback = MethodCallback(Ref(object), &Type::method);
//
// There was once a way to use a std::function, but I killed it, because:
// 1. std::functions are large (8 pointers in libc++) and thus bloat all Callbacks,
// 2. std::functions are not comparable for equality, so they can't be used in a CallbackList,
// 3. If you're using C++11 then you should just use std::function anyway (since a Callback is a valid function-like)
//
// At some point std::function will be ubiquitious and this file will serve as a warning to future generations of
// the perils of omitting crucial functionality from a programming lanuage.
//

#ifndef PRIME_CALLBACK_H
#define PRIME_CALLBACK_H

#include "Config.h"
#include <functional>
#include <memory>

#ifdef __OBJC__
#import <Foundation/NSObject.h>
#endif

namespace Prime {

//
// This is the ugly bit (but it works on every compiler I've tried it on, back to VC6).
// Invoking a callback involves a virtual call, followed by the actual method/function/selector call. The virtual
// call could potentially be replaced with an indirect function call by making the code significantly more ugly.
// Beyond that, brittle compiler dependent hacks would be needed.
//

namespace CallbackPrivate {

#ifdef PRIME_COMPILER_NO_RETURN_TEMPLATE_VOID

    // Visual C++ 6 doesn't have the ability to return void in a template, nor does it have the ability
    // to construct an object of any type using the Type() syntax (e.g., int() fails). This works around
    // both. It's the simplest workaround I could find considering that Visual C++ 6 also lacked SFINAE
    // and partial template specialisation.

    //
    // VoidReturnHack
    //

    template <typename T>
    struct VoidReturnHack {
        typedef T Return;

        Return result;

        template <typename Function>
        T invokeFunction(Function f, void* context)
        {
            return f(context);
        }

        template <typename Function, typename P1>
        T invokeFunction(Function f, void* context, P1 param1)
        {
            return f(context, param1);
        }

        template <typename Function, typename P1, typename P2>
        T invokeFunction(Function f, void* context, P1 param1, P2 param2)
        {
            return f(context, param1, param2);
        }

        template <typename Function, typename P1, typename P2, typename P3>
        T invokeFunction(Function f, void* context, P1 param1, P2 param2, P3 param3)
        {
            return f(context, param1, param2, param3);
        }

        template <typename Function, typename P1, typename P2, typename P3, typename P4>
        T invokeFunction(Function f, void* context, P1 param1, P2 param2, P3 param3, P4 param4)
        {
            return f(context, param1, param2, param3, param4);
        }

        template <typename Function, typename P1, typename P2, typename P3, typename P4, typename P5>
        T invokeFunction(Function f, void* context, P1 param1, P2 param2, P3 param3, P4 param4, P5 param5)
        {
            return f(context, param1, param2, param3, param4, param5);
        }

        template <typename Receiver, typename Method>
        T invokeMethod(Receiver receiver, Method method)
        {
            return (receiver->*method)();
        }

        template <typename Receiver, typename Method, typename P1>
        T invokeMethod(Receiver receiver, Method method, P1 param1)
        {
            return (receiver->*method)(param1);
        }

        template <typename Receiver, typename Method, typename P1, typename P2>
        T invokeMethod(Receiver receiver, Method method, P1 param1, P2 param2)
        {
            return (receiver->*method)(param1, param2);
        }

        template <typename Receiver, typename Method, typename P1, typename P2, typename P3>
        T invokeMethod(Receiver receiver, Method method, P1 param1, P2 param2, P3 param3)
        {
            return (receiver->*method)(param1, param2, param3);
        }

        template <typename Receiver, typename Method, typename P1, typename P2, typename P3, typename P4>
        T invokeMethod(Receiver receiver, Method method, P1 param1, P2 param2, P3 param3, P4 param4)
        {
            return (receiver->*method)(param1, param2, param3, param4);
        }

        template <typename Receiver, typename Method, typename P1, typename P2, typename P3, typename P4, typename P5>
        T invokeMethod(Receiver receiver, Method method, P1 param1, P2 param2, P3 param3, P4 param4, P5 param5)
        {
            return (receiver->*method)(param1, param2, param3, param4, param5);
        }
    };

    template <>
    struct VoidReturnHack<void> {
        typedef int Return;

        int result;

        template <typename Function>
        int invokeFunction(Function f, void* context)
        {
            f(context);
            return 0;
        }

        template <typename Function, typename P1>
        int invokeFunction(Function f, void* context, P1 param1)
        {
            f(context, param1);
            return 0;
        }

        template <typename Function, typename P1, typename P2>
        int invokeFunction(Function f, void* context, P1 param1, P2 param2)
        {
            f(context, param1, param2);
            return 0;
        }

        template <typename Function, typename P1, typename P2, typename P3>
        int invokeFunction(Function f, void* context, P1 param1, P2 param2, P3 param3)
        {
            f(context, param1, param2, param3);
            return 0;
        }

        template <typename Function, typename P1, typename P2, typename P3, typename P4>
        int invokeFunction(Function f, void* context, P1 param1, P2 param2, P3 param3, P4 param4)
        {
            f(context, param1, param2, param3, param4);
            return 0;
        }

        template <typename Function, typename P1, typename P2, typename P3, typename P4, typename P5>
        int invokeFunction(Function f, void* context, P1 param1, P2 param2, P3 param3, P4 param4, P5 param5)
        {
            f(context, param1, param2, param3, param4, param5);
            return 0;
        }

        template <typename Receiver, typename Method>
        int invokeMethod(Receiver receiver, Method method)
        {
            (receiver->*method)();
            return 0;
        }

        template <typename Receiver, typename Method, typename P1>
        int invokeMethod(Receiver receiver, Method method, P1 param1)
        {
            (receiver->*method)(param1);
            return 0;
        }

        template <typename Receiver, typename Method, typename P1, typename P2>
        int invokeMethod(Receiver receiver, Method method, P1 param1, P2 param2)
        {
            (receiver->*method)(param1, param2);
            return 0;
        }

        template <typename Receiver, typename Method, typename P1, typename P2, typename P3>
        int invokeMethod(Receiver receiver, Method method, P1 param1, P2 param2, P3 param3)
        {
            (receiver->*method)(param1, param2, param3);
            return 0;
        }

        template <typename Receiver, typename Method, typename P1, typename P2, typename P3, typename P4>
        int invokeMethod(Receiver receiver, Method method, P1 param1, P2 param2, P3 param3, P4 param4)
        {
            (receiver->*method)(param1, param2, param3, param4);
            return 0;
        }

        template <typename Receiver, typename Method, typename P1, typename P2, typename P3, typename P4, typename P5>
        int invokeMethod(Receiver receiver, Method method, P1 param1, P2 param2, P3 param3, P4 param4, P5 param5)
        {
            (receiver->*method)(param1, param2, param3, param4, param5);
            return 0;
        }
    };

#endif

    //
    // InvokableN
    //
    // Base classes for invokable callbacks with varying numbers of parameters.
    //

    enum InvokableType {
        InvokableTypeEmpty,
        InvokableTypeMethod,
        InvokableTypeFunction,
        InvokableTypeSelector
    };

    class InvokableBase {
    public:
        InvokableBase() { }

        virtual ~InvokableBase() { }

        virtual InvokableType getType() const PRIME_NOEXCEPT = 0;

        PRIME_UNCOPYABLE(InvokableBase);
    };

    template <typename Return>
    class Invokable0 : public InvokableBase {
    public:
        Invokable0() { }

#ifdef PRIME_COMPILER_NO_RETURN_TEMPLATE_VOID
        virtual typename VoidReturnHack<Return>::Return invoke() const = 0;
#else
        virtual Return invoke() const = 0;
#endif

        virtual bool isEqual(const Invokable0& other) const = 0;

        virtual Invokable0* createClone(void* memory) const = 0;

        PRIME_UNCOPYABLE(Invokable0);
    };

    template <typename Return, typename P1>
    class Invokable1 : public InvokableBase {
    public:
        Invokable1() { }

#ifdef PRIME_COMPILER_NO_RETURN_TEMPLATE_VOID
        virtual typename VoidReturnHack<Return>::Return invoke(P1) const = 0;
#else
        virtual Return invoke(P1) const = 0;
#endif

        virtual bool isEqual(const Invokable1& other) const = 0;

        virtual Invokable1* createClone(void* memory) const = 0;

        PRIME_UNCOPYABLE(Invokable1);
    };

    template <typename Return, typename P1, typename P2>
    class Invokable2 : public InvokableBase {
    public:
        Invokable2() { }

#ifdef PRIME_COMPILER_NO_RETURN_TEMPLATE_VOID
        virtual typename VoidReturnHack<Return>::Return invoke(P1, P2) const = 0;
#else
        virtual Return invoke(P1, P2) const = 0;
#endif

        virtual bool isEqual(const Invokable2& other) const = 0;

        virtual Invokable2* createClone(void* memory) const = 0;

        PRIME_UNCOPYABLE(Invokable2);
    };

    template <typename Return, typename P1, typename P2, typename P3>
    class Invokable3 : public InvokableBase {
    public:
        Invokable3() { }

#ifdef PRIME_COMPILER_NO_RETURN_TEMPLATE_VOID
        virtual typename VoidReturnHack<Return>::Return invoke(P1, P2, P3) const = 0;
#else
        virtual Return invoke(P1, P2, P3) const = 0;
#endif

        virtual bool isEqual(const Invokable3& other) const = 0;

        virtual Invokable3* createClone(void* memory) const = 0;

        PRIME_UNCOPYABLE(Invokable3);
    };

    template <typename Return, typename P1, typename P2, typename P3, typename P4>
    class Invokable4 : public InvokableBase {
    public:
        Invokable4() { }

#ifdef PRIME_COMPILER_NO_RETURN_TEMPLATE_VOID
        virtual typename VoidReturnHack<Return>::Return invoke(P1, P2, P3, P4) const = 0;
#else
        virtual Return invoke(P1, P2, P3, P4) const = 0;
#endif

        virtual bool isEqual(const Invokable4& other) const = 0;

        virtual Invokable4* createClone(void* memory) const = 0;

        PRIME_UNCOPYABLE(Invokable4);
    };

    template <typename Return, typename P1, typename P2, typename P3, typename P4, typename P5>
    class Invokable5 : public InvokableBase {
    public:
        Invokable5() { }

#ifdef PRIME_COMPILER_NO_RETURN_TEMPLATE_VOID
        virtual typename VoidReturnHack<Return>::Return invoke(P1, P2, P3, P4, P5) const = 0;
#else
        virtual Return invoke(P1, P2, P3, P4, P5) const = 0;
#endif

        virtual bool isEqual(const Invokable5& other) const = 0;

        virtual Invokable5* createClone(void* memory) const = 0;

        PRIME_UNCOPYABLE(Invokable5);
    };

    //
    // EmptyInvokable
    //
    // A do-nothing Invokable.
    //

    template <typename Return>
    class EmptyInvokable0 : public Invokable0<Return> {
    public:
        typedef Invokable0<Return> Super;

#ifdef PRIME_COMPILER_NO_RETURN_TEMPLATE_VOID
        virtual typename VoidReturnHack<Return>::Return invoke() const
        {
            return VoidReturnHack<Return>().result;
        }
#else
        virtual Return invoke() const
        {
            return Return();
        }
#endif

        InvokableType getType() const PRIME_NOEXCEPT
        {
            return InvokableTypeEmpty;
        }

        bool isEqual(const Super& other) const
        {
            return other.getType() == InvokableTypeEmpty;
        }

        Super* createClone(void* memory) const
        {
            return new (memory) EmptyInvokable0;
        }
    };

    template <typename Return, typename P1>
    class EmptyInvokable1 : public Invokable1<Return, P1> {
    public:
        typedef Invokable1<Return, P1> Super;

#ifdef PRIME_COMPILER_NO_RETURN_TEMPLATE_VOID
        virtual typename VoidReturnHack<Return>::Return invoke(P1) const
        {
            return VoidReturnHack<Return>().result;
        }
#else
        virtual Return invoke(P1) const
        {
            return Return();
        }
#endif

        InvokableType getType() const PRIME_NOEXCEPT
        {
            return InvokableTypeEmpty;
        }

        bool isEqual(const Super& other) const
        {
            return other.getType() == InvokableTypeEmpty;
        }

        Super* createClone(void* memory) const
        {
            return new (memory) EmptyInvokable1;
        }
    };

    template <typename Return, typename P1, typename P2>
    class EmptyInvokable2 : public Invokable2<Return, P1, P2> {
    public:
        typedef Invokable2<Return, P1, P2> Super;

#ifdef PRIME_COMPILER_NO_RETURN_TEMPLATE_VOID
        virtual typename VoidReturnHack<Return>::Return invoke(P1, P2) const
        {
            return VoidReturnHack<Return>().result;
        }
#else
        virtual Return invoke(P1, P2) const
        {
            return Return();
        }
#endif

        InvokableType getType() const PRIME_NOEXCEPT
        {
            return InvokableTypeEmpty;
        }

        bool isEqual(const Super& other) const
        {
            return other.getType() == InvokableTypeEmpty;
        }

        Super* createClone(void* memory) const
        {
            return new (memory) EmptyInvokable2;
        }
    };

    template <typename Return, typename P1, typename P2, typename P3>
    class EmptyInvokable3 : public Invokable3<Return, P1, P2, P3> {
    public:
        typedef Invokable3<Return, P1, P2, P3> Super;

#ifdef PRIME_COMPILER_NO_RETURN_TEMPLATE_VOID
        virtual typename VoidReturnHack<Return>::Return invoke(P1, P2, P3) const
        {
            return VoidReturnHack<Return>().result;
        }
#else
        virtual Return invoke(P1, P2, P3) const
        {
            return Return();
        }
#endif

        InvokableType getType() const PRIME_NOEXCEPT
        {
            return InvokableTypeEmpty;
        }

        bool isEqual(const Super& other) const
        {
            return other.getType() == InvokableTypeEmpty;
        }

        Super* createClone(void* memory) const
        {
            return new (memory) EmptyInvokable3;
        }
    };

    template <typename Return, typename P1, typename P2, typename P3, typename P4>
    class EmptyInvokable4 : public Invokable4<Return, P1, P2, P3, P4> {
    public:
        typedef Invokable4<Return, P1, P2, P3, P4> Super;

#ifdef PRIME_COMPILER_NO_RETURN_TEMPLATE_VOID
        virtual typename VoidReturnHack<Return>::Return invoke(P1, P2, P3, P4) const
        {
            return VoidReturnHack<Return>().result;
        }
#else
        virtual Return invoke(P1, P2, P3, P4) const
        {
            return Return();
        }
#endif

        InvokableType getType() const PRIME_NOEXCEPT
        {
            return InvokableTypeEmpty;
        }

        bool isEqual(const Super& other) const
        {
            return other.getType() == InvokableTypeEmpty;
        }

        Super* createClone(void* memory) const
        {
            return new (memory) EmptyInvokable4;
        }
    };

    template <typename Return, typename P1, typename P2, typename P3, typename P4, typename P5>
    class EmptyInvokable5 : public Invokable5<Return, P1, P2, P3, P4, P5> {
    public:
        typedef Invokable5<Return, P1, P2, P3, P4, P5> Super;

#ifdef PRIME_COMPILER_NO_RETURN_TEMPLATE_VOID
        virtual typename VoidReturnHack<Return>::Return invoke(P1, P2, P3, P4, P5) const
        {
            return VoidReturnHack<Return>().result;
        }
#else
        virtual Return invoke(P1, P2, P3, P4, P5) const
        {
            return Return();
        }
#endif

        InvokableType getType() const PRIME_NOEXCEPT
        {
            return InvokableTypeEmpty;
        }

        bool isEqual(const Super& other) const
        {
            return other.getType() == InvokableTypeEmpty;
        }

        Super* createClone(void* memory) const
        {
            return new (memory) EmptyInvokable5;
        }
    };

    //
    // MethodInvokable
    //
    // An Invokable that calls a member function.
    //

    template <typename Receiver, typename Class, typename Return>
    class MethodInvokable0 : public Invokable0<Return> {
    public:
        typedef Return (Class::*Method)();

        MethodInvokable0(const Receiver& receiver, Method method)
            : _receiver(receiver)
            , _method(method)
        {
        }

#ifdef PRIME_COMPILER_NO_RETURN_TEMPLATE_VOID
        virtual typename VoidReturnHack<Return>::Return invoke() const
        {
            VoidReturnHack<Return> hack;
            return hack.invokeMethod(_receiver, _method);
        }
#else
        Return invoke() const
        {
            return (_receiver->*_method)();
        }
#endif

        InvokableType getType() const PRIME_NOEXCEPT
        {
            return InvokableTypeMethod;
        }

        bool isEqual(const Invokable0<Return>& other) const
        {
            if (getType() != other.getType()) {
                return false;
            }

            const MethodInvokable0& other2 = static_cast<const MethodInvokable0&>(other);

            return _receiver == other2._receiver && _method == other2._method;
        }

        Invokable0<Return>* createClone(void* memory) const
        {
            return new (memory) MethodInvokable0(_receiver, _method);
        }

    private:
        Receiver _receiver;
        Method _method;

        PRIME_UNCOPYABLE(MethodInvokable0);
    };

    template <typename Receiver, typename Class, typename Return, typename P1>
    class MethodInvokable1 : public Invokable1<Return, P1> {
    public:
        typedef Return (Class::*Method)(P1);

        MethodInvokable1(const Receiver& receiver, Method method)
            : _receiver(receiver)
            , _method(method)
        {
        }

#ifdef PRIME_COMPILER_NO_RETURN_TEMPLATE_VOID
        virtual typename VoidReturnHack<Return>::Return invoke(P1 param1) const
        {
            VoidReturnHack<Return> hack;
            return hack.invokeMethod(_receiver, _method, param1);
        }
#else
        Return invoke(P1 param1) const
        {
            return (_receiver->*_method)(param1);
        }
#endif

        InvokableType getType() const PRIME_NOEXCEPT
        {
            return InvokableTypeMethod;
        }

        bool isEqual(const Invokable1<Return, P1>& other) const
        {
            if (getType() != other.getType()) {
                return false;
            }

            const MethodInvokable1& other2 = static_cast<const MethodInvokable1&>(other);

            return _receiver == other2._receiver && _method == other2._method;
        }

        Invokable1<Return, P1>* createClone(void* memory) const
        {
            return new (memory) MethodInvokable1(_receiver, _method);
        }

    private:
        Receiver _receiver;
        Method _method;

        PRIME_UNCOPYABLE(MethodInvokable1);
    };

    template <typename Receiver, typename Class, typename Return, typename P1, typename P2>
    class MethodInvokable2 : public Invokable2<Return, P1, P2> {
    public:
        typedef Return (Class::*Method)(P1, P2);

        MethodInvokable2(const Receiver& receiver, Method method)
            : _receiver(receiver)
            , _method(method)
        {
        }

#ifdef PRIME_COMPILER_NO_RETURN_TEMPLATE_VOID
        virtual typename VoidReturnHack<Return>::Return invoke(P1 param1, P2 param2) const
        {
            VoidReturnHack<Return> hack;
            return hack.invokeMethod(_receiver, _method, param1, param2);
        }
#else
        Return invoke(P1 param1, P2 param2) const
        {
            return (_receiver->*_method)(param1, param2);
        }
#endif

        InvokableType getType() const PRIME_NOEXCEPT
        {
            return InvokableTypeMethod;
        }

        bool isEqual(const Invokable2<Return, P1, P2>& other) const
        {
            if (getType() != other.getType()) {
                return false;
            }

            const MethodInvokable2& other2 = static_cast<const MethodInvokable2&>(other);

            return _receiver == other2._receiver && _method == other2._method;
        }

        Invokable2<Return, P1, P2>* createClone(void* memory) const
        {
            return new (memory) MethodInvokable2(_receiver, _method);
        }

    private:
        Receiver _receiver;
        Method _method;

        PRIME_UNCOPYABLE(MethodInvokable2);
    };

    template <typename Receiver, typename Class, typename Return, typename P1, typename P2, typename P3>
    class MethodInvokable3 : public Invokable3<Return, P1, P2, P3> {
    public:
        typedef Return (Class::*Method)(P1, P2, P3);

        MethodInvokable3(const Receiver& receiver, Method method)
            : _receiver(receiver)
            , _method(method)
        {
        }

#ifdef PRIME_COMPILER_NO_RETURN_TEMPLATE_VOID
        virtual typename VoidReturnHack<Return>::Return invoke(P1 param1, P2 param2, P3 param3) const
        {
            VoidReturnHack<Return> hack;
            return hack.invokeMethod(_receiver, _method, param1, param2, param3);
        }
#else
        Return invoke(P1 param1, P2 param2, P3 param3) const
        {
            return (_receiver->*_method)(param1, param2, param3);
        }
#endif

        InvokableType getType() const PRIME_NOEXCEPT
        {
            return InvokableTypeMethod;
        }

        bool isEqual(const Invokable3<Return, P1, P2, P3>& other) const
        {
            if (getType() != other.getType()) {
                return false;
            }

            const MethodInvokable3& other2 = static_cast<const MethodInvokable3&>(other);

            return _receiver == other2._receiver && _method == other2._method;
        }

        Invokable3<Return, P1, P2, P3>* createClone(void* memory) const
        {
            return new (memory) MethodInvokable3(_receiver, _method);
        }

    private:
        Receiver _receiver;
        Method _method;

        PRIME_UNCOPYABLE(MethodInvokable3);
    };

    template <typename Receiver, typename Class, typename Return, typename P1, typename P2, typename P3, typename P4>
    class MethodInvokable4 : public Invokable4<Return, P1, P2, P3, P4> {
    public:
        typedef Return (Class::*Method)(P1, P2, P3, P4);

        MethodInvokable4(const Receiver& receiver, Method method)
            : _receiver(receiver)
            , _method(method)
        {
        }

#ifdef PRIME_COMPILER_NO_RETURN_TEMPLATE_VOID
        virtual typename VoidReturnHack<Return>::Return invoke(P1 param1, P2 param2, P3 param3, P4 param4) const
        {
            VoidReturnHack<Return> hack;
            return hack.invokeMethod(_receiver, _method, param1, param2, param3, param4);
        }
#else
        Return invoke(P1 param1, P2 param2, P3 param3, P4 param4) const
        {
            return (_receiver->*_method)(param1, param2, param3, param4);
        }
#endif

        InvokableType getType() const PRIME_NOEXCEPT
        {
            return InvokableTypeMethod;
        }

        bool isEqual(const Invokable4<Return, P1, P2, P3, P4>& other) const
        {
            if (getType() != other.getType()) {
                return false;
            }

            const MethodInvokable4& other2 = static_cast<const MethodInvokable4&>(other);

            return _receiver == other2._receiver && _method == other2._method;
        }

        Invokable4<Return, P1, P2, P3, P4>* createClone(void* memory) const
        {
            return new (memory) MethodInvokable4(_receiver, _method);
        }

    private:
        Receiver _receiver;
        Method _method;

        PRIME_UNCOPYABLE(MethodInvokable4);
    };

    template <typename Receiver, typename Class, typename Return, typename P1, typename P2, typename P3, typename P4, typename P5>
    class MethodInvokable5 : public Invokable5<Return, P1, P2, P3, P4, P5> {
    public:
        typedef Return (Class::*Method)(P1, P2, P3, P4, P5);

        MethodInvokable5(const Receiver& receiver, Method method)
            : _receiver(receiver)
            , _method(method)
        {
        }

#ifdef PRIME_COMPILER_NO_RETURN_TEMPLATE_VOID
        virtual typename VoidReturnHack<Return>::Return invoke(P1 param1, P2 param2, P3 param3, P4 param4, P5 param5) const
        {
            VoidReturnHack<Return> hack;
            return hack.invokeMethod(_receiver, _method, param1, param2, param3, param4, param5);
        }
#else
        Return invoke(P1 param1, P2 param2, P3 param3, P4 param4, P5 param5) const
        {
            return (_receiver->*_method)(param1, param2, param3, param4, param5);
        }
#endif

        InvokableType getType() const PRIME_NOEXCEPT
        {
            return InvokableTypeMethod;
        }

        bool isEqual(const Invokable5<Return, P1, P2, P3, P4, P5>& other) const
        {
            if (getType() != other.getType()) {
                return false;
            }

            const MethodInvokable5& other2 = static_cast<const MethodInvokable5&>(other);

            return _receiver == other2._receiver && _method == other2._method;
        }

        Invokable5<Return, P1, P2, P3, P4, P5>* createClone(void* memory) const
        {
            return new (memory) MethodInvokable5(_receiver, _method);
        }

    private:
        Receiver _receiver;
        Method _method;

        PRIME_UNCOPYABLE(MethodInvokable5);
    };

    //
    // FunctionInvokable
    //
    // An Invokable that calls a function with a context pointer (always a void*).
    //

    template <typename Return>
    class FunctionInvokable0 : public Invokable0<Return> {
    public:
        typedef Return (*Function)(void*);

        FunctionInvokable0(Function function, void* context)
            : _function(function)
            , _context(context)
        {
        }

#ifdef PRIME_COMPILER_NO_RETURN_TEMPLATE_VOID
        virtual typename VoidReturnHack<Return>::Return invoke() const
        {
            VoidReturnHack<Return> hack;
            return hack.invokeFunction(_function, _context);
        }
#else
        Return invoke() const
        {
            return (*_function)(_context);
        }
#endif

        InvokableType getType() const PRIME_NOEXCEPT
        {
            return InvokableTypeFunction;
        }

        bool isEqual(const Invokable0<Return>& other) const
        {
            if (getType() != other.getType()) {
                return false;
            }

            const FunctionInvokable0& other2 = static_cast<const FunctionInvokable0&>(other);

            return _context == other2._context && _function == other2._function;
        }

        Invokable0<Return>* createClone(void* memory) const
        {
            return new (memory) FunctionInvokable0(_function, _context);
        }

    private:
        Function _function;
        void* _context;

        PRIME_UNCOPYABLE(FunctionInvokable0);
    };

    template <typename Return, typename P1>
    class FunctionInvokable1 : public Invokable1<Return, P1> {
    public:
        typedef Return (*Function)(void*, P1);

        FunctionInvokable1(Function function, void* context)
            : _function(function)
            , _context(context)
        {
        }

#ifdef PRIME_COMPILER_NO_RETURN_TEMPLATE_VOID
        virtual typename VoidReturnHack<Return>::Return invoke(P1 param1) const
        {
            VoidReturnHack<Return> hack;
            return hack.invokeFunction(_function, _context, param1);
        }
#else
        Return invoke(P1 param1) const
        {
            return (*_function)(_context, param1);
        }
#endif

        InvokableType getType() const PRIME_NOEXCEPT
        {
            return InvokableTypeFunction;
        }

        bool isEqual(const Invokable1<Return, P1>& other) const
        {
            if (getType() != other.getType()) {
                return false;
            }

            const FunctionInvokable1& other2 = static_cast<const FunctionInvokable1&>(other);

            return _context == other2._context && _function == other2._function;
        }

        Invokable1<Return, P1>* createClone(void* memory) const
        {
            return new (memory) FunctionInvokable1(_function, _context);
        }

    private:
        Function _function;
        void* _context;

        PRIME_UNCOPYABLE(FunctionInvokable1);
    };

    template <typename Return, typename P1, typename P2>
    class FunctionInvokable2 : public Invokable2<Return, P1, P2> {
    public:
        typedef Return (*Function)(void*, P1, P2);

        FunctionInvokable2(Function function, void* context)
            : _function(function)
            , _context(context)
        {
        }

#ifdef PRIME_COMPILER_NO_RETURN_TEMPLATE_VOID
        virtual typename VoidReturnHack<Return>::Return invoke(P1 param1, P2 param2) const
        {
            VoidReturnHack<Return> hack;
            return hack.invokeFunction(_function, _context, param1, param2);
        }
#else
        Return invoke(P1 param1, P2 param2) const
        {
            return (*_function)(_context, param1, param2);
        }
#endif

        InvokableType getType() const PRIME_NOEXCEPT
        {
            return InvokableTypeFunction;
        }

        bool isEqual(const Invokable2<Return, P1, P2>& other) const
        {
            if (getType() != other.getType()) {
                return false;
            }

            const FunctionInvokable2& other2 = static_cast<const FunctionInvokable2&>(other);

            return _context == other2._context && _function == other2._function;
        }

        Invokable2<Return, P1, P2>* createClone(void* memory) const
        {
            return new (memory) FunctionInvokable2(_function, _context);
        }

    private:
        Function _function;
        void* _context;

        PRIME_UNCOPYABLE(FunctionInvokable2);
    };

    template <typename Return, typename P1, typename P2, typename P3>
    class FunctionInvokable3 : public Invokable3<Return, P1, P2, P3> {
    public:
        typedef Return (*Function)(void*, P1, P2, P3);

        FunctionInvokable3(Function function, void* context)
            : _function(function)
            , _context(context)
        {
        }

#ifdef PRIME_COMPILER_NO_RETURN_TEMPLATE_VOID
        virtual typename VoidReturnHack<Return>::Return invoke(P1 param1, P2 param2, P3 param3) const
        {
            VoidReturnHack<Return> hack;
            return hack.invokeFunction(_function, _context, param1, param2, param3);
        }
#else
        Return invoke(P1 param1, P2 param2, P3 param3) const
        {
            return (*_function)(_context, param1, param2, param3);
        }
#endif

        InvokableType getType() const PRIME_NOEXCEPT
        {
            return InvokableTypeFunction;
        }

        bool isEqual(const Invokable3<Return, P1, P2, P3>& other) const
        {
            if (getType() != other.getType()) {
                return false;
            }

            const FunctionInvokable3& other2 = static_cast<const FunctionInvokable3&>(other);

            return _context == other2._context && _function == other2._function;
        }

        Invokable3<Return, P1, P2, P3>* createClone(void* memory) const
        {
            return new (memory) FunctionInvokable3(_function, _context);
        }

    private:
        Function _function;
        void* _context;

        PRIME_UNCOPYABLE(FunctionInvokable3);
    };

    template <typename Return, typename P1, typename P2, typename P3, typename P4>
    class FunctionInvokable4 : public Invokable4<Return, P1, P2, P3, P4> {
    public:
        typedef Return (*Function)(void*, P1, P2, P3, P4);

        FunctionInvokable4(Function function, void* context)
            : _function(function)
            , _context(context)
        {
        }

#ifdef PRIME_COMPILER_NO_RETURN_TEMPLATE_VOID
        virtual typename VoidReturnHack<Return>::Return invoke(P1 param1, P2 param2, P3 param3, P4 param4) const
        {
            VoidReturnHack<Return> hack;
            return hack.invokeFunction(_function, _context, param1, param2, param3, param4);
        }
#else
        Return invoke(P1 param1, P2 param2, P3 param3, P4 param4) const
        {
            return (*_function)(_context, param1, param2, param3, param4);
        }
#endif

        InvokableType getType() const PRIME_NOEXCEPT
        {
            return InvokableTypeFunction;
        }

        bool isEqual(const Invokable4<Return, P1, P2, P3, P4>& other) const
        {
            if (getType() != other.getType()) {
                return false;
            }

            const FunctionInvokable4& other2 = static_cast<const FunctionInvokable4&>(other);

            return _context == other2._context && _function == other2._function;
        }

        Invokable4<Return, P1, P2, P3, P4>* createClone(void* memory) const
        {
            return new (memory) FunctionInvokable4(_function, _context);
        }

    private:
        Function _function;
        void* _context;

        PRIME_UNCOPYABLE(FunctionInvokable4);
    };

    template <typename Return, typename P1, typename P2, typename P3, typename P4, typename P5>
    class FunctionInvokable5 : public Invokable5<Return, P1, P2, P3, P4, P5> {
    public:
        typedef Return (*Function)(void*, P1, P2, P3, P4, P5);

        FunctionInvokable5(Function function, void* context)
            : _function(function)
            , _context(context)
        {
        }

#ifdef PRIME_COMPILER_NO_RETURN_TEMPLATE_VOID
        virtual typename VoidReturnHack<Return>::Return invoke(P1 param1, P2 param2, P3 param3, P4 param4, P5 param5) const
        {
            VoidReturnHack<Return> hack;
            return hack.invokeFunction(_function, _context, param1, param2, param3, param4, param5);
        }
#else
        Return invoke(P1 param1, P2 param2, P3 param3, P4 param4, P5 param5) const
        {
            return (*_function)(_context, param1, param2, param3, param4, param5);
        }
#endif

        InvokableType getType() const PRIME_NOEXCEPT
        {
            return InvokableTypeFunction;
        }

        bool isEqual(const Invokable5<Return, P1, P2, P3, P4, P5>& other) const
        {
            if (getType() != other.getType()) {
                return false;
            }

            const FunctionInvokable5& other2 = static_cast<const FunctionInvokable5&>(other);

            return _context == other2._context && _function == other2._function;
        }

        Invokable5<Return, P1, P2, P3, P4, P5>* createClone(void* memory) const
        {
            return new (memory) FunctionInvokable5(_function, _context);
        }

    private:
        Function _function;
        void* _context;

        PRIME_UNCOPYABLE(FunctionInvokable5);
    };

    //
    // SelectorInvokable
    //
    // An Invokable that invokes a method of an Objective-C object.
    //

    // PRIME_SIZEOF_SELECTORINVOKABLE is verified by an assertion in all the SelectorCallback functions.
    // On platforms where we know we'll never have selector callbacks, it's safe to use zero.

#ifndef __OBJC__
#define PRIME_SIZEOF_SELECTORINVOKABLE 0
#else
#define PRIME_SIZEOF_SELECTORINVOKABLE (sizeof(void*) * 4)
#endif

#ifdef __OBJC__

    // All of these methods cache the function to invoke, at the cost of one extra pointer per object.

    template <typename Return>
    class SelectorInvokable0 : public Invokable0<Return> {
    public:
        typedef Return (*Function)(id target, SEL selector);

        SelectorInvokable0(id receiver, SEL selector)
            : _receiver(receiver)
            , _selector(selector)
        {
            _function = (Function)[_receiver methodForSelector:_selector];
            PRIME_ASSERT(_function);
        }

        Return invoke() const
        {
            return (*_function)(_receiver, _selector);
        }

        InvokableType getType() const PRIME_NOEXCEPT { return InvokableTypeSelector; }

        bool isEqual(const Invokable0<Return>& other) const
        {
            if (getType() != other.getType()) {
                return false;
            }

            const SelectorInvokable0& other2 = static_cast<const SelectorInvokable0&>(other);

            return _receiver == other2._receiver && _selector == other2._selector;
        }

        Invokable0<Return>* createClone(void* memory) const
        {
            return new (memory) SelectorInvokable0(_receiver, _selector);
        }

    private:
        id _receiver;
        SEL _selector;
        Function _function;

        PRIME_UNCOPYABLE(SelectorInvokable0);
    };

    template <typename Return, typename P1>
    class SelectorInvokable1 : public Invokable1<Return, P1> {
    public:
        typedef Return (*Function)(id target, SEL selector, P1);

        SelectorInvokable1(id receiver, SEL selector)
            : _receiver(receiver)
            , _selector(selector)
        {
            _function = (Function)[_receiver methodForSelector:_selector];
            PRIME_ASSERT(_function);
        }

        Return invoke(P1 param1) const
        {
            return (*_function)(_receiver, _selector, param1);
        }

        InvokableType getType() const PRIME_NOEXCEPT { return InvokableTypeSelector; }

        bool isEqual(const Invokable1<Return, P1>& other) const
        {
            if (getType() != other.getType()) {
                return false;
            }

            const SelectorInvokable1& other2 = static_cast<const SelectorInvokable1&>(other);

            return _receiver == other2._receiver && _selector == other2._selector;
        }

        Invokable1<Return, P1>* createClone(void* memory) const
        {
            return new (memory) SelectorInvokable1(_receiver, _selector);
        }

    private:
        id _receiver;
        SEL _selector;
        Function _function;

        PRIME_UNCOPYABLE(SelectorInvokable1);
    };

    template <typename Return, typename P1, typename P2>
    class SelectorInvokable2 : public Invokable2<Return, P1, P2> {
    public:
        typedef Return (*Function)(id target, SEL selector, P1, P2);

        SelectorInvokable2(id receiver, SEL selector)
            : _receiver(receiver)
            , _selector(selector)
        {
            _function = (Function)[_receiver methodForSelector:_selector];
            PRIME_ASSERT(_function);
        }

        Return invoke(P1 param1, P2 param2) const
        {
            return (*_function)(_receiver, _selector, param1, param2);
        }

        InvokableType getType() const PRIME_NOEXCEPT { return InvokableTypeSelector; }

        bool isEqual(const Invokable2<Return, P1, P2>& other) const
        {
            if (getType() != other.getType()) {
                return false;
            }

            const SelectorInvokable2& other2 = static_cast<const SelectorInvokable2&>(other);

            return _receiver == other2._receiver && _selector == other2._selector;
        }

        Invokable2<Return, P1, P2>* createClone(void* memory) const
        {
            return new (memory) SelectorInvokable2(_receiver, _selector);
        }

    private:
        id _receiver;
        SEL _selector;
        Function _function;

        PRIME_UNCOPYABLE(SelectorInvokable2);
    };

    template <typename Return, typename P1, typename P2, typename P3>
    class SelectorInvokable3 : public Invokable3<Return, P1, P2, P3> {
    public:
        typedef Return (*Function)(id target, SEL selector, P1, P2, P3);

        SelectorInvokable3(id receiver, SEL selector)
            : _receiver(receiver)
            , _selector(selector)
        {
            _function = (Function)[_receiver methodForSelector:_selector];
            PRIME_ASSERT(_function);
        }

        Return invoke(P1 param1, P2 param2, P3 param3) const
        {
            return (*_function)(_receiver, _selector, param1, param2, param3);
        }

        InvokableType getType() const PRIME_NOEXCEPT { return InvokableTypeSelector; }

        bool isEqual(const Invokable3<Return, P1, P2, P3>& other) const
        {
            if (getType() != other.getType()) {
                return false;
            }

            const SelectorInvokable3& other2 = static_cast<const SelectorInvokable3&>(other);

            return _receiver == other2._receiver && _selector == other2._selector;
        }

        Invokable3<Return, P1, P2, P3>* createClone(void* memory) const
        {
            return new (memory) SelectorInvokable3(_receiver, _selector);
        }

    private:
        id _receiver;
        SEL _selector;
        Function _function;

        PRIME_UNCOPYABLE(SelectorInvokable3);
    };

    template <typename Return, typename P1, typename P2, typename P3, typename P4>
    class SelectorInvokable4 : public Invokable4<Return, P1, P2, P3, P4> {
    public:
        typedef Return (*Function)(id target, SEL selector, P1, P2, P3, P4);

        SelectorInvokable4(id receiver, SEL selector)
            : _receiver(receiver)
            , _selector(selector)
        {
            _function = (Function)[_receiver methodForSelector:_selector];
            PRIME_ASSERT(_function);
        }

        Return invoke(P1 param1, P2 param2, P3 param3, P4 param4) const
        {
            return (*_function)(_receiver, _selector, param1, param2, param3, param4);
        }

        InvokableType getType() const PRIME_NOEXCEPT { return InvokableTypeSelector; }

        bool isEqual(const Invokable4<Return, P1, P2, P3, P4>& other) const
        {
            if (getType() != other.getType()) {
                return false;
            }

            const SelectorInvokable4& other2 = static_cast<const SelectorInvokable4&>(other);

            return _receiver == other2._receiver && _selector == other2._selector;
        }

        Invokable4<Return, P1, P2, P3, P4>* createClone(void* memory) const
        {
            return new (memory) SelectorInvokable4(_receiver, _selector);
        }

    private:
        id _receiver;
        SEL _selector;
        Function _function;

        PRIME_UNCOPYABLE(SelectorInvokable4);
    };

    template <typename Return, typename P1, typename P2, typename P3, typename P4, typename P5>
    class SelectorInvokable5 : public Invokable5<Return, P1, P2, P3, P4, P5> {
    public:
        typedef Return (*Function)(id target, SEL selector, P1, P2, P3, P4, P5);

        SelectorInvokable5(id receiver, SEL selector)
            : _receiver(receiver)
            , _selector(selector)
        {
            _function = (Function)[_receiver methodForSelector:_selector];
            PRIME_ASSERT(_function);
        }

        Return invoke(P1 param1, P2 param2, P3 param3, P4 param4, P5 param5) const
        {
            return (*_function)(_receiver, _selector, param1, param2, param3, param4, param5);
        }

        InvokableType getType() const PRIME_NOEXCEPT { return InvokableTypeSelector; }

        bool isEqual(const Invokable5<Return, P1, P2, P3, P4, P5>& other) const
        {
            if (getType() != other.getType()) {
                return false;
            }

            const SelectorInvokable5& other2 = static_cast<const SelectorInvokable5&>(other);

            return _receiver == other2._receiver && _selector == other2._selector;
        }

        Invokable5<Return, P1, P2, P3, P4, P5>* createClone(void* memory) const
        {
            return new (memory) SelectorInvokable5(_receiver, _selector);
        }

    private:
        id _receiver;
        SEL _selector;
        Function _function;

        PRIME_UNCOPYABLE(SelectorInvokable5);
    };

#endif // __OBJC__

    // Used by CallbackBase to reserve space for a smart pointer or other complex receiver instance.
    template <typename Class>
    struct DummySmartPtr {
        void* ptr[2];

        Class* operator->() const { return NULL; }

        operator Class*() const { return NULL; }

        operator void*() const { return NULL; }

        bool operator==(const DummySmartPtr&) const { return true; }

        bool operator!=(const DummySmartPtr&) const { return true; }
    };

    //
    // CallbackBase
    //

    /// Base class for all the CallbackN classes in the public interface.
    template <typename Subtype>
    class CallbackBase {
    public:
        CallbackBase() PRIME_NOEXCEPT { }

        bool isSet() const PRIME_NOEXCEPT
        {
            return (static_cast<const Subtype*>(this))->getInvokable()->getType() != InvokableTypeEmpty;
        }

        operator bool() const { return isSet(); }
        bool operator!() const { return !isSet(); }

        void* getMemory() const PRIME_NOEXCEPT { return _memory.bytes; }

        union LargestInvokable {
            char methodWithBigSmartPtr[sizeof(MethodInvokable1<DummySmartPtr<Invokable1<int, int>>, Invokable1<int, int>, int, int>)];
            char function[sizeof(FunctionInvokable1<int, int>)];
            char selector[PRIME_MAX(1, PRIME_SIZEOF_SELECTORINVOKABLE)];
            char bytes[1];
        };

    private:
        operator void*() const { return NULL; }

        mutable LargestInvokable _memory;

        PRIME_UNCOPYABLE(CallbackBase);
    };

} // namespace CallbackPrivate

//
// Callback0
//
// A callback with no parameters.
//

template <typename Return>
class Callback0 : public CallbackPrivate::CallbackBase<Callback0<Return>> {
public:
    typedef CallbackPrivate::CallbackBase<Callback0<Return>> Super;

    Callback0()
    {
        construct();
    }

    Callback0(const Callback0& copy)
    {
        construct();
        operator=(copy);
    }

    ~Callback0()
    {
        destruct();
    }

    void clear()
    {
        destruct();
        construct();
    }

#ifdef PRIME_COMPILER_NO_RETURN_TEMPLATE_VOID
    typename CallbackPrivate::VoidReturnHack<Return>::Return operator()() const
    {
        return getInvokable()->invoke();
    }
#else
    Return operator()() const
    {
        // This is a virtual call
        return getInvokable()->invoke();
    }
#endif

    CallbackPrivate::Invokable0<Return>* getInvokable() const
    {
        return reinterpret_cast<CallbackPrivate::Invokable0<Return>*>(Super::getMemory());
    }

    const Callback0& operator=(const Callback0& other)
    {
        other.getInvokable()->createClone(Super::getMemory());
        return *this;
    }

    bool operator==(const Callback0& other) const
    {
        return getInvokable()->isEqual(*other.getInvokable());
    }

    bool operator!=(const Callback0& other) const
    {
        return !operator==(other);
    }

private:
    void construct()
    {
        new (Super::getMemory()) CallbackPrivate::EmptyInvokable0<Return>;
    }

    void destruct()
    {
        reinterpret_cast<CallbackPrivate::Invokable0<Return>*>(Super::getMemory())->~Invokable0();
    }
};

//
// Callback1
//
// A callback with a single parameter.
//

template <typename Return, typename P1>
class Callback1 : public CallbackPrivate::CallbackBase<Callback1<Return, P1>> {
public:
    typedef CallbackPrivate::CallbackBase<Callback1<Return, P1>> Super;

    Callback1()
    {
        construct();
    }

    Callback1(const Callback1& copy)
    {
        construct();
        operator=(copy);
    }

    ~Callback1()
    {
        destruct();
    }

    void clear()
    {
        destruct();
        construct();
    }

#ifdef PRIME_COMPILER_NO_RETURN_TEMPLATE_VOID
    typename CallbackPrivate::VoidReturnHack<Return>::Return operator()(P1 param1) const
    {
        return getInvokable()->invoke(param1);
    }
#else
    Return operator()(P1 param1) const
    {
        // This is a virtual call
        return getInvokable()->invoke(param1);
    }
#endif

    CallbackPrivate::Invokable1<Return, P1>* getInvokable() const
    {
        return reinterpret_cast<CallbackPrivate::Invokable1<Return, P1>*>(Super::getMemory());
    }

    const Callback1& operator=(const Callback1& other)
    {
        other.getInvokable()->createClone(Super::getMemory());
        return *this;
    }

    bool operator==(const Callback1& other) const
    {
        return getInvokable()->isEqual(*other.getInvokable());
    }

    bool operator!=(const Callback1& other) const
    {
        return !operator==(other);
    }

private:
    void construct()
    {
        new (Super::getMemory()) CallbackPrivate::EmptyInvokable1<Return, P1>;
    }

    void destruct()
    {
        reinterpret_cast<CallbackPrivate::Invokable1<Return, P1>*>(Super::getMemory())->~Invokable1();
    }
};

//
// Callback2
//
// A callback with two parameters.
//

template <typename Return, typename P1, typename P2>
class Callback2 : public CallbackPrivate::CallbackBase<Callback2<Return, P1, P2>> {
public:
    typedef CallbackPrivate::CallbackBase<Callback2<Return, P1, P2>> Super;

    Callback2()
    {
        construct();
    }

    Callback2(const Callback2& copy)
    {
        construct();
        operator=(copy);
    }

    ~Callback2()
    {
        destruct();
    }

    void clear()
    {
        destruct();
        construct();
    }

#ifdef PRIME_COMPILER_NO_RETURN_TEMPLATE_VOID
    typename CallbackPrivate::VoidReturnHack<Return>::Return operator()(P1 param1, P2 param2) const
    {
        return getInvokable()->invoke(param1, param2);
    }
#else
    Return operator()(P1 param1, P2 param2) const
    {
        // This is a virtual call
        return getInvokable()->invoke(param1, param2);
    }
#endif

    CallbackPrivate::Invokable2<Return, P1, P2>* getInvokable() const
    {
        return reinterpret_cast<CallbackPrivate::Invokable2<Return, P1, P2>*>(Super::getMemory());
    }

    const Callback2& operator=(const Callback2& other)
    {
        other.getInvokable()->createClone(Super::getMemory());
        return *this;
    }

    bool operator==(const Callback2& other) const
    {
        return getInvokable()->isEqual(*other.getInvokable());
    }

    bool operator!=(const Callback2& other) const
    {
        return !operator==(other);
    }

private:
    void construct()
    {
        new (Super::getMemory()) CallbackPrivate::EmptyInvokable2<Return, P1, P2>;
    }

    void destruct()
    {
        reinterpret_cast<CallbackPrivate::Invokable2<Return, P1, P2>*>(Super::getMemory())->~Invokable2();
    }
};

//
// Callback3
//
// A callback with three parameters.
//

template <typename Return, typename P1, typename P2, typename P3>
class Callback3 : public CallbackPrivate::CallbackBase<Callback3<Return, P1, P2, P3>> {
public:
    typedef CallbackPrivate::CallbackBase<Callback3<Return, P1, P2, P3>> Super;

    Callback3()
    {
        construct();
    }

    Callback3(const Callback3& copy)
    {
        construct();
        operator=(copy);
    }

    ~Callback3()
    {
        destruct();
    }

    void clear()
    {
        destruct();
        construct();
    }

#ifdef PRIME_COMPILER_NO_RETURN_TEMPLATE_VOID
    typename CallbackPrivate::VoidReturnHack<Return>::Return operator()(P1 param1, P2 param2, P3 param3) const
    {
        return getInvokable()->invoke(param1, param2, param3);
    }
#else
    Return operator()(P1 param1, P2 param2, P3 param3) const
    {
        // This is a virtual call
        return getInvokable()->invoke(param1, param2, param3);
    }
#endif

    CallbackPrivate::Invokable3<Return, P1, P2, P3>* getInvokable() const
    {
        return reinterpret_cast<CallbackPrivate::Invokable3<Return, P1, P2, P3>*>(Super::getMemory());
    }

    const Callback3& operator=(const Callback3& other)
    {
        other.getInvokable()->createClone(Super::getMemory());
        return *this;
    }

    bool operator==(const Callback3& other) const
    {
        return getInvokable()->isEqual(*other.getInvokable());
    }

    bool operator!=(const Callback3& other) const
    {
        return !operator==(other);
    }

private:
    void construct()
    {
        new (Super::getMemory()) CallbackPrivate::EmptyInvokable3<Return, P1, P2, P3>;
    }

    void destruct()
    {
        reinterpret_cast<CallbackPrivate::Invokable3<Return, P1, P2, P3>*>(Super::getMemory())->~Invokable3();
    }
};

//
// Callback4
//
// A callback with four parameters.
//

template <typename Return, typename P1, typename P2, typename P3, typename P4>
class Callback4 : public CallbackPrivate::CallbackBase<Callback4<Return, P1, P2, P3, P4>> {
public:
    typedef CallbackPrivate::CallbackBase<Callback4<Return, P1, P2, P3, P4>> Super;

    Callback4()
    {
        construct();
    }

    Callback4(const Callback4& copy)
    {
        construct();
        operator=(copy);
    }

    ~Callback4()
    {
        destruct();
    }

    void clear()
    {
        destruct();
        construct();
    }

#ifdef PRIME_COMPILER_NO_RETURN_TEMPLATE_VOID
    typename CallbackPrivate::VoidReturnHack<Return>::Return operator()(P1 param1, P2 param2, P3 param3, P4 param4) const
    {
        return getInvokable()->invoke(param1, param2, param3, param4);
    }
#else
    Return operator()(P1 param1, P2 param2, P3 param3, P4 param4) const
    {
        // This is a virtual call
        return getInvokable()->invoke(param1, param2, param3, param4);
    }
#endif

    CallbackPrivate::Invokable4<Return, P1, P2, P3, P4>* getInvokable() const
    {
        return reinterpret_cast<CallbackPrivate::Invokable4<Return, P1, P2, P3, P4>*>(Super::getMemory());
    }

    const Callback4& operator=(const Callback4& other)
    {
        other.getInvokable()->createClone(Super::getMemory());
        return *this;
    }

    bool operator==(const Callback4& other) const
    {
        return getInvokable()->isEqual(*other.getInvokable());
    }

    bool operator!=(const Callback4& other) const
    {
        return !operator==(other);
    }

private:
    void construct()
    {
        new (Super::getMemory()) CallbackPrivate::EmptyInvokable4<Return, P1, P2, P3, P4>;
    }

    void destruct()
    {
        reinterpret_cast<CallbackPrivate::Invokable4<Return, P1, P2, P3, P4>*>(Super::getMemory())->~Invokable4();
    }
};

//
// Callback5
//
// A callback with five parameters.
//

template <typename Return, typename P1, typename P2, typename P3, typename P4, typename P5>
class Callback5 : public CallbackPrivate::CallbackBase<Callback5<Return, P1, P2, P3, P4, P5>> {
public:
    typedef CallbackPrivate::CallbackBase<Callback5<Return, P1, P2, P3, P4, P5>> Super;

    Callback5()
    {
        construct();
    }

    Callback5(const Callback5& copy)
    {
        construct();
        operator=(copy);
    }

    ~Callback5()
    {
        destruct();
    }

    void clear()
    {
        destruct();
        construct();
    }

#ifdef PRIME_COMPILER_NO_RETURN_TEMPLATE_VOID
    typename CallbackPrivate::VoidReturnHack<Return>::Return operator()(P1 param1, P2 param2, P3 param3, P4 param4, P5 param5) const
    {
        return getInvokable()->invoke(param1, param2, param3, param4, param5);
    }
#else
    Return operator()(P1 param1, P2 param2, P3 param3, P4 param4, P5 param5) const
    {
        // This is a virtual call
        return getInvokable()->invoke(param1, param2, param3, param4, param5);
    }
#endif

    CallbackPrivate::Invokable5<Return, P1, P2, P3, P4, P5>* getInvokable() const
    {
        return reinterpret_cast<CallbackPrivate::Invokable5<Return, P1, P2, P3, P4, P5>*>(Super::getMemory());
    }

    const Callback5& operator=(const Callback5& other)
    {
        other.getInvokable()->createClone(Super::getMemory());
        return *this;
    }

    bool operator==(const Callback5& other) const
    {
        return getInvokable()->isEqual(*other.getInvokable());
    }

    bool operator!=(const Callback5& other) const
    {
        return !operator==(other);
    }

private:
    void construct()
    {
        new (Super::getMemory()) CallbackPrivate::EmptyInvokable5<Return, P1, P2, P3, P4, P5>;
    }

    void destruct()
    {
        reinterpret_cast<CallbackPrivate::Invokable5<Return, P1, P2, P3, P4, P5>*>(Super::getMemory())->~Invokable5();
    }
};
}

//
// Use our own placement new/delete to verify the invokable will fit in a CallbackN's intrinsic memory.
//

enum PrimeInvokableNewType { PrimeInvokableNew };

inline void* operator new(size_t size, void* ptr, PrimeInvokableNewType) PRIME_NOEXCEPT
{
    (void)size;
    PRIME_ASSERTMSG(size <= sizeof(Prime::Callback0<int>::LargestInvokable), "Callback LargestInvokable not large enough");
    return ptr;
}

inline void operator delete(void*, void*, PrimeInvokableNewType)PRIME_NOEXCEPT
{
}

namespace Prime {

//
// FunctionCallback
//
// Constructs a CallbackN that will invoke a function with a void * context parameter.
//

template <typename Return>
inline Callback0<Return> FunctionCallback(Return (*function)(void*), void* context = NULL)
{
    Callback0<Return> callback;
    new (callback.getMemory(), PrimeInvokableNew) CallbackPrivate::FunctionInvokable0<Return>(function, context);
    return callback;
}

template <typename Return, typename P1>
inline Callback1<Return, P1> FunctionCallback(Return (*function)(void*, P1), void* context = NULL)
{
    Callback1<Return, P1> callback;
    new (callback.getMemory(), PrimeInvokableNew) CallbackPrivate::FunctionInvokable1<Return, P1>(function, context);
    return callback;
}

template <typename Return, typename P1, typename P2>
inline Callback2<Return, P1, P2> FunctionCallback(Return (*function)(void*, P1, P2), void* context = NULL)
{
    Callback2<Return, P1, P2> callback;
    new (callback.getMemory(), PrimeInvokableNew) CallbackPrivate::FunctionInvokable2<Return, P1, P2>(function, context);
    return callback;
}

template <typename Return, typename P1, typename P2, typename P3>
inline Callback3<Return, P1, P2, P3> FunctionCallback(Return (*function)(void*, P1, P2, P3), void* context = NULL)
{
    Callback3<Return, P1, P2, P3> callback;
    new (callback.getMemory(), PrimeInvokableNew) CallbackPrivate::FunctionInvokable3<Return, P1, P2, P3>(function, context);
    return callback;
}

template <typename Return, typename P1, typename P2, typename P3, typename P4>
inline Callback4<Return, P1, P2, P3, P4> FunctionCallback(Return (*function)(void*, P1, P2, P3, P4), void* context = NULL)
{
    Callback4<Return, P1, P2, P3, P4> callback;
    new (callback.getMemory(), PrimeInvokableNew) CallbackPrivate::FunctionInvokable4<Return, P1, P2, P3, P4>(function, context);
    return callback;
}

template <typename Return, typename P1, typename P2, typename P3, typename P4, typename P5>
inline Callback5<Return, P1, P2, P3, P4, P5> FunctionCallback(Return (*function)(void*, P1, P2, P3, P4, P5), void* context = NULL)
{
    Callback5<Return, P1, P2, P3, P4, P5> callback;
    new (callback.getMemory(), PrimeInvokableNew) CallbackPrivate::FunctionInvokable5<Return, P1, P2, P3, P4, P5>(function, context);
    return callback;
}

//
// MethodCallback
//
// Constructs a CallbackN that will invoke a member function of an object, with a void * context parameter.
// Can call virtual or non-virtual member functions.
//

template <typename Receiver, typename Class, typename Return>
inline Callback0<Return> MethodCallback(const Receiver& receiver, Return (Class::*method)())
{
    Callback0<Return> callback;
    new (callback.getMemory(), PrimeInvokableNew) CallbackPrivate::MethodInvokable0<Receiver, Class, Return>(receiver, method);
    return callback;
}

template <typename Receiver, typename Class, typename Return, typename P1>
inline Callback1<Return, P1> MethodCallback(const Receiver& receiver, Return (Class::*method)(P1))
{
    Callback1<Return, P1> callback;
    new (callback.getMemory(), PrimeInvokableNew) CallbackPrivate::MethodInvokable1<Receiver, Class, Return, P1>(receiver, method);
    return callback;
}

template <typename Receiver, typename Class, typename Return, typename P1, typename P2>
inline Callback2<Return, P1, P2> MethodCallback(const Receiver& receiver, Return (Class::*method)(P1, P2))
{
    Callback2<Return, P1, P2> callback;
    new (callback.getMemory(), PrimeInvokableNew) CallbackPrivate::MethodInvokable2<Receiver, Class, Return, P1, P2>(receiver, method);
    return callback;
}

template <typename Receiver, typename Class, typename Return, typename P1, typename P2, typename P3>
inline Callback3<Return, P1, P2, P3> MethodCallback(const Receiver& receiver, Return (Class::*method)(P1, P2, P3))
{
    Callback3<Return, P1, P2, P3> callback;
    new (callback.getMemory(), PrimeInvokableNew) CallbackPrivate::MethodInvokable3<Receiver, Class, Return, P1, P2, P3>(receiver, method);
    return callback;
}

template <typename Receiver, typename Class, typename Return, typename P1, typename P2, typename P3, typename P4>
inline Callback4<Return, P1, P2, P3, P4> MethodCallback(const Receiver& receiver, Return (Class::*method)(P1, P2, P3, P4))
{
    Callback4<Return, P1, P2, P3, P4> callback;
    new (callback.getMemory(), PrimeInvokableNew) CallbackPrivate::MethodInvokable4<Receiver, Class, Return, P1, P2, P3, P4>(receiver, method);
    return callback;
}

template <typename Receiver, typename Class, typename Return, typename P1, typename P2, typename P3, typename P4, typename P5>
inline Callback5<Return, P1, P2, P3, P4, P5> MethodCallback(const Receiver& receiver, Return (Class::*method)(P1, P2, P3, P4, P5))
{
    Callback5<Return, P1, P2, P3, P4, P5> callback;
    new (callback.getMemory(), PrimeInvokableNew) CallbackPrivate::MethodInvokable5<Receiver, Class, Return, P1, P2, P3, P4, P5>(receiver, method);
    return callback;
}

#ifdef __OBJC__

//
// SelectorCallback
//
// Constructs a CallbackN that will invoke a method of an Objective-C object.
//

template <typename Return>
inline Callback0<Return> SelectorCallback(id receiver, SEL selector)
{
    PRIME_COMPILE_TIME_ASSERT(PRIME_SIZEOF_SELECTORINVOKABLE >= sizeof(CallbackPrivate::SelectorInvokable0<Return>));
    Callback0<Return> callback;
    new (callback.getMemory(), PrimeInvokableNew) CallbackPrivate::SelectorInvokable0<Return>(receiver, selector);
    return callback;
}

template <typename Return, typename P1>
inline Callback1<Return, P1> SelectorCallback(id receiver, SEL selector)
{
    PRIME_COMPILE_TIME_ASSERT(PRIME_SIZEOF_SELECTORINVOKABLE >= sizeof(CallbackPrivate::SelectorInvokable1<Return, P1>));
    Callback1<Return, P1> callback;
    new (callback.getMemory(), PrimeInvokableNew) CallbackPrivate::SelectorInvokable1<Return, P1>(receiver, selector);
    return callback;
}

template <typename Return, typename P1, typename P2>
inline Callback2<Return, P1, P2> SelectorCallback(id receiver, SEL selector)
{
    PRIME_COMPILE_TIME_ASSERT(PRIME_SIZEOF_SELECTORINVOKABLE >= sizeof(CallbackPrivate::SelectorInvokable2<Return, P1, P2>));
    Callback2<Return, P1, P2> callback;
    new (callback.getMemory(), PrimeInvokableNew) CallbackPrivate::SelectorInvokable2<Return, P1, P2>(receiver, selector);
    return callback;
}

template <typename Return, typename P1, typename P2, typename P3>
inline Callback3<Return, P1, P2, P3> SelectorCallback(id receiver, SEL selector)
{
    PRIME_COMPILE_TIME_ASSERT(PRIME_SIZEOF_SELECTORINVOKABLE >= sizeof(CallbackPrivate::SelectorInvokable3<Return, P1, P2, P3>));
    Callback3<Return, P1, P2, P3> callback;
    new (callback.getMemory(), PrimeInvokableNew) CallbackPrivate::SelectorInvokable3<Return, P1, P2, P3>(receiver, selector);
    return callback;
}

template <typename Return, typename P1, typename P2, typename P3, typename P4>
inline Callback4<Return, P1, P2, P3, P4> SelectorCallback(id receiver, SEL selector)
{
    PRIME_COMPILE_TIME_ASSERT(PRIME_SIZEOF_SELECTORINVOKABLE >= sizeof(CallbackPrivate::SelectorInvokable4<Return, P1, P2, P3, P4>));
    Callback4<Return, P1, P2, P3, P4> callback;
    new (callback.getMemory(), PrimeInvokableNew) CallbackPrivate::SelectorInvokable4<Return, P1, P2, P3, P4>(receiver, selector);
    return callback;
}

template <typename Return, typename P1, typename P2, typename P3, typename P4, typename P5>
inline Callback5<Return, P1, P2, P3, P4, P5> SelectorCallback(id receiver, SEL selector)
{
    PRIME_COMPILE_TIME_ASSERT(PRIME_SIZEOF_SELECTORINVOKABLE >= sizeof(CallbackPrivate::SelectorInvokable5<Return, P1, P2, P3, P4, P5>));
    Callback5<Return, P1, P2, P3, P4, P5> callback;
    new (callback.getMemory(), PrimeInvokableNew) CallbackPrivate::SelectorInvokable5<Return, P1, P2, P3, P4, P5>(receiver, selector);
    return callback;
}

#endif

//
// CallbackList
//

template <typename CallbackType>
class CallbackListBase {
protected:
    struct ListEntry;

public:
    typedef CallbackType Callback;

    CallbackListBase()
    {
        construct();
    }

    explicit CallbackListBase(const Callback& callback)
    {
        construct();
        add(callback);
    }

    ~CallbackListBase()
    {
        deleteAllCallbacks();
    }

    /// Returns true if there are no Callbacks installed.
    bool isEmpty() const { return _firstCallback == 0; }

    void clear()
    {
        deleteAllCallbacks();
    }

    void add(const Callback& adding)
    {
        ListEntry* listEntry = new ListEntry;
        listEntry->callback = adding;
        add(listEntry);
    }

    /// Remove any matching callback.
    void remove(const Callback& removing)
    {
        ListEntry listEntry;
        listEntry.callback = removing;
        remove(listEntry);
    }

    CallbackListBase& operator+=(const Callback& adding)
    {
        add(adding);
        return *this;
    }

    CallbackListBase& operator-=(const Callback& removing)
    {
        remove(removing);
        return *this;
    }

protected:
    struct ListEntry {
        ListEntry* next;
        Callback callback;
    };

    void construct()
    {
        _firstCallback = 0;
    }

    void add(ListEntry* listEntry)
    {
        remove(*listEntry);

        listEntry->next = _firstCallback;
        _firstCallback = listEntry;
    }

    void remove(const ListEntry& match)
    {
        ListEntry** prevNext = &_firstCallback;

        for (ListEntry* listEntry = _firstCallback; listEntry; prevNext = &listEntry->next, listEntry = listEntry->next) {
            if (match.callback == listEntry->callback) {
                *prevNext = listEntry->next;
                delete listEntry;
                return;
            }
        }
    }

    void deleteAllCallbacks()
    {
        while (_firstCallback) {
            ListEntry* next = _firstCallback->next;
            delete _firstCallback;
            _firstCallback = next;
        }
    }

    ListEntry* _firstCallback;

    PRIME_UNCOPYABLE(CallbackListBase);
};

class CallbackList0 : public CallbackListBase<Callback0<void>> {
public:
    typedef CallbackListBase<Callback0<void>> Super;

    /// Invokes all the callbacks.
    void operator()() const
    {
        const Super::ListEntry* next;

        for (const Super::ListEntry* listEntry = Super::_firstCallback; listEntry; listEntry = next) {
            next = listEntry->next;
            listEntry->callback.operator()();
        }
    }
};

template <typename P1>
class CallbackList1 : public CallbackListBase<Callback1<void, P1>> {
public:
    typedef CallbackListBase<Callback1<void, P1>> Super;

    /// Invokes all the callbacks.
    void operator()(P1 param1) const
    {
        const typename Super::ListEntry* next;

        for (const typename Super::ListEntry* listEntry = Super::_firstCallback; listEntry; listEntry = next) {
            next = listEntry->next;
            listEntry->callback.operator()(param1);
        }
    }
};

template <typename P1, typename P2>
class CallbackList2 : public CallbackListBase<Callback2<void, P1, P2>> {
public:
    typedef CallbackListBase<Callback2<void, P1, P2>> Super;

    /// Invokes all the callbacks.
    void operator()(P1 param1, P2 param2) const
    {
        const typename Super::ListEntry* next;

        for (const typename Super::ListEntry* listEntry = Super::_firstCallback; listEntry; listEntry = next) {
            next = listEntry->next;
            listEntry->callback.operator()(param1, param2);
        }
    }
};

template <typename P1, typename P2, typename P3>
class CallbackList3 : public CallbackListBase<Callback3<void, P1, P2, P3>> {
public:
    typedef CallbackListBase<Callback3<void, P1, P2, P3>> Super;

    /// Invokes all the callbacks.
    void operator()(P1 param1, P2 param2, P3 param3) const
    {
        const typename Super::ListEntry* next;

        for (const typename Super::ListEntry* listEntry = Super::_firstCallback; listEntry; listEntry = next) {
            next = listEntry->next;
            listEntry->callback.operator()(param1, param2, param3);
        }
    }
};

template <typename P1, typename P2, typename P3, typename P4>
class CallbackList4 : public CallbackListBase<Callback4<void, P1, P2, P3, P4>> {
public:
    typedef CallbackListBase<Callback4<void, P1, P2, P3, P4>> Super;

    /// Invokes all the callbacks.
    void operator()(P1 param1, P2 param2, P3 param3, P4 param4) const
    {
        const typename Super::ListEntry* next;

        for (const typename Super::ListEntry* listEntry = Super::_firstCallback; listEntry; listEntry = next) {
            next = listEntry->next;
            listEntry->callback.operator()(param1, param2, param3, param4);
        }
    }
};

template <typename P1, typename P2, typename P3, typename P4, typename P5>
class CallbackList5 : public CallbackListBase<Callback5<void, P1, P2, P3, P4, P5>> {
public:
    typedef CallbackListBase<Callback5<void, P1, P2, P3, P4, P5>> Super;

    /// Invokes all the callbacks.
    void operator()(P1 param1, P2 param2, P3 param3, P4 param4, P5 param5) const
    {
        const typename Super::ListEntry* next;

        for (const typename Super::ListEntry* listEntry = Super::_firstCallback; listEntry; listEntry = next) {
            next = listEntry->next;
            listEntry->callback.operator()(param1, param2, param3, param4, param5);
        }
    }
};

//
// WhenFinished
//

class WhenFinished {
public:
    explicit WhenFinished(const Callback0<void>& callback)
        : _callback(callback)
    {
    }

    WhenFinished() { }

    ~WhenFinished()
    {
        if (_callback.isSet()) {
            _callback.operator()();
        }
    }

    void clear() { _callback.clear(); }

    void set(const Callback0<void>& callback) { _callback = callback; }

private:
    Callback0<void> _callback;
};
}

#endif
