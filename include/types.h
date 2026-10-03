//
// Created by Lenovo on 2026/10/3.
//

#ifndef TYPES_H
#define TYPES_H

#define mica_void new BaseType(BaseType::MVOID)
#define mica_string new TArrayType(new BaseType(BaseType::MCHAR))
#define mica_int new BaseType(BaseType::MINT)
#define mica_double new BaseType(BaseType::MDOUBLE)
#define mica_bool new BaseType(BaseType::MBOOL)
#define MFBEGIN [](Environment* env, std::vector<MicaValue> args) -> MicaValue

#endif //TYPES_H
