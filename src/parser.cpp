#include <utility>
#include "../include/parser.h"

Register::Register(): begin({"UNKNOWN", -1, -1}), end({"UNKNOWN", -1, -1}) {
    isSuc = false;
}

Register::Register(std::string error, Position begin, Position end): begin(std::move(begin)), end(std::move(end)) {
    this->error = error;
    this->isSuc = false;
}

Register::Register(AST* result): begin({"UNKNOWN", -1, -1}), end({"UNKNOWN", -1, -1}) {
    this->result = result;
    this->isSuc = true;
}

Parser::Parser() {}

void Parser::saveState() {
    states.push_back(current);
    lexer.saveState();
}

void Parser::restore() {
    current = states.back();
    states.pop_back();
    lexer.restore();
}

Register Parser::parseExpr(std::string expr, std::string file) {
    lexer.resetExpr(std::move(expr), file);
    advance(SYN_VALUE);
    return makeStmt();
}

void Parser::advance(int cs) {
    current = lexer.getToken(cs);
}

std::vector<Register> Parser::parseCode(std::string expr, std::string file) {
    lexer.resetExpr(std::move(expr), file);
    advance(SYN_VALUE);
    std::vector<Register> res;
    while (current.kind != TT_EOF) {
        auto tmp = makeStmt();
        res.push_back(tmp);
        if (!tmp.isSuc) break;
    }
    return res;
}

Position Parser::posBegin() { return current.begin; }
Position Parser::posEnd()   { return current.end; }

Register Parser::makeBinOpNode(MPCLBCK clb, std::vector<std::string> ops) {
    auto left = (this->*clb)();
    test(left);
    Position begin = left.result->begin;
    while (current.kind != TT_EOF && std::ranges::count(ops, current.data) > 0) {
        const std::string op = current.data;
        advance(SYN_VALUE);
        const auto right = (this->*clb)();
        test(right);
        Position end = right.result->end;
        left.result = new BinOpNode(op, left.result, right.result, begin, end);
    }
    return left;
}

Register Parser::makeNumberNode() {
    Position begin = posBegin();
    Position end = posEnd();
    Register res(new Number(current.data, begin, end));
    advance(SYN_VALUE);
    return res;
}

Register Parser::makeString() {
    Position begin = posBegin();
    Position end = posEnd();
    std::string str = current.data;
    advance(SYN_VALUE);
    std::vector<AST*> elements;
    for (int i = 0; i < (int)str.size(); ++i)
        elements.push_back(new Char(str[i], begin, end));
    return Register(new Array(elements, begin, end));
}

Register Parser::makeChar() {
    Position begin = posBegin();
    Position end = posEnd();
    Register res(new Char(current.data, begin, end));
    advance(SYN_VALUE);
    return res;
}

void Register::ok(AST* result) {
    this->result = result;
    this->isSuc = true;
}

void Register::fai(std::string error, Position begin, Position end) {
    this->error = error;
    this->isSuc = false;
    this->begin = begin;
    this->end = end;
}

Register Parser::makeValue() {
    if (equal(TT_INTEGER) || equal(TT_DOUBLE))
        return makeNumberNode();

    if (equal("++") || equal("--")) {
        Position begin = posBegin();
        std::string op = current.data;
        advance(SYN_VALUE);
        Register tmp = makeValue();
        test(tmp);
        Position end = tmp.result->end;
        return {new SelfChangeNode(tmp.result, op == "++", true, begin, end)};
    }

    if (equal(TT_KEY) && equal("new"))
        return makeNew();

    if (equal(TT_STRING))
        return makeString();

    if (equal(TT_OP) && equal("-")) {
        Position begin = posBegin();
        advance(SYN_VALUE);
        auto tmp = makeValue();
        test(tmp);
        Position end = tmp.result->end;
        tmp.result = new Neg(tmp.result, begin, end);
        return tmp;
    }

    if (equal(TT_BOOL)) {
        Position begin = posBegin();
        Position end = posEnd();
        std::string data = current.data;
        advance(SYN_VALUE);
        return new Bool(data, begin, end);
    }

    if (equal(TT_NULL)) {
        Position begin = posBegin();
        Position end = posEnd();
        advance(SYN_VALUE);
        return new Null(begin, end);
    }

    if (equal(TT_ID)) {
        auto tmp = makeId(SYN_VALUE);
        test(tmp);
        if (equal("++") || equal("--")) {
            Position end = posEnd();
            std::string op = current.data;
            advance(SYN_VALUE);
            return {new SelfChangeNode(tmp.result, op == "++", false,
                                       tmp.result->begin, end)};
        }
        return tmp;
    }

    if (equal(TT_OP) && equal("["))
        return makeArray();

    if (equal(TT_OP) && equal("(")) {
        advance(SYN_VALUE);
        auto tmp = makeExpr();
        test(tmp);
        setError(tmp, equal(")") && equal(TT_OP), "SyntaxError: Want a ')'");
        advance(SYN_VALUE);
        return makeElementGetN(tmp.result, {}, SYN_VALUE);
    }

    if (equal(TT_CHAR))
        return makeChar();

    Register res;
    setError(res, false, "SyntaxError: unknown operator symbol '" + current.data + "'");
}

