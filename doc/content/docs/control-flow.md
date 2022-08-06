---
title: "Control Flow"
date: 2022-03-05T15:30:40Z
weight: 7
draft: false
---

## **Control flows**

Control flows are usefull in Mosc, they allow jumping through code from one point to an other one. That allow executing a block of code if certain conditions are met or make a loop.  

## **Truth**
Any control is based on checking an expression, if it's evaluated to `tien` (true) we do somthing, else if it's evaluated to `galon`(false) we do an other thing.  
During expression evaluation, Mosc divide objects in two categories: the truth ones and the falsy ones. Of course `tien` is truth, and `galon` is falsy; Mosc considers all other object as truth, which means, contrarely in Javascript 0, "" are all truth

*  Boolean value `galon` is falsy 
*  Null value `gansan` is falsy 
*  everything else is truth. 


## **`Nii/Note` (if/else) statements**

Tell Mosc to execute a block of code based on some conditions is done using `nii`/`note` keywords. It conditions following `nii` are met (expression returns `tien`) then the block or staement following is executed, if not the statement or block following `note`, if any, is executed.  

```mosc

nin age = 26

nii (age < 26) {
    A.yira("Sii ka dogon niin 26 ye")
} note nii (age < 30) {
    A.yira("Sii ka kian niin 26 ye")
} note {
     A.yira("Sii man dogon niin 26 ye")
}

nii (age < 28) A.yira("Age < 28")
note nii (a > 40) A.yira("Age > 40")
note A.yira("None")

```

It's possible to omit the parentheses enclosing the condition expression, but you need to be carefull with this if you have a block instead of a statement, since you can fall on block argument case if the last token before the `{` is and identifier.

