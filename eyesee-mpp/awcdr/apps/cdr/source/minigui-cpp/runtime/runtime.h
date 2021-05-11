/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: runtime.h
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-18
 Description:
    Runtime is a factory to generate class statically.

    MAX_VIEW_TYPE_COUNT is necessary to be enlarged when adding more
    child View types.

    Apply DECLARE_DYNCRT_CLASS in the View class , which will generate a
    auxiliary static class Child##Register, so as to register a combination of
    class name and factory function to the base class Runtime.

    Factory function will be call to create View class
    as soon as the name is matched.
 History:
*****************************************************************************/

#pragma once

#include <string>
#include <vector>

#include "debug/app_log.h"
#include "type/types.h"

class View;
class Runtime;
class IComponent;

typedef Runtime *(*ClassGen)(View *);

typedef struct {
  std::string name;
  ClassGen factory_method;
} ClassInfo;

/* declare the base class which can create child class dynamically */
#define DECLARE_DYNCRT_BASE(Runtime)                                          \
 public:                                                                      \
  /* declare function pointer */                                              \
  typedef Runtime *(*ClassGen)(View *);                                       \
                                                                              \
  /* factory mode to generate child */                                        \
  static Runtime *Create(const char *class_name, View *owner) {               \
    std::vector<ClassInfo>::iterator it;                                      \
    for (it = view_set_.begin(); it != view_set_.end(); it++) {               \
      if (it->name == class_name) {                                           \
        return (it->factory_method)(owner);                                   \
      }                                                                       \
    }                                                                         \
    return NULL;                                                              \
  }                                                                           \
                                                                              \
 protected:                                                                   \
  /* a set stored View class type */                                          \
  static std::vector<ClassInfo> view_set_;                                    \
                                                                              \
  static void Register(const char *class_name, Runtime::ClassGen class_gen) { \
    /* call by the Child##Register */                                         \
    ClassInfo view;                                                           \
    view.name = class_name;                                                   \
    view.factory_method = class_gen;                                          \
    view_set_.push_back(view);                                                \
  }

/* implement the base class */
#define IMPLEMENT_DYNCRT_BASE(Runtime) \
  std::vector<ClassInfo> Runtime::view_set_;

/* defining a static class to assist child to register factory function */
#define DECLARE_DYNCRT_CLASS(Child, Runtime)                           \
 public:                                                               \
  struct Child##Register {                                             \
    Child##Register() {                                                \
      /* ensure registration only once */                              \
      static bool has_registered = false;                              \
      if (!has_registered) {                                           \
        Runtime::Register((#Child), (Runtime::ClassGen)Child::Create); \
        has_registered = true;                                         \
      }                                                                \
    }                                                                  \
  };                                                                   \
                                                                       \
  /* actual factory function */                                        \
  static Runtime *Create(View *owner) { return new Child(owner); }     \
  static struct Child##Register m_t##Child##Register;                  \
  static char _xyz_zyx_##Child[];                                      \
  static char *GetClassName() { return _xyz_zyx_##Child; }

/* implement the auxiliary class */
#define IMPLEMENT_DYNCRT_CLASS(Child)                 \
  static Child::Child##Register m_t##Child##Register; \
                                                      \
  /* an extra information class name */               \
  char Child::_xyz_zyx_##Child[] = #Child;

class Runtime {
  DECLARE_DYNCRT_BASE(Runtime)
};
