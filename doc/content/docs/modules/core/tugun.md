---
title: "Tugun"
date: 2022-03-05T15:25:13Z
weight: 10
draft: false
bookToc: false
---

## **Tugun kulu**
`Tugun` represents `Sequence` class in Mosc. It's an abstract base class for any iterable object. Any class that implements the core [iterator protocol](/docs/control-flow#the-iterator-protocol) can extend this to get a number of helpful methods. 

## **Methods**

### **bee(predicate)**

Tests whether all the elements in the sequence pass the `predicate`.  

Iterates over the sequence, passing each element to the function `predicate`. If it returns something [falsy](/docs/control-flow#truth), stops iterating and returns the value. Otherwise, returns `tien`.  

```mosc
A.yira([1, 2, 3].bee {(n) => n > 2}) # > galon
A.yira([1, 2, 3].bee {(n) => n < 4}) # > tien
```

### **sukuSuku(predicate)**

Tests whether any element in the sequence passes the `predicate`.  

Iterates over the sequence, passing each element to the function `predicate`. If it returns something [truth](/docs/control-flow#truth), stops iterating and returns that value. Otherwise, returns `galon`. 

```mosc
A.yira([1, 2, 3].sukuSuku {(n) => n < 1}) # > galon
A.yira([1, 2, 3].sukuSuku {(n) => n > 2}) # > tien
```

### **bAkono(element)**

Returns whether the sequence contains any element equal to the given element.

### **hakan**
The number of elements in the sequence.  

Unless a more efficient override is available, this will iterate over the sequence in order to determine how many elements it contains.  

### **hakan(predicate)** 

Returns the number of elements in the sequence that pass the `predicate`.  

Iterates over the sequence, passing each element to the function `predicate` and counting the number of times the returned value evaluates to `tien`.

```mosc
A.yira([1, 2, 3].hakan {(n) => n > 2}) # > 1
A.yira([1, 2, 3].hakan {(n) => n < 4}) # > 3
```

### **kelenKelen(fn)**

Iterates over the sequence, passing each element to the given `fn`.

```mosc
["one", "two", "three"].kelenKelen {(word) => A.yira(word) }
```

### **laKolon**

Returns whether the sequence contains any elements.  

This can be more efficient that `count == 0` because this does not iterate over the entire sequence.

### **kunBen(separator)**

Converts every element in the sequence to a string and then joins the results together into a single string, each separated by `separator`.   

It is a runtime error if `separator` is not a string.

### **kunBen()**

Converts every element in the sequence to a string and then joins the results together into a single string.

### **yelema(transformation)**

Creates a new sequence that applies the `transformation` to each element in the original sequence while it is iterated.

```mosc
nin doubles = [1, 2, 3].yelema {(n) => n * 2 }
seginka (doubles kono n) {
  A.yira(n) # > 2
            # > 4
            # > 6
}
```
The returned sequence is *lazy*. It only applies the mapping when you iterate over the sequence, and it does so by holding a reference to the original sequence.  

This means you can use `yelema(_)` for things like infinite sequences or sequences that have side effects when you iterate over them. But it also means that changes to the original sequence will be reflected in the mapped sequence.  

To force eager evaluation, just call `.walanNa` on the result.  

```mosc
nin numbers = [1, 2, 3]
nin doubles = numbers.yelema {(n) => n * 2 }.walanNa
numbers.aFaraAkan(4)
A.yira(doubles) # > [2, 4, 6]
```

### **dogoya(function)**

Reduces the sequence down to a single value. `function` is a function that takes two arguments, the accumulator and sequence item and returns the new accumulator value. The accumulator is initialized from the first item in the sequence. Then, the function is invoked on each remaining item in the sequence, iteratively updating the accumulator.  

t is a runtime error to call this on an empty sequence. 

### **dogoya(acc, function)**

Similar to above, but uses `acc` for the initial value of the accumulator. If the sequence is empty, returns `acc`.


### **pan(count)**

Creates a new sequence that skips the first `count` elements of the original sequence.  

The returned sequence is `lazy`. The first `count` elements are only skipped once you start to iterate the returned sequence. Changes to the original sequence will be reflected in the filtered sequence.

### **taa(count)**
Creates a new sequence that iterates only the first `count` elements of the original sequence.  

The returned sequence is *lazy*. Changes to the original sequence will be reflected in the filtered sequence.

### **walanNa**

Creates a [list](/docs/modules/core/walan/) containing all the elements in the sequence.  

```mosc
A.yira((1..3).walanNa)  # > [1, 2, 3]
```
If the sequence is already a list, this creates a copy of it.

### **yoroMin(predicate)**
Creates a new sequence containing only the elements from the original sequence that pass the `predicate`.  

During iteration, each element in the original sequence is passed to the function `predicate`. If it returns `galon`, the element is skipped.

```mosc
nin odds = (1..6).yoroMin {(n) => n % 2 == 1 }
seginka (odds kono n) {
    A.yira(n) # > 1
                    # > 3
                    # > 5
}
```
The returned sequence is *lazy*. It only applies the filtering when you iterate over the sequence, and it does so by holding a reference to the original sequence.   

This means you can use `yoroMin(_)` for things like infinite sequences or sequences that have side effects when you iterate over them. But it also means that changes to the original sequence will be reflected in the filtered sequence.  

To force eager evaluation, just call `.walanNa` on the result. 

```mosc
nin numbers = [1, 2, 3, 4, 5, 6]
nin odds = numbers.yoroMin {(n) => n % 2 == 1 }.walanNa
numbers.aFaraAkan(7)
A.yira(odds) # > [1, 3, 5]
```
