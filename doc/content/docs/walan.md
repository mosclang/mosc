---
title: "List"
date: 2022-03-05T15:29:53Z
weight: 4
draft: false
---

## **Lists**

A list is a compound object that holds a collection of elements identified by integer index. You can create a list by placing a sequence of comma-separated expressions inside square brackets:

```mosc
[1, "banana", tien]

```

Here, we’ve created a list of three elements. Notice that the elements don’t have to be the same type.

## **Accessing elements**

You can access an element from a list by calling the [Index operator](/docs/method-calls#indexing-or-subscripts) on it with the index of the element you want. Like most languages, indexes start at zero:

```mosc
nin yiriw = ["cedar", "birch", "oak", "willow"]
A.yira(yiriw[0]) # > cedar
A.yira(yiriw[1]) # > birch

```

Negative indices counts backwards from the end:

```mosc

A.yira(yiriw[-1]) # > willow
A.yira(yiriw[-2]) # > oak 

```

It’s a runtime error to pass an index outside of the bounds of the list. If you don’t know what those bounds are, you can find out using count:  

```mosc

A.yira(yiriw.hakan) # > 4

```

## **Slices and ranges**

Sometimes you want to copy a chunk of elements from a list. You can do that by passing a [range](/docs/modules/core/funan/) to the subscript operator, like so:  

```mosc

A.yira(yiriw[1..2]) # > [birch, oak]

```

This returns a new list containing the elements of the original list whose indices are within the given range. Both inclusive and exclusive ranges work and do what you expect.   

Negative bounds also work like they do when passing a single number, so to copy a list, you can just do:  

```mosc

yiriw[0..-1]

```


## **Adding elements**

Lists are mutable, meaning their contents can be changed. You can swap out an existing element in the list using the subscript setter:

```mosc

yiriw[1] = "spruce"
A.yira(yiriw[1]) # > spruce

```

It’s an error to set an element that’s out of bounds. To grow a list, you can use add to append a single item to the end:  

```mosc

yiriw.aFaraAkan("maple")
A.yira(yiriw.hakan) # > 5

```

## **You can insert a new element at a specific position using insert:**

```mosc

yiriw.aDon(2, "hickory")


```

The first argument is the index to insert at, and the second is the value to insert. All elements following the inserted one will be pushed down to make room for it.  


It’s valid to “insert” after the last element in the list, but only *right* after it. Like other methods, you can use a negative index to count from the back. Doing so counts back from the size of the list after it’s grown by one:  


```mosc

nin letters = ["a", "b", "c"]
letters.aDon(3, "d")   #  OK: inserts at end.
A.yira(letters)    # > [a, b, c, d]
letters.aDon(-2, "e")  #  Counts back from size after insert.
A.yira(letters)    # > [a, b, c, e, d]

```


## **Adding lists together**

Lists have the ability to be added together via the `+` operator. This is often known as concatenation.  

```mosc

nin letters = ["a", "b", "c"]
nin other = ["d", "e", "f"]
nin combined = letters + other
A.yira(combined)  # > [a, b, c, d, e, f]

```

## **Removing elements**

The opposite of `aDon` is `aBoOyorola`. It removes a single element from a given position in the list.  

To remove a specific value instead, use `aBoye`. The first value that matches using regular equality will be removed.  

```mosc

nin letters = ["a", "b", "c", "d"]
letters.aBoOyorola(1)
A.yira(letters) # > [a, c, d]
letters.aBoye("a")
A.yira(letters) # > [c, d]

```

Both the `aBoye` and `aBoOyorola` method return the removed item:

```mosc

A.yira(letters.aBoOyorola(1)) # > c

```


If `aBoye` couldn’t find the value in the list, it returns `gansan`: 

```mosc

A.yira(letters.aBoye("not found")) # > gansan

```

If you want to remove everything from the list, you can clear it:

```mosc

yiriw.diossi()
A.yira(yiriw) # > []

```
