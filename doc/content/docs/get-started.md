---
title: "Get Started"
date: 2022-03-06T15:36:43Z
weight: 1
draft: false
---
## **Getting Started**

Getting started with mosc is simple as drinking water!

## **Hello world**
```mosc
A.yira("Hello World")
```
## **Variable declaration**
```mosc
nin a = "Molo"
nin dec = 123
nin hex = 0xde122
nin raw = """
    Raw string
    Multiline strings
"""
# literal list
nin arr = ["Hello", "World", "\n"]
# literal map
nin obj = {"key": "Value", "key1": [], "key2": 12, [3]: 44}
# destructuration
nin [a0, a1] = [12, 23]
nin {d, c} = {"a": "AA", "d": "DD", "c": "CC"} 
```
[See more about variables declaration](/docs/variables/)

## **Module level function**

In Mosc everything is Object, same is valable for function, Mosc allows you to instantiate `Tii` class or declare module level function directly using keyword `tii`. Their declaration are only valid in the module scope. 


```mosc
 tii times2(a) {
     segin niin a * 2
 }
 tii test() {
     A.yira(times2(23))
 }
 tii test1() {
     A.yira(times2(23))
     segin
 }

```
The `segin` keyword ends the function execution and return `gansan` to the caller, if you want to return a specific value, you can use `niin` keyword as `segin niin 12`. 

`tii` keyword can be used to make class extension, which consists of adding additionnals method (can be static) to a class after its declaration, that way you can add custom methods to an existing class without having the source code. 

```mosc
    dialen tii Npalan.beye(path) {
        # here to can't access static fields, just static methods are accessibles since you're not anymore in the class scope
        segin niin tien
    }
    tii Npalan.read() {
         # here to can't access fields, just methods are accessibles since you're not anymore in the class scope
    }
```
## **Classes declaration**

Mosc is classfull, you can organise you code in class

```mosc

 kulu Mogo

 kulu Farafin ye Mogo {
     nin togo
     nin sugu
     # private attribute, is not exposed by default
     nin _gundo
     dilan kura(togo, sugu) {
         ale.togo = togo
         ale.sugu = sugu
     }

     method() {
        A.yira("Method")
     }
     getTogo { ale.togo }
     setTogo = (v) {
         ale.togo = v
     }
 }

 nin far = Farafin.kura("Madou", "finma")
 A.yira(far.getTogo)
 far.setTogo = "Abou"

```
[See more about classes](/docs/classes/)

## **Collections**
Collections, in Mosc, all, inherit from [Tugun](/docs/modules/core/tugun/) class which expose usefull methods for them. 
### **Lists**
Lists are collection in Mosc. They represents a continuous items in memory and are represented by `Walan` class

```mosc
# literal list
nin list = ["Hello", "World"]
A.yira(list.kunBen(" "))
## spreading list inside an other list
nin list2 = ["Hey", ...["!", ...list]]
```

[See more about lists](/docs/walan/)


### **Maps**
Maps are collections, they are like list, but their items are accessed by key name

```mosc
# literal map
nin map = {"k1": "Hello", "k2": "World"}

## spreading map inside an other map
nin map2 = {"k3": "Hey", ...{"k4": "!", ...map}}

```

[See more about maps](/docs/wala/)