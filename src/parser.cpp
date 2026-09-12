//
// Created by Lenovo on 2026/9/11.
//
#include <utility>

#include "../include/parser.h"

Register::Register() {
    isSuc = false;
}

Register::Register(std::string error, int lin, int col) {
    this->error = error;
    this->isSuc = false;
    this->lin = lin;
    this->col = col;
}

Register::Register(AST* result) {
    this->result = result;
    this->isSuc = true;
}

Parser::Parser() {
}

void Parser::saveState() {
    states.push_back(current);
    lexer.saveState();
}

void Parser::restore() {
    current = states.back();
    states.pop_back();
    lexer.restore();
}

Register Parser::parseExpr(std::string expr) {
    lexer.resetExpr(std::move(expr));
    advance(SYN_VALUE);
    return makeStmt();
}

void Parser::advance(int cs = SYN_VALUE) {
    current = lexer.getToken(cs);
}


std::vector<Register> Parser::parseCode(std::string expr) {
    lexer.resetExpr(std::move(expr));
    advance(SYN_VALUE);
    std::vector<Register> res;
    while (current.kind!=TT_EOF) {
        auto tmp = makeStmt();
        res.push_back(tmp);
        if (!tmp.isSuc) advance();
    }
    return res;
}

Register Parser::makeBinOpNode(MPCLBCK clb, std::vector<std::string> ops) {
    auto left = (this->*clb)();
    test(left);
    while (current.kind != TT_EOF && std::ranges::count(ops, current.data) > 0) {
        const std::string op = current.data;
        advance(SYN_VALUE);
        const auto right = (this->*clb)();
        test(right);
        left.result = new BinOpNode(op, left.result, right.result, current.lin, current.col);
    }
    return left;
}

Register Parser::makeNumberNode() {
    Register res(new Number(current.data, current.lin, current.col));
    advance(SYN_VALUE);
    return res;
}

Register Parser::makeString() {
    std::string str = current.data;
    advance(SYN_VALUE);
    std::vector<AST*> elements;
    for (auto i : str) elements.push_back(new Char(i, current.lin, current.col));
    Register res(new Array(elements, current.lin, current.col));
    return res;
}

Register Parser::makeChar() {
    Register res(new Char(current.data, current.lin, current.col));
    advance(SYN_VALUE);
    return res;
}

void Register::ok(AST* result) {
    this->result = result;
    this->isSuc = true;
}

void Register::fai(std::string error, int lin, int col) {
    this->error = error;
    this->isSuc = false;
    this->lin = lin;
    this->col = col;
}

Register Parser::makeValue() {
    if (equal(TT_INTEGER) || equal(TT_DOUBLE))
        return makeNumberNode();
    if (equal("++") || equal("--")) {
        std::string op = current.data;
        advance(SYN_VALUE);
        Register tmp = makeValue();
        test(tmp);
        return {new SelfChangeNode(tmp.result, op=="++", true, current.lin, current.col)};
    }
    if (equal(TT_STRING))
        return makeString();
    if (equal(TT_OP) && equal("-")) {
        advance(SYN_VALUE);
        auto tmp = makeValue();
        test(tmp);
        tmp.result = new Neg(tmp.result, current.lin, current.col);
        return tmp;
    }
    if (equal(TT_BOOL)) {
        auto tmp = current.data;
        advance(SYN_VALUE);
        return new Bool(tmp, current.lin, current.col);
    }
    if (equal(TT_NULL)) {
        advance(SYN_VALUE);
        return new Null(current.lin, current.col);
    }
    if (equal(TT_ID)) {
        auto tmp = makeId(SYN_VALUE);
        test(tmp);
        if (equal("++") || equal("--")) {
            std::string op = current.data;
            advance(SYN_VALUE);
            return {new SelfChangeNode(tmp.result, op=="++", false,
                                       tmp.result->lin, tmp.result->col)};
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
        return makeElementGetN(tmp.result, SYN_VALUE);
    }
    if (equal(TT_CHAR))
        return makeChar();
    Register res;
    setError(res, false, "SyntaxError: unknown operator symbol '" + current.data + "'");
}

Register Parser::makeArray() {
    Register res;
    advance(SYN_VALUE);
    std::vector<AST*> elements;
    if (!(equal("]") && equal(TT_OP))) {
        while (true) {
            auto elem = makeExpr();
            test(elem);
            elements.push_back(elem.result);
            if (equal("]") && equal(TT_OP)) break;
            setError(elem, equal(",") && equal(TT_OP),
                     "SyntaxError: want a ','");
            advance(SYN_VALUE);
        }
    }
    setError(res, equal("]") && equal(TT_OP),
             "SyntaxError: want a ']'");
    advance(SYN_VALUE);
    res.ok(new Array(elements, current.lin, current.col));
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
        first.ok(new AssignNode(tmp, first.result, t.result, current.lin, current.col));
    }
    return first;
}

