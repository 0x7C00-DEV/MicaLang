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
    lexer.resetExpr(expr);
    advance(SYN_VALUE);
    
    return makeExpr();
}

void Parser::advance(int cs = SYN_VALUE) {
    current = lexer.getToken(cs);
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
        auto tmp = makeId();
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
        return makeElementGetN(tmp.result);
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

Register Parser::makeElementGetN(AST* name) {
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
            auto tmp = makeCallNodeN(res.result);
            test(tmp);
            res.ok(tmp.result);          
        } else {
            auto tmp = makeMemberAccessN(res.result);
            test(tmp);
            res.ok(tmp.result);
        }
    }
    return res;
}

Register Parser::makeCallNodeN(AST* name) {
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
            auto tmp = makeElementGetN(res.result);
            test(tmp);
            res.ok(tmp.result);
        } else {
            auto tmp = makeMemberAccessN(res.result);
            test(tmp);
            res.ok(tmp.result);
        }
    }
    return res;
}


Register Parser::makeId() {
    auto tmp = makeMemberAccess();
    test(tmp);
    if (equal(TT_OP) && equal("("))
        return makeCallNodeN(tmp.result);
    if (equal(TT_OP) && equal("["))
        return makeElementGetN(tmp.result);
    return tmp;
}

Register Parser::makeElementGet() {
    auto tmp = makeMemberAccess();
    test(tmp);
    return makeElementGetN(tmp.result);
}

Register Parser::makeCallNode() {
    auto tmp = makeMemberAccess();
    test(tmp);
    return makeCallNodeN(tmp.result);
}

Register Parser::makeMemberAccess() {
    if (!equal(TT_ID))
        return {"SyntaxError: want a id node, but found '" + current.data + "'", current.lin, current.col};
    std::string tmp = current.data;
    advance(SYN_VALUE);
    return makeMemberAccessN(new Id(tmp, current.lin, current.col));
}

Register Parser::makeMemberAccessN(AST* name) {
    Register res;
    res.ok(name);
    while (current.kind != TT_EOF && equal(".")) {
        advance(SYN_VALUE);
        setError(res, equal(TT_ID), "SyntaxError: want a id node, but found '" + current.data + "'");
        std::string tmp = current.data;
        advance(SYN_VALUE);
        res.ok(new MemberAccess(res.result, tmp, current.lin, current.col));
    }
    return res;
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
        Register clid = makeMemberAccess();
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
    advance();

    std::vector<AST*> init;
    AST* condition = nullptr;
    std::vector<AST*> change;
    AST* body = nullptr;
}

Register Parser::makeWhile() {}

Register Parser::makeDoWhile() {}

Register Parser::makeSwitch() {}

Register Parser::makeGoto() {}

Register Parser::makeReturn() {}