Register Parser::makeArray() {
    Register res;
    Position begin = posBegin();
    advance(SYN_VALUE);
    std::vector<AST*> elements;

    if (!(equal("]") && equal(TT_OP))) {
        while (true) {
            auto elem = makeExpr();
            test(elem);
            elements.push_back(elem.result);
            if (equal("]") && equal(TT_OP)) break;
            setError(elem, equal(",") && equal(TT_OP), "SyntaxError: want a ','");
            advance(SYN_VALUE);
        }
    }
    Position end = posEnd();
    setError(res, equal("]") && equal(TT_OP), "SyntaxError: want a ']'");
    advance(SYN_VALUE);
    res.ok(new Array(elements, begin, end));
    return res;
}

Register Parser::makeFactor() {
    return makeBinOpNode(&Parser::makeValue, {"*", "/", "<<", ">>", "%", "|", "&", "^"});
}

Register Parser::makeTerm() {
    return makeBinOpNode(&Parser::makeFactor, {"+", "-", "in"});
}

Register Parser::makeExpr1() {
    return makeBinOpNode(&Parser::makeTerm, {"==", "!=", ">", "<", "<=", ">="});
}

Register Parser::makeExpr_() {
    return makeBinOpNode(&Parser::makeExpr1, {"&&", "||"});
}

Register Parser::makeExpr() {
    std::vector<std::string> unionSymbol = { "+=", "-=", "/=", "*=", "%=", "&=", "|=", ">>=", "<<=", "=" };
    auto first = makeExprA();
    test(first);
    if (equal(TT_OP) && std::count(unionSymbol.begin(), unionSymbol.end(), current.data) > 0) {
        std::string tmp = current.data;
        advance(SYN_VALUE);
        Register t = makeExpr();
        test(t);
        Position begin = first.result->begin;
        Position end = t.result->end;
        first.ok(new AssignNode(tmp, first.result, t.result, begin, end));
    }
    return first;
}

Register Parser::makeExprA() {
    Register tmp = makeExpr_();
    test(tmp);
    if (!equal("?")) return tmp;
    Position begin = tmp.result->begin;
    advance(SYN_VALUE);
    Register tvalue = makeExpr();
    test(tvalue);
    setError(tmp, equal(":") && equal(TT_OP), "SyntaxError: want a ':', found '" + current.data + "'");
    advance(SYN_VALUE);
    Register fvalue = makeExpr();
    test(fvalue);
    Position end = fvalue.result->end;
    tmp.ok(new ThreeOp(tmp.result, tvalue.result, fvalue.result, begin, end));
    return tmp;
}

bool Parser::equal(TokenKind kind) { return current.kind == kind; }
bool Parser::equal(std::string data) { return current.data == data; }

Register Parser::makeElementGetN(AST* name, std::vector<AST*> t_, int ptype) {
    Register res;
    res.ok(name);
    std::vector<AST*> t = std::move(t_);
    while (current.kind != TT_EOF) {
        if (equal("[")) {
            Position begin = res.result->begin;
            advance(SYN_VALUE);
            Register tmp = makeExpr();
            test(tmp);
            setError(tmp, equal("]") && equal(TT_OP), "SyntaxError: want a ']'");
            Position end = posEnd();
            advance(SYN_VALUE);
            res.ok(new ElementGet(res.result, tmp.result, begin, end));
        } else if (equal("<") && equal(TT_OP)) {
            saveState();
            TRegister tmp = makeTemplate();
            if (!tmp.isSuc) { restore(); break; }
            t = tmp.result;
        } else if (equal("(")) {
            auto tmp = makeCallNodeN(res.result, t, ptype);
            test(tmp);
            res.ok(tmp.result);
            t.clear();
        } else if (equal(".")) {
            auto tmp = makeMemberAccessN(res.result, {}, ptype);
            test(tmp);
            res.ok(tmp.result);
            t.clear();
        } else {
            break;
        }
    }
    return res;
}

