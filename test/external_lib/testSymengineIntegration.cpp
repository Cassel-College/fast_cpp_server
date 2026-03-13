#include <gtest/gtest.h>
// #include <symengine/expression.h>
// #include <symengine/symbol.h>

// using SymEngine::Expression;
// using SymEngine::symbol;

// TEST(SymEngineTest, CanCreateSymbolAndExpression) {
//     auto x = symbol("x");
//     auto expr = Expression(x) + 1;
    
//     EXPECT_EQ(expr.get_basic()->__str__(), "1 + x");
// }

// TEST(SymEngineTest, CanDifferentiate) {
//     auto x = symbol("x");
//     Expression expr = Expression(x) * Expression(x);  // x^2

//     auto deriv = expr.diff(x);  // 2*x
//     EXPECT_EQ(deriv.get_basic()->__str__(), "2*x");
// }
