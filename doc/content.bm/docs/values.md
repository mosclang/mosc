---
title: "Values"
date: 2022-03-05T15:29:49Z
weight: 5
draft: false
---
## **Value**

Values are the built-in atomic object types that all other objects are composed of. They can be created through literals, expressions that evaluate to a value. All values are immutable—once created, they do not change. The number 3 is always the number 3. The string "frozen" can never have its character array modified in place.


## **Boolean Values**

They are just two values of boolean : `tien` and `galon`, they are instance of [Tienya](/docs/modules/core/tienya/) class.  

## **Number Values**

Like other scripting languages, Mosc has a single numeric type: double-precision floating point. Number literals look like you expect coming from other languages: 

```mosc

0
29919
-21991
0.122911
-23411
0x123DEF
0b1110110001
0o217761

```

Numbers are instances of [Diat](/docs/modules/core/diat/) class.  

## **Strings** 

A string is an array of bytes. Typically, they store characters encoded in UTF-8, but you can put any byte values in there, even zero or invalid UTF-8 sequences. (You might have some trouble printing the latter to your terminal, though.)  

String literals are surrounded in double quotes: 

```mosc
"molo"
""
```

Strings can also span multiple lines. The newline character within the string will always be \n (\r\n is normalized to \n).

```mosc

"
Aw
nin 
tié
"

```
### **Escaping**

A handful of escape characters are supported:

```mosc
"\0" # The NUL byte: 0.
"\"" # A double quote character.
"\\" # A backslash.
"\%" # A percent sign.
"\a" # Alarm beep. (Who uses this?)
"\b" # Backspace.
"\e" # ESC character.
"\f" # Formfeed.
"\n" # Newline.
"\r" # Carriage return.
"\t" # Tab.
"\v" # Vertical tab.


"\x48"        # Unencoded byte     (2 hex digits)
"\u0041"      # Unicode code point (4 hex digits)
"\U0001F64A"  # Unicode code point (8 hex digits)

```
A \x followed by two hex digits specifies a single unencoded byte:

```mosc
A.yira("\x48\x69\x2e") # gives Hi

```
A \u followed by four hex digits can be used to specify a Unicode code point:

```mosc
A.yira("\u0041\u0b83\u00DE") # gives: AஃÞ

```
A capital \U followed by eight hex digits allows Unicode code points outside of the basic multilingual plane, like all-important emoji:

```mosc
A.yira("\U0001F64A\U0001F680") # gives: 🙊🚀

```

Strings comes from [Seben](/docs/modules/core/seben/) class.

### **Interpolation**

String literals also allow interpolation. If you have a dollar sign (`$`) followed by a block(`{}`) expression or an identifier, the expression is evaluated. The resulting object’s `sebenma` method is called and the result is inserted in the string:

```mosc
A.yira("Molo san ye ${12 + 3} ye")

```

Since the interpolation is block of expression you can have inside it a string that have also an interpolation.  


## **Raw String**

You can also create a string using triple quotes `"""` , such strings are exactly the same as any other string, but are parsed differently.