NAME
====

s1kd-uom - Convert units of measure in quantity data

SYNOPSIS
========

    s1kd-uom [-F <fmt>] [-u <uom> -t <uom> [-e <expr>] [-F <fmt>] ...]
             [-U <path>] [-p <fmt> [-P <path>]] [-flv,.h?] [<object>...]

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

-P &lt;path&gt;  
Use a custom `.uomdisplay` file.

-p &lt;fmt&gt;  
Preformat quantity data to the specified decimal format. The built-in
formats are:

-   SI - comma for decimal separator, space for grouping

-   euro - comma for decimal separator, full-stop for grouping

-   imperial - full-stop for decimal separator, comma for grouping

-t &lt;uom&gt;  
Unit of measure to convert to.

-U &lt;path&gt;  
Use a custom `.uom` file.

-u &lt;uom&gt;  
Unit of measure to convert from.

-v  
Verbose output.

-,  
Dump the default `.uom` file.

-.  
Dump the default `.uomdisplay` file.

--version  
Show version information.

&lt;object&gt;  
CSDB objects to convert quantities in.

In addition, the following options enable features of the XML parser
that are disabled as a precaution by default:

--dtdload  
Load the external DTD.

--net  
Allow network access to load external DTD and entities.

--noent  
Resolve entities.

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

When specifying a formula for conversion, the following variables can be
used:

`$pi`  
The constant π

`$value`  
The original quantity value

For example, the formula to convert degrees to radians can be given as
follows:

`$value * ($pi div 180)`

Preformatting UOMs (-p) and the `.uomdisplay` file
--------------------------------------------------

The tool can also convert semantic quantity data to presentation
quantity data. The -p option specifies which conventions to use for
formatting quantity values. For example:

    <para>Tighten the
    <quantity>
    <quantityGroup>
    <quantityValue quantityUnitOfMeasure="cm">6.35</quantityValue>
    </quantityGroup>
    </quantity>
    bolt.</para>

    $ s1kd-uom -p SI <DM>

    <para>Tighten the 6,35 cm bolt.</para>

This can also be combined with UOM conversions:

    $ s1kd-uom -u cm -t in -p imperial <DM>

    <para>Tighten the 2.5 in bolt.</para>

Custom formats for values or UOMs can be defined in the `.uomdisplay`
file. By default, the tool will search the current directory and parent
directories for a file named `.uomdisplay`, but any file can be
specified by using the -P option.

Example of a `.uomdisplay` file:

    <uomDisplay>
    <format name="custom" decimalSeparator="," groupingSeparator="."/>
    <uoms>
    <uom name="cm"> cm</uom>
    <uom name="cm2"> cm<superScript>2</superScript></uom>
    </uoms>
    </uomDisplay>

Units of measure that are not defined will be presented as their name
(e.g., "cm2") separated from the value by a space.

The tool contains a default set of formats and displays. These can be
used to create a default `.uomdisplay` file by use of the -. option:

    $ s1kd-uom -. > .uomdisplay

EXAMPLES
========

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

UOMDISPLAY FILE SCHEMA
======================

UOM display
-----------

*Markup element:* `<uomDisplay>`

*Attributes:*

-   None

*Child elements:*

-   `<format>`

-   `<groupTypePrefixes>`

-   `<wrapInto>`

-   `<uoms>`

Quantity value format
---------------------

*Markup element:* `<format>`

*Attributes:*

-   `name` (M), the name of the format

-   `decimalSeparator` (M), the decimal separator

-   `groupingSeparator` (M), the grouping separator

*Child elements:*

-   None

Group type prefixes
-------------------

The element `<groupTypePrefixes>` specifies prefixes which are added for
specific group types.

*Markup element:* `<groupTypePrefixes>`

*Attributes:*

-   None

*Child elements:*

-   `<nominal>`, text placed before a nominal group.

-   `<minimum>`, text placed before a minimum group.

-   `<minimumRange>`, text placed before a minimum group that is
    followed by a maximum group to specify a range.

-   `<maximum>`, text placed before a maximum group.

-   `<maximumRange>`, text placed before a maximum group that is
    preceded by a minimum group to specify a range.

Wrap into element
-----------------

*Markup element:* `<wrapInto>`

*Attributes:*

-   None

*Child elements:*

The element `<wrapInto>` contains one child element of any type, which
quantities will be wrapped in to after formatting.

Units of measure
----------------

*Markup element:* `<uoms>`

*Attributes:*

-   None

*Child elements:*

-   `<uom>`

Display of a unit of measure
----------------------------

*Markup element:* `<uom>`

*Attributes:*

-   `name` (M), the name of the UOM.

*Child elements:*

The element &lt;uom&gt; may contain mixed content, which will be used
for the display of the unit of measure.