TRegister::TRegister() : begin({"UNKNOWN", -1, -1}), end({"UNKNOWN", -1, -1}) {
    isSuc = false;
}

TRegister::TRegister(std::string error, Position begin, Position end) : begin(begin), end(end) {
    this->isSuc = false;
    this->error = error;
}

TRegister::TRegister(std::vector<AST*> suc) : begin({"UNKNOWN", -1, -1}), end({"UNKNOWN", -1, -1}) {
    this->result = suc;
    this->isSuc = true;
}

void TRegister::ok(std::vector<AST*> res) {
    this->isSuc = true;
    this->result = res;
}

void TRegister::fai(std::string error, Position begin, Position end) {
    this->error = error;
    this->begin = begin;
    this->end = end;
}

Register Parser::makeCallNodeN(AST* name, std::vector<AST*> t_, int ptype) {
    Register res;
    res.ok(name);
    std::vector<AST*> templates = std::move(t_);
    while (current.kind != TT_EOF) {
        if (equal("(")) {
            Position begin = res.result->begin;
            advance(SYN_CALL);
            std::vector<AST*> args;
            while (current.kind != TT_EOF && !(equal(TT_OP) && equal(")"))) {
                Register tmp = makeExpr();
                test(tmp);
                args.push_back(tmp.result);
                if (equal(")") && equal(TT_OP)) break;
                setError(res, equal(",") && equal(TT_OP), "SyntaxError: want a ','");
                advance(SYN_CALL);
            }
            Position end = posEnd();
            setError(res, equal(TT_OP) && equal(")"), "SyntaxError: want a ')'");
            advance(SYN_CALL);
            res.ok(new Call(res.result, args, templates, begin, end));
            templates.clear();
        } else if (equal("[")) {
            auto tmp = makeElementGetN(res.result, {}, ptype);
            test(tmp);
            res.ok(tmp.result);
        } else if (equal(".")) {
            auto tmp = makeMemberAccessN(res.result, {}, ptype);
            test(tmp);
            res.ok(tmp.result);
        } else {
            break;
        }
    }
    return res;
}

Register Parser::makeId(int ptype) {
    auto tmp = makeMemberAccess(ptype);
    test(tmp);
    TRegister t;
    if (tmp.result->kind == AST::AST_ID && equal("<") && equal(TT_OP)) {
        saveState();
        t = makeTemplate();
        if (!t.isSuc) restore();
    }
    if (equal(TT_OP) && equal("("))
        return makeCallNodeN(tmp.result, t.result, ptype);
    if (equal(TT_OP) && equal("["))
        return makeElementGetN(tmp.result, t.result, ptype);
    return tmp;
}

Register Parser::makeElementGet(int ptype) {
    auto tmp = makeMemberAccess(ptype);
    test(tmp);
    return makeElementGetN(tmp.result, {}, ptype);
}

Register Parser::makeCallNode() {
    auto tmp = makeMemberAccess(SYN_VALUE);
    test(tmp);
    TRegister temp;
    if (tmp.result->kind == AST::AST_ID && equal("<") && equal(TT_OP)) {
        saveState();
        temp = makeTemplate();
        if (!temp.isSuc) restore();
    }
    return makeCallNodeN(tmp.result, temp.result, SYN_VALUE);
}

Register Parser::makeMemberAccess(int ptype) {
    if (!equal(TT_ID))
        return {"SyntaxError: want a id node, but found '" + current.data + "'", posBegin(), posEnd()};
    Position begin = posBegin();
    Position end = posEnd();
    std::string tmp = current.data;
    advance(ptype);
    return makeMemberAccessN(new Id(tmp, begin, end), {}, ptype);
}

Register Parser::makeMemberAccessN(AST* name, std::vector<AST*> t_, int ptype) {
    Register res;
    res.ok(name);
    while (current.kind != TT_EOF && equal(".")) {
        Position begin = res.result->begin;
        advance(ptype);
        setError(res, equal(TT_ID), "SyntaxError: want a id node, but found '" + current.data + "'");
        std::string tmp = current.data;
        Position end = posEnd();
        advance(ptype);
        res.ok(new MemberAccess(res.result, tmp, {}, begin, end));
    }
    return res;
}

