---
title: "A"
date: 2022-03-05T15:24:18Z
bookToc: false
weight: 1
draft: false
---

## **A class**
The `A` class is a grab-bag of functionality exposed by the VM, mostly for use during development or debugging.

## **Static Methods**

### A.**waati**
Returns the number of seconds (including fractional seconds) since the program was started. This is usually used for benchmarking.

### A.**gc()**

Requests that the VM perform an immediate garbage collection to free unused memory.

### A.**yira()**

Prints a single newline to the console.

### A.**yira(object)**

Prints `object` to the console followed by a newline. If not already a string, the object is converted to a string by calling `sebenma` on it.

```mosc
A.yira("Namassa ka di") # > Namassa ka di
```

### A.**beeYira(sequence)**

Iterates over `sequence` and prints each element, then prints a single newline at the end. Each element is converted to a string by calling `sebenma` on it.

```mosc
A.beeYira([1, 2, [3, ...[4, 4]]]) # > 12[3, 4, 5]
```

### A.**seben(object)**

Prints a single value to the console, but does not print a newline character afterwards. Converts the value to a string by calling `sebenma` on it.

```mosc
A.seben(2 + 3) # > 5
```
In the above example, the result of `2 + 3` is printed, and then the prompt is printed on the same line because no newline character was printed afterwards.

### A.**beeSeben(sequence)**

Iterates over `sequence` and prints each element, but does not print a newline character afterwards. Each element is converted to a string by calling `sebenma` on it.
