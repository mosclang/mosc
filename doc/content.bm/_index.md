---
title: Mosc
type: docs
bookToc: false
---

## ߘߏߞߊ߲ 

Mosc ߦߋ ߣߔߚߍߝߝߎߣߣ  

Mosc is tiny simple programing language writen in C.  Mosc VM reads, compiles and interprets Mosc code.  Mosc is intended to allow **Bambara** speakers people to code easily in **Bambara**. That will simplify a lot programing learning curve for those people who don't have an easy access to software developement stuffs.  
The initial goal of Mosc will be focused on educationnal purposes.  

Mosc main purspose is to be embed inside a host application that can so execute `Mosc` code throuth its VM.
Mosc is very easy to handle.  
For a `bambara` and non `english` speaker it's a lot easy to write code like  
```mosc
 nin foli = "Djen nin tié"
 A.yira(foli)
```

instead of  

```java
public class Main {
    public static void main(java.lang.String[] args) {
        java.lang.String salutation = "Hello world";
        System.out.println(salutation);
    }
}
```

Mosc is compact and syntaxe sugar, syntaxically, it can be seen as `kotlin` (with block argument style) and `javascript` (with spread and rest operators).

* **Mosc is fast**, due to a single pass compiler to a tight bytecode and a compact obect representation.
* **Mosc is class-based**, in constrary to many other scripting language that do not really have a Object-models. Objects are everywhere in Mosc.
* **Mosc is concurrent** through its lightweight [Djurus](/docs/concurency/)(fibers or threads) implementations 