Register Parser::makeFunction() {
    Position begin = posBegin();
    advance();
    Register body, retType;
    std::string funcName;
    bool isNative = false;
    if (equal(TT_KEY) && equal("native")) {
        isNative = true;
        advance();
    }
    setError(body, equal(TT_ID), "SyntaxError: want a id, found '" + current.data + "'");
    funcName = current.data;
    advance();

    std::vector<std::string> templates;
    if (equal("<") && equal(TT_OP)) {
        advance(SYN_TYPE);
        while (current.kind != TT_EOF && !(equal(">") && equal(TT_OP))) {
            setError(body, equal(TT_ID), "SyntaxError: want a id node");
            templates.push_back(current.data);
            advance(SYN_TYPE);
            setError(body, equal(",") && equal(TT_OP), "SyntaxError: want a ','");
            advance();
        }
        setError(body, equal(">") && equal(TT_OP), "SyntaxError: want a '>'");
        advance();
    }

    setError(body, equal("(") && equal(TT_OP), "SyntaxError: want a '(', found '" + current.data + "'");
    advance();

    std::vector<AST*> args;
    while (current.kind != TT_EOF && !(equal(TT_OP) && equal(")"))) {
        Register tmp = makeVarDefine();
        test(tmp);
        args.push_back(tmp.result);
        if (equal(TT_OP) && equal(")")) break;
        setError(tmp, equal(",") && equal(TT_OP), "SyntaxError: want a ',', found '" + current.data + "'");
        advance();
    }
    setError(body, equal(")") && equal(TT_OP), "SyntaxError: want a ')', found '" + current.data + "'");
    advance();

    setError(body, equal(":") && equal(TT_OP), "SyntaxError: want a ':', found '" + current.data + "'");
    advance();
    retType = makeType();
    test(retType);

    Position end = posEnd();
    if (!isNative) {
        body = makeBlock();
        test(body);
        end = body.result->end;
    } else {
        body.ok(nullptr);
        setError(body, equal(";") && equal(TT_OP), "SyntaxError: need a ';'");
        end = posEnd();
        advance();
    }
    std::vector<AST*> argTypes;
    for (auto i : args) argTypes.push_back(((VarDef*)i)->type);
    return {new Func(funcName, body.result, args,
                     new FuncType(retType.result, argTypes, templates, begin, end),
                     isNative, begin, end)};
}

Register Parser::makeStmt() {
    if (equal(TT_KEY) && equal("for"))
        return makeFor();
    if (equal(TT_KEY) && equal("while"))
        return makeWhile();
    if (equal(TT_KEY) && equal("do"))
        return makeDoWhile();
    if (equal(TT_KEY) && equal("switch"))
        return makeSwitch();
    if (equal(TT_KEY) && equal("fn"))
        return makeFunction();
    if (equal(TT_KEY) && equal("class"))
        return makeClass();
    if (equal(TT_KEY) && equal("interface"))
        return makeInterface();
    if (equal(TT_KEY) && equal("if"))
        return makeIf();
    if (equal(TT_OP) && equal("{"))
        return makeBlock();

    Position begin = posBegin();
    if (equal(TT_KEY) && equal("break")) {
        Position end = posEnd();
        advance(SYN_VALUE);
        Register res;
        setError(res, equal(";") && equal(TT_OP), "SyntaxError: want a ';', found '" + current.data + "'");
        advance(SYN_VALUE);
        res.ok(new Break(begin, end));
        return res;
    }
    if (equal(TT_KEY) && equal("continue")) {
        Position end = posEnd();
        advance(SYN_VALUE);
        Register res;
        setError(res, equal(";") && equal(TT_OP), "SyntaxError: want a ';', found '" + current.data + "'");
        advance(SYN_VALUE);
        res.ok(new Continue(begin, end));
        return res;
    }
    if (equal(TT_KEY) && equal("let")) {
        auto tmp = makeVarDefGrp();
        setError(tmp, equal(TT_OP) && equal(";"), "SyntaxError: want a ';', found '" + current.data + "'");
        advance(SYN_VALUE);
        return tmp;
    }
    if (equal(TT_KEY) && equal("return")) {
        advance(SYN_VALUE);
        if (equal(";")) {
            Position end = posEnd();
            advance(SYN_VALUE);
            return {new Return(new Null(begin, end), begin, end)};
        }
        Register tmp = makeExpr();
        test(tmp);
        Position end = tmp.result->end;
        setError(tmp, equal(TT_OP) && equal(";"), "SyntaxError: want a ';', found '" + current.data + "'");
        advance(SYN_VALUE);
        return {new Return(tmp.result, begin, end)};
    }
    if (equal(TT_KEY) && equal("goto")) {
        advance(SYN_VALUE);
        Register tmp;
        setError(tmp, equal(TT_ID), "SyntaxError: want a id, found '" + current.data + "'");
        std::string label = current.data;
        Position end = posEnd();
        advance(SYN_VALUE);
        setError(tmp, equal(TT_OP) && equal(";"), "SyntaxError: want a ';', found '" + current.data + "'");
        advance(SYN_VALUE);
        return {new Goto(label, begin, end)};
    }

    Register tmp = makeExpr();
    test(tmp);
    setError(tmp, equal(TT_OP) && equal(";"), "SyntaxError: want a ';', found '" + current.data + "'");
    advance(SYN_VALUE);
    return tmp;
}

