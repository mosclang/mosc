---
title: "Maps"
date: 2022-03-05T15:29:56Z
weight: 5
draft: false
---

## **Maps**

A map is an associative collection. It holds a set of entries, each of which maps a key to a value. The same data structure has a variety of names in other languages: hash table, dictionary, association, table, etc.  

You can create a map by placing a series of comma-separated entries inside curly braces. Each entry is a key and a value separated by a colon: 

```mosc
{
  "madou"   : "Madou DOUMBIA",
  "ali"     : "Ali TRAORE",
  "ouss"    : "Ouss LO",
  "molo"    : "Molo DROID"
}

``` 

This creates a map that associates a type of tree (key) to a specific tree within that family (value). Syntactically, in a map literal, keys can be any literal or a  expression place in bracket `[]`. Values can be any expression. Here, we’re using string literals for both keys and values.  

```mosc
{
    ["molo" + "bala"]: "Molobala",
    [nii a == 23 "molo" note "mala"]: "Molo or Mala",
    "literal": "Literal Value"
}
```

*Semantically*, values can be any object, and multiple keys may map to the same value.  

Keys have a few limitations. They must be one of the immutable built-in [value types](/docs/values/) in Mosc. That means a number, string, range, bool, or null. You can also use a [class object](/docs/classes/) as a key (not an instance of that class, the actual class itself).  

The reason for this limitation—and the reason maps are called “hash tables” in other languages—is that each key is used to generate a numeric hash code. This lets a map locate the value associated with a key in constant time, even in very large maps. Since Mosc only knows how to hash certain built-in types, only those can be used as keys.

## **Adding entries**

You add new key-value pairs to the map using the [index operator](/docs/method-calls#indexing-or-subscripts):

```mosc
    nin capitals = {}
    capitals["Mali"] = "Bamako"
    capitals["Senegal"] = "Dakar"
    capitals["Maroc"] = "Rabat"

```

If the key isn’t already present, this adds it and associates it with the given value. If the key is already there, this just replaces its value.

## **Looking up values**

To find the value associated with some key, again you use your friend the [index operator](/docs/method-calls#indexing-or-subscripts):

```mosc
A.yira(capitals["Mali"]) # > Bamako
```

If the key is present, this returns its value. Otherwise, it returns `gansan`. Of course, `gansan` itself can also be used as a value, so seeing `gansan` here doesn’t necessarily mean the key wasn’t found.  

To tell definitively if a key exists, you can call `bAkono()`:

```mosc
nin capitals = {"Mali": gansan}
A.yira(capitals["Mali"]) # > gansan (though key exists)
A.yira(capitals["Senegal"])   # > gansan 
A.yira(capitals.bAkono("Mali")) # > tien
A.yira(capitals.bAkono("Senegal"))   # > galon
```
You can see how many entries a map contains using `hakan`:
```mosc
A.yira(capitals.hakan) # > 3
```

## **Removing entries**

To remove an entry from a map, call `aBoye()` and pass in the key for the entry you want to delete:

```mosc
capitals.aBoye("Senegal")
A.yira(capitals.bAkono("Sengal"))# > galon
```

If the key was found, this returns the value that was associated with it:

```mosc
A.yira(capitals.aBoye("Senegal")) # > Dakar
```
If the key wasn’t in the map to begin with, `aBoye()` just returns `gansan`.

If you want to remove everything from the map, like with lists, you call `diossi()`:

```mosc
capitals.diossi()
A.yira(capitals.hakan) # > 0
```

## **Iterating over the contents**

The index operator works well for finding values when you know the key you’re looking for, but sometimes you want to see everything that’s in the map. You can use a regular `seginka` loop to iterate the contents, and map exposes two additional methods to access the contents: `keys` and `values`.  

The `keys` method on a map returns a [Sequence](/docs/modules/core/tugun/) that [iterates](/docs/control-flow#the-iterator-protocol) over all of the keys in the map, and the `values` method returns one that iterates over the values.  

Regardless of how you iterate, the *order* that things are iterated in isn’t defined. Mosc makes no promises about what order keys and values are iterated. All it promises is that every entry will appear exactly once.  

**Iterating with for(entry in map)**   

When you iterate a map with `seginka`, you’ll be handed an *entry*, which contains a `key` and a `value` field. That gives you the info for each element in the map.

```mosc
nin birds = {
  "Mali": "Mosc Dove",
  "Tchad": "Raven",
  "Guinée": "Guinea fowl"
}

seginka (birds kono bird) {
  A.yira("The state bird of ${bird.key} is ${bird.value}")
}
```
**Iterating using the keys**  

You can also iterate over the keys and use each to look up its value:

```mosc
nin birds = {
  "Mali": "Mosc Dove",
  "Tchad": "Raven",
  "Guinée": "Guinea fowl"
}

seginka (birds.keys kono diamana) {
  A.yira("The state bird of $diamana is ${birds[diamana]}")
}
```