Register Parser::makeExprA() {
    Register tmp = makeExpr_();
    test(tmp);
    if (!equal("?")) return tmp;
    advance(SYN_VALUE);
    Register tvalue = makeExpr();          
    test(tvalue);
    setError(tmp, equal(":") && equal(TT_OP),
             "SyntaxError: want a ':', found '" + current.data + "'");
    advance(SYN_VALUE);
    Register fvalue = makeExpr();        
    test(fvalue);
    tmp.ok(new ThreeOp(tmp.result, tvalue.result, fvalue.result, current.lin, current.col));
    return tmp;
}

bool Parser::equal(TokenKind kind) {
    return current.kind == kind;
}

bool Parser::equal(std::string data) {
    return current.data == data;
}

Register Parser::makeElementGetN(AST* name, int ptype) {
    Register res;
    res.ok(name);
    while (current.kind != TT_EOF && (equal("[") || equal("(") || equal(".")) && equal(TT_OP)) {
        if (equal("[")) {
            advance(SYN_VALUE);
            Register tmp = makeExpr();
            test(tmp);
            setError(tmp, equal("]") && equal(TT_OP), "SyntaxError: want a ']'");
            advance(SYN_VALUE);
            res.ok(new ElementGet(res.result, tmp.result, current.lin, current.col));
        } else if (equal("(")) {         
            auto tmp = makeCallNodeN(res.result, ptype);
            test(tmp);
            res.ok(tmp.result);          
        } else {
            auto tmp = makeMemberAccessN(res.result,ptype);
            test(tmp);
            res.ok(tmp.result);
        }
    }
    return res;
}

Register Parser::makeCallNodeN(AST* name, int ptype) {
    Register res;
    res.ok(name);
    while (current.kind != TT_EOF && (equal("(") || equal("[") || equal(".")) && equal(TT_OP)) {
        if (equal("(")) {
            advance(SYN_CALL);
            std::vector<AST*> args;
            while (current.kind != TT_EOF && !(equal(TT_OP)&&equal(")"))) {
                Register tmp = makeExpr();
                test(tmp);
                args.push_back(tmp.result);
                if (equal(")")&&equal(TT_OP)) break;
                setError(res, equal(",")&&equal(TT_OP), "SyntaxError: want a ','");
                advance(SYN_CALL);
            }
            setError(res, equal(TT_OP)&&equal(")"), "SyntaxError: want a ')'");
            advance(SYN_CALL);
            res.ok(new Call(res.result, args, current.lin, current.col));
        } else if (equal("[")) {
            auto tmp = makeElementGetN(res.result, ptype);
            test(tmp);
            res.ok(tmp.result);
        } else {
            auto tmp = makeMemberAccessN(res.result, SYN_VALUE);
            test(tmp);
            res.ok(tmp.result);
        }
    }
    return res;
}


Register Parser::makeId(int ptype) {
    auto tmp = makeMemberAccess(ptype);
    test(tmp);
    if (equal(TT_OP) && equal("("))
        return makeCallNodeN(tmp.result,ptype);
    if (equal(TT_OP) && equal("["))
        return makeElementGetN(tmp.result,ptype);
    return tmp;
}

Register Parser::makeElementGet(int ptype) {
    auto tmp = makeMemberAccess(ptype);
    test(tmp);
    return makeElementGetN(tmp.result, ptype);
}

Register Parser::makeCallNode() {
    auto tmp = makeMemberAccess(SYN_VALUE);
    test(tmp);
    return makeCallNodeN(tmp.result, SYN_VALUE);
}

Register Parser::makeMemberAccess(int ptype) {
    if (!equal(TT_ID))
        return {"SyntaxError: want a id node, but found '" + current.data + "'", current.lin, current.col};
    std::string tmp = current.data;
    advance(ptype);
    return makeMemberAccessN(new Id(tmp, current.lin, current.col), ptype);
}

Register Parser::makeMemberAccessN(AST* name, int ptype) {
    Register res;
    res.ok(name);
    while (current.kind != TT_EOF && equal(".")) {
        advance(ptype);
        setError(res, equal(TT_ID), "SyntaxError: want a id node, but found '" + current.data + "'");
        std::string tmp = current.data;
        advance(ptype);
        res.ok(new MemberAccess(res.result, tmp, current.lin, current.col));
    }
    return res;
}

