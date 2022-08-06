---
title: "Syntax"
date: 2022-03-05T15:29:09Z
weight: 2
draft: false
---

Mosc follows c-like language syntax

## **Identifiers**
Identifiers are used to as function, method, variable or class name. They should match `[a-zA-Z][a-zA-Z_]` patterns
```mosc
  molo # valid
  _molo # valid
  __molo # valid
  11molo # invalid identifier
  $molo # invalid identifier

```
## **Keywords**

`ale`, `atike`, `dialen`, `dilan`, `dunan`, `faa`, `foo`, `inafo`, `ipan`, `kabo`, `kaj`, `kay`, `kono`, `kulu`, `nani`, `nii`, `niin`, `note`, `segin`, `seginka`, `tii`, `tumamin`, `ye`  

You can declare use those keywords as identifiers  

## **Comment**
Comments are divided in 2 categories: single ling comment and block comments. Single line comment starts with `#` while block comments starts with `#*` and ends with `*#`

```mosc
# Single line comment, ends at the end of the line
#*
 Block comment,
 Can have multi line
*#

```
## **New lines and ;**
New-line are important in Mosc, they are used to end an expression if those are not already ended by a `;`. 
```mosc

12 + 34 # new line
test()
```

You can put new line in arbitrary position in Mosc code as long as its not the end of the expression their will be ignored by Mosc.

## **Block**
Block is a statement and contains a group of statement, you can use it anywhere a statement is allowed, it starts at a `{` and ends at `}`. Things like function or methode body, [control flow](/docs/control-flow/) body In the block are allowed statements.
```mosc
 nii a > 3 {
     # statements here
 } note {
    # statements here
 }
```


## **Operators**

Mosc supports arithmitic operations like any language, operators are classified per priority as bellow

<table class="precedence">
  <tbody>
    <tr>
      <th>Prec</th>
      <th>Operator</th>
      <th>Description</th>
      <th>Associates</th>
    </tr>
    <tr>
      <td>1</td>
      <td><code>()</code> <code>[]</code> <code>.</code></td>
      <td>Grouping, <a href="/docs/method-calls">Subscript, Method call</a></td>
      <td>Left</td>
    </tr>
    <tr>
      <td>2</td>
      <td><code>-</code> <code>!</code> <code>~</code></td>
      <td><a href="/docs/method-calls/#operators">Negate, Not, Complement</a></td>
      <td>Right</td>
    </tr>
    <tr>
      <td>3</td>
      <td><code>*</code> <code>/</code> <code>%</code></td>
      <td><a href="/docs/method-calls/#operators">Multiply, Divide, Modulo</a></td>
      <td>Left</td>
    </tr>
    <tr>
      <td>4</td>
      <td><code>+</code> <code>-</code></td>
      <td><a href="/docs/method-calls/#operators">Add, Subtract</a></td>
      <td>Left</td>
    </tr>
    <tr>
      <td>5</td>
      <td><code>..</code> <code>...</code></td>
      <td><a href="/docs/method-calls/#operators">Inclusive range, Exclusive range</a></td>
      <td>Left</td>
    </tr>
    <tr>
      <td>6</td>
      <td><code>&lt;&lt;</code> <code>&gt;&gt;</code></td>
      <td><a href="/docs/method-calls/#operators">Left shift, Right shift</a></td>
      <td>Left</td>
    </tr>
    <tr>
      <td>7</td>
      <td><code>&amp;</code></td>
      <td><a href="/docs/method-calls/#operators">Bitwise and</a></td>
      <td>Left</td>
    </tr>
    <tr>
      <td>8</td>
      <td><code>^</code></td>
      <td><a href="/docs/method-calls/#operators">Bitwise xor</a></td>
      <td>Left</td>
    </tr>
    <tr>
      <td>9</td>
      <td><code>|</code></td>
      <td><a href="/docs/method-calls/#operators">Bitwise or</a></td>
      <td>Left</td>
    </tr>
    <tr>
      <td>10</td>
      <td><code>&lt;</code> <code>&lt;=</code> <code>&gt;</code> <code>&gt;=</code></td>
      <td><a href="/docs/method-calls/#operators">Comparison</a></td>
      <td>Left</td>
    </tr>
    <tr>
      <td>11</td>
      <td><code>is</code></td>
      <td><a href="/docs/method-calls/#operators">Type test</a></td>
      <td>Left</td>
    </tr>
    <tr>
      <td>12</td>
      <td><code>==</code> <code>!=</code></td>
      <td><a href="/docs/method-calls/#operators">Equals, Not equal</a></td>
      <td>Left</td>
    </tr>
    <tr>
      <td>13</td>
      <td><code>&amp;&amp;</code></td>
      <td><a href="/docs/control-flow/#logical-operators">Logical and</a></td>
      <td>Left</td>
    </tr>
    <tr>
      <td>14</td>
      <td><code>||</code></td>
      <td><a href="/docs/control-flow/#logical-operators">Logical or</a></td>
      <td>Left</td>
    </tr>
    <tr>
      <td>15</td>
      <td><code>=</code></td>
      <td><a href="/docs/variables/#assignment">Assignment</a>, <a href="/docs/method-calls/#setters">Setter</a></td>
      <td>Right</td>
    </tr>
  </tbody>
</table>

<br><hr>

