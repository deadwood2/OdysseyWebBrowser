class A extends Array { }
Object.defineProperty(Array, Symbol.species, { value: A, configurable: true });

foo = [1,2,3,4];
result = foo.concat([1]);
if (!(result instanceof A))
    throw "concat failed";

result = foo.splice();
if (!(result instanceof A))
    throw "splice failed";

result = foo.slice();
if (!(result instanceof A))
    throw "slice failed";

Object.defineProperty(Array, Symbol.species, { value: Int32Array, configurable: true });

result = foo.concat([1]);
if (!(result instanceof Int32Array))
    throw "concat failed";

result = foo.splice();
if (!(result instanceof Int32Array))
    throw "splice failed";

result = foo.slice();
if (!(result instanceof Int32Array))
    throw "slice failed";
