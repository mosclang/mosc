---
title: "Classes"
date: 2022-03-05T15:30:40Z
weight: 8
draft: false
---

## **Classes**
In Mosc, everything is `Object` which in its turn is an instance of a class. Mosc has buit-in class like `Gansan`, `Seben`, `Diat` and some built-in value such us `gansan`, `tien`, `galon` witch are direct instance of theire corresponding built-in classes.  
A class defines the dehavior of an object and holds the object state. Object state are store in class field and through methods a class exposes we can control the state or behavior of the object.

## **Define a class**
The keyword `kulu`is used to declare a class, followed by the class name. Class declaration is generally followed by the `{` in case the class has a body.
```mosc
kulu Molo

kulu Mogo {
   
}

```

## **`ale` keyword**
In Mosc, inside a class , when you need to access anything, including `fields` `and methods`, concerning the Object instance it-self, you need to use the keyword `ale`which points to the current instance of a Class.



## **Fields**

Class fields hold the `Object` states and by by default  they are all private, but Mosc exposes a public setter and getter for them except those which name start with `_` and to access them outside the class you need to, explicitly, define getter and setter for them. 

```mosc
kulu Mogo {
    nin togo
    nin sugu = "Mogo"
    # this field is private, is not exposed through public getter and setter
    nin _age # Accessing this field outside an Object instance gives an error: Mogo does not implements _age method
}
```

## **Methods**

Methods are an Object emissary, they allow an object to communicate with external world.  In Mosc the only way to change an `Object` internal state is to pass through a method.  
They can take 0 or more parameters.  With Mosc signature mecanism it possible to overload methods.

```mosc
kulu Mogo {
    nin togo
    nin sugu = "Mogo"
    # this field is private, is not exposed through public getter and setter
    nin _age

    kuma() {
        A.yira("Bè kuma")
    }
    diamou() { ale.togo.faraFara(" ")[1] }
    diamou(v) {
        nin els = ale.togo.faraFara(" ")
        segin niin "${els[0]} ${v}"
    }
}
```
### **Getters**
Getters are special methods, with no parametter, whose role is to return a single value, generally a class internal state. They are often used to expose an `Object` state to external world.

```mosc
kulu Mogo {
    nin _togo
    nin _sugu = "Mogo"
    # this field is private, is not exposed through public getter and setter
    nin _age
  
    togo { ale._togo }
    sugu {
        segin niin ale._sugu
    }
    age {
        segin niin "san " + ale._age
    }
}
```
### **Setters**

Setters are special methods whose role is to change an `Object` internal state.

```mosc
kulu Mogo {
    nin _togo
    nin _sugu = "Mogo"
    # this field is private, is not exposed through public getter and setter
    nin _age
   
    togo=(v) {
        ale._togo = v
    }
    sugu=(v) {
        ale._sugu = v 
    }
}
```

## **Operators**

In Mosc, its possible to declare `operator` methods. They act as normal methods, their name must be a valid Mosc operator. 

### **Prefix operators**
They are like getter without any parametter. 
```mosc
kulu Number {
    nin value
  
    - {
        segin niin - ale.value
    }
    ! { !ale.value }
}
```
### **Infix operators**
They take just and only 1 parametter wich is the right hand value of the operator.

```mosc
kulu Number {
    nin value
  
    - (other) {
        segin niin ale.value - other
    }
    + (other) { ale.value + other }
}
```

### **Index operator**
Index operator intends to access a value on an Object through 1 or more indices passed in parameters. The parameters are put inside a square bracket `[]`

```mosc
kulu Seq {
    nin _arr
   
    [index] { ale._arr[index] }
    [index, count] {
        segin niin ale._arr[index] + ale._arr[index + count]
    }
}
```


### **Index setter operator**
Index setter operator intends to change an Object state  through 1 or more indices passed in parameters. its a comination of an Index operator and a setter

```mosc
kulu Seq {
    nin _arr
   
    [index] = (value) { 
        ale._arr[index]  = value
    }
    [index, count] = (v) {
        ale._arr[index + count] = ale._arr[index] + v
    }
}
```
## **Constructor**
A class constructor is a special method prefixed by `dilan` keyword. The constructor role is to create a new instance of a class.  During an object instantiation process, the constructor is called on a new `empty` instance value. 


```mosc
kulu Mogo {
    nin togo
    nin sugu = "Mogo"
    nin _age
    dilan kura(togo, age) {
        ale.togo = togo
        ale._age = age
    }
}
```

A class can be instantiated by calling one of its constructor  

```mosc

nin mogo = Mogo.kura("Madou", 26)

A.yira(mogo.togo) # outputs Madou
A.yira(mogo._age) # This gives an error: Mogo does not implements _age method
mogo.togo = "Ali"

```
## **Static methods and fields**
In Mosc, each class is associated to a meta class to which it's possible to tie methods that can be accessed wihout creating an instance of the class.  
### **Static fields**
Static fields are just a class scoped variables which can be accessed only inside the class scope. Static fields accesses  are not preceded by `ale` keyword. Static fields are declared like normales ones, but are preceded by `dialen` keyword.

```mosc
kulu Mogo {
    nin togo
    dialen nin sukuya

    asukuya() {
        segin niin sukuya
    }
}
```

### **Static methods**

Static methods are declared like normal methods preceded by `dialen` keyword. Static methods are stored in a class meta class `Object`.  
Setter and getters can be declared as static by just preceding their declaration by `dialen`

```mosc
kulu Mogo {
     nin togo
    dialen nin sukuya

    dialen asukuya() {
        segin niin sukuya
    }
    dialen suku { "Sukuya" }
    dialen suku = (v) {
        sukuya = v
    }
}
```

Note that a class constructor is a static method stored in the class meta class.  

## **Inheritance**

`Object` can inherits methods from an other `Object` through class inheritance process.  Only methods declared inside parent class are inherited  not fields and static methods. The `ye` keyword is used to specify a superclasss.

```mosc
 kulu Mogo {
     nin togo
     togo { ale.togo }
 }
 kulu Farafin ye Mogo {
     #inherit automaticly togo method
     dilan kura() {}
 }
 nin far = Farafin.kura()
 A.yira(far.togo)

```

Class inheritance does not concerne its meta class, in short, a class does not inherits its base's meta class, which means that constructor are not inherited as whell static methods. In a constructor you can make a `faa` call to refer a base class constructor.

```mosc

kulu Mogo {
    nin togo
    dialen sukuya { "mogo" }
    dilan kura(t) {
        ale.togo = t
    }
}

kulu Farafin ye Mogo {
    dilan kura() {
        faa("Farafin")
    }
}

nin fa = Farafin.kura("Fara")
Farafin.sukuya ## gives an error

```
## **faa keyword**

`faa` refers to the parent Object in case of inheritance, so that allows You to call a method declared in superclasses. If `fa` is used as function call , its called `faa` call and refers to a constructor call from the base class.

```mosc

kulu Mogo {
    dilan kura(t) {
        ale.togo = t
    }
    dilan method() {
    }
}

kulu Farafin ye Mogo {
     dilan kura() {
        # call the base class constructor with the same signature as the current constructor
        faa("Farafin")
    }

    dilan method() {
        # call the base class method method()
        faa.method()
    }
}

```