Register Parser::makeIf() {
    Position begin = posBegin();
    Register res;
    advance();
    setError(res, equal("(") && equal(TT_OP), "SyntaxError: want a '(', but found '" + current.data + "'");
    advance();
    res = makeExpr();
    test(res);
    setError(res, equal(")") && equal(TT_OP), "SyntaxError: want a ')', but found '" + current.data + "'");
    advance();
    Register tblock = makeBlock();
    Register fblock;
    test(tblock);
    fblock.ok(nullptr);
    if (equal("else") && equal(TT_KEY)) {
        advance();
        fblock = makeBlock();
        test(fblock);
    }
    Position end = fblock.result ? fblock.result->end : tblock.result->end;
    res.ok(new If(res.result, tblock.result, fblock.result, begin, end));
    return res;
}

Register Parser::makeBlock() {
    Position begin = posBegin();
    std::vector<AST*> codes;
    Register res;
    if (equal("{") && equal(TT_OP)) {
        advance(SYN_VALUE);
        while (current.kind != TT_EOF && !(equal("}") && equal(TT_OP))) {
            auto tmp = makeStmt();
            test(tmp);
            codes.push_back(tmp.result);
        }
        Position end = posEnd();
        setError(res, equal("}") && equal(TT_OP), "SyntaxError: '{' is not close");
        advance(SYN_VALUE);
        res.ok(new Block(codes, begin, end));
    } else {
        auto tmp = makeStmt();
        test(tmp);
        codes.push_back(tmp.result);
        res.ok(new Block(codes, begin, tmp.result->end));
    }
    return res;
}

Register Parser::makeType() {
    Register res;
    Position begin = posBegin();
    if (equal("[") && equal(TT_OP)) {
        advance(SYN_TYPE);
        res = makeType();
        test(res);
        Register size;
        Position end = posEnd();
        size.ok(new Number("-1", begin, end));
        if (equal(",") && equal(TT_OP)) {
            advance(SYN_TYPE);
            size = makeExpr();
            test(size);
        }
        end = posEnd();
        setError(res, equal("]") && equal(TT_OP), "SyntaxError: '[' not close");
        advance(SYN_TYPE);
        res.ok(new ArrayType(res.result, size.result, begin, end));
    } else if (equal("(") && equal(TT_OP)) {
        advance(SYN_TYPE);
        std::vector<AST*> argsTypes;
        while (current.kind != TT_EOF && !(equal(")") && equal(TT_OP))) {
            Register st = makeType();
            test(st);
            argsTypes.push_back(st.result);
            if (equal(")") && equal(TT_OP)) break;
            setError(st, equal(",") && equal(TT_OP), "SyntaxError: need a ')'");
            advance(SYN_TYPE);
        }
        Position end = posEnd();
        setError(res, equal(TT_OP) && equal(")"), "SyntaxError: '(' not close");
        advance(SYN_TYPE);
        setError(res, equal(TT_OP) && equal(":"), "SyntaxError: want a ':', but found '" + current.data + "'");
        advance(SYN_TYPE);
        Register retT = makeType();
        test(retT);
        res.ok(new FuncType(retT.result, argsTypes, {}, begin, end));
    } else if (equal(TT_ID)) {
        Register clid = makeMemberAccess(SYN_TYPE);
        test(clid);
        std::vector<AST*> args;
        bool isT = false;
        if (equal("<") && equal(TT_OP)) {
            isT = true;
            advance(SYN_TYPE);
            while (current.kind != TT_EOF && !(equal(">") && equal(TT_OP))) {
                auto t = makeType();
                test(t);
                args.push_back(t.result);
                if (equal(TT_OP) && equal(">")) break;
                setError(res, equal(TT_OP) && equal(","), "SyntaxError: want a ',', but found '" + current.data + "'");
                advance(SYN_TYPE);
            }
            setError(res, equal(">") && equal(TT_OP), "SyntaxError: '<' not close");
            advance(SYN_TYPE);
        }
        Position end = posEnd();
        if (isT) res.ok(new TemplateType(clid.result, args, begin, end));
        else res.ok(new NormalType(clid.result, begin, end));
    } else {
        setError(res, false, "SyntaxError: not a type '" + current.data + "'");
    }
    return res;
}

