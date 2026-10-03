//
// Created by Lenovo on 2026/10/2.
//
#include "../include/native.h"

#include <algorithm>

#ifdef TEST

#define mica_void new BaseType(BaseType::MVOID)
#define mica_string new TArrayType(new BaseType(BaseType::MCHAR))
#define mica_int new BaseType(BaseType::MINT)
#define mica_double new BaseType(BaseType::MDOUBLE)
#define mica_bool new BaseType(BaseType::MBOOL)
#define MFBEGIN [](Environment* env, std::vector<MicaValue> args) -> MicaValue

int random_int(int left, int right) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(left, right);
    return distrib(gen);
}

NativeFunction::NativeFunction(std::string name, Function* fn, FunctionSymbol* fs) {
    this->nativeFn = fn;
    this->symbol  = fs;
    this->name = name;
}

std::vector<NativeFunction*> getFuncs() {
    std::vector<NativeFunction*> res;
    auto fs_print = new FunctionSymbol("print", new FunctionType(mica_void, {mica_string}));
    auto fn_print   = new Function("print", MFBEGIN {
        auto tmp = ((ObjArray*)((ObjInstance*)args[0].obj)->cls)->elements;
        std::string temp;
        for (auto i : tmp) temp += i.c;
        std::cout << temp;
        return MicaValue::Null();
    });
    res.push_back(new NativeFunction("print", fn_print, fs_print));

    auto fs_println = new FunctionSymbol("println", new FunctionType(mica_void, {mica_string}));
    auto fn_println   = new Function("println", MFBEGIN {
        auto tmp = ((ObjArray*)((ObjInstance*)args[0].obj)->cls)->elements;
        std::string temp;
        for (auto i : tmp) temp += i.c;
        std::cout << temp;
        std::cout << std::endl;
        return MicaValue::Null();
    });
    res.push_back(new NativeFunction("println", fn_println, fs_println));

    auto fs_itos = new FunctionSymbol("itos", new FunctionType(mica_string, {mica_int}));
    auto fn_itos = new Function("itos", MFBEGIN {
        long long n = args[0].i;
        bool neg = (n < 0);

        unsigned long long u;
        if (neg) u = (unsigned long long)(-(n + 1)) + 1ULL;
        else     u = (unsigned long long)n;

        std::string tmp;
        if (u == 0) {
            tmp = "0";
        } else {
            while (u) {
                tmp += (char)((u % 10) + '0');
                u /= 10;
            }
            std::reverse(tmp.begin(), tmp.end());
        }
        if (neg) tmp.insert(tmp.begin(), '-');

        ObjArray *obj = new ObjArray;
        obj->elements.clear();
        for (auto i : tmp) obj->elements.push_back(MicaValue::Char(i));

        ObjInstance* ins = new ObjInstance();
        ins->cls = obj;
        env->addObject(ins);
        return MicaValue::Object(ins);
    });
    res.push_back(new NativeFunction("itos", fn_itos, fs_itos));

    auto fs_stoi = new FunctionSymbol("stoi", new FunctionType(mica_int, {mica_string}));
    auto fn_stoi = new Function("stoi", MFBEGIN {
        std::string s;
        auto arr = (ObjArray*)((ObjInstance*)args[0].obj)->cls;
        for (auto i : arr->elements) s += i.c;

        long long v = 0;
        bool neg = false;
        size_t i = 0;

        while (i < s.size() && std::isspace((unsigned char)s[i])) ++i;
        if (i < s.size() && (s[i] == '+' || s[i] == '-')) {
            neg = (s[i] == '-');
            ++i;
        }
        for (; i < s.size() && s[i] >= '0' && s[i] <= '9'; ++i)
            v = v * 10 + (s[i] - '0');
        if (neg) v = -v;

        return MicaValue::Int(v);
    });
    res.push_back(new NativeFunction("stoi", fn_stoi, fs_stoi));

    auto fs_stof = new FunctionSymbol("stof", new FunctionType(mica_double, {mica_string}));
    auto fn_stof = new Function("stof", MFBEGIN {
        std::string s;
        auto arr = (ObjArray*)((ObjInstance*)args[0].obj)->cls;
        for (auto i : arr->elements) s += i.c;

        double v = 0.0;
        bool neg = false;
        size_t i = 0;

        while (i < s.size() && std::isspace((unsigned char)s[i])) ++i;
        if (i < s.size() && (s[i] == '+' || s[i] == '-')) {
            neg = (s[i] == '-');
            ++i;
        }
        for (; i < s.size() && s[i] >= '0' && s[i] <= '9'; ++i)
            v = v * 10.0 + (s[i] - '0');
        if (i < s.size() && s[i] == '.') {
            ++i;
            double scale = 0.1;
            for (; i < s.size() && s[i] >= '0' && s[i] <= '9'; ++i) {
                v += (s[i] - '0') * scale;
                scale *= 0.1;
            }
        }
        if (i < s.size() && (s[i] == 'e' || s[i] == 'E')) {
            ++i;
            bool eneg = false;
            if (i < s.size() && (s[i] == '+' || s[i] == '-')) {
                eneg = (s[i] == '-');
                ++i;
            }
            int e = 0;
            for (; i < s.size() && s[i] >= '0' && s[i] <= '9'; ++i)
                e = e * 10 + (s[i] - '0');
            while (e--) v = eneg ? v / 10.0 : v * 10.0;
        }

        if (neg) v = -v;
        return MicaValue::Float(v);
    });
    res.push_back(new NativeFunction("stof", fn_stof, fs_stof));

    auto fs_ftos = new FunctionSymbol("ftos", new FunctionType(mica_string, {mica_double}));
    auto fn_ftos = new Function("ftos", MFBEGIN {
        double d = (args[0].kind == MicaValue::FLOAT) ? args[0].f : (double)args[0].i;

        std::string tmp;
        if (std::isnan(d))      tmp = "nan";
        else if (std::isinf(d)) tmp = (d < 0) ? "-inf" : "inf";
        else {
            char buf[64];
            std::snprintf(buf, sizeof(buf), "%.5g", d);
            tmp = buf;
        }

        ObjArray* obj = new ObjArray;
        obj->elements.clear();
        for (char c : tmp) obj->elements.push_back(MicaValue::Char(c));

        ObjInstance* ins = new ObjInstance();
        ins->cls = obj;
        env->addObject(ins);
        return MicaValue::Object(ins);
    });
    res.push_back(new NativeFunction("ftos", fn_ftos, fs_ftos));

    auto fs_resize = new FunctionSymbol("resize", new FunctionType(mica_void, {mica_int}));
    auto fn_resize = new Function("resize", MFBEGIN {
        ((ObjArray*)((ObjInstance*)args[0].obj)->cls)->elements.resize(args[1].i);
            return MicaValue::Null();
    });
    res.push_back(new NativeFunction("resize", fn_resize, fs_resize));

    auto fs_strlen = new FunctionSymbol("strLen", new FunctionType(mica_int, {mica_string}));
    auto fn_strlen = new Function("strLen", MFBEGIN {
        return MicaValue::Int(((ObjArray*)((ObjInstance*)args[0].obj)->cls)->elements.size());
    });
    res.push_back(new NativeFunction("strLen", fn_strlen, fs_strlen));

    auto fs_randrange = new FunctionSymbol("random", new FunctionType(mica_int, {mica_int, mica_int}));
    auto fn_randrange = new Function("random",  MFBEGIN {
        auto left = args[0].i;
        auto right = args[1].i;
        return MicaValue::Int(random_int(left, right));
    });
    res.push_back(new NativeFunction("random", fn_randrange, fs_randrange));

    return res;
}



#endif