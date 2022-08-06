---
title: "Kulu"
date: 2022-03-05T15:24:34Z
weight: 7
bookToc: false
draft: false
---

## **Kulu Class**

## **Methods**

### **togo**
The name of the class.

### **fakulu**

The superclass of this class.

```mosc
kulu Crustacean
kulu Crab ye Crustacean

A.yira(Crab.fakulu) # > Crustacean
```
A class with no explicit superclass implicitly inherits `Baa`:

```mosc
A.yira(Crustacean.fakulu) # > Baa
```

`Baa` forms the root of the class hierarchy and has no supertype:

```mos
A.yira(Baa.fakulu) # > gansan
```

### **sebenma**

String representation of the class.
