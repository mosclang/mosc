---
title: "Funan"
date: 2022-03-05T15:25:07Z
bookToc: false
weight: 5
draft: false
---


## **Funan Class**
`Funan` represents `Range` class in Mosc and extends [Sequence](/docs/modules/core/tugun).  
A range defines a bounded range of values from a starting point to a possibly exclusive endpoint. [Here](/docs/values#ranges) is a friendly introduction.

## **Methods**

### **kabo**
The starting point of the range. A range may be backwards, so this can be greater than \[kata(to)\].

```mosc
A.yira((3..5).kabo) # > 3
A.yira((4..2).kabo) # > 4
```

### **kata**

The endpoint of the range. If the range is inclusive, this value is included, otherwise it is not.

```mosc
A.yira((3..5).kata) # > 5
A.yira((4..2).kata) # > 2
```

### **fitini**

The minimum bound of the range. Returns either `kabo`, or `kata`, whichever is lower.

```mosc
A.yira((3..5).fitini) # > 3
A.yira((4..2).fitini) # > 2
```

### **dan**

The maximum bound of the range. Returns either `kabo`, or `kata`, whichever is greater.

```mosc
A.yira((3..5).dan) # > 5
A.yira((4..2).dan) # > 4
```

### **kunBala**

Whether or not the range includes `kata`. (`kabo` is always included.)

```mosc
A.yira((3..5).kunBala)   # > tien
A.yira((3...5).kunBala)  # > galon
```

### **iterate(iterator, step), iteratorValue(iterator)**

Iterates over the range. Starts at `kabo` or `kata` according to the step sign and increments or decrements by step towards or backward to until the endpoint is reached.