---
title: "Variables"
date: 2022-03-05T15:29:46Z
weight: 6
draft: false
---


## **Variable declaration**

In Mosc, variables are named slot that can have a value. Their are declared using `nin` keyword, followed by their names which must be an identifier. The declaration can includes an initializer with is place after the `=` operator  
```mosc
nin v1
nin v2 = 12 + 33
```
In above example , we create a variable `v1` and variable `v2` and initialise `v2` by the expression following the `=` sign evaluation value. Those variables are created in the current scope. 

## **Scope**

A scope determines variables limit - till where they go! Mosc has a true block scope: a variable, when declared inside a block, lives from its declaration to the end of the block. 

```mosc
{
    A.yira(a) # gives an error, "a" doesnt exist yet
    nin a = "Ahh"
    A.yira(a) # Gives: Ahh
}
A.yira(a) # "a" doesn't exist anymore
```

Variables declared at the top level of a script are top-level and are visible to the [module](/docs/modularity/) system. Any other variable is local. If you decalre a with in an inner-scope with the same name as an outer one, the outer-one is shadowed and that is not considered as an error.  

```mosc

nin a = "outer"
{
  nin a = "inner"
  A.yira(a) # gives inner
}
A.yira(a) # gives outer

```

In the same scrope you can decalare more than one variable with the same name. 
```mosc
nin a = "AA"
nin a = "AS" # gives an error

```

## **Destructuration**

When declaring a variable with an in-place initialization, if the initializer expression evaluate to a `Walan` or `Wala` instance, you can use the destructuration technique to pick a specifiqu value directly instead of the whole value of the initializer.  Let's say you have a list, and you wan to declare two variables and , respectively, initialize them with the first and second items of the list you may write like this without destructuration:

```mosc
nin wala = [1, 2, 3, 4]
nin var1 = wala[0]
nin var2 = wala[1]

```
Using destructuration, you write instead

```mosc

nin wala = [1, 2, 3, 4]
nin [var1, var2] = [1, 2, 3, 4]
A.yira(var1) # gives: 1
A.yira(var2) # gives: 2

```

The same way you can destructure a map (`Wala` instance). 

```mosc
nin user = {"togo": "Molo", "diamou": "DOUMBIA", "sii": 26}
nin { togo, diamou } = user;
A.yira(togo) # gives: Molo
A.yira(diamou) # gives: DOUMBIA

```

You can use the rest operator `...` to store the remainig values of destructuration. 

```mosc

nin wala = [1, 2, 3, 4]
nin [var1, var2, ...rest] = [1, 2, 3, 4]
A.yira(rest) # gives: [3, 4]

nin user = {"togo": "Molo", "diamou": "DOUMBIA", "sii": 26}
nin { togo, ...other  } = user;
A.yira(other) # gives: { "diamou": "DOUMBIA", "sii": 26}

```

The destructuraton is available only in in-place variable initialization, not in assignation.   

## **Assignment**

After a variable has been declared, you can change its value by assigning to it a new value through `=` operator.  

```mosc

nin var = "molo"
var = "Droid"

```

Since an assignment walks up the scope stack to find where the named variable is declared, so it’s an error to assign to a variable that isn’t declared. Mosc doesn’t roll with implicit variable definition.  


When used in a larger expression, an assignment expression evaluates to the assigned value.  

```mosc
nin var = "molo"
A.yira(var = "Droid") # gives: Droid

```

If the left-hand side is some more complex expression than a bare variable name, then it isn’t an assignment. Instead, it’s calling a [setter method](/docs/method-calls#setter).