Register Parser::makeFunction() {
    advance();
    Register body, retType;
    std::string funcName;
    bool isNative=false;
    if (equal(TT_KEY)&&equal("native")) {
        isNative=true;
        advance();
    }
    setError(body, equal(TT_ID), "SyntaxError: want a id, found '"+current.data+"'");
    funcName = current.data;
    advance();

    setError(body, equal("(") && equal(TT_OP), "SyntaxError: want a '(', found '"+current.data+"'");
    advance();

    std::vector<AST*> args;
    while (current.kind!=TT_EOF && !(equal(TT_OP)&&equal(")"))) {
        Register tmp = makeVarDefine();
        test(tmp);
        args.push_back(tmp.result);
        if (equal(TT_OP) && equal(")")) break;
        setError(tmp, equal(",")&&equal(TT_OP), "SyntaxError: want a ',', found '"+current.data+"'");
        advance();
    }
    setError(body, equal(")")&&equal(TT_OP), "SyntaxError: want a ')', found '"+current.data+"'");
    advance();

    setError(body, equal(":")&&equal(TT_OP), "SyntaxError: want a ':', found '"+current.data+"'");
    advance();
    retType = makeType();
    test(retType);
    if (!isNative) {
        body = makeBlock();
        test(body);
    } else {
        body.ok(nullptr);
        setError(body, equal(";")&&equal(TT_OP), "SyntaxError: need a ';'");
        advance();
    }
    std::vector<AST*> argTypes;
    for (auto i : args) argTypes.push_back(((VarDef*)i)->type);
    return {new Func(funcName, body.result, args, new FuncType(retType.result, argTypes, current.lin,current.col), isNative, current.lin, current.col)};
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
    if (equal(TT_KEY) && equal("if"))
        return makeIf();
    if (equal(TT_OP) && equal("{"))
        return makeBlock();
    if (equal(TT_KEY) && equal("break")) {
        advance(SYN_VALUE);
        Register res;
        setError(res, equal(";")&&equal(TT_OP), "SyntaxError: want a ';', found '" + current.data + "'");
        advance(SYN_VALUE);
        res.ok(new Break(current.lin, current.col));
        return res;
    }
    if (equal(TT_KEY) && equal("continue")) {
        advance(SYN_VALUE);
        Register res;
        setError(res, equal(";")&&equal(TT_OP), "SyntaxError: want a ';', found '" + current.data + "'");
        advance(SYN_VALUE);
        res.ok(new Continue(current.lin, current.col));
        return res;
    }
    if (equal(TT_KEY) && equal("let")) {
        auto tmp = makeVarDefGrp();
        setError(tmp, equal(TT_OP)&&equal(";"), "SyntaxError: want a ';', found '" + current.data + "'");
        advance(SYN_VALUE);
        return tmp;
    }
    if (equal(TT_KEY) && equal("return")) {
        advance(SYN_VALUE);
        if (equal(";")) {
            advance(SYN_VALUE);
            return Register(new Return(new Null(current.lin, current.col), current.lin, current.col));
        }
        Register tmp = makeExpr();
        test(tmp);
        setError(tmp, equal(TT_OP)&&equal(";"), "SyntaxError: want a ';', found '" + current.data + "'");
        advance(SYN_VALUE);
        return Register(new Return(tmp.result, current.lin, current.col));
    }
    if (equal(TT_KEY) && equal("goto")) {
        advance(SYN_VALUE);
        Register tmp;
        setError(tmp, equal(TT_ID), "SyntaxError: want a id, found '" + current.data + "'");
        std::string label = current.data;
        advance(SYN_VALUE);
        setError(tmp, equal(TT_OP)&&equal(";"), "SyntaxError: want a ';', found '" + current.data + "'");
        advance(SYN_VALUE);
        return Register(new Goto(label, current.lin, current.col));
    }
    Register tmp = makeExpr();
    setError(tmp, equal(TT_OP)&&equal(";"), "SyntaxError: want a ';', found '" + current.data + "'");
    advance(SYN_VALUE);
    return tmp;
}

Register Parser::makeIf() {
    Register res;
    advance();
    setError(res, equal("(")&&equal(TT_OP), "SyntaxError: want a '(', but found '"+current.data+"'");
    advance();
    res = makeExpr();
    test(res);
    setError(res, equal(")")&&equal(TT_OP), "SyntaxError: want a ')', but found '"+current.data+"'");
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
    res.ok(new If(res.result, tblock.result, fblock.result, current.lin, current.col));
    return res;
}

