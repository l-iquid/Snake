/*
strings are always wrapped with ""
 * Root
 *** ND_VarDeclStatement a
 ***** ND_EqualsExpression =
 ******* ND_StringLiteral "hi"

 *** ND_CallExpressionStatement "print"
 ***** ND_ArgumentListExpression 0
 ******* ND_IdentifierExpression a

 *** ND_VarReassignStatement a
 ***** ND_EqualsExpression =
 ******* ND_TypeCastExpression "string"
 ********* ND_NumberLiteral 3

 *** ND_CallExpressionStatement "print"
 ***** ND_ArgumentListExpression 0
 ******* ND_ArithmeticExpression "+"
 ********* ND_NumberLiteral 1
 ********* ND_ArithmeticExpression "*"
 *********** ND_NumberLiteral 3
 *********** ND_NumberLiteral 5

 *** ND_IfStatement 0
 ***** ND_ConditionalExpression ">="
 ******* ND_NumberLiteral 3
 ******* ND_NumberLiteral 2
 ***** ND_PassStatement 0

 *** ND_FunctionDefStatement "eval"
 ***** ND_ArgumentListExpression 0
 ******* ND_ExplicitArgumentExpression "one"
 ********* ND_TypeResolveExpression "i32"
 ******* ND_ExplicitArgumentExpression "two"
 ********* ND_TypeResolveExpression "u8"
 ***** ND_TypeResolveExpression "i32" // btw this is the return type
 ***** ND_ReturnStatement 0
 ******* ND_ArithmeticExpression "+"
 ********* ND_IdentifierExpression "one"
 ********* ND_IdentifierExpression "two"


a = "hi"
print(a)
a = string(3)
print(1 + 3 * 5)
def eval(one: i32, two: u8) -> i32:
    return one + two
end
if 3 >= 2:
    pass
end

def gen<T>(foo: float, bar: T) -> T:
    
end
gen<i32>(1.3f, 64)
*/