---
title: "Module"
date: 2022-03-22T00:18:09Z
weight: 16
bookToc: false
draft: false
---


## **Modules**

Mosc comes with two kinds of modules, the core module (built-in), and a few optional modules that the host embedding Mosc can enable. 

## **Core module**

The core module is built directly into the VM and is implicitly imported by every other module. You don’t need to import anything to use it. It contains objects and types for the language itself like [numbers](/docs/modules/core/diat/) and [strings](/docs/modules/core/seben/).  

Because Mosc is designed for [embedding in applications][/docs/embedding/], its core module is minimal and is focused on working with objects within Mosc. For stuff like file IO, graphics, etc., it is up to the host application to provide interfaces for this.  

Core module exposes classes :

* [A (System)](/docs/modules/core/a)
* [Baa (Object)](/docs/modules/core/baa)
* [Diat (Number)](/docs/modules/core/diat)
* [Djuru (Thread)](/docs/modules/core/djuru)
* [Funan (Range)](/docs/modules/core/funan)
* [Gansan (Null)](/docs/modules/core/gansan)
* [Seben (String)](/docs/modules/core/seben)
* [Tienya (Boolean)](/docs/modules/core/tienya)
* [Tii (Function)](/docs/modules/core/tii)
* [Tugun (Sequence)](/docs/modules/core/tugun)
* [Wala (Map)](/docs/modules/core/wala)
* [Walan (List)](/docs/modules/core/walan)

## **Optional modules**

Optional modules are available in the Mosc project, but whether they are included is up to the host. They are written in Mosc and C, with no external dependencies, so including them in your application is as easy as a simple compile flag.  

Since they aren’t *needed* by the VM itself to function, you can disable some or all of them, so check if your host has them available.  

So far there are a few optional modules:

* [fan docs](/docs/modules/fan/fan/)
* [kunfe docs](/docs/modules/kunfe/kunfe/)