TRegister Parser::makeTemplate() {
    TRegister tmp;
    setError(tmp, equal("<") && equal(TT_OP), "SyntaxError: want a '<'");
    advance(SYN_TYPE);
    std::vector<AST*> temp;
    while (current.kind != TT_EOF && !(equal(">") && equal(TT_OP))) {
        Register t = makeType();
        if (!t.isSuc) return {t.error, posBegin(), posEnd()};
        temp.push_back(t.result);
        if (equal(">") && equal(TT_OP)) break;
        setError(tmp, equal(",") && equal(TT_OP), "SyntaxError: want a ','");
        advance();
    }
    setError(tmp, equal(">") && equal(TT_OP), "SyntaxError: want a '>'");
    advance(SYN_TYPE);
    return {temp};
}

Register Parser::makeVarDefGrp() {
    Position begin = posBegin();
    advance();
    Register res;
    std::vector<AST*> vars;
    while (current.kind != TT_EOF && !(equal(";") && equal(TT_OP))) {
        auto tmp = makeVarDefine();
        test(tmp);
        vars.push_back(tmp.result);
        if (equal(";") && equal(TT_OP)) break;
        setError(res, equal(TT_OP) && equal(","), "SyntaxError: want a ',', found '" + current.data + "'");
        advance();
    }
    Position end = posEnd();
    res.ok(new VarDefGrp(vars, begin, end));
    return res;
}

Register Parser::makeVarDefine() {
    Register res;
    Position begin = posBegin();
    setError(res, equal(TT_ID), "SyntaxError: want a id");
    std::string name = current.data;
    advance();
    setError(res, equal(":") && equal(TT_OP), "SyntaxError: want ':', found '" + current.data + "'");
    advance();
    Register type = makeType();
    test(type);
    Register init;
    init.ok(nullptr);
    if (equal("=")) {
        advance();
        init = makeExpr();
        test(init);
    }
    Position end = init.result ? init.result->end : type.result->end;
    res.ok(new VarDef(name, type.result, init.result, begin, end));
    return res;
}

Register Parser::makeFor() {
    Position begin = posBegin();
    Register init, cond, chang, body;

    advance();
    setError(init, equal("(") && equal(TT_OP), "SyntaxError: want a '('");
    advance();
    init = makeForInit();
    test(init);
    setError(init, equal(";") && equal(TT_OP), "SyntaxError: want a ';'");
    advance();
    cond = makeExpr();
    test(cond);
    setError(init, equal(";") && equal(TT_OP), "SyntaxError: want a ';'");
    advance();
    chang = makeForChange();
    test(chang);
    setError(init, equal(")") && equal(TT_OP), "SyntaxError: want a ')'");
    advance();
    body = makeBlock();
    test(body);
    Position end = body.result->end;
    return {new ForLoop(init.result, cond.result, chang.result, body.result, begin, end)};
}

Register Parser::makeForInit() {
    std::vector<AST*> res;
    Position begin = posBegin();
    while (current.kind != TT_EOF && !(equal(TT_OP) && equal(";"))) {
        if (equal(TT_KEY) && equal("let")) {
            Register temp = makeVarDefGrp();
            test(temp);
            res.push_back(temp.result);
            setError(temp, equal(";") && equal(TT_OP), "SyntaxError: want a ';'");
            break;
        }
        Register tmp = makeVarDefine();
        test(tmp);
        res.push_back(tmp.result);
        if (equal(";") && equal(TT_OP)) break;
        setError(tmp, equal(",") && equal(TT_OP), "SyntaxError: want a ','");
        advance();
    }
    Position end = posEnd();
    Register r;
    r.ok(new Block(res, begin, end));
    return r;
}

Register Parser::makeForChange() {
    std::vector<AST*> res;
    Position begin = posBegin();
    while (current.kind != TT_EOF && !(equal(TT_OP) && equal(")"))) {
        auto tmp = makeExpr();
        test(tmp);
        res.push_back(tmp.result);
        if (equal(")") && equal(TT_OP)) break;
        setError(tmp, equal(TT_OP) && equal(","), "SyntaxError: want a ','");
        advance();
    }
    Position end = posEnd();
    return {new Block(res, begin, end)};
}

Register Parser::makeWhile() {
    Position begin = posBegin();
    advance();
    Register res;
    setError(res, equal("(") && equal(TT_OP), "SyntaxError: want a '(', found '" + current.data + "'");
    advance();
    Register cond = makeExpr();
    test(cond);
    setError(res, equal(")") && equal(TT_OP), "SyntaxError: want a ')', found '" + current.data + "'");
    advance();
    Register body = makeBlock();
    test(body);
    Position end = body.result->end;
    res.ok(new WhileLoop(cond.result, body.result, begin, end));
    return res;
}

