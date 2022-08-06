---
title: "Djuru"
date: 2022-03-05T15:23:24Z
bookToc: false
weight: 4
draft: false
---

## **Djuru Class**
`Djuru` can be seen as `Thread` class in Mosc. Each Mosc process runs on a `Djuru` instance, djurus are lightweight coroutine, you can instatiate menu as we want.[Here](/docs/concurrency/) is a gentle introduction.

## **Static Methods**

### **atike(message)**
Raises a runtime error with the provided message:

```mosc
Djuru.atike("Fili kere")
```

If the message is `gansan`, does nothing.


### **sissanTa**
The currently executing djuru.

### **kura(function)**

Creates a new djuru that executes `function` in a separate coroutine when the djuru is run. Does not immediately start running the djuru.  

```mosc
nin thread = Djuru.kura {
  A.yira("I won't get printed")
}
```
`function` must be a function (an actual Fn instance, not just an object with a `call()` method) and it may only take zero or one parameters.

### **djo**

Pauses the current djuru, and stops the interpreter. Control returns to the host application.  

Typically, you store a reference to the djuru using `Djuru.sissanTa` before calling this. The djuru can be resumed later by calling or transferring to that reference. If there are no references to it, it is eventually garbage collected.  

Much like `mine()`, returns the value passed to `weele()` or `alaTeme()` when the djuru is resumed.  

### **mine()**

Pauses the current djuru and transfers control to the parent djuru. “Parent” here means the last djuru that was started using `weele` and not `alaTeme`.

```mosc
nin thread = Djuru.kura {
  A.yira("Before yield")
  Djuru.mine()
  A.yira("After yield")
}

thread.weele()              # > Before yield
A.yira("After call")        # > After call
thread.weele()              # > After yield
```

When resumed, the parent djuru’s `weele()` method returns `gansan`.  

If a yielded djuru is resumed by calling `weele()` or `alaTeme()` with an argument, `mine()` returns that value.  

```mosc
nin thread = Djuru.kura {
  A.yira(Djuru.mine()) # > value
}

thread.weele()        # Run until the first yield.
thread.weele("value") # Resume the djuru.
```

If it was resumed by calling `weele()` or `alaTeme()` with no argument, it returns `gansan`.  

If there is no parent djuru to return to, this exits the interpreter. This can be useful to pause execution until the host application wants to resume it later.

```mosc
Djuru.mine()
A.yira("this does not get reached")
```
### **mine()**

Similar to Djuru.mine but provides a value to return to the parent djuru's call.
```mosc
nin djuru = Djuru.kura {
  Djuru.mine("value")
}

A.yira(djuru.weele()) # > value
```

## **Methods**

### **weele()**
Starts or resumes the djuru if it is in a paused state. Equivalent to:

```mosc
djuru.weele()
```

### **weele(value)**

Start or resumes the djuru if it is in a paused state. If the djuru is being started for the first time, and its function takes a parameter, `value` is passed to it.

```mosc
nin djuru = Djuru.kura {(param) =>
  A.yira(param) # > begin
}

djuru.weeke("begin")
```

If the djuru is being resumed, `value` becomes the returned value of the djuru's call to `mine`.

```mosc
nin djuru = Djuru.kura {
  A.yira(Djuru.mine()) # > resume
}

djuru.weele()
djuru.weele("resume")
```

### **fili**

The error message that was passed when aborting the djuru, or null if the djuru has not been aborted. 

```mosc
nin djuru = Djuru.kura {
  123.badMethod
}

djuru.aladje()
A.yira(djuru.fili) # > Diat does not implement method 'badMethod'.
```

### **ok**

Whether the djuru's main function has completed and the djuru can no longer be run. This returns `galon` if the djuru is currently running or has yielded.


### **aladie()**

Tries to run the djuru. If a runtime error occurs in the called djuru, the error is captured and is returned as a string.


```mosc
nin djuru = Djuru.kura {
  123.badMethod
}

nin error = djuru.aladie()
A.yira("Caught error: " + error)
```
If the called djuru raises an error, it can no longer be used.

### **aladie(value)**
Tries to run the djuru. If a runtime error occurs in the called djuru, the error is captured and is returned as a string. If the djuru is being started for the first time, and its function takes a parameter, `value` is passed to it.

```mosc
nin djuru = Djuru.kura {(value)
  value.badMethod
}

nin error = djuru.aladie("just a string")
A.yira("Caught error: " + error)
```

If the called djuru raises an error, it can no longer be used.

### **alaTeme()**

Pauses execution of the current running djuru, and transfers control to this djuru.  

[Read more](/docs/concurency#transfering-control) about the difference between `weele` and `alaTeme`. Unlike `weele`, `alaTeme` doesn’t track the origin of the transfer.


```mosc
# keep hold of the djuru we start in
nin main = Djuru.sissanTa

# create a new djuru, note it doesn't execute yet!
nin djuru = Djuru.kura {
  A.yira("inside 'djuru'") # > #2: from #1
  main.alaTeme()                # > #3: go back to 'main'
}

djuru.alaTeme()      # > #1: print "inside 'djuru'" via #2
                      # > this djuru is now paused by #1

A.yira("main")  # > #4: prints "main", unpaused by #3
```

### **alaTeme(value)**
Pauses execution of the current running djuru, and transfers control to this djuru.  
Similar to transfer, but a value can be passed between the djurus.

```mosc
#  keep hold of the djuru we start in
nin main = Djuru.sissanTa

#  create a new djuru, note it doesn't execute yet
#  also note that we're accepting a 'value' parameter
nin djuru = Djuru.kura {(value) =>
  A.yira("in 'djuru' = $value")   # > #2: in 'djuru' = 5
  nin result = main.alaTeme("hello?")    # > #3: send to 'message'
  A.yira("end 'djuru' = $result") # > #6: end 'djuru' = 32
}

nin message = djuru.alaTeme(5)   # > #1: send to 'value'
A.yira("... $message")    # > #4: ... hello?
djuru.alaTeme(32)                # > #5: send to 'result'
```

### **filiLaTeme(error)**

Transfer to this djuru, but set this djuru into an error state. The `djuru.fili` value will be populated with the value in `error`.

```mosc
nin A = Djuru.kura {
  A.yira("transferred to A")     # > #4
  B.filiLaTeme("error!")            # > #5
}

nin B = Djuru.kura {
  A.yira("started B")            # > #2 
  A.alaTeme()                         # > #3
  A.yira("should not get here")
}

B.aladie()                   # > #1
A.yira(B.fili)     # > #6: prints "error!" from #5

#  B djuru can no longer be used

B.weele()                  # > #7: Cannot call an aborted djuru.
```

