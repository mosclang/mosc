---
title: "Tii"
date: 2022-03-05T15:23:19Z
weight: 9
draft: false
bookToc: false
---

## **Tii Class**
`Tii` represents `Function` class in Mosc. It's first class function — an object that wraps an executable chunk of code. [Here](/docs/functions) is a friendly introduction.  

## **Static Methods**

### **kura(fun)**

Creates a new function from… `fun`. Of course, `fun` is already a function, so this really just returns the argument. It exists mainly to let you create a “bare” function when you don’t want to immediately pass it as a [block argument](/docs/functions#block-arguments) to some other method.

```mosc
nin fn = Tii.kura {
  A.yira("The body")
}
```

It is a runtime error if `fun` is not a function.

## **Methods**

### **arity**

The number of arguments the function requires.

```mosc
A.yira(Tii.kura {}.arity)             # > 0
A.yira(Tii.kura {(a, b, c) => a }.arity) # > 3
```

### **weele(args...)**

Invokes the function with the given arguments.

```mosc
nin fn = Tii.kura { (arg) =>
  A.yira(arg)     # > Hello world
}

fn.weele("Hello world")
```
It is a runtime error if the number of arguments given is less than the arity of the function. If more arguments are given than the function’s arity they are ignored.