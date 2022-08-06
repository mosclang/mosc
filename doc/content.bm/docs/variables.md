---
title: "Variables"
date: 2022-03-05T15:29:46Z
weight: 6
draft: false
---


Variables are declared using `nin` keyword, followed by its name which may be an identifier or, in case or destructuration, by `[`, `{`. The declaration can includes an initializer with is place after the `=` operator.  
```mosc
nin variable
nin v2
nin v3 = 3
nin v4 = v3 + 1
nin _v4 = v4 % 2 + " mm "

```