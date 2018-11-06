NAME
====

s1kd-uom - Convert units of measure in quantity data

SYNOPSIS
========

    s1kd-uom [-F <fmt>] [-u <uom> -t <uom> [-e <expr>] [-F <fmt>] ...]
             [-U <path>] [-fl,h?] [<object>...]

DESCRIPTION
===========

The *s1kd-uom* tool converts between specified units of measure in
quantity data, for example, to automatically localize units of measure
in data modules.

OPTIONS
=======

-e &lt;expr&gt;  
Specify the formula for a conversion, given as an XPath expression.

-F &lt;fmt&gt;  
Specify the format for quantity values. When used before -u, this
specifies the format for all conversions. Otherwise, this specifies the
format for each individual conversion.

-f  
Overwrite input CSDB objects.

-h -?  
Show help/usage message.

-l  
Treat input (stdin or arguments) as lists of filenames of CSDB objects
to list references in, rather than CSDB objects themselves.

-t &lt;uom&gt;  
Unit of measure to convert to.

-U &lt;path&gt;  
Use a custom `.uom` file.

-u &lt;uom&gt;  
Unit of measure to convert from.

--version  
Show version information.

&lt;object&gt;  
CSDB objects to convert quantities in.

`.uom` file
-----------

This file contains the rules for converting units of measure. If no
specific conversions are given with the -u and -t options, this file
also acts as a list of all conversions to perform.

By default, the program will search the current directory and parent
directories for a file named `.uom`, but any file can be specified by
using the -U option.

Example of a `.uom` file:

    <uom>
    <convert from="degF" to="degC" formula="($value - 32) * (5 div 9)"/>
    <convert from="in" to="cm" formula="$value * 2.54"/>
    <convert from="lbm" to="kg" formula="$value div 2.205"/>
    </uom>

The tool contains a default set of rules for common units of measure.
This can be used to create a default `.uom` file by use of the -,
option:

    $ s1kd-uom -, > .uom

To select only certain common rules when generating a `.uom` file, the
-u and -t options can be used:

    $ s1kd-uom -, -u in -t cm -u degF -t degC > .uom

This will generate a `.uom` file containing rules to convert inches to
centimetres, and degrees Fahrenheit to degrees Celsius.

Conversion formula variables (-e)
---------------------------------

When specifying a formula for conversion, the variable `$value`
represents the original quantity value. For example, the formula to
convert between degrees Fahrenheit and degrees Celsius can be given as
follows:

`($value - 32) * (5 div 9)`

In additions to this, the following common variables can be used:

-   `$pi`

EXAMPLE
=======

Common units of measure
-----------------------

Input:

    <quantity>
    <quantityGroup>
    <quantityValue quantityUnitOfMeasure="cm">15</quantityValue>
    </quantityGroup>
    </quantity>

Command:

    $ s1kd-uom -u cm -t in <DM>

Output:

    <quantity>
    <quantityGroup>
    <quantityValue quantityUnitOfMeasure="in">5.91</quantityValue>
    </quantityGroup>
    </quantity>

Using a custom formula and format
---------------------------------

Input:

    <quantity
    quantityType="qty02"
    quantityTypeSpecifics="CAD">10.00</quantity>

Command:

    $ s1kd-uom -u CAD -t USD -e '$value div 1.31' -F '0.00'

Output:

    <quantity
    quantityType="qty02"
    quantityTypeSpecifics="USD">7.36</quantity>

UOM FILE SCHEMA
===============

UOM
---

*Markup element:* `<uom>`

*Attributes:*

-   `format` (O), the number format for all rules.

*Child elements:*

-   `<convert>`

Conversion rule
---------------

The element `<convert>` defines a rule to convert one unit of measure to
another.

*Markup element:* `<convert>`

*Attributes:*

-   `format` (O), the number format for this specific rule.

-   `formula` (M), the expression used to convert the quantity value.

-   `from` (M), unit of measure to convert from.

-   `to` (M), unit of measure to convert to.

*Child elements:*

-   None