Register Parser::makeBlock() {
    std::vector<AST*> codes;
    Register res;
    if (equal("{") && equal(TT_OP)) {
        advance(SYN_VALUE);
        while (current.kind != TT_EOF && !(equal("}") && equal(TT_OP))) {
            auto tmp = makeStmt();
            test(tmp);
            codes.push_back(tmp.result);
        }
        setError(res, equal("}")&&equal(TT_OP), "SyntaxError: '{' is not close");
        advance(SYN_VALUE);
    } else {
        auto tmp = makeStmt();
        test(tmp);
        codes.push_back(tmp.result);
    }
    res.ok(new Block(codes, current.lin, current.col));
    return res;
}


Register Parser::makeType() {
    Register res;
    if (equal("[")&&equal(TT_OP)) {
        advance(SYN_TYPE);
        res = makeType();
        Register size;
        size.ok(new Number("-1", current.lin, current.col));
        test(res);
        if (equal(",") && equal(TT_OP)) {
            advance(SYN_TYPE);
            size = makeExpr();
            test(size);
        }
        setError(res, equal("]")&&equal(TT_OP), "SyntaxError: '[' not close");
        advance(SYN_TYPE);
        res.ok(new ArrayType(res.result, size.result, current.lin, current.col));
    } else if (equal("(")&&equal(TT_OP)) {
        advance(SYN_TYPE);
        std::vector<AST*> argsTypes;
        while (current.kind != TT_EOF && !(equal(")") && equal(TT_OP))) {
            Register st = makeType();
            test(st);
            argsTypes.push_back(st.result);
            if (equal(")")&&equal(TT_OP)) break;
            setError(st, equal(",")&&equal(TT_OP), "SyntaxError: need a ')");
            advance(SYN_TYPE);
        }
        setError(res, equal(TT_OP)&&equal(")"), "SyntaxError: '(' not close");
        advance(SYN_TYPE);
        setError(res, equal(TT_OP)&&equal(":"), "SyntaxError: want a ':', but found '"+current.data+"'");
        advance(SYN_TYPE);
        Register retT = makeType();
        test(retT);
        res.ok(new FuncType(retT.result, argsTypes, current.lin, current.col));
    } else if (equal(TT_ID)) {
        Register clid = makeMemberAccess(SYN_TYPE);
        test(clid);
        std::vector<AST*> args;
        bool isT = false;
        if (equal("<")&&equal(TT_OP)) {
            isT = true;
            advance(SYN_TYPE);
            while (current.kind!=TT_EOF && !(equal(">")&&equal(TT_OP))) {
                auto t = makeType();
                test(t);
                args.push_back(t.result);
                if (equal(TT_OP)&&equal(">")) break;
                setError(res, equal(TT_OP)&&equal(","), "SyntaxError: want a ',', but found '"+current.data+"'");
                advance(SYN_TYPE);
            }
            setError(res, equal(">")&&equal(TT_OP), "SyntaxError: '<' not close");
            advance(SYN_TYPE);
        }
        if (isT) res.ok(new TemplateType(clid.result, args, current.lin, current.col));
        else res.ok(new NormalType(clid.result, current.lin, current.col));
    } else {
        setError(res, false, "SyntaxError: not a type '" + current.data+"'");
    }
    return res;
}

Register Parser::makeVarDefGrp() {
    advance();
    Register res;
    std::vector<AST*> vars;
    while (current.kind != TT_EOF && !(equal(";") && equal(TT_OP))) {
        auto tmp = makeVarDefine();
        test(tmp);
        vars.push_back(tmp.result);
        if (equal(";")&&equal(TT_OP)) break;
        setError(res, equal(TT_OP)&&equal(","), "SyntaxError: want a ',', found '"+current.data+"'");
        advance();
    }
    res.ok(new VarDefGrp(vars, current.lin, current.col));
    return res;
}

Register Parser::makeVarDefine() {
    Register res;
    setError(res, equal(TT_ID), "SyntaxError: want a id");
    std::string name = current.data;
    advance();
    setError(res, equal(":")&&equal(TT_OP), "SyntaxError: want ':', found '"+current.data+"'");
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
    res.ok(new VarDef(name, type.result, init.result, current.lin, current.col));
    return res;
}

