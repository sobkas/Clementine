/* This file is part of Clementine.
   Copyright 2012, David Sansome <me@davidsansome.com>

   Clementine is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Clementine is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Clementine.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SIGNALCHECKER_H
#define SIGNALCHECKER_H

#include <cxxabi.h>
#include <typeinfo>

#include <glib-object.h>

#include <boost/function_types/function_arity.hpp>
#include <boost/typeof/typeof.hpp>

#include <QtDebug>
#include <QList>

// Do not call this directly, use CHECKED_GCONNECT instead.
bool CheckedGConnect(
    gpointer source,
    const char* signal,
    GCallback callback,
    gpointer data,
    const int callback_param_count);

#define FUNCTION_ARITY(callback) boost::function_types::function_arity<BOOST_TYPEOF(callback)>::value

#define CHECKED_GCONNECT(source, signal, callback, data) \
    CheckedGConnect(source, signal, G_CALLBACK(callback), data, FUNCTION_ARITY(callback));

template <typename T>
QString GetTypeName() {
  int status = 0;
  char* demangled = abi::__cxa_demangle(
      typeid(T).name(), NULL, NULL, &status);
  QString ret(demangled);
  free(demangled);
  return ret;
}

template<typename... Tail>
struct ParamPrinter {
  void PrintParam(QList<QString>*) {}
};

template<typename Head, typename... Tail>
struct ParamPrinter<Head, Tail...> {
  void PrintParam(QList<QString>* params) {
    params->push_back(GetTypeName<Head>());
    ParamPrinter<Tail...> f;
    f.PrintParam(params);
  }
};

template<typename R, typename... Params>
QList<QString> GetFunctionParamTypes(R (*func) (Params...)) {
  QList<QString> ret;
  ParamPrinter<Params...> f;
  f.PrintParams(&ret);
  return ret;
}

template<typename R, typename... Params>
QString GetFunctionReturnType(R (*func) (Params...)) {
  return GetTypeName<R>();
}

template<typename R, typename... Params>
void PrintFunctionParamTypes(R (*func) (Params...)) {
  QList<QString> params;
  ParamPrinter<Params...> f;
  f.PrintParam(&params);
  qDebug() << params;
}

#endif  // SIGNALCHECKER_H
