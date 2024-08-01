//@ skip
function testSyntax(script) {
    try {
        eval(script);
    } catch (error) {
        if (error instanceof SyntaxError)
            throw new Error("Bad error: " + String(error));
    }
}

function testSyntaxError(script, message) {
    var error = null;
    try {
        eval(script);
    } catch (e) {
        error = e;
    }
    if (!error)
        throw new Error("Expected syntax error not thrown");

    if (String(error) !== message)
        throw new Error("Bad error: " + String(error));
}

testSyntaxError(`
function *t1() {
    var yield = 20;
}
`, `SyntaxError: Cannot use the keyword 'yield' as a variable name.`);
testSyntaxError(`
function *t1() {
    let yield = 20;
}
`, `SyntaxError: Cannot use the keyword 'yield' as a variable name.`);
testSyntaxError(`
function *t1() {
    const yield = 20;
}
`, `SyntaxError: Cannot use the keyword 'yield' as a variable name.`);

testSyntaxError(`
function *t1() {
    var { yield } = 20;
}
`, `SyntaxError: Cannot use abbreviated destructuring syntax for keyword 'yield'.`);
testSyntaxError(`
function *t1() {
    let { yield } = 20;
}
`, `SyntaxError: Cannot use abbreviated destructuring syntax for keyword 'yield'.`);
testSyntaxError(`
function *t1() {
    const { yield } = 20;
}
`, `SyntaxError: Cannot use abbreviated destructuring syntax for keyword 'yield'.`);

testSyntaxError(`
function *t1() {
    var { i: yield } = 20;
}
`, `SyntaxError: Cannot use the keyword 'yield' as a variable name.`);
testSyntaxError(`
function *t1() {
    let { i: yield } = 20;
}
`, `SyntaxError: Cannot use the keyword 'yield' as a variable name.`);
testSyntaxError(`
function *t1() {
    const { i: yield } = 20;
}
`, `SyntaxError: Cannot use the keyword 'yield' as a variable name.`);

testSyntaxError(`
function *t1() {
    var [ yield ] = 20;
}
`, `SyntaxError: Cannot use the keyword 'yield' as a variable name.`);
testSyntaxError(`
function *t1() {
    let [ yield ] = 20;
}
`, `SyntaxError: Cannot use the keyword 'yield' as a variable name.`);
testSyntaxError(`
function *t1() {
    const [ yield ] = 20;
}
`, `SyntaxError: Cannot use the keyword 'yield' as a variable name.`);

testSyntaxError(`
function *t1() {
    function yield() { }
}
`, `SyntaxError: Cannot use the keyword 'yield' as a function name.`);
testSyntax(`
function t1() {
    function *yield() {
    }
}
`);

testSyntaxError(`
function *t1() {
    try {
    } catch (yield) {
    }
}
`, `SyntaxError: Cannot use the keyword 'yield' as a catch variable name.`);

testSyntax(`
function *t1() {
    (function yield() {})
}
`);