Register Parser::makeDoWhile() {
    Position begin = posBegin();
    advance();
    Register body = makeBlock();
    test(body);
    setError(body, equal("while") && equal(TT_KEY), "SyntaxError: want a 'while'");
    advance();
    setError(body, equal("(") && equal(TT_OP), "SyntaxError: want a '('");
    advance();
    Register cond = makeExpr();
    test(cond);
    setError(body, equal(")") && equal(TT_OP), "SyntaxError: want a ')'");
    Position end = posEnd();
    advance();
    return {new DoWhile(cond.result, body.result, begin, end)};
}

Register Parser::makeCase() {
    Register res;
    Position begin = posBegin();
    if (equal("case") && equal(TT_KEY)) {
        advance();
        Register tmp = makeExpr();
        test(tmp);
        setError(tmp, equal(":") && equal(TT_OP), "SyntaxError: want a ':', found '" + current.data + "'");
        advance();
        Register body = makeBlock();
        test(body);
        Position end = body.result->end;
        res.ok(new Case(tmp.result, body.result, begin, end));
    } else if (equal("default") && equal(TT_KEY)) {
        advance();
        setError(res, equal(":") && equal(TT_OP), "SyntaxError: want a ':', found '" + current.data + "'");
        advance();
        Register tmp = makeBlock();
        test(tmp);
        Position end = tmp.result->end;
        res.ok(new Case(nullptr, tmp.result, begin, end));
    } else {
        setError(res, false, "SyntaxError: unknown key '" + current.data + "'");
    }
    return res;
}

Register Parser::makeFunctionTag(AccessType at) {
    Position begin = posBegin();
    advance();
    Register res;
    setError(res, equal(TT_ID), "SyntaxError: want a id");
    std::string name = current.data;
    advance();

    std::vector<std::string> templates;
    if (equal("<") && equal(TT_OP)) {
        advance(SYN_TYPE);
        while (current.kind != TT_EOF && !(equal(">") && equal(TT_OP))) {
            setError(res, equal(TT_ID), "SyntaxError: want a id node");
            templates.push_back(current.data);
            advance(SYN_TYPE);
            setError(res, equal(",") && equal(TT_OP), "SyntaxError: want a ','");
            advance();
        }
        setError(res, equal(">") && equal(TT_OP), "SyntaxError: want a '>'");
        advance();
    }

    setError(res, equal(TT_OP) && equal("("), "SyntaxError: want a '('");
    advance();
    std::vector<AST*> types;
    Register retType;
    while (current.kind != TT_EOF && !(equal(TT_OP) && equal(")"))) {
        Register tmp = makeType();
        test(tmp);
        types.push_back(tmp.result);
        if (equal(")") && equal(TT_OP)) break;
        setError(tmp, equal(",") && equal(TT_OP), "SyntaxError: want a ','");
        advance();
    }
    setError(res, equal(TT_OP) && equal(")"), "SyntaxError: want a ')'");
    advance();
    setError(res, equal(TT_OP) && equal(":"), "SyntaxError: want a ':'");
    advance();

    retType = makeType();
    test(retType);
    Position end = retType.result->end;
    setError(res, equal(TT_OP) && equal(";"), "SyntaxError: want a ';'");
    advance();
    res.ok(new Interface::FunctionTag(name, new FuncType(retType.result, types, templates, begin, end), at, begin, end));
    return res;
}

Register Parser::makeInterface() {
    Position begin = posBegin();
    advance();
    Register res;
    setError(res, equal(TT_ID), "SyntaxError: want a id");
    std::string name = current.data;
    std::vector<AST*> funcs;
    advance();

    AccessType at = APRIVATE;
    setError(res, equal("{") && equal(TT_OP), "SyntaxError: want a '{'");
    advance();
    while (current.kind != TT_EOF && !(equal(TT_OP) && equal("}"))) {
        if (equal(TT_KEY) && equal("public")) { at = APUBLIC; advance(); }
        else if (equal(TT_KEY) && equal("private")) { at = APRIVATE; advance(); }
        else if (equal(TT_KEY) && equal("protected")) { at = APROTECTED; advance(); }
        Register tmp = makeFunctionTag(at);
        test(tmp);
        funcs.push_back(tmp.result);
        at = APRIVATE;
    }
    Position end = posEnd();
    setError(res, equal("}") && equal(TT_OP), "SyntaxError: want a '}'");
    advance();
    res.ok(new Interface(name, funcs, begin, end));
    return res;
}

