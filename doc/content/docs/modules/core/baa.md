---
title: "Baa"
date: 2022-03-05T15:23:04Z
bookToc: false
weight: 2
draft: false
---

## **Baa class**
`Baa` is the mothers of everything in Mosc. All class (`kulu`) inherit from it.  It exposes static methods and is not instatiable.

## **Static Methods**

### **sukuKelen(obj1, obj2)**

Returns `tien` if obj1 and obj2 are the same. For [value types](/docs/values/), this returns `tien` if the objects have equivalent state. In other words, numbers, strings, booleans, and ranges compare by value.  

For all other objects, this returns `tien` only if *obj1* and *obj2* refer to the exact same object in memory.  

This is similar to the built in `==` operator in `Baa` except that this cannot be overriden. It allows you to reliably access the built-in equality semantics even on user-defined classes.  

## **Methods**

### **! operator**

Returns `galon`, since most objects are considered true.  

### **==(other) and !=(other) operators**

Compares two objects using built-in equality. This compares [value types](/docs/values/) by value, and all other objects are compared by identity—two objects are equal only if they are the exact same object.   

### **ye(class) operator**

Returns `tien` if this object’s class or one of its superclasses is `class`.

```mosc
A.yira(123 ye Diat)     # > tien
A.yira("s" ye Diat)     # > galon
A.yira(gansan ye Seben) # > galon
A.yira([] ye Walan)     # > tien
A.yira([] ye Tugun) # > tien

```
It is a runtime error if class is not a [Class](/docs/classes/). 

### **sebenma**

A default string representation of the object.

### **suku**

The [Class](/docs/classes/) of the object.