Register Parser::makeFor() {
    Register init;
    Register cond;
    Register chang;
    Register body;

    advance();
    setError(init, equal("(")&&equal(TT_OP), "SyntaxError: want a '('");
    advance();
    init = makeForInit();
    test(init);
    setError(init, equal(";")&&equal(TT_OP), "SyntaxError: want a ';'");
    advance();
    cond = makeExpr();
    test(cond);
    setError(init, equal(";")&&equal(TT_OP), "SyntaxError: want a ';'");
    advance();
    chang = makeForChange();
    test(chang);
    setError(init, equal(")")&&equal(TT_OP), "SyntaxError: want a ')'");
    advance();
    body = makeBlock();
    test(body);
    return {new ForLoop(init.result, cond.result, chang.result, body.result, current.lin, current.col)};
}

Register Parser::makeForInit() {
    std::vector<AST*> res;
    while (current.kind != TT_EOF && !(equal(TT_OP)&&equal(";"))) {
        if (equal(TT_KEY)&&equal("let")) {
            Register temp = makeVarDefGrp();
            test(temp);
            res.push_back(temp.result);
            setError(temp, equal(";")&&equal(TT_OP), "SyntaxError: want a ';'");
            break;
        }
        Register tmp = makeVarDefine();
        test(tmp);
        res.push_back(tmp.result);
        if (equal(";") && equal(TT_OP)) break;
        setError(tmp, equal(",")&&equal(TT_OP), "SyntaxError: want a ','");
        advance();
    }
    Register r;
    r.ok(new Block(res, current.lin, current.col));
    return r;
}

Register Parser::makeForChange() {
    std::vector<AST*> res;
    while (current.kind != TT_EOF && !(equal(TT_OP) && equal(";"))) {
        auto tmp = makeExpr();
        test(tmp);
        res.push_back(tmp.result);
        if (equal(")")&&equal(TT_OP)) break;
        setError(tmp, equal(TT_OP)&&equal(","), "SyntaxError: want a ','");
        advance();
    }
    return {new Block(res, current.lin, current.col)};
}

Register Parser::makeWhile() {
    advance();
    Register res;
    setError(res, equal("(")&&equal(TT_OP), "SyntaxError: want a '(', found '"+current.data+"'");
    advance();
    Register cond = makeExpr();
    test(cond);
    setError(res, equal(")")&&equal(TT_OP), "SyntaxError: want a ')', found '"+current.data+"'");
    advance();
    Register body = makeBlock();
    test(body);
    res.ok(new WhileLoop(cond.result, body.result, current.lin, current.col));
    return res;
}

Register Parser::makeDoWhile() {
    advance();
    Register body = makeBlock();
    test(body);
    setError(body, equal("while")&&equal(TT_KEY), "SyntaxError: want a 'while'");
    advance();
    setError(body, equal("(")&&equal(TT_OP), "SyntaxError: want a '('");
    advance();
    Register cond = makeExpr();
    test(cond);
    setError(body, equal(")")&&equal(TT_OP), "SyntaxError: want a ')'");
    advance();
    return {new DoWhile(cond.result, body.result, current.lin, current.col)};
}


Register Parser::makeCase() {
    Register res;
    if (equal("case") && equal(TT_KEY)) {
        advance();
        Register tmp = makeExpr();
        test(tmp);
        setError(tmp, equal(":")&&equal(TT_OP), "SyntaxError: want a ':', found '" + current.data + "'");
        advance();
        Register body = makeBlock();
        test(body);
        res.ok(new Case(tmp.result, body.result, current.lin, current.col));
    } else if (equal("default") && equal(TT_KEY)) {
        advance();
        setError(res, equal(":")&&equal(TT_OP), "SyntaxError: want a ':', found '"+current.data+"'");
        advance();
        Register tmp = makeBlock();
        test(tmp);
        res.ok(new Case(nullptr, tmp.result, current.lin, current.col));
    } else {
        setError(res, false, "SyntaxError: unkonwn key '"+current.data+"'");
    }
    return res;
}

Register Parser::makeSwitch() {
    advance();
    Register res;
    setError(res, equal("(")&&equal(TT_OP), "SyntaxError: want a '('");
    advance();
    res = makeExpr();
    setError(res, equal(")")&&equal(TT_OP), "SyntaxError: want a ')'");
    advance();

    setError(res, equal("{")&&equal(TT_OP), "SyntaxError: want a '{'");
    advance();
    std::vector<AST*> cases;
    while (current.kind!=TT_EOF && !(equal(TT_OP)&&equal("}"))) {
        auto tmp = makeCase();
        test(tmp);
        cases.push_back(tmp.result);
    }
    setError(res, equal("}")&&equal(TT_OP), "SyntaxError: want a '}'");
    advance();
    res.ok(new Switch(res.result, cases, current.lin, current.col));
    return res;
}