```mosc
nin age = 24

## no probleme as long as you have expression as nii consequent
nii age < 18 A.yira("Minor")


## this is valid
nii age < 25 {
    A.yira("A < 25")
}
## this fill fail to compile
## because `age {` will be treated as a block argument function call

nii 25 > age {
     A.yira("A < 25")
}

```

`nii/note` can be used as expression instead of statement, in this case the last expression value of the block is return as the whole expression value.

```mosc
nin a = 21
nin a = nii a > 20 {
    A.yira("a > 20")
    # can make some operation here
    a * 2 # will be the value of `nii` expression
} note {
    a # will be the value of `nii` expression
}

```

## **Logical operators**

Unlike others operators in Mosc which have exactly [methods call](/docs/method-call/) simulated behind , the `&&` and `||`  operators are specials since they only evaluate the right hand operand conditionnaly (they shor-circuit).

A `&&` ("logical and") expression evaluates the left-hand argument. If it’s false, it returns that value. Otherwise it evaluates and returns the right-hand argument.

```mosc

A.yira(false && 2) # gives false
A.yira(1 && 2)# gives 2

```

A `||` ("logical or") expression is reversed. If the left-hand argument is truth, it’s returned, otherwise the right-hand argument is evaluated and returned:

```mosc
A.yira(2 || gansan) # gives 2
A.yira(false || 4) # gives 2
A.yira(3 || 4) # gives 3

```

## **`foo`(while) statement**
You may be in need to repeat a certain operation untill some conditions are met, which make your code execute in loop. The loop block or statement continues till the loop condition hold `tien` as value

```mosc

nin n = 40
foo (a > 0) {
    A.yira(a)
    a = a - 1
}
nin obj = SomeClass.kura()
foo (obj.isTrue) A.yira(obj.update())
# foo obj.isTrue A.yira(obj.update())

```
You can omit the parentheses surrounding the condition, be carefull to not fall on block argument method call.


## **`seginka` (for) statement**

You may need to loop through a collections of object or range of numbers or an iterable object, this can be achieved through `seginka` statement. `Seginka` passes through a collection or range of numbers or an iterable object, and assigns each item to a variable.

```mosc

nin col = [1, 2, 3, 4, 5]
seginka (col kono item) {
    A.yira(item)
}

seginka (1..100 kono item) {
    A.yira(item)
}

```
A `seginka` loop has three components:

1. A variable name to bind. In the example above, that’s `item`. Mosc will create a new variable with that name whose scope is the body of the loop.

2. A sequence expression. This determines what you’re looping over. It gets evaluated once before the body of the loop. In this case, it’s variable `col` or range `1..100`, but it can be any expression.

3. A body. This is a curly block or a single statement. It gets executed once for each iteration of the loop.


Note that you can omit the  parentheses surrounding `seginka` operand.

You can specify the loop direction with `kay`(up or incremental) and `kaj`(down or decremental) keywords ; and the loop step with `niin` keyword followed by the step value. Note that negative step values change the loop direction implicitly. 

```mosc

nin col = [1, 2, 3, 4, 5]
seginka col kono item kaj {
    A.yira(item)
}

seginka 1..100 kono item kay niin 2 {
    A.yira(item)
}

```

## **`atike`(break) Statement**

`atike` statement allow you to make a break anywhere in a loop. 

```mosc
nin n = 100

foo(tien) {
    nii(n - 3 <  12) atike
    n = n - n * 0.001 + 1
}

```

## **`ipan`(continue) Statement**

You may need to skip a loop iteration, let's say you want to pass to the next iteration of a loop just right now withou the normal end of the current iteration. you can use `ipan` keyword.  

```mosc

seginka 1..100 kono i kay niin 2 {
    A.yira(i)
    nii i % 2 == 0 ipan
}

```


## **Numeric Range**

Its common to see list in `seginka` loop statement , but you can have a range of number you want to walk over or loop a numbers of times, and that's possible with [ranges](/docs/modules/core/funan/). Ranges are created using `..` infix operator on `Diat` class.


```mosc

seginka 1..100 kono i {
    A.yira(i)
}

```
In below example the loop walks from 1 to 100 included. You may want to exclude the range uper bound, to do so you can create an exclusive range using `...` operator.

```mosc

seginka 1...100 kono i {
    A.yira(i)
}

```

## **Map iteration**

Like `Walan`(list) instances you can iterate over `Wala`(map) instances using `seginka` keyword the same way.

```mosc

seginka {"k1": "value1", "k2": "Value2"} kono keyValue {
    A.yira("${keyValue.key} to ${keyValue.value}")
}

```

## **Loop variable destructuration**

Since `seginka` loop create a variable to store the loop iteration value, you can use destructuration on that variable. 

```mosc

seginka [{"name": "Molo", "age": 12}, {"name": "Issa", "age": 14}, ...] kono {name, age} {
    A.yira("${name} is ${name} years old")
}

seginka [[1, 2, 3], [4, 4, 5], [5, 3, 1]] kono [v1, v2] {
    A.yira("[${v1}, ${v2}]")
}

```

## **The iterator protocol**

The semantic of loop is writen in iterator pattern, that way, like `Walan`, `Wala` and `Funan` which are iterable, you can define your own class as iterable, so you can your its instances in `seginka` statement. The loop, in it-self, have no idea about the object to iterate, it just knows about two particular methods to call on this object to evaluate the sequence expression.

```mosc

seginka [1, 2, 3, 4] kono a {
    A.yira(a)
} 

```

`Walan`, `Wala`, `Funan`, all implement this protocol, you can do the same for your custom classes.


Mosc evaluate the above code like this

```mosc

nin seq_ = [1, 2, 3, 4]
nin iter_ = gansan
nin step_ = 1

foo (iter_ = seq_.iterate(iter_, step_)) {
    nin a = seq_.iteratorValue(iter_);
    A.yira(a)
}

```

## **`tumamin`(when) expression**

You me need to chain a lot if statement on the same expression, to make you write compact and less code, Mosc has the `tumanin` keyword for that. If you are familiar with C like langage, this is equivalent to `switch case` statement.

Note that `tumamin` always avaluate to an expression. `tumamin` take an expression or an assignment statement and check its value to match branches, once a branch matches, the others are ignored.

```mosc

nin a = 24
tumamin(a) {

    > 20 => A.yira("a > 20")
    < 10 => A.yira("a > 10")
    ye Seben => A.yira("ye Seben ye")
    == 3 => A.yira(" == 3")
    24 => A.yira(" == 24")
    "molo" => {
        A.yira(" == molo")
    }
    note => A.yira("no matches")

}

```

A `tumamin` expression has following components:

1. The expressions or assignment surrounded with parentheses that can be ignore in some case. Expresison `a` in the example above
2. Branches consisting of:
    * Check part which can be an expression or a constant or semi-expression. Semi-expression allow you implicitly insert the value evaluated in `tumamin` expression as part of an expression.
    * `=>` separator
    * The statement or expression to evaluate if matches
3. `note` handler matches if no branch macthed above.


As `tumamin` is an expression you can do 

```mosc
nin b = tumamin(nin a = 18) {
    < 20  => a * 2
    > 20 => a * 1.3
    18 => a
    note => 0
}

A.yira(b)


```