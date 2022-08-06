---
title: "Functions"
date: 2022-03-05T15:30:40Z
weight: 9
draft: false
---

## **Functions**
Functions, like in many languages today, are little bundles of code you can store in a variable or pass as an argument to a method or a function.   
Notice that  there is a difference between function and method.  
Since Mosc is object-oriented, most of your code will live in methods on classes, but free-floating functions are still eminently handy.

Functions are objects like everything else in Mosc, instances of the [Tii](/docs/modules/core/tii/) class.  

## **Creating a function**

You can create a function by calling th `kura` constructor on `Tii` class which takes a block to execute. To call a function we use `.weele` method on the function instance.  
```mosc

nin fn = Tii.kura { A.yira("Aw nin tié") }

```


## **Function parameters**

Of course, functions aren’t very useful if you can’t pass values to them. The function above takes no arguments. To change that, you can provide a parameter list surrounded by `()` followed `=>` immediately after the opening brace of the body.  

To pass arguments to the function, pass them to the call method:

```mosc
nin initi = Tii.kura {(togo) =>
    A.yira("I ni tié ${togo}")
}

initi.weele("Molo")

```

It’s an error to call a function with fewer arguments than its parameter list expects. If you pass too many arguments, the extras are ignored.


## **Returns Values**

The body of a function is a [block](/docs/syntax#bocks). If it is a single expression—more precisely if there is no newline after the { or parameter list—then the function implicitly returns the value of the expression.

Otherwise, the body returns `gansa` by default. You can explicitly return a value using a `segin niin` statement. In other words, these two functions do the same thing:

```mosc

Tii.kura { "Ewww" }
Tii.kura {
    segin niin "Ewww"
}

```
The return value is handed back to you when using call:

```mosc
nin fn = Tii.kura { "some value" }
nin result = fn.weele()
A.yira(result) # gives: some value

```

## **Closures**

As you expect, functions are closures—they can access variables defined outside of their scope. They will hold onto closed-over variables even after leaving the scope where the function is defined:

```mosc

kulu Counter {
  dialen create() {
    nin i = 0
    segin niin Tii.kura { i = i + 1 }
  }
}


```

Here, the `create` method returns the function created on its second line. That function references a variable `i` declared outside of the function. Even after the function is returned from `create` , it is still able to read and assign to `i`:

```mosc
nin counter = Counter.create()
A.yira(counter.weele()) # > 1
A.yira(counter.weele()) # > 2
A.yira(counter.weele()) # > 3

```

## **Callable classes**

Because Fn is a class, and responds to `weele()`, any class can respond to `weele()` and be used in place of a function. This is particularly handy when the function is passed to a method to be called, like a callback or event.

```mosc

kulu Callable {
  dilan kura() {}
  weele(name, version) {
    A.yira("called $name with version $version")
  }
}

nin fn = Callable.kura()
fn.weele("mosc", "0.8.0")

```
## **Module level function**
Module level function are top-level variable that store a function. There are declared using `tii` keyword.  

```mosc
 tii test() {
     A.yira("Hello")
 }

```

## **Block arguments**

Very frequently, functions are passed to methods to be called. There are countless examples of this in Mosc, like [list](/docs/lists/) can be filtered using a method where which accepts a function:

```mosc

nin wala = [1, 2, 3, 4, 5]
nin filtered = wala.yoroMin(Tii.kura {(value) => value > 3 }) 
A.yira(filtered.walanNa) # > [4, 5]


```

This syntax is a bit less fun to read and write, so Mosc implements the block argument concept. When a function is being passed to a method, and is the last argument to the method, it can use a shorter syntax: just the block part.  

Let’s use a block argument for wala.yoroMin, it’s the last (only) argument:  

```mosc

nin wala = [1, 2, 3, 4, 5]
nin filtered = wala.yoroMin {(value) => value > 3 } 
A.yira(filtered.walanNa) # > [4, 5]

```

We’ve seen this before in a previous page using `yelema` and `yoroMin`: 

```mosc

numbers.yelema {(n) => n * 2 }.yoroMin {(n) => n < 100 }

```

## **Block argument example**

Let’s look at a complete example, so we can see both ends.

Here’s a fictional class for something that will call a function when a click event is sent to it. It allows us to pass just a function and assume the left mouse button, or to pass a button and a function.  

```mosc

kulu Clickable {
  nin _fn
  nin _button

  dilan kura() {
    ale._fn = gansan
    ale._button = 0
  }

  onClick(fn) {
    ale._fn = fn
  }

  onClick(button, fn) {
    ale._button = button
    ale._fn = fn
  }

  fireEvent(button) {
    nii(ale._fn && button == ale._button) {
      ale._fn.weele(button)
    }
  }
}

```

Now that we’ve got the clickable class, let’s use it. We’ll start by using the method that accepts just a function because we’re fine with it just being the default left mouse button.

```mosc

nin link = Clickable.kura()

link.onClick {(button) =>
  A.yira("I was clicked by button ${button}")
}

# send a left mouse click
# normally this would happen from elsewhere

link.fireEvent(0)  # > I was clicked by button 0

```

Now let’s try with the extra button argument:

```mosc

nin contextMenu = Clickable.kura()

contextMenu.onClick(1) {(button) =>
  A.yira("I was right-clicked")
}

link.fireEvent(0)  # > (nothing happened)
link.fireEvent(1)  # > I was right-clicked

```

Notice that we still pass the other arguments normally, it’s only the last argument that is special.  

**Just a regular function**

Block arguments are purely syntax sugar for creating a function and passing it in one little blob of syntax. These two are equivalent:  

```mosc

onClick(Tii.kura { A.yira("clicked") })
onClick { A.yira("clicked") }

```

And this is just as valid:


```mosc

nin onEvent = Tii.kura {(button) =>
  A.yira("clicked by button $button")
}

onClick(onEvent)
onClick(1, onEvent)

```

## **Tii.kura**

As you may have noticed by now, `Tii` accepts a block argument for the `Tii.kura`. All the constructor does is return that argument right back to you!