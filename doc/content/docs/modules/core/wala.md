---
title: "Wala"
date: 2022-03-05T15:22:47Z
weight: 11
draft: false
bookToc: false
---

## **Wala Class**
`Wala` represents `Map` class in Mosc and extends [Tugun](/docs/modules/core/tugun/) class.

## **Static Method**

### **kura()**

Creates a new empty map. Equivalent to {}.

## **Methods**

### **diossi()**

Removes all entries from the map.

### **bAkono**

Returns `tien` if the map contains key or `galon` otherwise.

### **hakan**

The number of entries in the map.

### **keys**

A [Sequence](/docs/modules/core/tugun) that can be used to iterate over the keys in the map. Note that iteration order is undefined. All keys will be iterated over, but may be in any order, and may even change between invocations of Mosc.

### **aBoye(key)**

Removes `key` and the value associated with it from the map. Returns the value.  

If the key was not present, returns `gansan`.

### **values**

A [Sequence](/docs/modules/core/tugun) that can be used to iterate over the values in the map. Note that iteration order is undefined. All values will be iterated over, but may be in any order, and may even change between invocations of Mosc.  

If multiple keys are associated with the same value, the value will appear multiple times in the sequence.

### **\[key\] operator**

Gets the value associated with `key` in the map. If `key` is not present in the map, returns `gansan`.

```mosc
nin map = {"molo": "Doumbia", "issa": "Diallo"}
A.yira(map["molo"]) # > Doumbia
A.yira(map["madou"])  # > gansan
```
### **\[key\]=(value) operator**

Associates value with `key` in the map. If `key` was already in the map, this replaces the previous association.  

It is a runtime error if the key is not a [Bool](/docs/modules/core/tienya), [Class](/docs/modules/core/kulu), [Null](/docs/modules/core/gansan), [Num](/docs/modules/core/diat), [Range](/docs/modules/core/funan), or [String](/docs/modules/core/seben).

### **iterate(iterator), iteratorValue(iterator)**

Implements the [iterator protocol](/docs/control-flow#the-iterator-protocol) for iterating over the keys and values of a map at the same time.  

When a map (as opposed to its keys or values separately) is iterated over, each key/value pair is wrapped in a `WalaEntry` object. `WalaEntry` is a small helper class which has read-only `key` and `value` properties and a familiar `sebenma` representation.

```mosc
nin map = {"molo": "droid"}
seginka (map kono entry) {
  A.yira(entry.type)                    # WalaEntry
  A.yira(entry.key + " " + entry.value) # molo droid
  A.yira(entry)                         # molo:droid
}
```

All map entries will be iterated over, but may be in any order, and may even change between invocations of Mosc.
