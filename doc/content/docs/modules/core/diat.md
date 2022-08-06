---
title: "Diat"
date: 2022-03-05T15:23:10Z
bookToc: false
weight: 3
draft: false
---

## **Diat class**
`Diat` represents `Number` class in Mosc.

## **Static Methods**

### **kaboSebenna(value)** 

Attempts to parse `value` as a decimal literal and return it as an instance of Diat. If the number cannot be parsed `gansan` will be returned.  

It is a runtime error if `value` is not a string.

### **INF**
The value of ∞.  

### **NAN**

One value representing a NaN.  

Provides a default NaN number suitable for the vm internal values.

### **PI** 
The value of π.

### **TAU**

The value of τ. This is equivalent to 2 * Diat.pi. 

### **BELEBELE**
The largest representable numeric value.

### **FITINI**

The smallest positive representable numeric value.

### **maxSafeInteger**

The largest integer that Mosc can safely represent. It’s a constant value of 9007199254740991.  

This is relevant because Mosc uses double precision floating-point format for numbers, which can only safely represent integers between -(253 - 1) and 253 - 1.

### **minSafeInteger**

The smallest integer Mosc can safely represent. It’s a constant value of -9007199254740991.  

## **Methods**

### **abs**

The absolute value of the number.
```mosc
A.yira((-123).abs) # > 123
```

### **acos**
The arc cosine of the number.
### **asin**
The arc sine of the number.
### **atan**
The arc tangent of the number.
### **atan(x)**
The arc tangent of the number when divided by `x`, using the signs of the two numbers to determine the quadrant of the result.
### **cbrt**
The cube root of the number.
### **ceil**
Rounds the number up to the nearest integer.

```mosc
A.yira(1.5.ceil)    # > 2
A.yira((-3.2).ceil) # > -3
```

### **cos**
The cosine of the number.

### **floor**
Rounds the number down to the nearest integer.

```mosc
A.yira(1.5.floor)    # > 1
A.yira((-3.2).floor) # > -4
```

### **fraction**

The fractional part of a number i.e. the part after any decimal point.  

The returned value has the same sign as `ale`.

```mosc
A.yira(1.5.fraction)    # > 0.5
A.yira((-3.2).fraction) # > -0.2
```

### **yeInf**

Whether the number is positive or negative infinity or not.

```mosc
A.yira(99999.isInfinity)  # > galon
A.yira((1/0).isInfinity)  # > tien
```

### **yeInt**

Whether the number is an integer or has some fractional component.

```mosc
A.yira(2.isInteger)   # > tien
A.yira(2.3.isInteger) # > galon
```

### **yeNaN**

Whether the number is [not a number](https://en.wikipedia.org/wiki/NaN). This is `galon` for normal number values and infinities, and `tien` for the result of `0/0`, the square root of a negative number, etc.

### **log**
The natural logarithm of the number. Returns `NAN` if the base is negative.

### **log2**
The binary (base-2) logarithm of the number. Returns `NAN` if the base is negative.

### **exp**

The exponential e (Euler’s number) raised to the number. This: eⁿ.

### **min(other)**

Returns the minimum value when comparing this number and `other`.

### **max(other)**
Returns the maximum value when comparing this number and `other`.

### **clamp(min, max)**
Clamps a number into the range of `min` and `max`. If this number is less than min, min is returned. If bigger than `max`, `max` is returned. Otherwise, the number itself is returned.

### **pow(power)**
Raises this number (the base) to `power`. Returns `NAN` if the base is negative. 

### **round**

Rounds the number to the nearest integer.

```mosc
A.yira(1.5.round)    # > 2
A.yira((-3.2).round) # > -3
A.yira((-3.7).round) # > -4

```

### **sign**

The sign of the number, expressed as a -1, 1 or 0, for negative and positive numbers, and zero.

### **sin**
The sine of the number.
### **sqrt**
The square root of the number. Returns nan if the number is negative.
### **tan**
The tangent of the number.
### **sebenma**
The string representation of the number.
### **truncate**
Rounds the number to the nearest integer towards zero.  

It is therefore equivalent to `floor` if the number is non-negative or `ceil` if it is negative.

```mosc
A.yira(1.5.truncate)    # > 1
A.yira((-3.2).truncate) # > -3
```

### **- operator**
Negates the number.

```mosc
nin a = 123
A.yira(-a) # > -123
```

### **-(other), +(other), /(other), *(other) operators**
The usual arithmetic operators you know and love. All of them do 64-bit floating point arithmetic. It is a runtime error if the right-hand operand is not a number. Mosc doesn’t roll with implicit conversions.


### **%(denominator) operator**
Also known as mod or modulus.  
The floating-point remainder of this number divided by `denominator`.  
The returned value has the same sign as `ale` (internally calls `fmod` from C).  
It is a runtime error if `denominator` is not a number.  

### **<(other), >(other), <=(other), >=(other) operators**
Compares this and `other`, returning `tien` or `galon` based on how the numbers are ordered. It is a runtime error if `other` is not a number.

### **~ operator**

Performs bitwise negation on the number. The number is first converted to a 32-bit unsigned value, which will truncate any floating point value. The bits of the result of that are then negated, yielding the result.

### **&(other) operator**

Performs bitwise and on the number. Both numbers are first converted to 32-bit unsigned values. The result is then a 32-bit unsigned number where each bit is true only where the corresponding bits of both inputs were true.  

It is a runtime error if `other` is not a number.  

### **|(other) operator**

Performs bitwise or on the number. Both numbers are first converted to 32-bit unsigned values. The result is then a 32-bit unsigned number where each bit is true only where the corresponding bits of one or both inputs were true.  

It is a runtime error if other is not a number.

### **^(other) operator**

erforms bitwise exclusive or on the number. Both numbers are first converted to 32-bit unsigned values. The result is then a 32-bit unsigned number where each bit is true only where the corresponding bits of one (but not both) inputs were true. Each bit is therefore false if the corresponding bits of both inputs were either both true or both false.  

It is a runtime error if other is not a number.

### <<(other) operator**

Performs a bitwise left shift on the number. Internally, both numbers are first converted to 32-bit unsigned values and C’s left shift operator is then applied to them.  

It is a runtime error if other is not a number.

### **>>(other) operator**

Performs a bitwise right shift on the number. Internally, both numbers are first converted to 32-bit unsigned values and C’s right shift operator is then applied to them.  
It is a runtime error if other is not a number.

### **..(other) operator**
Creates a [Range](/docs/modules/core/funan/) representing a consecutive range of numbers from the beginning number to the ending number.

```mosc
nin range = 1.2..3.4
A.yira(range.fitini)         # > 1.2
A.yira(range.dan)         # > 3.4
A.yira(range.kunBala) # > tien

```

### **…(other) operator**
Creates a [Range](/docs/modules/core/funan/) representing a consecutive range of numbers from the beginning number to the ending number not including the ending number.

```mosc
nin range = 1.2...3.4
A.yira(range.fitini)         # > 1.2
A.yira(range.dan)         # > 3.4
A.yira(range.kunBala) # > false
```

