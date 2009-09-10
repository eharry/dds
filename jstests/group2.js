t = db.group2;
t.drop();

t.save({a: 2});
t.save({b: 5});
t.save({a: 1});

result = t.group({key: {a: 1},
                  initial: {count: 0},
                  reduce: function(obj, prev) {
                      prev.count++;
                  }});

assert.eq(3, result.length, "A");
assert.eq(null, result[1].a, "C");
assert("a" in result[1], "D");
assert.eq(1, result[2].a, "E");

assert.eq(1, result[0].count, "F");
assert.eq(1, result[1].count, "G");
assert.eq(1, result[2].count, "H");
