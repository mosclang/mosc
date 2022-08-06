---
title: "Method call"
date: 2022-03-05T15:30:40Z
weight: 3
draft: false
---

## **Method Call**
In Mosc, method calls, usually, consist of an receiver object, followed by dot `.` , then the method name which may be followed by parentheses to receive arguments like in this:
```mosc
A.yira("Aww")

```
In below eexample
* `A`is the receiver
* `yira` is the method name
* `"Aww"` is the argument

Mutiple arguments are separated by comma `,` 

```mosc

walan.aDon(2, "molo")

```

The arguments list can be empty

```mosc

walan.diossi()

```
A method call is handled by the VM like this:
* Evaluate the receiver object and arguments from left to right
* Look up the method on the receiver class
* Invoke the method with first argument the receiver followed by the arguments values
## **Signature**
In Mosc, a class can have multiple methods with the same *name* as long as their have different number of arguments , A method signature includes its name and its arity(number of arguments it takes). This way you can overload a method.  


For instance, [Kunfe](/docs/modules/kunfe/kunfe/) class expose 2 methode `int` with different number of arguments

```mosc

nin kunfe = Kunfe.kura()

kunfe.int(3, 10)
kunfe.int(40)

```

In a language like Python or JavaScript, these would both call a single int() method, which has some kind of “optional” parameter. The body of the method figures out how many arguments were passed and uses control flow to handle the two different behaviors. That means first parameter represents “max unless another parameter was passed, in which case it’s min”.

This type of ‘variadic’ code isn’t ideal, so Mosc doesn’t encourage it.

In Mosc, these are calls to two entirely separate methods, int(_,_) and int(_). This makes it easier to define “overloads” like this since you don’t need optional parameters or any kind of control flow to handle the different cases.

It’s also faster to execute. Since we know how many arguments are passed at compile time, we can compile this to directly call the right method and avoid any “if I got two arguments do this…” runtime work.
## **Getter**

Some methods exist to expose a stored or computed property of an object. These are getters and have no parentheses:

```mosc
"seben".hakan # gives 5
(2..10).fitini # gives 2
[1, 2, 3].laKolon # gives galon

```

A getter is not the same as a method with an empty argument list. The () is part of the signature, so count and count() have different signatures. Unlike Ruby’s optional parentheses, Mosc wants to make sure you call a getter like a getter and a () method like a () method. These don’t work:

```mosc

"seben".hakan()
[1, 2, 3].djossi

```
If you’re defining some member that doesn’t need any parameters, you need to decide if it should be a getter or a method with an empty () parameter list. The general guidelines are:

* If it modifies the object or has some other side effect, make it a method:
```mosc

walan.diossi() # clear a list

```
* If the method supports multiple arities, make the zero-parameter case a () method to be consistent with the other versions:
```mosc

Djuru.mine()
Djuru.mine("Value")

```

* Otherwise, it can probably be a getter.

## **Setter**

A getter lets an object expose a public “property” that you can read. Likewise, a setter lets you write to a property:
```mosc

mogo.dianya = 120

```

Despite the =, this is just another syntax for a method call. From the language’s perspective, the above line is just a call to the dianya=(_) method on mogo, passing in 120.  
Since the =(_) is in the setter’s signature, an object can have both a getter and setter with the same name without a collision. Defining both lets you provide a read/write property.

## **Operators**

Mosc has most of the same operators you know and love with the same precedence and associativity. We have three prefix operators:

```mosc
! ~ -
```
They are just method calls on their operand without any other arguments. An expression like !possible means “call the ! method on possible”.

We also have a slew of infix operators—they have operands on both sides. They are:

```mosc

* / % + - .. ... << >> < <= > >= == != & ^ | ye

```

Like prefix operators, they are all funny ways of writing method calls. The left operand is the receiver, and the right operand gets passed to it. So a + b is semantically interpreted as “invoke the +(_) method on a, passing it b”.

Note that - is both a prefix and an infix operator. Since they have different signatures (- and -(_)), there’s no ambiguity between them.  


Most of these are probably familiar already. The .. and ... operators are “range” operators. The number type implements those to create [range](/docs/modules/core/funan/) objects, but they are method calls like other operators.  

The is keyword `ye` a “type test” operator. The base [Baa](/docs/modules/core/baa/) class implements it to tell if an object is an instance of a given class. You’ll rarely need to, but you can override `ye` in your own classes. That can be useful for things like mocks or proxies where you want an object to masquerade as a certain class.  

## **Indexing or Subscripts**

Another familiar syntax from math is Indexing using square brackets ([]). It’s handy for working with collection-like objects. For example:
```mosc

walan[0] # get the first element of walan
wala["togo"] # get the value associated to "togo" key in wala

```

You know the refrain by now. In Mosc, these are method calls. In the above examples, the signature is [_]. Index operators may also take multiple arguments, which is useful for things like multi-dimensional arrays:

```mosc

matrix[3, 5]

```


These examples are index “getters”, and there are also corresponding index setters:

```mosc

walan[0] = "item"
wala["togo"] = "Ali"

```

These are equivalent to method calls whose signature is [_]=(_) and whose arguments are both the subscript (or subscripts) and the value on the right-hand side.