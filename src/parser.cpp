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

void Parser::advance(int cs) {
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
        left.result = new BinOpNode(op, left.result, right.result);
    }
    return left;
}

Register Parser::makeNumberNode() {
    Register res(new Number(current.data));
    advance(SYN_VALUE);
    return res;
}

Register Parser::makeString() {
    std::string str = current.data;
    advance(SYN_VALUE);
    std::vector<AST*> elements;
    for (auto i : str) elements.push_back(new Char(i));
    Register res(new Array(elements));
    return res;
}

Register Parser::makeChar() {
    Register res(new Char(current.data));
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
    if (equal(TT_STRING))
        return makeString();
    if (equal(TT_OP) && equal("-")) {
        advance(SYN_VALUE);
        auto tmp = makeValue();
        test(tmp);
        tmp.result = new Neg(tmp.result);
        return tmp;
    }
    if (equal(TT_ID))
        return makeId();
    if (equal(TT_OP) && equal("["))
        return makeArray();
    if (equal(TT_OP) && equal("(")) {
        advance(SYN_VALUE);
        auto tmp = makeExpr();
        test(tmp);
        setError(tmp, equal(")"), "SyntaxError: Want a ')'");
        advance(SYN_VALUE);
        return tmp;
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
    res.ok(new Array(elements));
    return res;
}

Register Parser::makeFactor() {
    return makeBinOpNode(&Parser::makeValue, {"*", "/", "<<", ">>", "%", "|", "&", "^"});
}

Register Parser::makeTerm() {
    return makeBinOpNode(&Parser::makeFactor, {"+", "-"});
} 

Register Parser::makeExpr1() {
    return makeBinOpNode(&Parser::makeTerm, {"==", "!=", ">", "<", "<=", ">="});
}

Register Parser::makeExpr() {
    return makeBinOpNode(&Parser::makeExpr1, {"&&", "||"});
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
            setError(tmp, equal("]")&&equal(TT_OP), "SyntaxError: want a ']'");
            res.ok(new ElementGet(res.result, tmp.result));
            advance(SYN_VALUE);
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
            res.ok(new Call(res.result, args));
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
    return makeMemberAccessN(new Id(tmp));
}

Register Parser::makeMemberAccessN(AST* name) {
    Register res;
    res.ok(name);
    while (current.kind != TT_EOF && equal(".")) {
        advance(SYN_VALUE);
        setError(res, equal(TT_ID), "SyntaxError: want a id node, but found '" + current.data + "'");
        std::string tmp = current.data;
        advance(SYN_VALUE);
        res.ok(new MemberAccess(res.result, tmp));
    }
    return res;
}