Register Parser::makeSwitch() {
    Position begin = posBegin();
    advance();
    Register res;
    setError(res, equal("(") && equal(TT_OP), "SyntaxError: want a '('");
    advance();
    res = makeExpr();
    test(res);
    setError(res, equal(")") && equal(TT_OP), "SyntaxError: want a ')'");
    advance();
    setError(res, equal("{") && equal(TT_OP), "SyntaxError: want a '{'");
    advance();
    std::vector<AST*> cases;
    while (current.kind != TT_EOF && !(equal(TT_OP) && equal("}"))) {
        auto tmp = makeCase();
        test(tmp);
        cases.push_back(tmp.result);
    }
    Position end = posEnd();
    setError(res, equal("}") && equal(TT_OP), "SyntaxError: want a '}'");
    advance();
    res.ok(new Switch(res.result, cases, begin, end));
    return res;
}

Register Parser::makeClass() {
    auto begin = posBegin();
    Register res;
    setError(res, equal("class") && equal(TT_KEY), "SyntaxError: want a 'class'");
    advance();
    std::string name, extend;
    std::vector<std::string> impls;
    std::vector<std::string> temp;
    setError(res, equal(TT_ID), "SyntaxError: want a id");
    name = current.data;
    advance();
    if (equal("<") && equal(TT_OP)) {
        advance(SYN_TYPE);
        while (current.kind != TT_EOF && !(equal(">") && equal(TT_OP))) {
            setError(res, equal(TT_ID), "SyntaxError: want a id");
            temp.push_back(current.data);
            advance();
            if (equal(">") && equal(TT_OP)) break;
            setError(res, equal(",") && equal(TT_ID), "SyntaxError: want a ','");
            advance();
        }
        setError(res, equal(">") && equal(TT_OP), "SyntaxError: want a '>'");
        advance();
    }

    if (equal("extend")) {
        advance();
        setError(res, equal(TT_ID), "SyntaxError: want a id");
        extend = current.data;
        advance();
    }
    if (equal("implement")) {
        advance();
        while (current.kind != TT_EOF && !(equal("{") && equal(TT_OP))) {
            setError(res, equal(TT_ID), "SyntaxError: want a id");
            impls.push_back(current.data);
            advance();
            if (equal("{") && equal(TT_OP)) break;
            setError(res, equal(",") && equal(TT_OP), "SyntaxError: want a ,");
            advance();
        }
    }

    std::vector<AST*> fields;
    std::vector<AST*> methods;
    setError(res, equal("{") && equal(TT_OP), "SyntaxError: want a '{'");
    advance();
    while (current.kind != TT_EOF && !(equal(TT_OP) && equal("}"))) {
        if (!(equal(TT_KEY) && equal("fn"))) {
            Register tmp = makeVarDefine();
            test(tmp);
            fields.push_back(tmp.result);
            setError(res, equal(";") && equal(TT_OP), "SyntaxError: want a ';'");
            advance();
        } else {
            Register tmp = makeFunction();
            test(tmp);
            methods.push_back(tmp.result);
        }
    }
    setError(res, equal("}") && equal(TT_OP), "SyntaxError: want a '}'");
    advance();
    auto end = posEnd();
    res.ok(new Class(name, fields, methods, extend, impls, temp, begin, end));
    return res;
}

Register Parser::makeNew() {
    std::string name;
    TRegister temps;
    std::vector<AST*> initArgs;

    Position begin = posBegin();
    Register res;
    setError(res, equal(TT_KEY) && equal("new"), "SyntaxError: want a 'new'");
    advance();

    setError(res, equal(TT_ID), "SyntaxError: want a id");
    name = current.data;
    advance();

    if (equal("<") && equal(TT_OP)) {
        temps = makeTemplate();
        if (!temps.isSuc)
            return {temps.error, posBegin(), posEnd()};
    }

    if (equal("(") && equal(TT_OP)) {
        advance();
        while (current.kind != TT_EOF && !(equal(")") && equal(TT_OP))) {
            auto tmp = makeExpr();
            test(tmp);
            initArgs.push_back(tmp.result);
            if (equal(")") && equal(TT_OP)) break;
            setError(res, equal(",") && equal(TT_OP), "SyntaxError: want a ','");
            advance();
        }
        setError(res, equal(")") && equal(TT_OP), "SyntaxError: want a ')'");
        advance();
    }

    auto end = posEnd();
    res.ok(new NewClass(name, initArgs, temps.result, begin, end));
    return res;
}